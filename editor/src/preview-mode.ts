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
 * syntax-highlighted via highlight.js. Math expressions delimited by
 * `$inline$` / `$$display$$` are rendered with KaTeX via
 * `marked-katex-extension` (errors fall back to a readable inline message).
 *
 * ` ```mermaid ` code fences are rendered as SVG diagrams via Mermaid.
 * The mermaid library (~600KB) is dynamically imported on first use to
 * keep the initial bundle lean; rendering happens asynchronously after the
 * preview HTML is committed, and parse errors fall back to the original
 * code block (highlight.js styled) so the document stays readable.
 *
 * The preview is debounced to ~60ms for sub-100ms responsiveness even on
 * large documents.
 */

import type { Editor } from '@tiptap/core';
import DOMPurify from 'dompurify';
import hljs from 'highlight.js';
import 'katex/dist/katex.css';
import { Marked } from 'marked';
import markedKatex from 'marked-katex-extension';
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

// === Mermaid pipeline =====================================================
// We collect mermaid code per render pass and replace placeholders after
// the (sanitized) HTML is committed. Tracking via a render-pass token lets
// us cancel stale async renders if the user keeps typing.

interface MermaidJob {
  id: string;       // DOM element id of the placeholder
  code: string;     // raw diagram source
}

let mermaidPass = 0;
let mermaidJobs: MermaidJob[] = [];
let mermaidIdSeq = 0;
let mermaidLib: typeof import('mermaid').default | null = null;
let mermaidLoading: Promise<typeof import('mermaid').default> | null = null;
let mermaidCurrentTheme = '';

// === Markdown renderer (CommonMark + GFM, syntax-highlighted) ==============

const previewMarked = new Marked({
  gfm: true,
  breaks: false,
  async: false,
  renderer: {
    code(token) {
      const lang = (token.lang || '').trim().split(/\s+/)[0] || '';
      const code = token.text || '';

      // Mermaid: emit a placeholder; the real SVG is filled in asynchronously
      // by `processMermaidJobs()` after DOMPurify sanitizes the HTML. We keep
      // the raw source on the placeholder so a render error can fall back to
      // the original code block.
      if (lang === 'mermaid') {
        const id = `colason-mermaid-${++mermaidIdSeq}`;
        mermaidJobs.push({ id, code });
        return `<div id="${id}" class="mermaid-rendered" data-mermaid-pending="1">`
          + `<pre class="hljs-pre mermaid-source-fallback"><code class="hljs language-mermaid">${escapeHtml(code)}</code></pre>`
          + `</div>\n`;
      }

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

// Wire KaTeX into marked.
//   - throwOnError: false  -> KaTeX renders an error span instead of throwing,
//     so a malformed expression degrades gracefully without breaking the rest
//     of the preview.
//   - output: 'html'       -> emits HTML+CSS only (no MathML), which keeps the
//     DOM smaller for documents with 100+ formulas.
previewMarked.use(
  markedKatex({
    throwOnError: false,
    output: 'html',
  }),
);

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
  // KaTeX emits spans with inline `style` (for spacing) and `aria-hidden`
  // attributes; both are allowed by DOMPurify's html profile by default,
  // but we add them explicitly so the policy is self-documenting.
  return DOMPurify.sanitize(raw, {
    USE_PROFILES: { html: true },
    ADD_ATTR: ['data-heading-index', 'aria-hidden', 'style'],
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

  // Reset mermaid job queue for this pass.
  mermaidJobs = [];
  const passToken = ++mermaidPass;

  let safeHtml: string;
  try {
    safeHtml = renderMarkdownToSafeHtml(md);
  } catch (err) {
    safeHtml = `<p class="preview-error">Preview render failed: ${escapeHtml(String(err))}</p>`;
  }

  previewEl.innerHTML = safeHtml;
  tagPreviewHeadings();

  if (mermaidJobs.length > 0) {
    // Don't await — let the preview paint immediately with the fallback
    // code blocks; SVGs swap in when ready.
    void processMermaidJobs(mermaidJobs.slice(), passToken);
  }
}

function tagPreviewHeadings() {
  if (!previewEl) return;
  const headings = previewEl.querySelectorAll<HTMLHeadingElement>('h1, h2, h3, h4, h5, h6');
  headings.forEach((h, idx) => {
    h.setAttribute('data-heading-index', String(idx));
  });
}

// === Mermaid rendering =====================================================

/**
 * Lazy-load mermaid and render every queued diagram for this pass. If the
 * pass is superseded (user kept typing) we abort to avoid trampling a newer
 * preview. Errors are kept inline as the fallback code block, so the AC
 * "error -> show original code" is satisfied without DOM rewrites.
 */
async function processMermaidJobs(jobs: MermaidJob[], passToken: number) {
  let lib: typeof import('mermaid').default;
  try {
    lib = await loadMermaid();
  } catch (err) {
    console.warn('[Colason] mermaid lazy-load failed', err);
    return; // Fallback code blocks are already in place.
  }
  if (passToken !== mermaidPass || !previewEl) return;

  syncMermaidTheme(lib);

  for (const job of jobs) {
    if (passToken !== mermaidPass || !previewEl) return;
    const host = previewEl.querySelector<HTMLElement>(`#${cssEscape(job.id)}`);
    if (!host) continue;

    // mermaid.parse() throws on syntax errors; do a check first so we don't
    // even attempt render() on bad input.
    try {
      // parse() returns void/true; throws on error.
      // suppressErrors:true returns false instead of throwing — we want the throw
      // path to keep the fallback in place uniformly.
      await lib.parse(job.code);
    } catch (err) {
      markMermaidFallback(host, err);
      continue;
    }

    const renderId = `colason-mermaid-svg-${job.id}`;
    try {
      const { svg, bindFunctions } = await lib.render(renderId, job.code);
      if (passToken !== mermaidPass) return;
      // Replace the fallback code block with the SVG. We keep the host element
      // so heading indices and surrounding CSS aren't disturbed.
      host.innerHTML = svg;
      host.removeAttribute('data-mermaid-pending');
      host.setAttribute('data-mermaid-rendered', '1');
      if (typeof bindFunctions === 'function') {
        try { bindFunctions(host); } catch { /* non-fatal */ }
      }
    } catch (err) {
      markMermaidFallback(host, err);
    } finally {
      // mermaid v11 leaves temporary nodes on document.body; clean up.
      const stray = document.getElementById(renderId);
      if (stray && stray.parentElement === document.body) stray.remove();
    }
  }
}

