/**
 * File Manager for Colason — Issue #2.
 *
 * Responsibilities:
 *  - Open / Save / Save As against the host environment (Qt WebChannel, Electron / Tauri,
 *    or the browser File System Access API as a final fallback).
 *  - Track unsaved changes and reflect them in the document title (* marker).
 *  - Optional auto-save (default OFF, configurable in settings) that overwrites the
 *    currently open file at a fixed cadence while dirty.
 *  - Crash recovery: periodically snapshot the current document to a recovery store.
 *    On Qt the snapshot lives at ~/.colason/recovery/<id>.md; on the web fallback it
 *    is mirrored to localStorage so the next session can offer to restore it.
 *
 * The C++ layer is *not* required to participate. When QWebChannel is unavailable
 * (e.g. running editor/ in `vite dev` for tests) the manager degrades gracefully.
 */

import type { Editor } from '@tiptap/core';
import { htmlToSimpleMarkdown, simpleMarkdownToHtml } from './source-mode';
import { getSettings, onSettingsChange } from './settings';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

type SaveResult =
  | { ok: true; path: string }
  | { ok: false; reason: 'cancelled' | 'unsupported' | 'error'; message?: string };

type OpenResult =
  | { ok: true; path: string; content: string }
  | { ok: false; reason: 'cancelled' | 'unsupported' | 'error'; message?: string };

interface RecoverySnapshot {
  id: string;
  path: string | null;
  markdown: string;
  savedAt: number;
}

interface HostBridge {
  /** Human-readable name, only used for logs. */
  name: string;
  open(): Promise<OpenResult>;
  save(path: string | null, markdown: string): Promise<SaveResult>;
  saveAs(markdown: string): Promise<SaveResult>;
  /** Best-effort recovery write. May be a no-op on the web fallback. */
  writeRecovery(snapshot: RecoverySnapshot): Promise<void>;
  clearRecovery(id: string): Promise<void>;
  listRecovery(): Promise<RecoverySnapshot[]>;
}

// ---------------------------------------------------------------------------
// Recovery store (localStorage mirror, used by every backend)
// ---------------------------------------------------------------------------

