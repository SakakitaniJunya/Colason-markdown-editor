/**
 * File / Workspace Manager
 *
 * 機能:
 *   - Workspace (root directory) 概念
 *   - 再帰的なファイル / フォルダ列挙 (FileTreeNode)
 *   - File 新規 / 削除 / リネーム / 移動
 *   - 最近開いたファイル履歴 (localStorage)
 *
 * バックエンド (file system access) は 3 層フォールバック:
 *   1. C++ host bridge (cppBridge.fileSystem.*) — Qt WebChannel 経由
 *   2. File System Access API (browser, Chromium) — showDirectoryPicker / FileSystemDirectoryHandle
 *   3. In-memory mock — テスト / デモ用
 *
 * UI 層からは FileManager インターフェイスのみを参照する。
 */

export type FileTreeNode = {
  /** Display name (basename) */
  name: string;
  /** ワークスペースルートからの相対パス (POSIX 区切り) */
  path: string;
  kind: 'file' | 'directory';
  /** ディレクトリのみ。展開時に lazy-load される場合は未設定可 */
  children?: FileTreeNode[];
};

export type WorkspaceInfo = {
  rootName: string;
  rootPath: string;
};

export interface FileManager {
  /** Workspace を開く (root 選択ダイアログ) */
  openWorkspace(): Promise<WorkspaceInfo | null>;
  /** 現在の workspace 情報 */
  getWorkspace(): WorkspaceInfo | null;
  /** ツリー全体 (ルートの children) を取得 */
  listTree(): Promise<FileTreeNode[]>;
  /** ファイル内容を読む */
  readFile(path: string): Promise<string>;
  /** ファイル内容を書く (作成 or 上書き) */
  writeFile(path: string, content: string): Promise<void>;
  /** 新規ファイル / フォルダ作成 */
  createNode(parentPath: string, name: string, kind: 'file' | 'directory'): Promise<FileTreeNode>;
  /** リネーム (basename 変更のみ) */
  rename(path: string, newName: string): Promise<string>;
  /** 削除 (確認は呼び出し側) */
  remove(path: string): Promise<void>;
  /** 移動 (drag-drop) */
  move(srcPath: string, destDirPath: string): Promise<string>;
}

// =================================================================
// Recent files (localStorage)
// =================================================================

const RECENT_FILES_KEY = 'colason.recentFiles';
const RECENT_LIMIT = 10;

export type RecentFile = {
  path: string;
  name: string;
  /** workspace ルート名 (区別用) */
  workspace: string;
  openedAt: number;
};

export function getRecentFiles(): RecentFile[] {
  try {
    const raw = localStorage.getItem(RECENT_FILES_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) return [];
    return parsed.filter((r): r is RecentFile =>
      typeof r === 'object' && r !== null
      && typeof r.path === 'string'
      && typeof r.name === 'string'
      && typeof r.workspace === 'string'
      && typeof r.openedAt === 'number'
    );
  } catch {
    return [];
  }
}

export function pushRecentFile(entry: Omit<RecentFile, 'openedAt'>): void {
  const list = getRecentFiles().filter(r => !(r.path === entry.path && r.workspace === entry.workspace));
  list.unshift({ ...entry, openedAt: Date.now() });
  const trimmed = list.slice(0, RECENT_LIMIT);
  try {
    localStorage.setItem(RECENT_FILES_KEY, JSON.stringify(trimmed));
  } catch {
    // quota exceeded — ignore silently
  }
}

export function clearRecentFiles(): void {
  try {
    localStorage.removeItem(RECENT_FILES_KEY);
  } catch {
    // ignore
  }
}

// =================================================================
// Path helpers
// =================================================================

function joinPath(parent: string, name: string): string {
  if (!parent || parent === '/' || parent === '.') return name;
  if (parent.endsWith('/')) return `${parent}${name}`;
  return `${parent}/${name}`;
}

function basename(p: string): string {
  const idx = p.lastIndexOf('/');
  return idx >= 0 ? p.slice(idx + 1) : p;
}

function dirname(p: string): string {
  const idx = p.lastIndexOf('/');
  return idx > 0 ? p.slice(0, idx) : '';
}

// =================================================================
// File System Access API backend (Chromium)
// =================================================================

