/**
 * Outline View — heading 構造から TOC を生成
 *
 * TipTap Editor の `update` イベントを subscribe して heading リストをリアルタイム更新。
 * TOC 項目クリックで対応 DOM node にスクロール。
 * H2 以下の子 heading を collapse / expand するトグルボタン付き。
 */

import type { Editor } from '@tiptap/core';

export interface HeadingItem {
  level: number;
  text: string;
  pos: number;
}

export class OutlineView {
  private container: HTMLElement;
  private listEl: HTMLElement;
  /** H2 pos → collapsed state */
  private collapsed = new Set<number>();

  constructor(parent: HTMLElement, private editor: Editor) {
    this.container = document.createElement('aside');
    this.container.className = 'colason-outline';
    this.container.setAttribute('aria-label', 'Outline');

    // Header
    const header = document.createElement('div');
    header.className = 'colason-outline__header';
    const title = document.createElement('span');
    title.className = 'colason-outline__title';
    title.textContent = 'OUTLINE';
    header.appendChild(title);
    this.container.appendChild(header);

    // TOC list
    this.listEl = document.createElement('div');
    this.listEl.className = 'colason-outline__list';
    this.container.appendChild(this.listEl);

    parent.appendChild(this.container);

    // Initial render
    this.update();
  }

  /** TipTap の editor.on('update', ...) から呼ぶ */
  update(): void {
    const headings = this.extractHeadings();
    this.render(headings);
  }

  private extractHeadings(): HeadingItem[] {
    const headings: HeadingItem[] = [];
    this.editor.state.doc.descendants((node, pos) => {
      if (node.type.name === 'heading') {
        headings.push({
          level: node.attrs['level'] as number,
          text: node.textContent,
          pos,
        });
      }
    });
    return headings;
  }

  private render(headings: HeadingItem[]): void {
    this.listEl.innerHTML = '';

    if (headings.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'colason-outline__empty';
      empty.textContent = '見出しがありません';
      this.listEl.appendChild(empty);
      return;
    }

    // H2 ごとの collapsed 状態を保持するため、現在の H2 pos を追跡
    let currentH2Pos: number | null = null;

    for (const item of headings) {
      const isH2 = item.level === 2;
      const isUnderH2 = item.level >= 3 && currentH2Pos !== null;

      if (isH2) {
        currentH2Pos = item.pos;
      }

      // H3+ は対応する H2 が collapsed なら非表示
      if (isUnderH2 && currentH2Pos !== null && this.collapsed.has(currentH2Pos)) {
        continue;
      }

      const row = document.createElement('div');
      row.className = 'colason-outline__item';
      row.dataset['level'] = String(item.level);

      // インデント: H1=0, H2=0.5rem, H3=1rem, H4=1.5rem, H5=2rem, H6=2.5rem
      const indent = (item.level - 1) * 0.5;
      row.style.paddingLeft = `${8 + indent * 16}px`;

      // H2 にはトグルボタンを付ける
      if (isH2) {
        const toggle = document.createElement('button');
        toggle.className = 'colason-outline__toggle';
        toggle.setAttribute('aria-label', this.collapsed.has(item.pos) ? '展開' : '折りたたみ');
        toggle.textContent = this.collapsed.has(item.pos) ? '▶' : '▼';
        toggle.addEventListener('click', (e) => {
          e.stopPropagation();
          if (this.collapsed.has(item.pos)) {
            this.collapsed.delete(item.pos);
          } else {
            this.collapsed.add(item.pos);
          }
          this.update();
        });
        row.appendChild(toggle);
      } else {
        // 非 H2 はトグルの幅を確保するスペーサー
        const spacer = document.createElement('span');
        spacer.className = 'colason-outline__toggle-spacer';
        row.appendChild(spacer);
      }

      // ラベル
      const label = document.createElement('span');
      label.className = 'colason-outline__label';
      label.textContent = item.text || '(empty)';
      row.appendChild(label);

      // クリックでスクロール
      row.addEventListener('click', () => {
        this.scrollToHeading(item);
      });

      this.listEl.appendChild(row);
    }
  }

  private scrollToHeading(item: HeadingItem): void {
    try {
      const domAtPos = this.editor.view.domAtPos(item.pos + 1);
      let el: HTMLElement | null = domAtPos.node as HTMLElement;
      // テキストノードの場合は親要素を使う
      if (el.nodeType === Node.TEXT_NODE) {
        el = el.parentElement;
      }
      // 最も近い heading 要素を探す
      if (el) {
        const heading = el.closest('h1,h2,h3,h4,h5,h6') as HTMLElement | null ?? el;
        heading.scrollIntoView({ behavior: 'smooth', block: 'start' });
      }
    } catch (e) {
      console.warn('[OutlineView] scrollToHeading failed', e);
    }
  }
}
