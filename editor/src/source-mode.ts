import { EditorState, Compartment } from '@codemirror/state';
import { EditorView, keymap, drawSelection } from '@codemirror/view';
import { markdown } from '@codemirror/lang-markdown';
import { languages } from '@codemirror/language-data';
import { defaultKeymap, history, historyKeymap } from '@codemirror/commands';
import { syntaxHighlighting, defaultHighlightStyle, bracketMatching } from '@codemirror/language';
import { searchKeymap, highlightSelectionMatches } from '@codemirror/search';
import type { Editor } from '@tiptap/core';
import { getKeybindingExtension, type KeybindingMode } from './keybindings';

let cmView: EditorView | null = null;
let isSourceMode = false;
const keybindingCompartment = new Compartment();

const cmTheme = EditorView.theme({
  '&': {
    fontSize: '16px',
    fontFamily: "'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace",
  },
  '.cm-content': {
    caretColor: '#333',
    padding: '4px 0',
    minHeight: 'calc(100vh - 80px)',
  },
  '&.cm-focused': {
    outline: 'none',
  },
  '&.cm-focused .cm-cursor': {
    borderLeftColor: '#333',
  },
});

export function createSourceEditor(container: HTMLElement): EditorView {
  const state = EditorState.create({
    doc: '',
    extensions: [
      drawSelection(),
      bracketMatching(),
      highlightSelectionMatches(),
      history(),
      markdown({ codeLanguages: languages }),
      syntaxHighlighting(defaultHighlightStyle),
      keymap.of([...defaultKeymap, ...historyKeymap, ...searchKeymap]),
      keybindingCompartment.of([]),
      cmTheme,
      EditorView.lineWrapping,
    ],
  });

  cmView = new EditorView({ state, parent: container });
  cmView.dom.style.display = 'none';
  return cmView;
}

export function toggleSourceMode(tiptapEditor: Editor, cmEditor: EditorView): boolean {
  isSourceMode = !isSourceMode;

  const tiptapEl = tiptapEditor.view.dom.closest('.tiptap')?.parentElement;
  if (!tiptapEl) return isSourceMode;

  if (isSourceMode) {
    // WYSIWYG -> Source: serialize TipTap content to markdown-like HTML, then show in CM
    const html = tiptapEditor.getHTML();
    const markdownText = htmlToSimpleMarkdown(html);

    cmEditor.dispatch({
      changes: { from: 0, to: cmEditor.state.doc.length, insert: markdownText },
    });

    tiptapEl.style.display = 'none';
    cmEditor.dom.style.display = '';
    cmEditor.focus();
  } else {
    // Source -> WYSIWYG: get CM text and parse back
    const markdownText = cmEditor.state.doc.toString();
    const html = simpleMarkdownToHtml(markdownText);
    tiptapEditor.commands.setContent(html || '<p></p>');

    cmEditor.dom.style.display = 'none';
    tiptapEl.style.display = '';
    tiptapEditor.commands.focus();
  }

  return isSourceMode;
}

export function isInSourceMode(): boolean {
  return isSourceMode;
}

export function getSourceView(): EditorView | null {
  return cmView;
}

export function setSourceKeybinding(mode: KeybindingMode) {
  if (cmView) {
    cmView.dispatch({
      effects: keybindingCompartment.reconfigure(getKeybindingExtension(mode)),
    });
  }
}

// Simple HTML to markdown conversion
export function htmlToSimpleMarkdown(html: string): string {
  const doc = new DOMParser().parseFromString(html, 'text/html');
  return nodeToMarkdown(doc.body).trim();
}

