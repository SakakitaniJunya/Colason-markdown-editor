/**
 * File Tree Sidebar UI
 *
 * 軽量 DOM ベースのフォルダツリー。React/Vue 等は導入しない (editor の他部分と統一)。
 *
 * 機能:
 *   - フォルダ展開 / 折りたたみ
 *   - ファイル click で `onOpenFile` callback
 *   - 右クリック (ContextMenu) で 新規 / リネーム / 削除
 *   - HTML5 drag-drop でファイル / フォルダ移動
 *   - 最近開いた履歴セクション
 *   - Workspace 未選択時は "ワークスペースを開く" ボタンのみ表示
 */

import {
  type FileManager,
  type FileTreeNode,
  type WorkspaceInfo,
  getRecentFiles,
  pushRecentFile,
} from './file-manager';

export type FileTreeOptions = {
  manager: FileManager;
  /** ファイルがクリックされた時 (内容を Editor に流す) */
  onOpenFile: (path: string, content: string) => void;
  /** 現在開いている file path (highlight 用) */
  getActivePath?: () => string | null;
};

const DRAG_MIME = 'application/x-colason-path';

export class FileTreeView {
  private container: HTMLElement;
  private treeEl: HTMLElement;
  private recentEl: HTMLElement;
  private headerEl: HTMLElement;
  private workspace: WorkspaceInfo | null = null;
  private nodes: FileTreeNode[] = [];
  private expanded = new Set<string>();

  constructor(parent: HTMLElement, private opts: FileTreeOptions) {
    this.container = document.createElement('aside');
    this.container.className = 'colason-sidebar';
    this.container.setAttribute('aria-label', 'File explorer');

    this.headerEl = document.createElement('div');
    this.headerEl.className = 'colason-sidebar__header';

    this.treeEl = document.createElement('div');
    this.treeEl.className = 'colason-sidebar__tree';
    this.treeEl.setAttribute('role', 'tree');

    const recentSection = document.createElement('div');
    recentSection.className = 'colason-sidebar__recent';
    const recentTitle = document.createElement('div');
    recentTitle.className = 'colason-sidebar__section-title';
    recentTitle.textContent = '最近開いたファイル';
    this.recentEl = document.createElement('div');
    this.recentEl.className = 'colason-sidebar__recent-list';
    recentSection.appendChild(recentTitle);
    recentSection.appendChild(this.recentEl);

    this.container.appendChild(this.headerEl);
    this.container.appendChild(this.treeEl);
    this.container.appendChild(recentSection);

    parent.appendChild(this.container);

    this.renderHeader();
    this.renderRecent();
    void this.tryReopenWorkspace();
  }

  /** Workspace が既に開かれていれば listTree して描画する */
  private async tryReopenWorkspace(): Promise<void> {
    const ws = this.opts.manager.getWorkspace();
    if (ws) {
      this.workspace = ws;
      await this.refresh();
    }
  }

  // -----------------------------------------------------------------
  // Header (workspace open / refresh / new)
  // -----------------------------------------------------------------

  private renderHeader(): void {
    this.headerEl.innerHTML = '';

    const title = document.createElement('div');
    title.className = 'colason-sidebar__title';
    title.textContent = this.workspace ? this.workspace.rootName : 'ワークスペース';
    title.title = this.workspace ? this.workspace.rootPath : '未選択';

    const actions = document.createElement('div');
    actions.className = 'colason-sidebar__actions';

    if (!this.workspace) {
      const openBtn = this.makeButton('フォルダを開く', () => void this.openWorkspace());
      actions.appendChild(openBtn);
    } else {
      actions.appendChild(this.makeIconButton('新規ファイル', '+', () => void this.createNodeAt('', 'file')));
      actions.appendChild(this.makeIconButton('新規フォルダ', '⊞', () => void this.createNodeAt('', 'directory')));
      actions.appendChild(this.makeIconButton('再読み込み', '↻', () => void this.refresh()));
      actions.appendChild(this.makeIconButton('別のフォルダを開く', '⋯', () => void this.openWorkspace()));
    }

    this.headerEl.appendChild(title);
    this.headerEl.appendChild(actions);
  }

