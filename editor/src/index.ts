import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';
import { FileManager } from './file-manager';
import { installFileShortcuts } from './file-shortcuts';
import { mountMenuBar, maybeOfferRecovery } from './menu-bar';

async function main() {
  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    console.error('[Colason] Editor element not found');
    return;
  }

  const editor = createEditor(editorElement);
  setupGlobalAPI(editor);
  await initBridge(editor);

  // Issue #2: file open/save/restore stack.
  const fileManager = new FileManager(editor);
  (window as any).colasonFiles = fileManager;
  mountMenuBar(fileManager);
  installFileShortcuts(fileManager);
  void maybeOfferRecovery(fileManager).catch((err) =>
    console.warn('[Colason] recovery offer failed:', err),
  );

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