function nodeToMarkdown(node: Node): string {
  let result = '';

  for (const child of Array.from(node.childNodes)) {
    if (child.nodeType === Node.TEXT_NODE) {
      result += child.textContent || '';
      continue;
    }

    if (child.nodeType !== Node.ELEMENT_NODE) continue;
    const el = child as HTMLElement;
    const tag = el.tagName.toLowerCase();

    switch (tag) {
      case 'h1': case 'h2': case 'h3': case 'h4': case 'h5': case 'h6': {
        const level = parseInt(tag[1]);
        result += '#'.repeat(level) + ' ' + el.textContent + '\n\n';
        break;
      }
      case 'p':
        result += nodeToMarkdown(el) + '\n\n';
        break;
      case 'strong':
      case 'b':
        result += '**' + nodeToMarkdown(el) + '**';
        break;
      case 'em':
      case 'i':
        result += '*' + nodeToMarkdown(el) + '*';
        break;
      case 's':
      case 'del':
        result += '~~' + nodeToMarkdown(el) + '~~';
        break;
      case 'u':
        result += '<u>' + nodeToMarkdown(el) + '</u>';
        break;
      case 'code':
        if (el.parentElement?.tagName.toLowerCase() === 'pre') {
          result += el.textContent || '';
        } else {
          result += '`' + (el.textContent || '') + '`';
        }
        break;
      case 'pre': {
        const codeEl = el.querySelector('code');
        const lang = codeEl?.className?.match(/language-(\w+)/)?.[1] || '';
        result += '```' + lang + '\n' + (codeEl?.textContent || el.textContent || '') + '\n```\n\n';
        break;
      }
      case 'blockquote':
        result += nodeToMarkdown(el).split('\n').map(l => '> ' + l).join('\n') + '\n\n';
        break;
      case 'ul': {
        const items = el.querySelectorAll(':scope > li');
        const isTaskList = el.getAttribute('data-type') === 'taskList';
        items.forEach(li => {
          if (isTaskList) {
            const checked = li.getAttribute('data-checked') === 'true';
            result += (checked ? '- [x] ' : '- [ ] ') + li.textContent?.trim() + '\n';
          } else {
            result += '- ' + li.textContent?.trim() + '\n';
          }
        });
        result += '\n';
        break;
      }
      case 'ol': {
        const items = el.querySelectorAll(':scope > li');
        items.forEach((li, i) => {
          result += `${i + 1}. ` + li.textContent?.trim() + '\n';
        });
        result += '\n';
        break;
      }
      case 'hr':
        result += '---\n\n';
        break;
      case 'a': {
        const href = el.getAttribute('href') || '';
        result += '[' + nodeToMarkdown(el) + '](' + href + ')';
        break;
      }
      case 'img': {
        const src = el.getAttribute('src') || '';
        const alt = el.getAttribute('alt') || '';
        result += '![' + alt + '](' + src + ')\n\n';
        break;
      }
      case 'table':
        result += tableToMarkdown(el) + '\n\n';
        break;
      case 'mark':
        result += '==' + nodeToMarkdown(el) + '==';
        break;
      case 'sup':
        result += '^' + nodeToMarkdown(el) + '^';
        break;
      case 'sub':
        result += '~' + nodeToMarkdown(el) + '~';
        break;
      case 'br':
        result += '\n';
        break;
      case 'div': {
        if (el.getAttribute('data-type') === 'mermaid-block') {
          const code = el.getAttribute('code') || el.textContent || '';
          result += '```mermaid\n' + code + '\n```\n\n';
        } else {
          result += nodeToMarkdown(el);
        }
        break;
      }
      default:
        result += nodeToMarkdown(el);
    }
  }

  return result;
}

function tableToMarkdown(table: HTMLElement): string {
  const rows = table.querySelectorAll('tr');
  if (rows.length === 0) return '';

  const lines: string[] = [];
  rows.forEach((row, i) => {
    const cells = Array.from(row.querySelectorAll('th, td'));
    const line = '| ' + cells.map(c => c.textContent?.trim() || '').join(' | ') + ' |';
    lines.push(line);
    if (i === 0) {
      lines.push('| ' + cells.map(() => '---').join(' | ') + ' |');
    }
  });
  return lines.join('\n');
}

