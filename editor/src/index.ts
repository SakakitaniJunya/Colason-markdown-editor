import './themes/tokens.css';
import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';
import { initThemeController } from './theme-controller';
import { mountStandaloneThemePicker } from './theme-menu';

async function main() {
  // Apply the persisted theme before the editor is constructed so the
  // first paint already uses the correct palette (no light->dark flash).
  initThemeController();

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

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