  private makeButton(label: string, onClick: () => void): HTMLButtonElement {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'colason-sidebar__btn';
    btn.textContent = label;
    btn.addEventListener('click', onClick);
    return btn;
  }

  private makeIconButton(label: string, glyph: string, onClick: () => void): HTMLButtonElement {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'colason-sidebar__icon-btn';
    btn.title = label;
    btn.setAttribute('aria-label', label);
    btn.textContent = glyph;
    btn.addEventListener('click', onClick);
    return btn;
  }

  // -----------------------------------------------------------------
  // Workspace open / refresh
  // -----------------------------------------------------------------

  async openWorkspace(): Promise<void> {
    try {
      const ws = await this.opts.manager.openWorkspace();
      if (!ws) return;
      this.workspace = ws;
      this.expanded.clear();
      await this.refresh();
    } catch (err) {
      console.error('[Colason] openWorkspace failed', err);
      this.toast('ワークスペースを開けませんでした');
    }
  }

  async refresh(): Promise<void> {
    if (!this.workspace) {
      this.nodes = [];
      this.renderHeader();
      this.renderTree();
      return;
    }
    try {
      this.nodes = await this.opts.manager.listTree();
    } catch (err) {
      console.error('[Colason] listTree failed', err);
      this.nodes = [];
      this.toast('ツリーの読み込みに失敗しました');
    }
    this.renderHeader();
    this.renderTree();
    this.renderRecent();
  }

  // -----------------------------------------------------------------
  // Tree render
  // -----------------------------------------------------------------

  private renderTree(): void {
    this.treeEl.innerHTML = '';

    if (!this.workspace) {
      const empty = document.createElement('div');
      empty.className = 'colason-sidebar__empty';
      empty.textContent = 'ワークスペースが選択されていません';
      this.treeEl.appendChild(empty);
      return;
    }

    if (this.nodes.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'colason-sidebar__empty';
      empty.textContent = '(空のフォルダ)';
      this.treeEl.appendChild(empty);
    }

    // root drop target so users can drag to root
    this.treeEl.addEventListener('dragover', (e) => {
      if (e.dataTransfer?.types.includes(DRAG_MIME)) {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'move';
      }
    });
    this.treeEl.addEventListener('drop', (e) => {
      const src = e.dataTransfer?.getData(DRAG_MIME);
      if (!src) return;
      e.preventDefault();
      void this.moveNode(src, '');
    });

    for (const node of this.nodes) {
      this.treeEl.appendChild(this.renderNode(node, 0));
    }
  }

