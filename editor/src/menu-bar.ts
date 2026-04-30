/**
 * Web-side File menu and Settings dialog (Issue #2).
 *
 * Rendered into the page so the editor exposes Open / Save / Save As without
 * relying on the Qt native menubar — useful when running the editor inside a
 * non-Qt host (Electron preload, Tauri, plain browser, dev mode).
 */

import { FileManager } from './file-manager';
import { getSettings, updateSettings } from './settings';

const STYLE_ID = 'colason-menu-bar-style';

const STYLE = `
.colason-menubar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 32px;
  display: flex;
  align-items: center;
  padding: 0 12px;
  gap: 8px;
  background: rgba(255, 255, 255, 0.92);
  backdrop-filter: blur(8px);
  border-bottom: 1px solid rgba(0, 0, 0, 0.08);
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  font-size: 12px;
  color: #333;
  z-index: 9000;
  user-select: none;
}
.colason-menubar__title {
  flex: 1;
  text-align: center;
  font-weight: 500;
  letter-spacing: 0.02em;
}
.colason-menubar__menu {
  position: relative;
}
.colason-menubar__btn {
  background: transparent;
  border: none;
  padding: 4px 10px;
  border-radius: 4px;
  cursor: pointer;
  font: inherit;
  color: inherit;
}
.colason-menubar__btn:hover,
.colason-menubar__btn[aria-expanded="true"] {
  background: rgba(0, 0, 0, 0.06);
}
.colason-menubar__dropdown {
  position: absolute;
  top: calc(100% + 4px);
  left: 0;
  min-width: 220px;
  background: #fff;
  border: 1px solid rgba(0, 0, 0, 0.1);
  border-radius: 6px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
  padding: 4px 0;
  display: none;
  z-index: 9001;
}
.colason-menubar__menu[aria-expanded="true"] .colason-menubar__dropdown {
  display: block;
}
.colason-menubar__item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  background: transparent;
  border: none;
  padding: 6px 14px;
  font: inherit;
  color: inherit;
  text-align: left;
  cursor: pointer;
}
.colason-menubar__item:hover {
  background: rgba(0, 0, 0, 0.06);
}
.colason-menubar__shortcut {
  margin-left: 24px;
  color: #888;
  font-size: 11px;
}
body.colason-has-menubar {
  padding-top: 32px;
}

/* Settings dialog */
.colason-settings-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.35);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9100;
}
.colason-settings-dialog {
  background: #fff;
  border-radius: 8px;
  padding: 20px 24px;
  min-width: 360px;
  max-width: 90vw;
  box-shadow: 0 12px 32px rgba(0, 0, 0, 0.2);
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  font-size: 14px;
  color: #222;
}
.colason-settings-dialog h2 {
  margin: 0 0 12px;
  font-size: 16px;
}
.colason-settings-dialog label {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 8px 0;
  cursor: pointer;
}
.colason-settings-dialog .colason-settings-row {
  margin: 12px 0;
}
.colason-settings-dialog input[type="number"] {
  width: 80px;
  padding: 4px 6px;
  font: inherit;
}
.colason-settings-actions {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  margin-top: 16px;
}
.colason-settings-actions button {
  padding: 6px 14px;
  border-radius: 4px;
  border: 1px solid rgba(0, 0, 0, 0.15);
  background: #fff;
  cursor: pointer;
  font: inherit;
}
.colason-settings-actions button.primary {
  background: #2b7cff;
  color: #fff;
  border-color: #2b7cff;
}

/* Recovery banner */
.colason-recovery-banner {
  position: fixed;
  top: 40px;
  left: 50%;
  transform: translateX(-50%);
  background: #fff8c5;
  border: 1px solid #d9c66e;
  border-radius: 6px;
  padding: 10px 14px;
  display: flex;
  align-items: center;
  gap: 12px;
  z-index: 9050;
  box-shadow: 0 6px 18px rgba(0, 0, 0, 0.12);
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  font-size: 13px;
}
.colason-recovery-banner button {
  padding: 4px 10px;
  border: 1px solid rgba(0, 0, 0, 0.15);
  background: #fff;
  border-radius: 4px;
  cursor: pointer;
  font: inherit;
}
.colason-recovery-banner button.primary {
  background: #2b7cff;
  color: #fff;
  border-color: #2b7cff;
}
`;

function ensureStyle() {
  if (document.getElementById(STYLE_ID)) return;
  const style = document.createElement('style');
  style.id = STYLE_ID;
  style.textContent = STYLE;
  document.head.appendChild(style);
}