class FileSystemAccessBackend implements FileManager {
  private rootHandle: FileSystemDirectoryHandle | null = null;
  private rootName = '';

  async openWorkspace(): Promise<WorkspaceInfo | null> {
    const showPicker = (window as unknown as {
      showDirectoryPicker?: (opts?: { mode?: 'read' | 'readwrite' }) => Promise<FileSystemDirectoryHandle>;
    }).showDirectoryPicker;
    if (!showPicker) return null;
    try {
      const handle = await showPicker({ mode: 'readwrite' });
      this.rootHandle = handle;
      this.rootName = handle.name;
      return { rootName: this.rootName, rootPath: this.rootName };
    } catch {
      // user cancelled
      return null;
    }
  }

  getWorkspace(): WorkspaceInfo | null {
    if (!this.rootHandle) return null;
    return { rootName: this.rootName, rootPath: this.rootName };
  }

  async listTree(): Promise<FileTreeNode[]> {
    if (!this.rootHandle) return [];
    return await this.readDirRecursive(this.rootHandle, '');
  }

  private async readDirRecursive(dir: FileSystemDirectoryHandle, parentPath: string): Promise<FileTreeNode[]> {
    const out: FileTreeNode[] = [];
    // FileSystemDirectoryHandle is async-iterable (entries())
    const iter = (dir as unknown as { entries: () => AsyncIterable<[string, FileSystemHandle]> }).entries();
    for await (const [name, handle] of iter) {
      const path = joinPath(parentPath, name);
      if (handle.kind === 'directory') {
        const subdir = handle as FileSystemDirectoryHandle;
        out.push({
          name,
          path,
          kind: 'directory',
          children: await this.readDirRecursive(subdir, path),
        });
      } else {
        out.push({ name, path, kind: 'file' });
      }
    }
    out.sort((a, b) => {
      if (a.kind !== b.kind) return a.kind === 'directory' ? -1 : 1;
      return a.name.localeCompare(b.name);
    });
    return out;
  }

  private async resolveDir(path: string, create: boolean): Promise<FileSystemDirectoryHandle> {
    if (!this.rootHandle) throw new Error('No workspace');
    if (!path) return this.rootHandle;
    const parts = path.split('/').filter(Boolean);
    let cur: FileSystemDirectoryHandle = this.rootHandle;
    for (const part of parts) {
      cur = await cur.getDirectoryHandle(part, { create });
    }
    return cur;
  }

  private async resolveFile(path: string, create: boolean): Promise<FileSystemFileHandle> {
    if (!this.rootHandle) throw new Error('No workspace');
    const dir = await this.resolveDir(dirname(path), false);
    return await dir.getFileHandle(basename(path), { create });
  }

  async readFile(path: string): Promise<string> {
    const fh = await this.resolveFile(path, false);
    const file = await fh.getFile();
    return await file.text();
  }

  async writeFile(path: string, content: string): Promise<void> {
    const fh = await this.resolveFile(path, true);
    const writable = await (fh as unknown as { createWritable: () => Promise<{
      write: (data: string) => Promise<void>;
      close: () => Promise<void>;
    }>}).createWritable();
    await writable.write(content);
    await writable.close();
  }

  async createNode(parentPath: string, name: string, kind: 'file' | 'directory'): Promise<FileTreeNode> {
    const dir = await this.resolveDir(parentPath, false);
    if (kind === 'directory') {
      await dir.getDirectoryHandle(name, { create: true });
    } else {
      await dir.getFileHandle(name, { create: true });
    }
    const path = joinPath(parentPath, name);
    return { name, path, kind, ...(kind === 'directory' ? { children: [] } : {}) };
  }

  async rename(path: string, newName: string): Promise<string> {
    // File System Access API has no direct rename. Implement via copy + delete.
    const parent = dirname(path);
    const dir = await this.resolveDir(parent, false);
    const oldName = basename(path);
    const oldHandle = await this.getHandle(dir, oldName);
    if (!oldHandle) throw new Error(`Not found: ${path}`);

    if (oldHandle.kind === 'file') {
      const oldFile = oldHandle as FileSystemFileHandle;
      const data = await (await oldFile.getFile()).text();
      const newHandle = await dir.getFileHandle(newName, { create: true });
      const writable = await (newHandle as unknown as { createWritable: () => Promise<{
        write: (data: string) => Promise<void>;
        close: () => Promise<void>;
      }>}).createWritable();
      await writable.write(data);
      await writable.close();
      await dir.removeEntry(oldName);
    } else {
      // Recursive copy for directory rename
      const oldDir = oldHandle as FileSystemDirectoryHandle;
      const newDir = await dir.getDirectoryHandle(newName, { create: true });
      await this.copyDirContents(oldDir, newDir);
      await dir.removeEntry(oldName, { recursive: true });
    }
    return joinPath(parent, newName);
  }