function markMermaidFallback(host: HTMLElement, err: unknown) {
  // The fallback `<pre><code>` block was already rendered inside `host` at
  // marked-time; we just annotate state and surface a small error caption.
  host.removeAttribute('data-mermaid-pending');
  host.setAttribute('data-mermaid-error', '1');
  if (!host.querySelector('.mermaid-error-caption')) {
    const caption = document.createElement('div');
    caption.className = 'mermaid-error-caption';
    caption.textContent = `Mermaid render error: ${String((err as Error)?.message ?? err)}`;
    host.insertBefore(caption, host.firstChild);
  }
}

function loadMermaid(): Promise<typeof import('mermaid').default> {
  if (mermaidLib) return Promise.resolve(mermaidLib);
  if (mermaidLoading) return mermaidLoading;
  mermaidLoading = import('mermaid').then((mod) => {
    mermaidLib = mod.default;
    return mermaidLib;
  });
  return mermaidLoading;
}

function syncMermaidTheme(lib: typeof import('mermaid').default) {
  // Match the editor-side MermaidBlock heuristic: sample the CSS --bg
  // variable; if it's dark, use the 'dark' theme. The PR #13 theme controller
  // updates --bg on theme change, so this stays in sync automatically.
  const bg = getComputedStyle(document.documentElement).getPropertyValue('--bg').trim();
  const theme = isColorDark(bg) ? 'dark' : 'default';
  if (theme === mermaidCurrentTheme) return;
  mermaidCurrentTheme = theme;
  lib.initialize({
    startOnLoad: false,
    theme,
    securityLevel: 'strict', // preview is read-only; lock down click handlers
    fontFamily: 'inherit',
    flowchart: { htmlLabels: true, curve: 'basis' },
  });
}