interface MenuItemConfig {
  label: string;
  shortcut?: string;
  onClick: () => void;
}

function makeMenu(label: string, items: MenuItemConfig[]): HTMLElement {
  const wrapper = document.createElement('div');
  wrapper.className = 'colason-menubar__menu';
  wrapper.setAttribute('aria-expanded', 'false');

  const btn = document.createElement('button');
  btn.type = 'button';
  btn.className = 'colason-menubar__btn';
  btn.textContent = label;
  btn.setAttribute('aria-haspopup', 'true');
  btn.setAttribute('aria-expanded', 'false');
  wrapper.appendChild(btn);

  const dropdown = document.createElement('div');
  dropdown.className = 'colason-menubar__dropdown';
  dropdown.setAttribute('role', 'menu');

  for (const item of items) {
    const itemBtn = document.createElement('button');
    itemBtn.type = 'button';
    itemBtn.className = 'colason-menubar__item';
    itemBtn.setAttribute('role', 'menuitem');
    const text = document.createElement('span');
    text.textContent = item.label;
    itemBtn.appendChild(text);
    if (item.shortcut) {
      const sc = document.createElement('span');
      sc.className = 'colason-menubar__shortcut';
      sc.textContent = item.shortcut;
      itemBtn.appendChild(sc);
    }
    itemBtn.addEventListener('click', () => {
      closeMenu(wrapper, btn);
      try {
        item.onClick();
      } catch (err) {
        console.error('[Colason] menu item handler threw:', err);
      }
    });
    dropdown.appendChild(itemBtn);
  }

  wrapper.appendChild(dropdown);

  btn.addEventListener('click', (event) => {
    event.stopPropagation();
    const open = wrapper.getAttribute('aria-expanded') === 'true';
    document.querySelectorAll('.colason-menubar__menu[aria-expanded="true"]').forEach((el) => {
      el.setAttribute('aria-expanded', 'false');
      el.querySelector('.colason-menubar__btn')?.setAttribute('aria-expanded', 'false');
    });
    if (!open) {
      wrapper.setAttribute('aria-expanded', 'true');
      btn.setAttribute('aria-expanded', 'true');
    }
  });

  return wrapper;
}

function closeMenu(wrapper: HTMLElement, btn: HTMLElement) {
  wrapper.setAttribute('aria-expanded', 'false');
  btn.setAttribute('aria-expanded', 'false');
}

export function mountMenuBar(fileManager: FileManager): HTMLElement {
  ensureStyle();

  const bar = document.createElement('div');
  bar.className = 'colason-menubar';
  bar.setAttribute('role', 'menubar');

  const fileMenu = makeMenu('File', [
    { label: 'Open...', shortcut: shortcutLabel('O'), onClick: () => void fileManager.openFile() },
    { label: 'Save', shortcut: shortcutLabel('S'), onClick: () => void fileManager.save() },
    {
      label: 'Save As...',
      shortcut: shortcutLabel('S', true),
      onClick: () => void fileManager.saveAs(),
    },
  ]);

  const editMenu = makeMenu('Settings', [
    { label: 'Preferences...', onClick: () => openSettingsDialog() },
  ]);

  const titleEl = document.createElement('div');
  titleEl.className = 'colason-menubar__title';
  titleEl.textContent = 'Untitled.md';

  bar.appendChild(fileMenu);
  bar.appendChild(editMenu);
  bar.appendChild(titleEl);

  document.body.appendChild(bar);
  document.body.classList.add('colason-has-menubar');

  document.addEventListener('click', () => {
    document.querySelectorAll('.colason-menubar__menu[aria-expanded="true"]').forEach((el) => {
      el.setAttribute('aria-expanded', 'false');
      el.querySelector('.colason-menubar__btn')?.setAttribute('aria-expanded', 'false');
    });
  });

  fileManager.onChange((state) => {
    titleEl.textContent = `${state.dirty ? '* ' : ''}${state.fileName}`;
  });

  return bar;
}

function shortcutLabel(key: string, shift = false): string {
  const isMac = navigator.platform.toLowerCase().includes('mac');
  const mod = isMac ? '\u2318' : 'Ctrl+';
  const shiftSym = shift ? (isMac ? '\u21E7' : 'Shift+') : '';
  return `${mod}${shiftSym}${key}`;
}

// ---------------------------------------------------------------------------
// Settings dialog
// ---------------------------------------------------------------------------

