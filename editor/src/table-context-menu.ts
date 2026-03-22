import type { Editor } from '@tiptap/core';

interface MenuItem {
  label: string;
  action: (editor: Editor) => void;
  enabled?: (editor: Editor) => boolean;
  separator?: boolean;
}

let menuEl: HTMLElement | null = null;

function isInTable(editor: Editor): boolean {
  return editor.isActive('table');
}

function getMenuItems(): MenuItem[] {
  return [
    {
      label: '上に行を追加',
      action: (e) => e.chain().focus().addRowBefore().run(),
      enabled: isInTable,
    },
    {
      label: '下に行を追加',
      action: (e) => e.chain().focus().addRowAfter().run(),
      enabled: isInTable,
    },
    { label: '', action: () => {}, separator: true },
    {
      label: '左に列を追加',
      action: (e) => e.chain().focus().addColumnBefore().run(),
      enabled: isInTable,
    },
    {
      label: '右に列を追加',
      action: (e) => e.chain().focus().addColumnAfter().run(),
      enabled: isInTable,
    },
    { label: '', action: () => {}, separator: true },
    {
      label: '行を削除',
      action: (e) => e.chain().focus().deleteRow().run(),
      enabled: isInTable,
    },
    {
      label: '列を削除',
      action: (e) => e.chain().focus().deleteColumn().run(),
      enabled: isInTable,
    },
    { label: '', action: () => {}, separator: true },
    {
      label: 'セルを結合',
      action: (e) => e.chain().focus().mergeCells().run(),
      enabled: (e) => isInTable(e) && e.can().mergeCells(),
    },
    {
      label: 'セルを分割',
      action: (e) => e.chain().focus().splitCell().run(),
      enabled: (e) => isInTable(e) && e.can().splitCell(),
    },
    { label: '', action: () => {}, separator: true },
    {
      label: 'ヘッダー行の切替',
      action: (e) => e.chain().focus().toggleHeaderRow().run(),
      enabled: isInTable,
    },
    {
      label: 'ヘッダー列の切替',
      action: (e) => e.chain().focus().toggleHeaderColumn().run(),
      enabled: isInTable,
    },
    { label: '', action: () => {}, separator: true },
    {
      label: 'テーブルを削除',
      action: (e) => e.chain().focus().deleteTable().run(),
      enabled: isInTable,
    },
  ];
}

function createMenuElement(): HTMLElement {
  const el = document.createElement('div');
  el.className = 'colason-table-menu';
  document.body.appendChild(el);
  return el;
}

function hideMenu() {
  if (menuEl) {
    menuEl.style.display = 'none';
    menuEl.innerHTML = '';
  }
}

function showMenu(editor: Editor, x: number, y: number) {
  if (!menuEl) menuEl = createMenuElement();

  menuEl.innerHTML = '';
  const items = getMenuItems();

  for (const item of items) {
    if (item.separator) {
      const sep = document.createElement('div');
      sep.className = 'colason-table-menu-separator';
      menuEl.appendChild(sep);
      continue;
    }

    const btn = document.createElement('div');
    btn.className = 'colason-table-menu-item';
    btn.textContent = item.label;

    const enabled = item.enabled ? item.enabled(editor) : true;
    if (!enabled) {
      btn.classList.add('disabled');
    } else {
      btn.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        item.action(editor);
        hideMenu();
      });
    }

    menuEl.appendChild(btn);
  }

  // Position the menu, ensuring it stays within viewport
  menuEl.style.display = 'block';
  menuEl.style.left = `${x}px`;
  menuEl.style.top = `${y}px`;

  requestAnimationFrame(() => {
    if (!menuEl) return;
    const rect = menuEl.getBoundingClientRect();
    if (rect.right > window.innerWidth) {
      menuEl.style.left = `${window.innerWidth - rect.width - 8}px`;
    }
    if (rect.bottom > window.innerHeight) {
      menuEl.style.top = `${window.innerHeight - rect.height - 8}px`;
    }
  });
}

export function setupTableContextMenu(editor: Editor) {
  editor.view.dom.addEventListener('contextmenu', (event: MouseEvent) => {
    if (!isInTable(editor)) return;

    event.preventDefault();
    showMenu(editor, event.clientX, event.clientY);
  });

  // Hide menu on click outside or Escape
  document.addEventListener('click', (e) => {
    if (menuEl && !menuEl.contains(e.target as Node)) {
      hideMenu();
    }
  });

  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') hideMenu();
  });

  // Hide menu on scroll
  document.addEventListener('scroll', hideMenu, true);
}
