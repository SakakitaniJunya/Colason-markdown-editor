import { Node, mergeAttributes } from '@tiptap/core';
import { Plugin, PluginKey } from '@tiptap/pm/state';
import mermaid from 'mermaid';

// Initialize mermaid
mermaid.initialize({
  startOnLoad: false,
  theme: 'default',
  securityLevel: 'loose',
  fontFamily: 'inherit',
  flowchart: { htmlLabels: true, curve: 'basis' },
});

let renderCounter = 0;

const mermaidConvertPluginKey = new PluginKey('mermaidConvert');

export const MermaidBlock = Node.create({
  name: 'mermaidBlock',
  group: 'block',
  atom: true,
  defining: true,
  // Higher priority than CodeBlockLowlight (default 100) so mermaid
  // <pre><code class="language-mermaid"> is parsed as MermaidBlock, not CodeBlock
  priority: 101,

  addAttributes() {
    return {
      code: {
        default: 'graph TD\n    A-->B',
        parseHTML: (element: HTMLElement) => {
          // From <div data-type="mermaid-block">
          if (element.getAttribute('data-type') === 'mermaid-block') {
            return element.getAttribute('code') || element.textContent || '';
          }
          // From <pre><code class="language-mermaid">
          const code = element.querySelector('code');
          return code?.textContent || element.textContent || '';
        },
        renderHTML: (attributes: Record<string, any>) => {
          return { code: attributes.code };
        },
      },
    };
  },

  parseHTML() {
    return [
      { tag: 'div[data-type="mermaid-block"]' },
      {
        tag: 'pre',
        getAttrs(node) {
          const el = node as HTMLElement;
          const code = el.querySelector('code');
          if (code?.classList.contains('language-mermaid') || el.getAttribute('data-language') === 'mermaid') {
            return {};
          }
          return false;
        },
      },
    ];
  },

  renderHTML({ HTMLAttributes }) {
    return ['div', mergeAttributes(HTMLAttributes, { 'data-type': 'mermaid-block' })];
  },

  // Auto-convert code blocks with language "mermaid" to MermaidBlock
  addProseMirrorPlugins() {
    const mermaidBlockType = this.type;

    return [
      new Plugin({
        key: mermaidConvertPluginKey,
        appendTransaction(transactions, _oldState, newState) {
          // Only check if there were actual changes
          if (!transactions.some(tr => tr.docChanged)) return null;

          let tr = newState.tr;
          let changed = false;

          newState.doc.descendants((node, pos) => {
            if (node.type.name === 'codeBlock' && node.attrs.language === 'mermaid') {
              tr.replaceWith(pos, pos + node.nodeSize, mermaidBlockType.create({
                code: node.textContent,
              }));
              changed = true;
              return false; // stop after first replacement to avoid position issues
            }
          });

          return changed ? tr : null;
        },
      }),
    ];
  },

  addNodeView() {
    return ({ node, getPos, editor }) => {
      const container = document.createElement('div');
      container.classList.add('mermaid-block');
      container.setAttribute('data-type', 'mermaid-block');

      const codeArea = document.createElement('textarea');
      codeArea.classList.add('mermaid-code');
      codeArea.value = node.attrs.code;
      codeArea.spellcheck = false;

      const preview = document.createElement('div');
      preview.classList.add('mermaid-preview');

      const errorArea = document.createElement('div');
      errorArea.classList.add('mermaid-error');

      container.appendChild(codeArea);
      container.appendChild(preview);
      container.appendChild(errorArea);

      let debounceTimer: ReturnType<typeof setTimeout>;

      const renderMermaid = async (code: string) => {
        if (!code.trim()) {
          preview.innerHTML = '';
          return;
        }
        errorArea.textContent = '';
        errorArea.style.display = 'none';
        try {
          const id = `mermaid-render-${++renderCounter}`;
          const { svg } = await mermaid.render(id, code);
          preview.innerHTML = svg;
          preview.style.display = '';
        } catch (err: any) {
          preview.innerHTML = '';
          errorArea.textContent = err.message || 'Mermaid syntax error';
          errorArea.style.display = '';
        }
      };

      // Initial render
      renderMermaid(node.attrs.code);

      // Handle code changes
      codeArea.addEventListener('input', () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
          const newCode = codeArea.value;
          renderMermaid(newCode);

          if (typeof getPos === 'function') {
            editor.view.dispatch(
              editor.view.state.tr.setNodeMarkup(getPos(), undefined, {
                ...node.attrs,
                code: newCode,
              })
            );
          }
        }, 300);
      });

      // Show/hide code on focus
      codeArea.style.display = 'none';

      container.addEventListener('click', () => {
        codeArea.style.display = '';
        codeArea.focus();
      });

      codeArea.addEventListener('blur', () => {
        if (codeArea.value.trim()) {
          codeArea.style.display = 'none';
        }
      });

      // Prevent TipTap from intercepting keyboard events
      codeArea.addEventListener('keydown', (e) => {
        e.stopPropagation();
      });

      return {
        dom: container,
        update(updatedNode) {
          if (updatedNode.type.name !== 'mermaidBlock') return false;
          if (updatedNode.attrs.code !== codeArea.value) {
            codeArea.value = updatedNode.attrs.code;
            renderMermaid(updatedNode.attrs.code);
          }
          return true;
        },
        destroy() {
          clearTimeout(debounceTimer);
        },
        stopEvent(event: Event) {
          return container.contains(event.target as globalThis.Node);
        },
        ignoreMutation() {
          return true;
        },
      };
    };
  },
});
