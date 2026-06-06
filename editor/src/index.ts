import './themes/tokens.css';
import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';
import { initThemeController } from './theme-controller';
import { mountStandaloneThemePicker } from './theme-menu';
import { OutlineView } from './outline';

async function main() {
  // Apply the persisted theme before the editor is constructed so the
  // first paint already uses the correct palette (no light->dark flash).
  initThemeController();

  // Restore wide mode preference
  try {
    if (localStorage.getItem('colason.wideMode') === '1') {
      document.body.classList.add('wide-mode');
    }
  } catch {}

  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    console.error('[Colason] Editor element not found');
    return;
  }

  const editor = createEditor(editorElement);
  setupGlobalAPI(editor);
  await initBridge(editor);

  // Standalone Theme picker. When the menu-bar from PR #11 is merged,
  // its `makeMenu()` can consume `buildViewThemeMenuItems()` instead and
  // this floating fallback can be removed.
  mountStandaloneThemePicker();

  // Outline view (right sidebar) — heading 構造から TOC をリアルタイム生成
  document.body.classList.add('colason-has-outline');
  const outlineHost = document.createElement('div');
  document.body.appendChild(outlineHost);
  const outlineView = new OutlineView(outlineHost, editor);

  // TipTap の transaction subscribe でリアルタイム更新
  editor.on('update', () => outlineView.update());

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
