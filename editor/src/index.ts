import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';

async function main() {
  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    console.error('[Colason] Editor element not found');
    return;
  }

  const editor = createEditor(editorElement);
  setupGlobalAPI(editor);
  await initBridge(editor);

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
