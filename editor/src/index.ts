import './themes/preview.css';
import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';
import { initPreviewMode, refreshPreviewForThemeChange } from './preview-mode';

async function main() {
  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    console.error('[Colason] Editor element not found');
    return;
  }

  const editor = createEditor(editorElement);
  setupGlobalAPI(editor);
  await initBridge(editor);
  initPreviewMode(editor);

  // Re-render the preview (and re-init mermaid colors) whenever the global
  // theme changes. The theme-controller (PR #13) dispatches this event;
  // we also watch <html data-theme> mutations so this works even when the
  // theme is toggled via DevTools or by other code paths.
  document.addEventListener('colason:theme-change', () => refreshPreviewForThemeChange());
  if (typeof MutationObserver !== 'undefined') {
    const obs = new MutationObserver((mutations) => {
      for (const m of mutations) {
        if (m.type === 'attributes' && m.attributeName === 'data-theme') {
          refreshPreviewForThemeChange();
          break;
        }
      }
    });
    obs.observe(document.documentElement, { attributes: true, attributeFilter: ['data-theme'] });
  }

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
