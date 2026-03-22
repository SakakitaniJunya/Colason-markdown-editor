import { EditorState, Compartment } from '@codemirror/state';
import { EditorView, keymap, drawSelection } from '@codemirror/view';
import { markdown } from '@codemirror/lang-markdown';
import { languages } from '@codemirror/language-data';
import { defaultKeymap, history, historyKeymap } from '@codemirror/commands';
import { syntaxHighlighting, defaultHighlightStyle, bracketMatching } from '@codemirror/language';
import { searchKeymap, highlightSelectionMatches } from '@codemirror/search';
import type { Editor } from '@tiptap/core';
import { getKeybindingExtension, type KeybindingMode } from './keybindings';
import { Marked, type TokenizerAndRendererExtension, type Tokens } from 'marked';

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

// === Markdown to HTML conversion using marked ===

// Custom inline extensions for TipTap-specific syntax
const highlightExtension: TokenizerAndRendererExtension = {
  name: 'highlight',
  level: 'inline',
  start(src: string) {
    return src.indexOf('==');
  },
  tokenizer(src: string) {
    const match = src.match(/^==(.+?)==/);
    if (match) {
      return {
        type: 'highlight',
        raw: match[0],
        text: match[1],
        tokens: this.lexer.inlineTokens(match[1]),
      };
    }
  },
  renderer(token) {
    return `<mark>${this.parser.parseInline(token.tokens!)}</mark>`;
  },
};

const superscriptExtension: TokenizerAndRendererExtension = {
  name: 'superscript',
  level: 'inline',
  start(src: string) {
    return src.indexOf('^');
  },
  tokenizer(src: string) {
    const match = src.match(/^\^([^\s^][^^]*?)\^/);
    if (match) {
      return {
        type: 'superscript',
        raw: match[0],
        text: match[1],
        tokens: this.lexer.inlineTokens(match[1]),
      };
    }
  },
  renderer(token) {
    return `<sup>${this.parser.parseInline(token.tokens!)}</sup>`;
  },
};

const subscriptExtension: TokenizerAndRendererExtension = {
  name: 'subscript',
  level: 'inline',
  start(src: string) {
    // Find single ~ not followed by another ~
    const idx = src.search(/~(?!~)/);
    return idx >= 0 ? idx : -1;
  },
  tokenizer(src: string) {
    const match = src.match(/^~(?!~)([^\s~][^~]*?)~(?!~)/);
    if (match) {
      return {
        type: 'subscript',
        raw: match[0],
        text: match[1],
        tokens: this.lexer.inlineTokens(match[1]),
      };
    }
  },
  renderer(token) {
    return `<sub>${this.parser.parseInline(token.tokens!)}</sub>`;
  },
};

// Create a configured marked instance
const markedInstance = new Marked({
  gfm: true,
  breaks: false,
  extensions: [highlightExtension, superscriptExtension, subscriptExtension],
  renderer: {
    // Produce TipTap-compatible task list HTML
    list(token: Tokens.List) {
      const isTaskList = token.items.some((item: Tokens.ListItem) => item.task);
      const tag = token.ordered ? 'ol' : 'ul';
      const startAttr = token.ordered && token.start !== 1 ? ` start="${token.start}"` : '';
      const typeAttr = isTaskList ? ' data-type="taskList"' : '';
      let body = '';
      for (const item of token.items) {
        body += this.listitem(item);
      }
      return `<${tag}${startAttr}${typeAttr}>${body}</${tag}>`;
    },
    listitem(item: Tokens.ListItem) {
      let text = this.parser.parse(item.tokens);
      if (item.task) {
        const checkedAttr = item.checked ? 'true' : 'false';
        // Remove checkbox that marked auto-inserts
        text = text.replace(/<input[^>]*>/, '');
        return `<li data-type="taskItem" data-checked="${checkedAttr}">${text}</li>`;
      }
      return `<li>${text}</li>`;
    },
  },
});

export function simpleMarkdownToHtml(md: string): string {
  return markedInstance.parse(md) as string;
}