  private async getHandle(dir: FileSystemDirectoryHandle, name: string): Promise<FileSystemHandle | null> {
    try {
      return await dir.getFileHandle(name, { create: false });
    } catch {
      try {
        return await dir.getDirectoryHandle(name, { create: false });
      } catch {
        return null;
      }
    }
  }

  private async copyDirContents(src: FileSystemDirectoryHandle, dest: FileSystemDirectoryHandle): Promise<void> {
    const iter = (src as unknown as { entries: () => AsyncIterable<[string, FileSystemHandle]> }).entries();
    for await (const [name, handle] of iter) {
      if (handle.kind === 'file') {
        const f = await (handle as FileSystemFileHandle).getFile();
        const text = await f.text();
        const dst = await dest.getFileHandle(name, { create: true });
        const writable = await (dst as unknown as { createWritable: () => Promise<{
          write: (data: string) => Promise<void>;
          close: () => Promise<void>;
        }>}).createWritable();
        await writable.write(text);
        await writable.close();
      } else {
        const subSrc = handle as FileSystemDirectoryHandle;
        const subDst = await dest.getDirectoryHandle(name, { create: true });
        await this.copyDirContents(subSrc, subDst);
      }
    }
  }

  async remove(path: string): Promise<void> {
    const parent = dirname(path);
    const dir = await this.resolveDir(parent, false);
    await dir.removeEntry(basename(path), { recursive: true });
  }

  async move(srcPath: string, destDirPath: string): Promise<string> {
    if (srcPath === destDirPath) return srcPath;
    if (destDirPath.startsWith(srcPath + '/')) {
      throw new Error('Cannot move a directory into itself');
    }
    const newPath = joinPath(destDirPath, basename(srcPath));
    if (newPath === srcPath) return srcPath;

    const srcParent = dirname(srcPath);
    const srcDir = await this.resolveDir(srcParent, false);
    const destDir = await this.resolveDir(destDirPath, false);
    const handle = await this.getHandle(srcDir, basename(srcPath));
    if (!handle) throw new Error(`Not found: ${srcPath}`);

    if (handle.kind === 'file') {
      const data = await (await (handle as FileSystemFileHandle).getFile()).text();
      const dst = await destDir.getFileHandle(basename(srcPath), { create: true });
      const writable = await (dst as unknown as { createWritable: () => Promise<{
        write: (data: string) => Promise<void>;
        close: () => Promise<void>;
      }>}).createWritable();
      await writable.write(data);
      await writable.close();
      await srcDir.removeEntry(basename(srcPath));
    } else {
      const dstDir = await destDir.getDirectoryHandle(basename(srcPath), { create: true });
      await this.copyDirContents(handle as FileSystemDirectoryHandle, dstDir);
      await srcDir.removeEntry(basename(srcPath), { recursive: true });
    }
    return newPath;
  }
}

// =================================================================
// C++ host bridge backend (Qt WebChannel)
//
// 期待する C++ API (将来追加されるまで存在しないことが多い):
//   cppBridge.fileSystem.openWorkspace()        => { rootName, rootPath }
//   cppBridge.fileSystem.listTree()             => FileTreeNode[]
//   cppBridge.fileSystem.readFile(path)         => string
//   cppBridge.fileSystem.writeFile(path, body)  => void
//   cppBridge.fileSystem.createNode(parent,name,kind) => FileTreeNode
//   cppBridge.fileSystem.rename(path,newName)   => string (newPath)
//   cppBridge.fileSystem.remove(path)           => void
//   cppBridge.fileSystem.move(src,destDir)      => string (newPath)
//
// 各メソッドは Promise を返す前提。利用可能な API のみを優先し、未実装メソッドは
// File System Access API へフォールバックする。
// =================================================================

