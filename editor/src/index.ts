import { createEditor } from './editor';
import { initBridge, setupGlobalAPI } from './bridge';
import { createFileManager, pushRecentFile } from './file-manager';
import { FileTreeView } from './file-tree';
import { simpleMarkdownToHtml } from './source-mode';

let activePath: string | null = null;

async function main() {
  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    console.error('[Colason] Editor element not found');
    return;
  }

  const editor = createEditor(editorElement);
  setupGlobalAPI(editor);
  await initBridge(editor);

  // Mount file-tree sidebar
  const manager = createFileManager();
  document.body.classList.add('colason-has-sidebar');
  // Insert sidebar before #editor so flex layout puts it on the left
  const sidebarHost = document.createElement('div');
  document.body.insertBefore(sidebarHost, editorElement);

  const tree = new FileTreeView(sidebarHost, {
    manager,
    getActivePath: () => activePath,
    onOpenFile: (path: string, content: string) => {
      activePath = path;
      // Markdown content -> HTML via existing helper, then load into editor.
      const html = simpleMarkdownToHtml(content);
      editor.commands.setContent(html || '<p></p>');
      editor.commands.focus();

      // record recent
      const ws = manager.getWorkspace();
      const wsName = ws ? ws.rootName : '';
      pushRecentFile({
        path,
        name: path.split('/').pop() ?? path,
        workspace: wsName,
      });
    },
  });

  // Expose for E2E / debugging
  (window as unknown as { colasonFileTree?: FileTreeView }).colasonFileTree = tree;
  (window as unknown as { colasonFileManager?: ReturnType<typeof createFileManager> }).colasonFileManager = manager;

  console.log('[Colason] Editor initialized');
}

main().catch(console.error);