// Simple markdown to HTML conversion
export function simpleMarkdownToHtml(md: string): string {
  const lines = md.split('\n');
  let html = '';
  let i = 0;
  let inCodeBlock = false;
  let codeLang = '';
  let codeContent = '';

  while (i < lines.length) {
    const line = lines[i];

    // Code blocks
    if (line.startsWith('```')) {
      if (!inCodeBlock) {
        inCodeBlock = true;
        codeLang = line.slice(3).trim();
        codeContent = '';
      } else {
        html += `<pre><code class="language-${codeLang}">${escapeHtml(codeContent)}</code></pre>`;
        inCodeBlock = false;
      }
      i++;
      continue;
    }

    if (inCodeBlock) {
      codeContent += (codeContent ? '\n' : '') + line;
      i++;
      continue;
    }

    // Empty line
    if (line.trim() === '') {
      i++;
      continue;
    }

    // Headings
    const headingMatch = line.match(/^(#{1,6})\s+(.+)/);
    if (headingMatch) {
      const level = headingMatch[1].length;
      const text = inlineFormat(headingMatch[2]);
      html += `<h${level}>${text}</h${level}>`;
      i++;
      continue;
    }

    // Horizontal rule
    if (/^(-{3,}|\*{3,}|_{3,})$/.test(line.trim())) {
      html += '<hr>';
      i++;
      continue;
    }

    // Blockquote
    if (line.startsWith('> ')) {
      let quoteContent = '';
      while (i < lines.length && lines[i].startsWith('> ')) {
        quoteContent += lines[i].slice(2) + '\n';
        i++;
      }
      html += `<blockquote><p>${inlineFormat(quoteContent.trim())}</p></blockquote>`;
      continue;
    }

    // Unordered list
    if (/^[-*+]\s/.test(line)) {
      html += '<ul>';
      while (i < lines.length && /^[-*+]\s/.test(lines[i])) {
        const taskMatch = lines[i].match(/^[-*+]\s\[([ x])\]\s(.+)/);
        if (taskMatch) {
          html += `<li data-checked="${taskMatch[1] === 'x'}">${inlineFormat(taskMatch[2])}</li>`;
        } else {
          html += `<li>${inlineFormat(lines[i].replace(/^[-*+]\s/, ''))}</li>`;
        }
        i++;
      }
      html += '</ul>';
      continue;
    }

    // Ordered list
    if (/^\d+\.\s/.test(line)) {
      html += '<ol>';
      while (i < lines.length && /^\d+\.\s/.test(lines[i])) {
        html += `<li>${inlineFormat(lines[i].replace(/^\d+\.\s/, ''))}</li>`;
        i++;
      }
      html += '</ol>';
      continue;
    }

    // Table
    if (line.includes('|')) {
      const tableLines: string[] = [];
      while (i < lines.length && lines[i].includes('|')) {
        tableLines.push(lines[i]);
        i++;
      }
      html += parseTable(tableLines);
      continue;
    }

    // Paragraph
    html += `<p>${inlineFormat(line)}</p>`;
    i++;
  }

  return html;
}

function inlineFormat(text: string): string {
  return text
    .replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>')
    .replace(/\*(.+?)\*/g, '<em>$1</em>')
    .replace(/~~(.+?)~~/g, '<s>$1</s>')
    .replace(/`(.+?)`/g, '<code>$1</code>')
    .replace(/==(.+?)==/g, '<mark>$1</mark>')
    .replace(/\^(.+?)\^/g, '<sup>$1</sup>')
    .replace(/~(.+?)~/g, '<sub>$1</sub>')
    .replace(/\[(.+?)\]\((.+?)\)/g, '<a href="$2">$1</a>')
    .replace(/!\[(.+?)\]\((.+?)\)/g, '<img src="$2" alt="$1">');
}

function escapeHtml(text: string): string {
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function parseTable(lines: string[]): string {
  if (lines.length < 2) return '';
  const rows = lines.filter(l => !l.match(/^\|[\s-:|]+\|$/));
  let html = '<table>';
  rows.forEach((row, i) => {
    const cells = row.split('|').filter(c => c.trim() !== '');
    const tag = i === 0 ? 'th' : 'td';
    html += '<tr>' + cells.map(c => `<${tag}>${inlineFormat(c.trim())}</${tag}>`).join('') + '</tr>';
  });
  html += '</table>';
  return html;
}
