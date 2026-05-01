import type { Editor } from '@tiptap/core';
import type { EditorView } from '@codemirror/view';
import { createSourceEditor, toggleSourceMode, isInSourceMode, setSourceKeybinding, simpleMarkdownToHtml, htmlToSimpleMarkdown } from './source-mode';
import { renderMarkdownForExport } from './preview-mode';
import type { KeybindingMode } from './keybindings';
import { QWebChannel } from './qwebchannel';

let cppBridge: any = null;
let cmEditor: EditorView | null = null;

export function initBridge(editor: Editor): Promise<void> {
  return new Promise((resolve) => {
    if (typeof (window as any).qt !== 'undefined') {
      new QWebChannel(
        (window as any).qt.webChannelTransport,
        (channel) => {
          cppBridge = channel.objects;
          setupCppSignalHandlers(editor);
          console.log('[Colason] QWebChannel connected');
          resolve();
        }
      );
    } else {
      console.log('[Colason] Running in standalone mode (no QWebChannel)');
      resolve();
    }
  });
}

function setupCppSignalHandlers(editor: Editor) {
  if (!cppBridge?.editor) return;

  cppBridge.editor.setContentRequested?.connect((html: string) => {
    window.colasonAPI.setContent(html);
  });

  cppBridge.editor.setMarkdownRequested?.connect((markdown: string) => {
    window.colasonAPI.setMarkdown(markdown);
  });

  cppBridge.editor.executeCommandRequested?.connect((cmd: string, args: string) => {
    window.colasonAPI.executeCommand(cmd, args);
  });

  cppBridge.editor.toggleSourceModeRequested?.connect(() => {
    window.colasonAPI.toggleSourceMode();
  });

  cppBridge.editor.getContentRequested?.connect(() => {
    const content = window.colasonAPI.getContent();
    cppBridge.editor.receiveContent(content);
  });

  cppBridge.outline?.scrollToHeadingRequested?.connect((id: string) => {
    window.colasonAPI.scrollToHeading(id);
  });

  cppBridge.search?.findRequested?.connect((query: string, caseSensitive: boolean, regex: boolean) => {
    window.colasonAPI.find(query, caseSensitive, regex);
  });

  cppBridge.search?.replaceRequested?.connect((from: string, to: string, all: boolean) => {
    window.colasonAPI.replace(from, to, all);
  });

  cppBridge.search?.findNextRequested?.connect(() => {
    window.colasonAPI.findNext();
  });

  cppBridge.search?.findPreviousRequested?.connect(() => {
    window.colasonAPI.findPrevious();
  });

  cppBridge.theme?.setThemeRequested?.connect((css: string) => {
    window.colasonAPI.setTheme(css);
  });
}