type CppFsApi = {
  openWorkspace?: () => Promise<WorkspaceInfo | null>;
  listTree?: () => Promise<FileTreeNode[]>;
  readFile?: (path: string) => Promise<string>;
  writeFile?: (path: string, content: string) => Promise<void>;
  createNode?: (parent: string, name: string, kind: 'file' | 'directory') => Promise<FileTreeNode>;
  rename?: (path: string, newName: string) => Promise<string>;
  remove?: (path: string) => Promise<void>;
  move?: (src: string, destDir: string) => Promise<string>;
};

class CppBridgeBackend implements FileManager {
  private workspace: WorkspaceInfo | null = null;
  constructor(private fs: CppFsApi, private fallback: FileManager) {}

  async openWorkspace(): Promise<WorkspaceInfo | null> {
    if (this.fs.openWorkspace) {
      this.workspace = await this.fs.openWorkspace();
      return this.workspace;
    }
    this.workspace = await this.fallback.openWorkspace();
    return this.workspace;
  }
  getWorkspace(): WorkspaceInfo | null {
    return this.workspace ?? this.fallback.getWorkspace();
  }
  async listTree(): Promise<FileTreeNode[]> {
    if (this.fs.listTree) return this.fs.listTree();
    return this.fallback.listTree();
  }
  async readFile(path: string): Promise<string> {
    if (this.fs.readFile) return this.fs.readFile(path);
    return this.fallback.readFile(path);
  }
  async writeFile(path: string, content: string): Promise<void> {
    if (this.fs.writeFile) return this.fs.writeFile(path, content);
    return this.fallback.writeFile(path, content);
  }
  async createNode(parentPath: string, name: string, kind: 'file' | 'directory'): Promise<FileTreeNode> {
    if (this.fs.createNode) return this.fs.createNode(parentPath, name, kind);
    return this.fallback.createNode(parentPath, name, kind);
  }
  async rename(path: string, newName: string): Promise<string> {
    if (this.fs.rename) return this.fs.rename(path, newName);
    return this.fallback.rename(path, newName);
  }
  async remove(path: string): Promise<void> {
    if (this.fs.remove) return this.fs.remove(path);
    return this.fallback.remove(path);
  }
  async move(srcPath: string, destDirPath: string): Promise<string> {
    if (this.fs.move) return this.fs.move(srcPath, destDirPath);
    return this.fallback.move(srcPath, destDirPath);
  }
}

// =================================================================
// In-memory mock backend (test / demo fallback)
// =================================================================

class InMemoryBackend implements FileManager {
  private workspace: WorkspaceInfo | null = null;
  private files = new Map<string, string>();
  private dirs = new Set<string>();

