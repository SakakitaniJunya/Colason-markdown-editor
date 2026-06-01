/**
 * Theme Controller (Issue #4)
 *
 * Manages 4 user-facing theme modes:
 *   - 'light'  : explicit light palette (default)
 *   - 'dark'   : explicit dark palette
 *   - 'sepia'  : warm paper palette for long-form writing
 *   - 'system' : follow the OS color-scheme preference (light or dark)
 *
 * Responsibilities:
 *   1. Persist the selected mode in localStorage and restore on init.
 *   2. Resolve 'system' through `matchMedia('(prefers-color-scheme: dark)')`
 *      and listen for OS-level changes while 'system' is active.
 *   3. Write the resolved theme name to `<html data-theme="...">` so all
 *      CSS variables defined in `themes/tokens.css` cascade through the
 *      editor and the preview pane.
 *   4. Swap the highlight.js stylesheet (`<link>` element with id
 *      `colason-hljs-theme`) so syntax-highlighted code blocks track
 *      the active theme. We use github.css for light/sepia and
 *      github-dark.css for dark.
 *   5. Dispatch a `colason:themechange` CustomEvent so other modules
 *      (preview, mermaid, katex) can re-render if needed.
 */

export type ThemeMode = 'light' | 'dark' | 'sepia' | 'system';
export type ResolvedTheme = 'light' | 'dark' | 'sepia';

const STORAGE_KEY = 'colason.theme';
const HLJS_LINK_ID = 'colason-hljs-theme';
const TRANSITION_CLASS = 'theme-transitioning';
const TRANSITION_MS = 220;

// highlight.js theme stylesheets shipped with the package.
// Vite resolves these to bundled URLs at build time.
import hljsLight from 'highlight.js/styles/github.css?url';
import hljsDark from 'highlight.js/styles/github-dark.css?url';

const VALID_MODES: ReadonlyArray<ThemeMode> = ['light', 'dark', 'sepia', 'system'];

function isValidMode(value: unknown): value is ThemeMode {
  return typeof value === 'string' && (VALID_MODES as ReadonlyArray<string>).includes(value);
}

let currentMode: ThemeMode = 'light';
let mediaQuery: MediaQueryList | null = null;
let mediaListenerAttached = false;

function loadStoredMode(): ThemeMode {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw && isValidMode(raw)) return raw;
  } catch {
    // localStorage may be unavailable (private mode, sandboxed iframe).
  }
  return 'light';
}

function persistMode(mode: ThemeMode): void {
  try {
    localStorage.setItem(STORAGE_KEY, mode);
  } catch {
    // Ignore — theme will simply not persist.
  }
}

function resolveMode(mode: ThemeMode): ResolvedTheme {
  if (mode === 'system') {
    if (!mediaQuery && typeof window !== 'undefined' && typeof window.matchMedia === 'function') {
      mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
    }
    return mediaQuery && mediaQuery.matches ? 'dark' : 'light';
  }
  return mode;
}

function ensureHljsLinkElement(): HTMLLinkElement {
  let link = document.getElementById(HLJS_LINK_ID) as HTMLLinkElement | null;
  if (!link) {
    link = document.createElement('link');
    link.id = HLJS_LINK_ID;
    link.rel = 'stylesheet';
    document.head.appendChild(link);
  }
  return link;
}

function applyHljsStylesheet(resolved: ResolvedTheme): void {
  const link = ensureHljsLinkElement();
  // Sepia uses the light hljs base; tokens.css overrides the colors so they
  // blend with the warm paper palette.
  const target = resolved === 'dark' ? hljsDark : hljsLight;
  if (link.href !== new URL(target, document.baseURI).href) {
    link.href = target;
  }
}

function applyTheme(mode: ThemeMode, opts: { animate?: boolean } = {}): void {
  const resolved = resolveMode(mode);
  const root = document.documentElement;

  if (opts.animate) {
    root.classList.add(TRANSITION_CLASS);
    window.setTimeout(() => root.classList.remove(TRANSITION_CLASS), TRANSITION_MS);
  }

  root.setAttribute('data-theme', resolved);
  root.setAttribute('data-theme-mode', mode);
  // `color-scheme` lets the UA style native form controls / scrollbars to
  // match the active theme without further CSS work.
  root.style.colorScheme = resolved === 'dark' ? 'dark' : 'light';

  applyHljsStylesheet(resolved);

  document.dispatchEvent(
    new CustomEvent<ThemeChangeDetail>('colason:themechange', {
      detail: { mode, resolved },
    }),
  );
}

function attachSystemListener(): void {
  if (mediaListenerAttached) return;
  if (!mediaQuery && typeof window !== 'undefined' && typeof window.matchMedia === 'function') {
    mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
  }
  if (!mediaQuery) return;

  const handler = () => {
    if (currentMode === 'system') {
      applyTheme('system', { animate: true });
    }
  };

  // Modern browsers: addEventListener; Safari < 14: addListener fallback.
  if (typeof mediaQuery.addEventListener === 'function') {
    mediaQuery.addEventListener('change', handler);
  } else if (typeof (mediaQuery as MediaQueryList).addListener === 'function') {
    (mediaQuery as MediaQueryList).addListener(handler);
  }
  mediaListenerAttached = true;
}

// === Public API ============================================================

export interface ThemeChangeDetail {
  mode: ThemeMode;
  resolved: ResolvedTheme;
}

export function initThemeController(): void {
  currentMode = loadStoredMode();
  attachSystemListener();
  applyTheme(currentMode, { animate: false });
}

export function getThemeMode(): ThemeMode {
  return currentMode;
}

export function getResolvedTheme(): ResolvedTheme {
  return resolveMode(currentMode);
}

export function setThemeMode(mode: ThemeMode): void {
  if (!isValidMode(mode)) {
    console.warn('[Colason] setThemeMode: invalid mode', mode);
    return;
  }
  if (mode === currentMode) return;
  currentMode = mode;
  persistMode(mode);
  applyTheme(mode, { animate: true });
}

/**
 * Convenience metadata for menu rendering. Keys match `ThemeMode`.
 */
export const THEME_LABELS: Readonly<Record<ThemeMode, string>> = {
  light: 'Light',
  dark: 'Dark',
  sepia: 'Sepia',
  system: 'System',
};
