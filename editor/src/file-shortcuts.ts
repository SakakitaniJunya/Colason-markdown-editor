/**
 * Global keyboard shortcuts for file operations (Issue #2).
 *
 * Ctrl/Cmd+O      → Open
 * Ctrl/Cmd+S      → Save
 * Ctrl/Cmd+Shift+S → Save As
 *
 * The handler runs in the *capture* phase so it wins over CodeMirror / TipTap
 * default keymaps. Native menus (Qt) may also bind these shortcuts; in that
 * case the host fires `colasonFile.openFile/saveFile/...` directly and this
 * fallback is benign.
 */

import { FileManager } from './file-manager';

export function installFileShortcuts(manager: FileManager): () => void {
  const handler = (event: KeyboardEvent) => {
    if (!isModifier(event)) return;
    const key = event.key.toLowerCase();
    if (key === 'o' && !event.shiftKey && !event.altKey) {
      event.preventDefault();
      void manager.openFile();
    } else if (key === 's' && event.shiftKey && !event.altKey) {
      event.preventDefault();
      void manager.saveAs();
    } else if (key === 's' && !event.shiftKey && !event.altKey) {
      event.preventDefault();
      void manager.save();
    }
  };

  document.addEventListener('keydown', handler, true);
  return () => document.removeEventListener('keydown', handler, true);
}

function isModifier(event: KeyboardEvent): boolean {
  // Mac uses Cmd; everything else uses Ctrl. Keep both alive for cross-platform builds.
  return event.metaKey || event.ctrlKey;
}
