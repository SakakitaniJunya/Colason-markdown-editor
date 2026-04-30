/**
 * Preview Mode Controller
 *
 * Provides 3 view modes:
 *   - 'edit'    : editor only (default)
 *   - 'split'   : editor (left) + preview (right) with synchronized scrolling
 *   - 'preview' : preview only
 *
 * Toolbar buttons + keyboard shortcuts (Ctrl/Cmd+1/2/3) switch modes.
 * Preview rendering uses `marked` for CommonMark + GFM parsing, and the
 * resulting HTML is sanitized with DOMPurify (XSS-safe). Code blocks are
 * syntax-highlighted via highlight.js.
 *
 * The preview is debounced to ~60ms for sub-100ms responsiveness even on
 * large documents.
 */

import type { Editor } from '@tiptap/core';
import DOMPurify from 'dompurify';
import hljs from 'highlight.js';
import { Marked } from 'marked';
import { htmlToSimpleMarkdown } from './source-mode';

export type ViewMode = 'edit' | 'split' | 'preview';

const STORAGE_KEY = 'colason.viewMode';
const RENDER_DEBOUNCE_MS = 60; // <100ms target

let currentMode: ViewMode = 'edit';
let previewEl: HTMLElement | null = null;
let editorWrapperEl: HTMLElement | null = null;
let layoutEl: HTMLElement | null = null;
let toolbarEl: HTMLElement | null = null;
let editorRef: Editor | null = null;
let renderTimer: number | null = null;
let scrollSyncing = false;

// === Markdown renderer (CommonMark + GFM, syntax-highlighted) ==============

const previewMarked = new Marked({
  gfm: true,
  breaks: false,
  async: false,
  renderer: {
    code(token) {
      const lang = (token.lang || '').trim().split(/\s+/)[0] || '';
      const code = token.text || '';
      let highlighted: string;
      if (lang && hljs.getLanguage(lang)) {
        try {
          highlighted = hljs.highlight(code, { language: lang, ignoreIllegals: true }).value;
        } catch {
          highlighted = escapeHtml(code);
        }
      } else {
        try {
          highlighted = hljs.highlightAuto(code).value;
        } catch {
          highlighted = escapeHtml(code);
        }
      }
      const langClass = lang ? ` language-${escapeAttr(lang)}` : '';
      return `<pre class="hljs-pre"><code class="hljs${langClass}">${highlighted}</code></pre>\n`;
    },
  },
});