  private renderNode(node: FileTreeNode, depth: number): HTMLElement {
    const row = document.createElement('div');
    row.className = `colason-tree__row colason-tree__row--${node.kind}`;
    row.dataset.path = node.path;
    row.dataset.kind = node.kind;
    row.setAttribute('role', 'treeitem');
    row.draggable = true;
    row.style.paddingLeft = `${depth * 14 + 6}px`;

    const activePath = this.opts.getActivePath?.();
    if (activePath && activePath === node.path) {
      row.classList.add('colason-tree__row--active');
    }

    const twisty = document.createElement('span');
    twisty.className = 'colason-tree__twisty';
    if (node.kind === 'directory') {
      twisty.textContent = this.expanded.has(node.path) ? '▾' : '▸';
    } else {
      twisty.textContent = '';
    }

    const icon = document.createElement('span');
    icon.className = 'colason-tree__icon';
    icon.textContent = node.kind === 'directory' ? '📁' : '📄';

    const label = document.createElement('span');
    label.className = 'colason-tree__label';
    label.textContent = node.name;

    row.appendChild(twisty);
    row.appendChild(icon);
    row.appendChild(label);

    // click
    row.addEventListener('click', (e) => {
      e.stopPropagation();
      if (node.kind === 'directory') {
        if (this.expanded.has(node.path)) this.expanded.delete(node.path);
        else this.expanded.add(node.path);
        this.renderTree();
      } else {
        void this.openFile(node.path);
      }
    });

    // double-click directory: also expand (already handled by click)

    // context menu
    row.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.showContextMenu(e.pageX, e.pageY, node);
    });

    // drag-drop
    row.addEventListener('dragstart', (e) => {
      if (!e.dataTransfer) return;
      e.dataTransfer.setData(DRAG_MIME, node.path);
      e.dataTransfer.effectAllowed = 'move';
    });
    row.addEventListener('dragover', (e) => {
      if (node.kind !== 'directory') return;
      if (!e.dataTransfer?.types.includes(DRAG_MIME)) return;
      e.preventDefault();
      e.stopPropagation();
      e.dataTransfer.dropEffect = 'move';
      row.classList.add('colason-tree__row--drop');
    });
    row.addEventListener('dragleave', () => {
      row.classList.remove('colason-tree__row--drop');
    });
    row.addEventListener('drop', (e) => {
      row.classList.remove('colason-tree__row--drop');
      if (node.kind !== 'directory') return;
      const src = e.dataTransfer?.getData(DRAG_MIME);
      if (!src) return;
      e.preventDefault();
      e.stopPropagation();
      void this.moveNode(src, node.path);
    });

    if (node.kind === 'directory' && this.expanded.has(node.path) && node.children) {
      const childWrap = document.createElement('div');
      childWrap.className = 'colason-tree__children';
      for (const c of node.children) {
        childWrap.appendChild(this.renderNode(c, depth + 1));
      }
      const wrapper = document.createElement('div');
      wrapper.appendChild(row);
      wrapper.appendChild(childWrap);
      return wrapper;
    }

    return row;
  }

  // -----------------------------------------------------------------
  // Recent files
  // -----------------------------------------------------------------

  private renderRecent(): void {
    this.recentEl.innerHTML = '';
    const ws = this.workspace?.rootName ?? '';
    const recents = getRecentFiles().filter(r => !ws || r.workspace === ws);
    if (recents.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'colason-sidebar__empty';
      empty.textContent = '(履歴なし)';
      this.recentEl.appendChild(empty);
      return;
    }
    for (const r of recents) {
      const item = document.createElement('div');
      item.className = 'colason-recent__item';
      item.title = r.path;
      const icon = document.createElement('span');
      icon.className = 'colason-tree__icon';
      icon.textContent = '📄';
      const label = document.createElement('span');
      label.className = 'colason-recent__label';
      label.textContent = r.name;
      item.appendChild(icon);
      item.appendChild(label);
      item.addEventListener('click', () => {
        void this.openFile(r.path);
      });
      this.recentEl.appendChild(item);
    }
  }

  // -----------------------------------------------------------------
  // File operations
  // -----------------------------------------------------------------

  private async openFile(path: string): Promise<void> {
    try {
      const content = await this.opts.manager.readFile(path);
      this.opts.onOpenFile(path, content);
      const ws = this.workspace?.rootName ?? '';
      pushRecentFile({ path, name: basename(path), workspace: ws });
      this.renderRecent();
      this.renderTree();
    } catch (err) {
      console.error('[Colason] readFile failed', err);
      this.toast('ファイルを開けませんでした');
    }
  }

  private async createNodeAt(parentPath: string, kind: 'file' | 'directory'): Promise<void> {
    const defaultName = kind === 'file' ? 'untitled.md' : 'New Folder';
    const name = window.prompt(kind === 'file' ? 'ファイル名' : 'フォルダ名', defaultName);
    if (!name) return;
    if (!isValidName(name)) {
      this.toast('使用できない名前です');
      return;
    }
    try {
      await this.opts.manager.createNode(parentPath, name, kind);
      if (parentPath) this.expanded.add(parentPath);
      await this.refresh();
    } catch (err) {
      console.error('[Colason] createNode failed', err);
      this.toast('作成に失敗しました');
    }
  }

  private async renameNode(node: FileTreeNode): Promise<void> {
    const name = window.prompt('新しい名前', node.name);
    if (!name || name === node.name) return;
    if (!isValidName(name)) {
      this.toast('使用できない名前です');
      return;
    }
    try {
      await this.opts.manager.rename(node.path, name);
      await this.refresh();
    } catch (err) {
      console.error('[Colason] rename failed', err);
      this.toast('リネームに失敗しました');
    }
  }

  private async removeNode(node: FileTreeNode): Promise<void> {
    const ok = window.confirm(
      node.kind === 'directory'
        ? `フォルダ "${node.name}" とその中身を削除しますか？`
        : `ファイル "${node.name}" を削除しますか？`
    );
    if (!ok) return;
    try {
      await this.opts.manager.remove(node.path);
      await this.refresh();
    } catch (err) {
      console.error('[Colason] remove failed', err);
      this.toast('削除に失敗しました');
    }
  }

  private async moveNode(srcPath: string, destDirPath: string): Promise<void> {
    if (srcPath === destDirPath) return;
    if (destDirPath.startsWith(srcPath + '/') || destDirPath === srcPath) {
      this.toast('同じ場所には移動できません');
      return;
    }
    // no-op: same dir already
    if (dirname(srcPath) === destDirPath) return;
    try {
      await this.opts.manager.move(srcPath, destDirPath);
      if (destDirPath) this.expanded.add(destDirPath);
      await this.refresh();
    } catch (err) {
      console.error('[Colason] move failed', err);
      this.toast('移動に失敗しました');
    }
  }

  // -----------------------------------------------------------------
  // Context menu
  // -----------------------------------------------------------------

  private showContextMenu(x: number, y: number, node: FileTreeNode): void {
    const existing = document.querySelector('.colason-context-menu');
    if (existing) existing.remove();

    const menu = document.createElement('div');
    menu.className = 'colason-context-menu';
    menu.style.left = `${x}px`;
    menu.style.top = `${y}px`;

    const items: Array<{ label: string; action: () => void; enabled?: boolean }> = [];
    if (node.kind === 'directory') {
      items.push({ label: '新規ファイル', action: () => void this.createNodeAt(node.path, 'file') });
      items.push({ label: '新規フォルダ', action: () => void this.createNodeAt(node.path, 'directory') });
    }
    items.push({ label: 'リネーム', action: () => void this.renameNode(node) });
    items.push({ label: '削除', action: () => void this.removeNode(node) });

    for (const it of items) {
      const li = document.createElement('div');
      li.className = 'colason-context-menu__item';
      li.textContent = it.label;
      li.addEventListener('click', () => {
        menu.remove();
        it.action();
      });
      menu.appendChild(li);
    }

    document.body.appendChild(menu);

    const dismiss = (e: Event) => {
      if (!menu.contains(e.target as Node)) {
        menu.remove();
        document.removeEventListener('click', dismiss, true);
        document.removeEventListener('contextmenu', dismiss, true);
      }
    };
    setTimeout(() => {
      document.addEventListener('click', dismiss, true);
      document.addEventListener('contextmenu', dismiss, true);
    }, 0);
  }

  // -----------------------------------------------------------------
  // Toast (lightweight)
  // -----------------------------------------------------------------

  private toast(message: string): void {
    const el = document.createElement('div');
    el.className = 'colason-toast';
    el.textContent = message;
    document.body.appendChild(el);
    setTimeout(() => el.remove(), 2400);
  }
}

function isValidName(name: string): boolean {
  if (!name.trim()) return false;
  if (name.includes('/') || name.includes('\\')) return false;
  if (name === '.' || name === '..') return false;
  return true;
}

function basename(p: string): string {
  const i = p.lastIndexOf('/');
  return i >= 0 ? p.slice(i + 1) : p;
}

function dirname(p: string): string {
  const i = p.lastIndexOf('/');
  return i > 0 ? p.slice(0, i) : '';
}