const RECOVERY_KEY = 'colason.recovery.v1';
const SESSION_ID = `${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;

function readRecoveryMirror(): RecoverySnapshot[] {
  try {
    const raw = localStorage.getItem(RECOVERY_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    return Array.isArray(parsed) ? parsed : [];
  } catch {
    return [];
  }
}

function writeRecoveryMirror(snapshots: RecoverySnapshot[]) {
  try {
    localStorage.setItem(RECOVERY_KEY, JSON.stringify(snapshots));
  } catch (err) {
    console.warn('[Colason] Failed to persist recovery mirror:', err);
  }
}

function upsertRecoveryMirror(snapshot: RecoverySnapshot) {
  const list = readRecoveryMirror().filter((s) => s.id !== snapshot.id);
  list.push(snapshot);
  writeRecoveryMirror(list);
}

function removeRecoveryMirror(id: string) {
  writeRecoveryMirror(readRecoveryMirror().filter((s) => s.id !== id));
}

// ---------------------------------------------------------------------------
// Host bridges
// ---------------------------------------------------------------------------

interface QtFileBridge {
  openFile?: (callback: (json: string) => void) => void;
  saveFile?: (path: string, content: string, callback: (json: string) => void) => void;
  saveFileAs?: (content: string, callback: (json: string) => void) => void;
  writeRecovery?: (id: string, path: string, content: string, callback?: (json: string) => void) => void;
  clearRecovery?: (id: string, callback?: (json: string) => void) => void;
}

function getQtBridge(): QtFileBridge | null {
  const w = window as any;
  // Resolved by initBridge() in bridge.ts and assigned to window.colasonFile when present.
  return w.colasonFile ?? null;
}

function qtCallback<T>(resolve: (value: T) => void, parser: (json: string) => T) {
  return (json: string) => {
    try {
      resolve(parser(json));
    } catch (err) {
      console.error('[Colason] Qt bridge response parse failed:', err);
      resolve(parser('{}'));
    }
  };
}

function makeQtBridge(qt: QtFileBridge): HostBridge {
  return {
    name: 'qt',
    async open(): Promise<OpenResult> {
      if (!qt.openFile) return { ok: false, reason: 'unsupported' };
      return new Promise<OpenResult>((resolve) => {
        qt.openFile!(
          qtCallback<OpenResult>(resolve, (json): OpenResult => {
            const data = json ? JSON.parse(json) : {};
            if (data?.cancelled) return { ok: false, reason: 'cancelled' };
            if (typeof data?.path === 'string' && typeof data?.content === 'string') {
              return { ok: true, path: data.path, content: data.content };
            }
            return { ok: false, reason: 'error', message: data?.error };
          }),
        );
      });
    },
    async save(path, markdown): Promise<SaveResult> {
      if (!path) return this.saveAs(markdown);
      if (!qt.saveFile) return { ok: false, reason: 'unsupported' };
      return new Promise<SaveResult>((resolve) => {
        qt.saveFile!(
          path,
          markdown,
          qtCallback<SaveResult>(resolve, (json): SaveResult => {
            const data = json ? JSON.parse(json) : {};
            if (data?.cancelled) return { ok: false, reason: 'cancelled' };
            if (data?.ok && typeof data?.path === 'string') return { ok: true, path: data.path };
            return { ok: false, reason: 'error', message: data?.error };
          }),
        );
      });
    },
    async saveAs(markdown): Promise<SaveResult> {
      if (!qt.saveFileAs) return { ok: false, reason: 'unsupported' };
      return new Promise<SaveResult>((resolve) => {
        qt.saveFileAs!(
          markdown,
          qtCallback<SaveResult>(resolve, (json): SaveResult => {
            const data = json ? JSON.parse(json) : {};
            if (data?.cancelled) return { ok: false, reason: 'cancelled' };
            if (data?.ok && typeof data?.path === 'string') return { ok: true, path: data.path };
            return { ok: false, reason: 'error', message: data?.error };
          }),
        );
      });
    },
    async writeRecovery(snapshot) {
      upsertRecoveryMirror(snapshot);
      if (qt.writeRecovery) {
        try {
          qt.writeRecovery(snapshot.id, snapshot.path ?? '', snapshot.markdown);
        } catch (err) {
          console.warn('[Colason] Qt writeRecovery failed:', err);
        }
      }
    },
    async clearRecovery(id) {
      removeRecoveryMirror(id);
      if (qt.clearRecovery) {
        try {
          qt.clearRecovery(id);
        } catch (err) {
          console.warn('[Colason] Qt clearRecovery failed:', err);
        }
      }
    },
    async listRecovery() {
      return readRecoveryMirror();
    },
  };
}

// Electron / Tauri / preload-style bridge: a single object exposed on window.
interface NativeFileBridge {
  open(): Promise<{ path: string; content: string } | null>;
  save(path: string, content: string): Promise<{ path: string }>;
  saveAs(content: string): Promise<{ path: string } | null>;
  writeRecovery?(id: string, path: string, content: string): Promise<void>;
  clearRecovery?(id: string): Promise<void>;
}

function getNativeBridge(): NativeFileBridge | null {
  const w = window as any;
  return w.colasonNativeFile ?? null;
}

function makeNativeBridge(native: NativeFileBridge): HostBridge {
  return {
    name: 'native',
    async open() {
      try {
        const result = await native.open();
        if (!result) return { ok: false, reason: 'cancelled' };
        return { ok: true, path: result.path, content: result.content };
      } catch (err) {
        return { ok: false, reason: 'error', message: String(err) };
      }
    },
    async save(path, markdown) {
      if (!path) return this.saveAs(markdown);
      try {
        const result = await native.save(path, markdown);
        return { ok: true, path: result.path };
      } catch (err) {
        return { ok: false, reason: 'error', message: String(err) };
      }
    },
    async saveAs(markdown) {
      try {
        const result = await native.saveAs(markdown);
        if (!result) return { ok: false, reason: 'cancelled' };
        return { ok: true, path: result.path };
      } catch (err) {
        return { ok: false, reason: 'error', message: String(err) };
      }
    },
    async writeRecovery(snapshot) {
      upsertRecoveryMirror(snapshot);
      if (native.writeRecovery) {
        try {
          await native.writeRecovery(snapshot.id, snapshot.path ?? '', snapshot.markdown);
        } catch (err) {
          console.warn('[Colason] native writeRecovery failed:', err);
        }
      }
    },
    async clearRecovery(id) {
      removeRecoveryMirror(id);
      if (native.clearRecovery) {
        try {
          await native.clearRecovery(id);
        } catch (err) {
          console.warn('[Colason] native clearRecovery failed:', err);
        }
      }
    },
    async listRecovery() {
      return readRecoveryMirror();
    },
  };
}

// Browser fallback: File System Access API where available, <input type=file> +
// download anchor otherwise. We keep the active FileSystemFileHandle in memory
// so plain Save can overwrite without re-prompting.
let webFileHandle: FileSystemFileHandle | null = null;
let webFilePath: string | null = null;

async function fsaOpen(): Promise<OpenResult> {
  const w = window as any;
  if (typeof w.showOpenFilePicker !== 'function') {
    return fallbackInputOpen();
  }
  try {
    const [handle] = await w.showOpenFilePicker({
      types: [{ description: 'Markdown', accept: { 'text/markdown': ['.md', '.markdown'] } }],
      excludeAcceptAllOption: false,
      multiple: false,
    });
    const file: File = await handle.getFile();
    const content = await file.text();
    webFileHandle = handle;
    webFilePath = file.name;
    return { ok: true, path: file.name, content };
  } catch (err: any) {
    if (err?.name === 'AbortError') return { ok: false, reason: 'cancelled' };
    return { ok: false, reason: 'error', message: String(err) };
  }
}

function fallbackInputOpen(): Promise<OpenResult> {
  return new Promise<OpenResult>((resolve) => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.md,.markdown,text/markdown,text/plain';
    let settled = false;
    input.onchange = async () => {
      settled = true;
      const file = input.files?.[0];
      if (!file) return resolve({ ok: false, reason: 'cancelled' });
      try {
        const content = await file.text();
        webFileHandle = null;
        webFilePath = file.name;
        resolve({ ok: true, path: file.name, content });
      } catch (err) {
        resolve({ ok: false, reason: 'error', message: String(err) });
      }
    };
    // Best-effort cancellation detection.
    window.addEventListener(
      'focus',
      () => {
        setTimeout(() => {
          if (!settled) resolve({ ok: false, reason: 'cancelled' });
        }, 500);
      },
      { once: true },
    );
    input.click();
  });
}

async function fsaSave(path: string | null, markdown: string): Promise<SaveResult> {
  if (webFileHandle && (!path || path === webFilePath)) {
    try {
      const writable = await webFileHandle.createWritable();
      await writable.write(markdown);
      await writable.close();
      return { ok: true, path: webFilePath ?? path ?? 'untitled.md' };
    } catch (err) {
      return { ok: false, reason: 'error', message: String(err) };
    }
  }
  return fsaSaveAs(markdown);
}

async function fsaSaveAs(markdown: string): Promise<SaveResult> {
  const w = window as any;
  if (typeof w.showSaveFilePicker === 'function') {
    try {
      const handle: FileSystemFileHandle = await w.showSaveFilePicker({
        suggestedName: webFilePath ?? 'untitled.md',
        types: [{ description: 'Markdown', accept: { 'text/markdown': ['.md', '.markdown'] } }],
      });
      const writable = await handle.createWritable();
      await writable.write(markdown);
      await writable.close();
      const file: File = await handle.getFile();
      webFileHandle = handle;
      webFilePath = file.name;
      return { ok: true, path: file.name };
    } catch (err: any) {
      if (err?.name === 'AbortError') return { ok: false, reason: 'cancelled' };
      return { ok: false, reason: 'error', message: String(err) };
    }
  }
  // Final fallback: trigger a download. The "path" is just the filename.
  try {
    const blob = new Blob([markdown], { type: 'text/markdown' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = webFilePath ?? 'untitled.md';
    document.body.appendChild(a);
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
    return { ok: true, path: a.download };
  } catch (err) {
    return { ok: false, reason: 'error', message: String(err) };
  }
}

const webBridge: HostBridge = {
  name: 'web',
  open: fsaOpen,
  save: fsaSave,
  saveAs: fsaSaveAs,
  async writeRecovery(snapshot) {
    upsertRecoveryMirror(snapshot);
  },
  async clearRecovery(id) {
    removeRecoveryMirror(id);
  },
  async listRecovery() {
    return readRecoveryMirror();
  },
};

function pickHostBridge(): HostBridge {
  const qt = getQtBridge();
  if (qt) return makeQtBridge(qt);
  const native = getNativeBridge();
  if (native) return makeNativeBridge(native);
  return webBridge;
}

// ---------------------------------------------------------------------------
// FileManager (singleton, attached to window.colasonFiles)
// ---------------------------------------------------------------------------

export interface FileState {
  path: string | null;
  fileName: string;
  dirty: boolean;
}

type FileStateListener = (state: FileState) => void;

export class FileManager {
  private editor: Editor;
  private host: HostBridge;
  private state: FileState = { path: null, fileName: 'Untitled.md', dirty: false };
  private listeners = new Set<FileStateListener>();
  private autoSaveTimer: number | null = null;
  private recoveryTimer: number | null = null;
  private lastSavedMarkdown = '';
  private lastRecoveryMarkdown = '';
  private settingsDispose: (() => void) | null = null;

  constructor(editor: Editor) {
    this.editor = editor;
    this.host = pickHostBridge();
    console.log(`[Colason] FileManager using host=${this.host.name}`);

    // React to editor updates: dirty tracking is the cheap part; auto-save and
    // recovery use timers so we don't write on every keystroke.
    editor.on('update', () => this.handleEditorUpdate());

    this.settingsDispose = onSettingsChange(() => this.restartTimers());
    this.restartTimers();
    this.notify();

    window.addEventListener('beforeunload', (event) => {
      if (this.state.dirty) {
        event.preventDefault();
        event.returnValue = '';
      }
      // Always flush a final recovery snapshot before unloading.
      void this.writeRecoverySnapshot(true).catch(() => {});
    });
  }

  // ---- Public API ------------------------------------------------------

  getState(): FileState {
    return { ...this.state };
  }

  onChange(listener: FileStateListener): () => void {
    this.listeners.add(listener);
    listener(this.getState());
    return () => this.listeners.delete(listener);
  }

  async openFile(): Promise<OpenResult> {
    const result = await this.host.open();
    if (!result.ok) {
      if (result.reason === 'error') {
        console.error('[Colason] open failed:', result.message);
      }
      return result;
    }
    this.loadMarkdown(result.path, result.content);
    return result;
  }

  async save(): Promise<SaveResult> {
    const markdown = this.currentMarkdown();
    const result = await this.host.save(this.state.path, markdown);
    if (result.ok) {
      this.state.path = result.path;
      this.state.fileName = basename(result.path) || this.state.fileName;
      this.state.dirty = false;
      this.lastSavedMarkdown = markdown;
      this.notify();
      void this.host.clearRecovery(SESSION_ID).catch(() => {});
    }
    return result;
  }

  async saveAs(): Promise<SaveResult> {
    const markdown = this.currentMarkdown();
    const result = await this.host.saveAs(markdown);
    if (result.ok) {
      this.state.path = result.path;
      this.state.fileName = basename(result.path) || 'Untitled.md';
      this.state.dirty = false;
      this.lastSavedMarkdown = markdown;
      this.notify();
      void this.host.clearRecovery(SESSION_ID).catch(() => {});
    }
    return result;
  }

  /** Replace the current document and reset dirty state. Used by recovery flow. */
  loadMarkdown(path: string | null, markdown: string) {
    const html = simpleMarkdownToHtml(markdown);
    this.editor.commands.setContent(html || '<p></p>');
    this.editor.commands.focus();
    this.state.path = path;
    this.state.fileName = path ? basename(path) || 'Untitled.md' : 'Untitled.md';
    this.state.dirty = false;
    this.lastSavedMarkdown = markdown;
    this.lastRecoveryMarkdown = markdown;
    this.notify();
  }

  /** Drop unsaved state without prompting. Caller is responsible for confirmation. */
  resetToBlank() {
    this.editor.commands.setContent('<p></p>');
    this.state.path = null;
    this.state.fileName = 'Untitled.md';
    this.state.dirty = false;
    this.lastSavedMarkdown = '';
    this.lastRecoveryMarkdown = '';
    this.notify();
  }

  async listRecovery(): Promise<RecoverySnapshot[]> {
    return this.host.listRecovery();
  }

  async dismissRecovery(id: string) {
    return this.host.clearRecovery(id);
  }

  // ---- Internals -------------------------------------------------------

  private currentMarkdown(): string {
    const w = window as any;
    if (w.colasonAPI?.getMarkdown) return w.colasonAPI.getMarkdown();
    return htmlToSimpleMarkdown(this.editor.getHTML());
  }

  private handleEditorUpdate() {
    const md = this.currentMarkdown();
    const dirty = md !== this.lastSavedMarkdown;
    if (dirty !== this.state.dirty) {
      this.state.dirty = dirty;
      this.notify();
    }
  }

  private restartTimers() {
    const settings = getSettings();
    if (this.autoSaveTimer != null) {
      window.clearInterval(this.autoSaveTimer);
      this.autoSaveTimer = null;
    }
    if (this.recoveryTimer != null) {
      window.clearInterval(this.recoveryTimer);
      this.recoveryTimer = null;
    }

    if (settings.autoSave) {
      this.autoSaveTimer = window.setInterval(() => {
        if (!this.state.dirty || !this.state.path) return;
        void this.save().catch((err) => console.error('[Colason] auto-save failed:', err));
      }, Math.max(1000, settings.autoSaveIntervalMs));
    }

    this.recoveryTimer = window.setInterval(() => {
      void this.writeRecoverySnapshot(false).catch((err) =>
        console.warn('[Colason] recovery snapshot failed:', err),
      );
    }, Math.max(1000, settings.recoveryIntervalMs));
  }

  private async writeRecoverySnapshot(force: boolean) {
    const markdown = this.currentMarkdown();
    if (!force && markdown === this.lastRecoveryMarkdown) return;
    if (!force && !this.state.dirty) return;
    this.lastRecoveryMarkdown = markdown;
    await this.host.writeRecovery({
      id: SESSION_ID,
      path: this.state.path,
      markdown,
      savedAt: Date.now(),
    });
  }

  private notify() {
    const snapshot = this.getState();
    this.listeners.forEach((l) => {
      try {
        l(snapshot);
      } catch (err) {
        console.error('[Colason] file state listener threw:', err);
      }
    });
    updateDocumentTitle(snapshot);
    // Mirror dirty state to the existing C++ bridge so the title bar in Qt updates.
    const w = window as any;
    if (w.colasonBridgeNotifyDirty) {
      w.colasonBridgeNotifyDirty(snapshot.dirty);
    }
  }

  dispose() {
    if (this.autoSaveTimer != null) window.clearInterval(this.autoSaveTimer);
    if (this.recoveryTimer != null) window.clearInterval(this.recoveryTimer);
    this.settingsDispose?.();
  }
}

function basename(path: string): string {
  const cleaned = path.replace(/[\\/]+$/, '');
  const idx = Math.max(cleaned.lastIndexOf('/'), cleaned.lastIndexOf('\\'));
  return idx >= 0 ? cleaned.slice(idx + 1) : cleaned;
}

function updateDocumentTitle(state: FileState) {
  const marker = state.dirty ? '* ' : '';
  document.title = `${marker}${state.fileName} — Colason`;
}