function escapeHtml(s: string): string {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function escapeAttr(s: string): string {
  return s.replace(/[^a-zA-Z0-9_\-]/g, '');
}

function renderMarkdownToSafeHtml(md: string): string {
  const raw = previewMarked.parse(md) as string;
  // DOMPurify keeps the highlight classes; we restrict to a safe subset.
  return DOMPurify.sanitize(raw, {
    USE_PROFILES: { html: true },
    ADD_ATTR: ['data-heading-index'],
    FORBID_TAGS: ['style', 'script', 'iframe', 'object', 'embed'],
    FORBID_ATTR: ['onerror', 'onload', 'onclick', 'onmouseover'],
  });
}

// === Public API ============================================================

export function initPreviewMode(editor: Editor) {
  editorRef = editor;

  const editorContainer = document.getElementById('editor');
  if (!editorContainer) {
    console.warn('[Colason] preview-mode: #editor not found');
    return;
  }

  buildLayout(editorContainer);
  buildToolbar();
  attachShortcuts();
  attachEditorUpdateHook(editor);
  attachScrollSync();

  const saved = loadSavedMode();
  applyMode(saved);
}

export function setViewMode(mode: ViewMode) {
  applyMode(mode);
}

export function getViewMode(): ViewMode {
  return currentMode;
}

// === Layout ================================================================

function buildLayout(editorContainer: HTMLElement) {
  // Wrap #editor inside a layout container without disturbing existing CSS that
  // styles #editor / .tiptap. We move #editor under a wrapper, then put the
  // preview pane next to it.
  const parent = editorContainer.parentElement;
  if (!parent) return;

  layoutEl = document.createElement('div');
  layoutEl.id = 'colason-layout';
  layoutEl.dataset.viewMode = 'edit';

  editorWrapperEl = document.createElement('div');
  editorWrapperEl.id = 'colason-editor-pane';

  previewEl = document.createElement('div');
  previewEl.id = 'colason-preview-pane';
  previewEl.setAttribute('aria-label', 'Markdown preview');

  parent.insertBefore(layoutEl, editorContainer);
  editorWrapperEl.appendChild(editorContainer);
  layoutEl.appendChild(editorWrapperEl);
  layoutEl.appendChild(previewEl);
}

function buildToolbar() {
  toolbarEl = document.createElement('div');
  toolbarEl.id = 'colason-view-toolbar';
  toolbarEl.setAttribute('role', 'toolbar');
  toolbarEl.setAttribute('aria-label', 'View mode');

  const modes: Array<{ id: ViewMode; label: string; title: string }> = [
    { id: 'edit', label: 'Edit', title: 'Edit only (Ctrl/Cmd+1)' },
    { id: 'split', label: 'Split', title: 'Editor + Preview (Ctrl/Cmd+2)' },
    { id: 'preview', label: 'Preview', title: 'Preview only (Ctrl/Cmd+3)' },
  ];

  for (const m of modes) {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'colason-view-btn';
    btn.dataset.mode = m.id;
    btn.title = m.title;
    btn.textContent = m.label;
    btn.addEventListener('click', () => applyMode(m.id));
    toolbarEl.appendChild(btn);
  }

  document.body.appendChild(toolbarEl);
}

function applyMode(mode: ViewMode) {
  currentMode = mode;

  if (layoutEl) {
    layoutEl.dataset.viewMode = mode;
  }
  if (toolbarEl) {
    for (const btn of Array.from(toolbarEl.querySelectorAll<HTMLButtonElement>('.colason-view-btn'))) {
      btn.classList.toggle('is-active', btn.dataset.mode === mode);
      btn.setAttribute('aria-pressed', btn.dataset.mode === mode ? 'true' : 'false');
    }
  }

  if (mode !== 'edit') {
    renderPreviewNow();
  }

  saveMode(mode);
}

// === Render pipeline =======================================================

function attachEditorUpdateHook(editor: Editor) {
  editor.on('update', () => {
    if (currentMode === 'edit') return;
    schedulePreviewRender();
  });
}

function schedulePreviewRender() {
  if (renderTimer !== null) {
    window.clearTimeout(renderTimer);
  }
  renderTimer = window.setTimeout(() => {
    renderTimer = null;
    renderPreviewNow();
  }, RENDER_DEBOUNCE_MS);
}

function renderPreviewNow() {
  if (!previewEl || !editorRef) return;

  let md: string;
  try {
    // Convert TipTap HTML -> markdown via the existing serializer, then
    // re-parse with marked. This keeps the preview in sync with the WYSIWYG
    // representation (round-trip stable for supported syntax).
    md = htmlToSimpleMarkdown(editorRef.getHTML());
  } catch (err) {
    md = '';
    console.warn('[Colason] preview serialize failed', err);
  }

  let safeHtml: string;
  try {
    safeHtml = renderMarkdownToSafeHtml(md);
  } catch (err) {
    safeHtml = `<p class="preview-error">Preview render failed: ${escapeHtml(String(err))}</p>`;
  }

  previewEl.innerHTML = safeHtml;
  tagPreviewHeadings();
}

function tagPreviewHeadings() {
  if (!previewEl) return;
  const headings = previewEl.querySelectorAll<HTMLHeadingElement>('h1, h2, h3, h4, h5, h6');
  headings.forEach((h, idx) => {
    h.setAttribute('data-heading-index', String(idx));
  });
}

// === Keyboard shortcuts ====================================================

function attachShortcuts() {
  document.addEventListener('keydown', (e) => {
    if (!(e.ctrlKey || e.metaKey)) return;
    if (e.shiftKey || e.altKey) return;

    if (e.key === '1') {
      e.preventDefault();
      applyMode('edit');
    } else if (e.key === '2') {
      e.preventDefault();
      applyMode('split');
    } else if (e.key === '3') {
      e.preventDefault();
      applyMode('preview');
    }
  });
}

// === Scroll sync (Split mode, heading-anchored) ============================

function attachScrollSync() {
  if (!editorRef || !previewEl) return;
  const editorScroller = getEditorScroller();
  if (!editorScroller) return;

  editorScroller.addEventListener('scroll', () => {
    if (currentMode !== 'split' || scrollSyncing) return;
    syncFromEditorToPreview();
  }, { passive: true });

  previewEl.addEventListener('scroll', () => {
    if (currentMode !== 'split' || scrollSyncing) return;
    syncFromPreviewToEditor();
  }, { passive: true });
}

function getEditorScroller(): HTMLElement | null {
  // The editor container itself is rarely the scroller; the page (html/body)
  // typically scrolls. We pick whichever ancestor has overflow.
  if (!editorRef) return null;
  let el: HTMLElement | null = editorRef.view.dom;
  while (el) {
    const style = getComputedStyle(el);
    if (/(auto|scroll)/.test(style.overflowY)) return el;
    el = el.parentElement;
  }
  return document.scrollingElement as HTMLElement | null;
}

function getEditorHeadings(): HTMLElement[] {
  if (!editorRef) return [];
  return Array.from(
    editorRef.view.dom.querySelectorAll<HTMLElement>('h1, h2, h3, h4, h5, h6')
  );
}

function getPreviewHeadings(): HTMLElement[] {
  if (!previewEl) return [];
  return Array.from(previewEl.querySelectorAll<HTMLElement>('h1, h2, h3, h4, h5, h6'));
}

/**
 * Find the active heading: the last heading whose top is at or above the
 * scroller's reference line. Returns the heading index plus the position
 * ratio between the active heading and the next one (0..1).
 */
function findActiveHeading(headings: HTMLElement[], scrollerTop: number): { index: number; ratio: number } {
  if (headings.length === 0) return { index: -1, ratio: 0 };
  let active = -1;
  for (let i = 0; i < headings.length; i++) {
    const top = headings[i].getBoundingClientRect().top - scrollerTop;
    if (top <= 0) active = i;
    else break;
  }
  if (active < 0) return { index: -1, ratio: 0 };

  const cur = headings[active].getBoundingClientRect().top - scrollerTop;
  const next = active + 1 < headings.length
    ? headings[active + 1].getBoundingClientRect().top - scrollerTop
    : null;
  const ratio = next !== null && next > cur ? Math.min(1, Math.max(0, (0 - cur) / (next - cur))) : 0;
  return { index: active, ratio };
}

function syncFromEditorToPreview() {
  if (!previewEl) return;
  const editorScroller = getEditorScroller();
  if (!editorScroller) return;

  const editorHeadings = getEditorHeadings();
  const previewHeadings = getPreviewHeadings();
  if (editorHeadings.length === 0 || previewHeadings.length === 0) {
    fallbackRatioSync(editorScroller, previewEl);
    return;
  }

  const scrollerTop = editorScroller.getBoundingClientRect().top;
  const { index, ratio } = findActiveHeading(editorHeadings, scrollerTop);
  if (index < 0) {
    previewEl.scrollTop = 0;
    return;
  }
  const clampedIdx = Math.min(index, previewHeadings.length - 1);
  const previewScrollerTop = previewEl.getBoundingClientRect().top;
  const cur = previewHeadings[clampedIdx].getBoundingClientRect().top - previewScrollerTop + previewEl.scrollTop;
  const nextIdx = Math.min(clampedIdx + 1, previewHeadings.length - 1);
  const next = clampedIdx === nextIdx
    ? cur
    : previewHeadings[nextIdx].getBoundingClientRect().top - previewScrollerTop + previewEl.scrollTop;

  const target = cur + (next - cur) * ratio;
  scrollSyncing = true;
  previewEl.scrollTop = target;
  requestAnimationFrame(() => { scrollSyncing = false; });
}

function syncFromPreviewToEditor() {
  if (!previewEl) return;
  const editorScroller = getEditorScroller();
  if (!editorScroller) return;

  const editorHeadings = getEditorHeadings();
  const previewHeadings = getPreviewHeadings();
  if (editorHeadings.length === 0 || previewHeadings.length === 0) {
    fallbackRatioSync(previewEl, editorScroller);
    return;
  }

  const previewScrollerTop = previewEl.getBoundingClientRect().top;
  const { index, ratio } = findActiveHeading(previewHeadings, previewScrollerTop);
  if (index < 0) {
    scrollSyncing = true;
    editorScroller.scrollTop = 0;
    requestAnimationFrame(() => { scrollSyncing = false; });
    return;
  }
  const clampedIdx = Math.min(index, editorHeadings.length - 1);
  const editorScrollerTop = editorScroller.getBoundingClientRect().top;
  const cur = editorHeadings[clampedIdx].getBoundingClientRect().top - editorScrollerTop + editorScroller.scrollTop;
  const nextIdx = Math.min(clampedIdx + 1, editorHeadings.length - 1);
  const next = clampedIdx === nextIdx
    ? cur
    : editorHeadings[nextIdx].getBoundingClientRect().top - editorScrollerTop + editorScroller.scrollTop;

  const target = cur + (next - cur) * ratio;
  scrollSyncing = true;
  editorScroller.scrollTop = target;
  requestAnimationFrame(() => { scrollSyncing = false; });
}

function fallbackRatioSync(from: HTMLElement, to: HTMLElement) {
  const fromMax = Math.max(1, from.scrollHeight - from.clientHeight);
  const toMax = Math.max(1, to.scrollHeight - to.clientHeight);
  const r = from.scrollTop / fromMax;
  scrollSyncing = true;
  to.scrollTop = toMax * r;
  requestAnimationFrame(() => { scrollSyncing = false; });
}

// === Persistence ===========================================================

function loadSavedMode(): ViewMode {
  try {
    const v = localStorage.getItem(STORAGE_KEY);
    if (v === 'edit' || v === 'split' || v === 'preview') return v;
  } catch {
    /* localStorage unavailable */
  }
  return 'edit';
}

function saveMode(mode: ViewMode) {
  try {
    localStorage.setItem(STORAGE_KEY, mode);
  } catch {
    /* localStorage unavailable */
  }
}