function isColorDark(color: string): boolean {
  if (!color) return false;
  let r = 0, g = 0, b = 0;
  const hex = color.startsWith('#') ? color.slice(1) : '';
  if (hex.length === 3) {
    r = parseInt(hex[0] + hex[0], 16);
    g = parseInt(hex[1] + hex[1], 16);
    b = parseInt(hex[2] + hex[2], 16);
  } else if (hex.length >= 6) {
    r = parseInt(hex.slice(0, 2), 16);
    g = parseInt(hex.slice(2, 4), 16);
    b = parseInt(hex.slice(4, 6), 16);
  } else {
    // rgb(...) form
    const m = color.match(/rgba?\(([^)]+)\)/i);
    if (m) {
      const parts = m[1].split(',').map(s => parseFloat(s.trim()));
      r = parts[0] || 0;
      g = parts[1] || 0;
      b = parts[2] || 0;
    }
  }
  const luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255;
  return luminance < 0.5;
}

function cssEscape(s: string): string {
  // CSS.escape is widely available; fall back to a minimal escape for
  // older runtimes (Qt WebEngine 5.15 supports CSS.escape).
  if (typeof CSS !== 'undefined' && typeof CSS.escape === 'function') {
    return CSS.escape(s);
  }
  return s.replace(/[^a-zA-Z0-9_-]/g, '\\$&');
}

/**
 * Force a re-render of the preview (used when the global theme changes so
 * mermaid can re-initialize with the new colors). Safe to call from outside.
 */
export function refreshPreviewForThemeChange() {
  // Resetting the cached theme makes the next render recompute it.
  mermaidCurrentTheme = '';
  if (currentMode !== 'edit') {
    renderPreviewNow();
  }
}

/**
 * Render markdown to a self-contained, sanitized HTML string with all
 * Mermaid diagrams already replaced by inline SVG. Used by the export
 * pipeline so that exported HTML/PDF includes the rendered diagrams
 * regardless of the current view mode.
 *
 * Returns a Promise because mermaid is loaded lazily and renders async.
 * Mermaid render errors fall back to a `<pre><code class="language-mermaid">`
 * block (same fallback as the live preview).
 */
export async function renderMarkdownForExport(md: string): Promise<string> {
  // Use a fresh job queue so a concurrent live-preview render isn't disturbed.
  const savedJobs = mermaidJobs;
  const savedPass = mermaidPass;
  mermaidJobs = [];
  const myPass = ++mermaidPass;

  let html: string;
  try {
    html = renderMarkdownToSafeHtml(md);
  } catch (err) {
    mermaidJobs = savedJobs;
    mermaidPass = savedPass;
    return `<p class="preview-error">Export render failed: ${escapeHtml(String(err))}</p>`;
  }

  const jobs = mermaidJobs.slice();
  mermaidJobs = savedJobs;
  // Keep mermaidPass moving forward so live-preview cancellation logic is happy.

  if (jobs.length === 0) return html;

  let lib: typeof import('mermaid').default;
  try {
    lib = await loadMermaid();
  } catch {
    return html; // SVGs missing, but fallback <pre><code> is already in HTML.
  }
  syncMermaidTheme(lib);

  // Render each diagram into a detached container so we can swap the
  // placeholder by id into the HTML string.
  const tmp = document.createElement('div');
  tmp.style.position = 'absolute';
  tmp.style.left = '-99999px';
  tmp.innerHTML = html;
  for (const job of jobs) {
    const host = tmp.querySelector<HTMLElement>(`#${cssEscape(job.id)}`);
    if (!host) continue;
    try {
      await lib.parse(job.code);
      const renderId = `colason-export-mermaid-${myPass}-${job.id}`;
      const { svg } = await lib.render(renderId, job.code);
      host.innerHTML = svg;
      host.removeAttribute('data-mermaid-pending');
      host.setAttribute('data-mermaid-rendered', '1');
      const stray = document.getElementById(renderId);
      if (stray && stray.parentElement === document.body) stray.remove();
    } catch (err) {
      markMermaidFallback(host, err);
    }
  }
  return tmp.innerHTML;
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
