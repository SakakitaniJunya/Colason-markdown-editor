import { vim } from '@replit/codemirror-vim';
import type { Extension } from '@codemirror/state';

export type KeybindingMode = 'default' | 'vim' | 'emacs';

let currentMode: KeybindingMode = 'default';

export function getKeybindingExtension(mode: KeybindingMode): Extension[] {
  currentMode = mode;
  switch (mode) {
    case 'vim':
      return [vim()];
    case 'emacs':
      // Emacs bindings can be added later
      return [];
    default:
      return [];
  }
}

export function getCurrentMode(): KeybindingMode {
  return currentMode;
}
