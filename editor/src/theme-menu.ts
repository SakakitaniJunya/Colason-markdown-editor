/**
 * Theme menu UI (Issue #4)
 *
 * Renders a small "View > Theme" picker. Designed to be merge-compatible
 * with the menu-bar introduced by Issue #2 / PR #11:
 *
 *   - When `mountMenuBar()` from `./menu-bar` is present at runtime, the
 *     editor entrypoint can call `buildViewThemeMenuItems()` and inject
 *     them as a "View > Theme" submenu (see helper export below).
 *   - When the menu-bar is absent (current main, before PR #11 lands),
 *     `mountStandaloneThemePicker()` renders an unobtrusive floating
 *     button with a dropdown containing the same options.
 *
 * Either path drives the same `theme-controller` API, so the resulting
 * behaviour is identical: localStorage persistence, OS-follow for system
 * mode, and `data-theme` attribute on `<html>`.
 */

import {
  getThemeMode,
  setThemeMode,
  THEME_LABELS,
  type ThemeMode,
} from './theme-controller';

const STYLE_ID = 'colason-theme-menu-style';
const PICKER_ID = 'colason-theme-picker';

const STYLE = `
#${PICKER_ID} {
  position: fixed;
  top: 12px;
  right: 16px;
  z-index: 10001;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
  font-size: 12px;
  user-select: none;
}
/* When the preview-mode toolbar (PR #12) is present, shift the theme
 * picker to the left so the two controls don't overlap.
 */
body:has(#colason-view-toolbar) #${PICKER_ID} {
  right: calc(16px + 220px);
}
#${PICKER_ID} .colason-theme-btn {
  appearance: none;
  background: var(--surface, #f6f8fa);
  color: var(--fg, #333333);
  border: 1px solid var(--border, #e1e4e8);
  border-radius: 8px;
  padding: 6px 12px;
  font: inherit;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 6px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.06);
}
#${PICKER_ID} .colason-theme-btn:hover {
  background: var(--surface-alt, #f8f9fa);
}
#${PICKER_ID} .colason-theme-btn:focus-visible {
  outline: 2px solid var(--link, #4183c4);
  outline-offset: 1px;
}
#${PICKER_ID} .colason-theme-caret {
  font-size: 10px;
  opacity: 0.6;
}
#${PICKER_ID} .colason-theme-dropdown {
  position: absolute;
  top: calc(100% + 4px);
  right: 0;
  min-width: 160px;
  background: var(--bg, #ffffff);
  color: var(--fg, #333333);
  border: 1px solid var(--border, #e1e4e8);
  border-radius: 8px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
  padding: 4px 0;
  display: none;
}
#${PICKER_ID}[aria-expanded='true'] .colason-theme-dropdown {
  display: block;
}
#${PICKER_ID} .colason-theme-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  background: transparent;
  color: inherit;
  border: none;
  padding: 6px 14px;
  font: inherit;
  text-align: left;
  cursor: pointer;
}
#${PICKER_ID} .colason-theme-item:hover {
  background: var(--surface-alt, #f8f9fa);
}
#${PICKER_ID} .colason-theme-item[aria-checked='true']::after {
  content: '\\2713';
  margin-left: 12px;
  color: var(--link, #4183c4);
}
`;

function ensureStyle(): void {
  if (document.getElementById(STYLE_ID)) return;
  const el = document.createElement('style');
  el.id = STYLE_ID;
  el.textContent = STYLE;
  document.head.appendChild(el);
}

const ORDER: ReadonlyArray<ThemeMode> = ['light', 'dark', 'sepia', 'system'];

/**
 * Items intended to be spread into the existing menu-bar's makeMenu() helper
 * (Issue #2 / PR #11). Re-exported so the entrypoint can wire it once both
 * features land in the same branch.
 */
export interface ThemeMenuItem {
  label: string;
  mode: ThemeMode;
  onClick: () => void;
}

export function buildViewThemeMenuItems(): ThemeMenuItem[] {
  return ORDER.map((mode) => ({
    mode,
    label: THEME_LABELS[mode],
    onClick: () => setThemeMode(mode),
  }));
}

/**
 * Standalone floating picker — used when no host menu-bar is present.
 * Idempotent: calling twice is a no-op.
 */
export function mountStandaloneThemePicker(): HTMLElement | null {
  if (typeof document === 'undefined') return null;
  const existing = document.getElementById(PICKER_ID);
  if (existing) return existing;

  ensureStyle();

  const wrap = document.createElement('div');
  wrap.id = PICKER_ID;
  wrap.setAttribute('aria-expanded', 'false');

  const btn = document.createElement('button');
  btn.type = 'button';
  btn.className = 'colason-theme-btn';
  btn.setAttribute('aria-haspopup', 'menu');
  btn.setAttribute('aria-expanded', 'false');

  const labelSpan = document.createElement('span');
  labelSpan.className = 'colason-theme-label';
  const caret = document.createElement('span');
  caret.className = 'colason-theme-caret';
  caret.textContent = '\u25BE';
  btn.appendChild(labelSpan);
  btn.appendChild(caret);

  const dropdown = document.createElement('div');
  dropdown.className = 'colason-theme-dropdown';
  dropdown.setAttribute('role', 'menu');

  const items = new Map<ThemeMode, HTMLButtonElement>();
  for (const mode of ORDER) {
    const item = document.createElement('button');
    item.type = 'button';
    item.className = 'colason-theme-item';
    item.setAttribute('role', 'menuitemradio');
    item.dataset.mode = mode;
    const text = document.createElement('span');
    text.textContent = THEME_LABELS[mode];
    item.appendChild(text);
    item.addEventListener('click', () => {
      setThemeMode(mode);
      close();
    });
    dropdown.appendChild(item);
    items.set(mode, item);
  }

  wrap.appendChild(btn);
  wrap.appendChild(dropdown);
  document.body.appendChild(wrap);

  function refreshActive(): void {
    const active = getThemeMode();
    labelSpan.textContent = `Theme: ${THEME_LABELS[active]}`;
    for (const [mode, el] of items) {
      el.setAttribute('aria-checked', mode === active ? 'true' : 'false');
    }
  }

  function open(): void {
    wrap.setAttribute('aria-expanded', 'true');
    btn.setAttribute('aria-expanded', 'true');
  }
  function close(): void {
    wrap.setAttribute('aria-expanded', 'false');
    btn.setAttribute('aria-expanded', 'false');
  }

  btn.addEventListener('click', (event) => {
    event.stopPropagation();
    if (wrap.getAttribute('aria-expanded') === 'true') close();
    else open();
  });

  document.addEventListener('click', (event) => {
    if (!wrap.contains(event.target as Node)) close();
  });

  document.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') close();
  });

  document.addEventListener('colason:themechange', () => refreshActive());
  refreshActive();

  return wrap;
}
