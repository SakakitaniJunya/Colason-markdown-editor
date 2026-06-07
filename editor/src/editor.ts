import { Editor } from '@tiptap/core';
import StarterKit from '@tiptap/starter-kit';
import Placeholder from '@tiptap/extension-placeholder';
import CodeBlockLowlight from '@tiptap/extension-code-block-lowlight';
import TaskList from '@tiptap/extension-task-list';
import TaskItem from '@tiptap/extension-task-item';
import Table from '@tiptap/extension-table';
import TableRow from '@tiptap/extension-table-row';
import TableCell from '@tiptap/extension-table-cell';
import TableHeader from '@tiptap/extension-table-header';
import Image from '@tiptap/extension-image';
import Link from '@tiptap/extension-link';
import Underline from '@tiptap/extension-underline';
import Subscript from '@tiptap/extension-subscript';
import Superscript from '@tiptap/extension-superscript';
import Highlight from '@tiptap/extension-highlight';
import Typography from '@tiptap/extension-typography';
import { common, createLowlight } from 'lowlight';
import { MermaidBlock } from './extensions/mermaid-block';
import { KaTeXBlock } from './extensions/katex-block';
import { KaTeXInline } from './extensions/katex-inline';
import { MarkdownPaste } from './extensions/markdown-paste';
import {
  notifyContentChanged,
  notifyWordCount,
  notifyDocumentDirty,
  notifyHeadingsChanged,
  notifyCursorPosition,
  notifyOpenLink,
} from './bridge';
import { simpleMarkdownToHtml } from './source-mode';
import { setupTableContextMenu } from './table-context-menu';

const lowlight = createLowlight(common);

let isDirty = false;
let lastContent = '';

export function createEditor(element: HTMLElement): Editor {
  const editor = new Editor({
    element,
    extensions: [
      StarterKit.configure({
        codeBlock: false,
        heading: { levels: [1, 2, 3, 4, 5, 6] },
      }),
      Placeholder.configure({
        placeholder: 'Start writing...',
      }),
      CodeBlockLowlight.configure({
        lowlight,
        defaultLanguage: 'plaintext',
      }),
      TaskList,
      TaskItem.configure({ nested: true }),
      Table.configure({ resizable: true }),
      TableRow,
      TableCell,
      TableHeader,
      Image.configure({ allowBase64: true, inline: false }),
      Link.configure({ openOnClick: false, autolink: true }),
      Underline,
      Subscript,
      Superscript,
      Highlight.configure({ multicolor: false }),
      Typography,
      MermaidBlock,
      KaTeXBlock,
      KaTeXInline,
      MarkdownPaste,
    ],
    content: '<p></p>',
    autofocus: true,
    editable: true,

    onUpdate: ({ editor }) => {
      const html = editor.getHTML();
      notifyContentChanged(html);

      if (html !== lastContent) {
        if (!isDirty) {
          isDirty = true;
          notifyDocumentDirty(true);
        }
      }

      const text = editor.state.doc.textContent;
      const words = text.trim() ? text.trim().split(/\s+/).length : 0;
      notifyWordCount(words, text.length);

      extractAndNotifyHeadings(editor);
    },

    onSelectionUpdate: ({ editor }) => {
      const { from } = editor.state.selection;
      let line = 0;
      let col = from;

      editor.state.doc.forEach((_node, offset, index) => {
        if (offset < from) {
          line = index + 1;
          col = from - offset;
        }
      });

      notifyCursorPosition(line + 1, col);
    },
  });

  // Markdown paste handler
  editor.view.dom.addEventListener('paste', (event: ClipboardEvent) => {
    // If clipboard has HTML, let TipTap handle it
    const html = event.clipboardData?.getData('text/html');
    if (html && html.trim()) return;

    const text = event.clipboardData?.getData('text/plain');
    if (!text || !looksLikeMarkdown(text)) return;

    event.preventDefault();
    const converted = simpleMarkdownToHtml(text);
    editor.commands.insertContent(converted);
  });

  setupTableContextMenu(editor);
  setupLinkClickHandler(editor);

  lastContent = editor.getHTML();
  return editor;
}

// Cmd/Ctrl + click on links opens externally via the C++ bridge
// (TipTap's Link extension is configured with openOnClick: false to
// keep the cursor inside the link in edit-mode, but users still need
// a way to actually visit the URL.)
function setupLinkClickHandler(editor: Editor) {
  editor.view.dom.addEventListener('click', (e: MouseEvent) => {
    const target = e.target as HTMLElement | null;
    if (!target) return;
    const anchor = target.closest('a[href]') as HTMLAnchorElement | null;
    if (!anchor) return;

    const href = anchor.getAttribute('href');
    if (!href) return;

    // Cmd (Mac) / Ctrl (Win/Linux) → open externally.
    // Plain click on an http(s) link also opens — Typora-style behavior.
    const isExternal = /^(https?:|mailto:|file:)/i.test(href);
    const wantOpen = e.metaKey || e.ctrlKey || isExternal;
    if (!wantOpen) return;

    e.preventDefault();
    e.stopPropagation();
    notifyOpenLink(href);
  }, true);
}

function looksLikeMarkdown(text: string): boolean {
  // Check for common markdown patterns
  return /^#{1,6}\s/m.test(text)           // headings
    || /^```/m.test(text)                   // code blocks
    || /^[-*+]\s/m.test(text)               // unordered lists
    || /^\d+\.\s/m.test(text)               // ordered lists
    || /^>\s/m.test(text)                   // blockquotes
    || /\[.+?\]\(.+?\)/.test(text)          // links
    || /^[-*_]{3,}$/m.test(text)            // horizontal rules
    || /^\|.+\|$/m.test(text)               // tables
    || /\*\*.+?\*\*/.test(text)             // bold
    || /!\[.*?\]\(.+?\)/.test(text);        // images
}

function extractAndNotifyHeadings(editor: Editor) {
  const headings: Array<{ id: string; text: string; level: number; pos: number }> = [];

  editor.state.doc.descendants((node, pos) => {
    if (node.type.name === 'heading') {
      headings.push({
        id: `heading-${pos}`,
        text: node.textContent,
        level: node.attrs.level,
        pos,
      });
    }
  });

  notifyHeadingsChanged(headings);
}

export function markClean() {
  isDirty = false;
  lastContent = '';
}