  async openWorkspace(): Promise<WorkspaceInfo | null> {
    this.workspace = { rootName: 'demo-workspace', rootPath: 'demo-workspace' };
    if (this.files.size === 0) {
      this.files.set('README.md', '# Demo workspace\n\nファイルツリー機能のお試し用です。');
      this.dirs.add('notes');
      this.files.set('notes/example.md', '# Note 1\n');
    }
    return this.workspace;
  }
  getWorkspace(): WorkspaceInfo | null {
    return this.workspace;
  }
  async listTree(): Promise<FileTreeNode[]> {
    const all = new Map<string, FileTreeNode>();
    for (const dir of this.dirs) {
      all.set(dir, { name: basename(dir), path: dir, kind: 'directory', children: [] });
    }
    for (const file of this.files.keys()) {
      all.set(file, { name: basename(file), path: file, kind: 'file' });
    }
    const roots: FileTreeNode[] = [];
    for (const node of all.values()) {
      const parent = dirname(node.path);
      if (!parent) {
        roots.push(node);
      } else {
        const p = all.get(parent);
        if (p && p.kind === 'directory') {
          p.children = p.children ?? [];
          p.children.push(node);
        } else {
          roots.push(node);
        }
      }
    }
    const sortRec = (nodes: FileTreeNode[]): void => {
      nodes.sort((a, b) => {
        if (a.kind !== b.kind) return a.kind === 'directory' ? -1 : 1;
        return a.name.localeCompare(b.name);
      });
      for (const n of nodes) if (n.children) sortRec(n.children);
    };
    sortRec(roots);
    return roots;
  }
  async readFile(path: string): Promise<string> {
    return this.files.get(path) ?? '';
  }
  async writeFile(path: string, content: string): Promise<void> {
    this.files.set(path, content);
  }
  async createNode(parentPath: string, name: string, kind: 'file' | 'directory'): Promise<FileTreeNode> {
    const path = joinPath(parentPath, name);
    if (kind === 'directory') {
      this.dirs.add(path);
      return { name, path, kind, children: [] };
    }
    this.files.set(path, '');
    return { name, path, kind };
  }
  async rename(path: string, newName: string): Promise<string> {
    const newPath = joinPath(dirname(path), newName);
    if (this.files.has(path)) {
      const data = this.files.get(path) ?? '';
      this.files.delete(path);
      this.files.set(newPath, data);
    } else if (this.dirs.has(path)) {
      this.dirs.delete(path);
      this.dirs.add(newPath);
      // re-key children
      for (const k of [...this.files.keys()]) {
        if (k.startsWith(path + '/')) {
          const nk = newPath + k.slice(path.length);
          this.files.set(nk, this.files.get(k) ?? '');
          this.files.delete(k);
        }
      }
      for (const d of [...this.dirs]) {
        if (d.startsWith(path + '/')) {
          const nd = newPath + d.slice(path.length);
          this.dirs.add(nd);
          this.dirs.delete(d);
        }
      }
    }
    return newPath;
  }
  async remove(path: string): Promise<void> {
    if (this.files.has(path)) {
      this.files.delete(path);
      return;
    }
    if (this.dirs.has(path)) {
      this.dirs.delete(path);
      for (const k of [...this.files.keys()]) if (k.startsWith(path + '/')) this.files.delete(k);
      for (const d of [...this.dirs]) if (d.startsWith(path + '/')) this.dirs.delete(d);
    }
  }
  async move(srcPath: string, destDirPath: string): Promise<string> {
    if (destDirPath.startsWith(srcPath + '/')) {
      throw new Error('Cannot move a directory into itself');
    }
    const newPath = joinPath(destDirPath, basename(srcPath));
    if (newPath === srcPath) return srcPath;
    if (this.files.has(srcPath)) {
      const data = this.files.get(srcPath) ?? '';
      this.files.delete(srcPath);
      this.files.set(newPath, data);
      return newPath;
    }
    if (this.dirs.has(srcPath)) {
      // rename the prefix
      this.dirs.delete(srcPath);
      this.dirs.add(newPath);
      for (const k of [...this.files.keys()]) {
        if (k.startsWith(srcPath + '/')) {
          const nk = newPath + k.slice(srcPath.length);
          this.files.set(nk, this.files.get(k) ?? '');
          this.files.delete(k);
        }
      }
      for (const d of [...this.dirs]) {
        if (d.startsWith(srcPath + '/')) {
          const nd = newPath + d.slice(srcPath.length);
          this.dirs.add(nd);
          this.dirs.delete(d);
        }
      }
    }
    return newPath;
  }
}

// =================================================================
// Factory
// =================================================================

/**
 * 利用可能なバックエンドを優先順 (cpp > FSAccess > memory) で組み立てて返す。
 * cppBridge は initBridge 完了後にしか入らないので呼び出し側は workspace 操作の
 * 直前にこの関数を呼ぶ。
 */
export function createFileManager(): FileManager {
  // detect cpp bridge
  let cppFs: CppFsApi | null = null;
  try {
    const w = window as unknown as { cppBridge?: { fileSystem?: CppFsApi } };
    if (w.cppBridge && w.cppBridge.fileSystem) {
      cppFs = w.cppBridge.fileSystem;
    }
  } catch {
    cppFs = null;
  }

  // detect File System Access API
  const hasFSA = typeof (window as unknown as { showDirectoryPicker?: unknown }).showDirectoryPicker === 'function';

  if (hasFSA) {
    const fsa = new FileSystemAccessBackend();
    return cppFs ? new CppBridgeBackend(cppFs, fsa) : fsa;
  }
  const mem = new InMemoryBackend();
  return cppFs ? new CppBridgeBackend(cppFs, mem) : mem;
}

// Export for test
export const __test__ = {
  joinPath,
  basename,
  dirname,
  InMemoryBackend,
};