export function setupGlobalAPI(editor: Editor) {
  // Create CodeMirror source editor
  const editorContainer = document.getElementById('editor')!;
  cmEditor = createSourceEditor(editorContainer);

  // Setup scroll-based active heading tracking
  setupHeadingObserver(editor);

  (window as any).colasonAPI = {
    setContent(content: string) {
      // Exit source mode if active, so the user returns to WYSIWYG
      if (isInSourceMode() && cmEditor) {
        toggleSourceMode(editor, cmEditor);
      }
      // Auto-detect: treat as HTML only if content starts with an HTML tag
      const isHtml = /^\s*<(?:!doctype|html|head|body|div|p|h[1-6]|ul|ol|li|table|pre|blockquote)\b/im.test(content);
      const html = isHtml ? content : simpleMarkdownToHtml(content);
      editor.commands.setContent(html || '<p></p>');
      editor.commands.focus();
    },

    setMarkdown(md: string) {
      // Exit source mode if active
      if (isInSourceMode() && cmEditor) {
        toggleSourceMode(editor, cmEditor);
      }
      // Always parse as markdown — used for file opens
      const html = simpleMarkdownToHtml(md);
      editor.commands.setContent(html || '<p></p>');
      editor.commands.focus();
    },

    getContent(): string {
      return editor.getHTML();
    },

    getMarkdown(): string {
      return htmlToSimpleMarkdown(editor.getHTML());
    },

    /**
     * Returns a self-contained HTML string suitable for export (HTML/PDF).
     * Mermaid code blocks are rendered to inline SVG so the exported
     * artefact includes diagrams without requiring a JS runtime. This is
     * async because Mermaid is dynamically imported.
     *
     * NOTE (2026-05-01): The C++ ExportManager currently calls
     * `colasonAPI.getContent()`. To pick up rendered diagrams it should
     * invoke `colasonAPI.getExportHtml()` instead and await the JS
     * Promise via `runJavaScript`. Tracked as a follow-up.
     */
    async getExportHtml(): Promise<string> {
      const md = htmlToSimpleMarkdown(editor.getHTML());
      try {
        return await renderMarkdownForExport(md);
      } catch (err) {
        console.warn('[Colason] getExportHtml failed', err);
        return editor.getHTML();
      }
    },

    executeCommand(command: string, argsJson: string) {
      const args = argsJson ? JSON.parse(argsJson) : {};
      executeEditorCommand(editor, command, args);
    },

    toggleSourceMode() {
      if (cmEditor) {
        toggleSourceMode(editor, cmEditor);
      }
    },

    toggleFocusMode() {
      document.body.classList.toggle('focus-mode');
    },

    toggleTypewriterMode() {
      document.body.classList.toggle('typewriter-mode');
    },

    scrollToHeading(id: string) {
      const pos = parseInt(id.replace('heading-', ''));
      if (!isNaN(pos)) {
        try {
          editor.commands.setTextSelection(pos);
          const dom = editor.view.domAtPos(pos);
          const node = dom.node instanceof HTMLElement ? dom.node : dom.node.parentElement;
          node?.scrollIntoView({ behavior: 'smooth', block: 'start' });
        } catch {
          const el = document.getElementById(id);
          if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
      }
    },

    setTheme(css: string) {
      let styleEl = document.getElementById('colason-theme');
      if (!styleEl) {
        styleEl = document.createElement('style');
        styleEl.id = 'colason-theme';
      }
      styleEl.textContent = css;
      // Always re-append to end of <head> to ensure highest cascade priority
      document.head.appendChild(styleEl);
    },

    setZoom(percent: number) {
      document.documentElement.style.fontSize = `${percent}%`;
    },

    find(query: string, caseSensitive: boolean, regex: boolean) {
      performSearch(editor, query, caseSensitive, regex);
    },

    findNext() {
      navigateSearch(editor, 'next');
    },

    findPrevious() {
      navigateSearch(editor, 'prev');
    },

    replace(from: string, to: string, all: boolean) {
      performReplace(editor, from, to, all);
    },

    clearSearch() {
      clearSearchDecorations();
    },

    isSourceMode(): boolean {
      return isInSourceMode();
    },

    setKeybinding(mode: string) {
      setSourceKeybinding(mode as KeybindingMode);
    },
  };
}

function executeEditorCommand(editor: Editor, command: string, args: any) {
  const chain = editor.chain().focus();

  switch (command) {
    case 'toggleBold': chain.toggleBold().run(); break;
    case 'toggleItalic': chain.toggleItalic().run(); break;
    case 'toggleStrike': chain.toggleStrike().run(); break;
    case 'toggleUnderline': chain.toggleUnderline().run(); break;
    case 'setHeading': chain.toggleHeading({ level: args.level }).run(); break;
    case 'setParagraph': chain.setParagraph().run(); break;
    case 'toggleBulletList': chain.toggleBulletList().run(); break;
    case 'toggleOrderedList': chain.toggleOrderedList().run(); break;
    case 'toggleTaskList': chain.toggleTaskList().run(); break;
    case 'toggleBlockquote': chain.toggleBlockquote().run(); break;
    case 'setCodeBlock': chain.toggleCodeBlock(args.language ? { language: args.language } : undefined).run(); break;
    case 'insertTable': chain.insertTable({ rows: args.rows || 3, cols: args.cols || 3, withHeaderRow: true }).run(); break;
    case 'addRowBefore': chain.addRowBefore().run(); break;
    case 'addRowAfter': chain.addRowAfter().run(); break;
    case 'deleteRow': chain.deleteRow().run(); break;
    case 'addColumnBefore': chain.addColumnBefore().run(); break;
    case 'addColumnAfter': chain.addColumnAfter().run(); break;
    case 'deleteColumn': chain.deleteColumn().run(); break;
    case 'deleteTable': chain.deleteTable().run(); break;
    case 'mergeCells': chain.mergeCells().run(); break;
    case 'splitCell': chain.splitCell().run(); break;
    case 'toggleHeaderRow': chain.toggleHeaderRow().run(); break;
    case 'toggleHeaderColumn': chain.toggleHeaderColumn().run(); break;
    case 'insertHorizontalRule': chain.setHorizontalRule().run(); break;
    case 'insertImage': chain.setImage({ src: args.src, alt: args.alt || '' }).run(); break;
    case 'toggleHighlight': chain.toggleHighlight().run(); break;
    case 'toggleSuperscript': chain.toggleSuperscript().run(); break;
    case 'toggleSubscript': chain.toggleSubscript().run(); break;
    case 'clearFormat': chain.unsetAllMarks().clearNodes().run(); break;
    case 'undo': chain.undo().run(); break;
    case 'redo': chain.redo().run(); break;
    case 'selectAll': chain.selectAll().run(); break;
    case 'insertMermaid':
      editor.chain().focus().insertContent({
        type: 'mermaidBlock',
        attrs: { code: args.code || 'graph TD\n    A-->B' },
      }).run();
      break;
    case 'insertMathBlock':
      editor.chain().focus().insertContent({
        type: 'katexBlock',
        attrs: { code: args.code || 'E = mc^2' },
      }).run();
      break;
    case 'insertMathInline':
      editor.chain().focus().insertContent({
        type: 'katexInline',
        attrs: { code: args.code || 'x^2' },
      }).run();
      break;
    default: console.warn(`[Colason] Unknown command: ${command}`);
  }
}

// === Heading Observer for scroll sync ===
let headingObserver: IntersectionObserver | null = null;
let activeHeadingId = '';

function setupHeadingObserver(editor: Editor) {
  headingObserver = new IntersectionObserver(
    (entries) => {
      const visible = entries
        .filter(e => e.isIntersecting)
        .sort((a, b) => a.boundingClientRect.top - b.boundingClientRect.top);

      if (visible.length > 0) {
        const topHeading = visible[0].target;
        const id = topHeading.getAttribute('data-heading-id') || '';
        if (id && id !== activeHeadingId) {
          activeHeadingId = id;
          notifyActiveHeading(id);
        }
      }
    },
    { root: null, rootMargin: '-10% 0px -80% 0px', threshold: 0 }
  );

  editor.on('update', () => {
    requestAnimationFrame(() => observeHeadings(editor));
  });
}

function observeHeadings(editor: Editor) {
  if (!headingObserver) return;
  headingObserver.disconnect();

  const headings = editor.view.dom.querySelectorAll('h1, h2, h3, h4, h5, h6');
  headings.forEach((heading) => {
    const pos = editor.view.posAtDOM(heading, 0);
    heading.setAttribute('data-heading-id', `heading-${pos}`);
    headingObserver!.observe(heading);
  });
}

// === Search functionality ===
let searchMatches: Array<{ from: number; to: number }> = [];
let currentMatchIndex = -1;

function performSearch(editor: Editor, query: string, caseSensitive: boolean, useRegex: boolean) {
  searchMatches = [];
  currentMatchIndex = -1;

  if (!query) {
    clearSearchDecorations();
    notifySearchResults(0, -1);
    return;
  }

  const doc = editor.state.doc;
  const flags = caseSensitive ? 'g' : 'gi';

  let regex: RegExp;
  try {
    regex = useRegex ? new RegExp(query, flags) : new RegExp(escapeRegex(query), flags);
  } catch {
    notifySearchResults(0, -1);
    return;
  }

  doc.descendants((node, pos) => {
    if (node.isText && node.text) {
      let match: RegExpExecArray | null;
      const nodeRegex = new RegExp(regex.source, regex.flags);
      while ((match = nodeRegex.exec(node.text)) !== null) {
        searchMatches.push({
          from: pos + match.index,
          to: pos + match.index + match[0].length,
        });
      }
    }
  });

  if (searchMatches.length > 0) {
    currentMatchIndex = 0;
    scrollToMatch(editor);
  }

  notifySearchResults(searchMatches.length, currentMatchIndex);
}

function navigateSearch(editor: Editor, direction: 'next' | 'prev') {
  if (searchMatches.length === 0) return;

  if (direction === 'next') {
    currentMatchIndex = (currentMatchIndex + 1) % searchMatches.length;
  } else {
    currentMatchIndex = (currentMatchIndex - 1 + searchMatches.length) % searchMatches.length;
  }

  scrollToMatch(editor);
  notifySearchResults(searchMatches.length, currentMatchIndex);
}

function performReplace(editor: Editor, from: string, to: string, all: boolean) {
  if (searchMatches.length === 0) return;

  if (all) {
    const tr = editor.state.tr;
    const reversedMatches = [...searchMatches].reverse();
    for (const match of reversedMatches) {
      tr.replaceWith(match.from, match.to, editor.state.schema.text(to));
    }
    editor.view.dispatch(tr);
    searchMatches = [];
    currentMatchIndex = -1;
    clearSearchDecorations();
  } else {
    if (currentMatchIndex >= 0 && currentMatchIndex < searchMatches.length) {
      const match = searchMatches[currentMatchIndex];
      editor.chain().focus()
        .setTextSelection({ from: match.from, to: match.to })
        .insertContent(to)
        .run();
      performSearch(editor, from, false, false);
    }
  }
}

function scrollToMatch(editor: Editor) {
  if (currentMatchIndex >= 0 && currentMatchIndex < searchMatches.length) {
    const match = searchMatches[currentMatchIndex];
    editor.commands.setTextSelection({ from: match.from, to: match.to });
    const dom = editor.view.domAtPos(match.from);
    const node = dom.node instanceof HTMLElement ? dom.node : dom.node.parentElement;
    node?.scrollIntoView({ behavior: 'smooth', block: 'center' });
  }
}

function clearSearchDecorations() {
  searchMatches = [];
  currentMatchIndex = -1;
}

function escapeRegex(str: string): string {
  return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// === Notify C++ side ===
export function notifyContentChanged(html: string) {
  cppBridge?.editor?.contentChanged?.(html);
}

export function notifyCursorPosition(line: number, col: number) {
  cppBridge?.editor?.cursorPositionChanged?.(line, col);
}

export function notifyWordCount(words: number, chars: number) {
  cppBridge?.editor?.wordCountChanged?.(words, chars);
}

export function notifyDocumentDirty(dirty: boolean) {
  cppBridge?.editor?.documentDirty?.(dirty);
}

export function notifyHeadingsChanged(headings: any[]) {
  cppBridge?.outline?.headingsChanged?.(JSON.stringify(headings));
}

export function notifyActiveHeading(id: string) {
  cppBridge?.outline?.activeHeadingChanged?.(id);
}

export function notifySearchResults(matchCount: number, currentIndex: number) {
  cppBridge?.search?.searchResultsChanged?.(matchCount, currentIndex);
}

declare global {
  interface Window {
    colasonAPI: any;
    QWebChannel: any;
    qt: any;
  }
}