function openSettingsDialog() {
  ensureStyle();
  const settings = getSettings();

  const overlay = document.createElement('div');
  overlay.className = 'colason-settings-overlay';

  const dialog = document.createElement('div');
  dialog.className = 'colason-settings-dialog';
  dialog.setAttribute('role', 'dialog');
  dialog.setAttribute('aria-modal', 'true');

  const heading = document.createElement('h2');
  heading.textContent = '設定 / Preferences';
  dialog.appendChild(heading);

  const autoSaveRow = document.createElement('div');
  autoSaveRow.className = 'colason-settings-row';
  const autoSaveLabel = document.createElement('label');
  const autoSaveInput = document.createElement('input');
  autoSaveInput.type = 'checkbox';
  autoSaveInput.checked = settings.autoSave;
  const autoSaveText = document.createElement('span');
  autoSaveText.textContent = '自動保存を有効にする (open 中のファイルを上書き保存)';
  autoSaveLabel.appendChild(autoSaveInput);
  autoSaveLabel.appendChild(autoSaveText);
  autoSaveRow.appendChild(autoSaveLabel);

  const intervalRow = document.createElement('div');
  intervalRow.className = 'colason-settings-row';
  const intervalLabel = document.createElement('label');
  intervalLabel.textContent = '自動保存間隔 (秒): ';
  const intervalInput = document.createElement('input');
  intervalInput.type = 'number';
  intervalInput.min = '1';
  intervalInput.max = '600';
  intervalInput.step = '1';
  intervalInput.value = String(Math.max(1, Math.round(settings.autoSaveIntervalMs / 1000)));
  intervalLabel.appendChild(intervalInput);
  intervalRow.appendChild(intervalLabel);

  dialog.appendChild(autoSaveRow);
  dialog.appendChild(intervalRow);

  const actions = document.createElement('div');
  actions.className = 'colason-settings-actions';
  const cancel = document.createElement('button');
  cancel.type = 'button';
  cancel.textContent = 'キャンセル';
  const save = document.createElement('button');
  save.type = 'button';
  save.className = 'primary';
  save.textContent = '保存';

  cancel.addEventListener('click', () => overlay.remove());
  save.addEventListener('click', () => {
    const intervalSec = Math.max(1, parseInt(intervalInput.value, 10) || 10);
    updateSettings({
      autoSave: autoSaveInput.checked,
      autoSaveIntervalMs: intervalSec * 1000,
    });
    overlay.remove();
  });

  actions.appendChild(cancel);
  actions.appendChild(save);
  dialog.appendChild(actions);

  overlay.appendChild(dialog);
  overlay.addEventListener('click', (e) => {
    if (e.target === overlay) overlay.remove();
  });
  document.body.appendChild(overlay);
}

// ---------------------------------------------------------------------------
// Recovery banner
// ---------------------------------------------------------------------------

export async function maybeOfferRecovery(fileManager: FileManager) {
  ensureStyle();
  let snapshots = await fileManager.listRecovery();
  // Ignore empty snapshots and snapshots from the *current* session (handled live).
  snapshots = snapshots.filter((s) => s.markdown && s.markdown.trim().length > 0);
  if (snapshots.length === 0) return;

  // Pick the most recent snapshot.
  snapshots.sort((a, b) => b.savedAt - a.savedAt);
  const latest = snapshots[0];

  const banner = document.createElement('div');
  banner.className = 'colason-recovery-banner';

  const text = document.createElement('span');
  const when = new Date(latest.savedAt);
  const label = latest.path ? basename(latest.path) : 'Untitled.md';
  text.textContent = `前回終了時の編集内容が見つかりました (${label} / ${when.toLocaleString()})`;
  banner.appendChild(text);

  const restore = document.createElement('button');
  restore.type = 'button';
  restore.className = 'primary';
  restore.textContent = '復元';
  restore.addEventListener('click', async () => {
    fileManager.loadMarkdown(latest.path, latest.markdown);
    await fileManager.dismissRecovery(latest.id);
    banner.remove();
  });

  const discard = document.createElement('button');
  discard.type = 'button';
  discard.textContent = '破棄';
  discard.addEventListener('click', async () => {
    await fileManager.dismissRecovery(latest.id);
    banner.remove();
  });

  banner.appendChild(restore);
  banner.appendChild(discard);
  document.body.appendChild(banner);
}

function basename(path: string): string {
  const cleaned = path.replace(/[\\/]+$/, '');
  const idx = Math.max(cleaned.lastIndexOf('/'), cleaned.lastIndexOf('\\'));
  return idx >= 0 ? cleaned.slice(idx + 1) : cleaned;
}
