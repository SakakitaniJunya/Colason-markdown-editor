import { Node, mergeAttributes } from '@tiptap/core';
import { Plugin, PluginKey } from '@tiptap/pm/state';
import mermaid from 'mermaid';

let renderCounter = 0;
let currentMermaidTheme = '';

/** Detect dark/light from CSS --bg variable and re-initialize mermaid if needed */
function syncMermaidTheme() {
  const bg = getComputedStyle(document.documentElement).getPropertyValue('--bg').trim();
  const isDark = isColorDark(bg);
  const theme = isDark ? 'dark' : 'default';
  if (theme !== currentMermaidTheme) {
    currentMermaidTheme = theme;
    mermaid.initialize({
      startOnLoad: false,
      theme,
      securityLevel: 'loose',
      fontFamily: 'inherit',
      flowchart: { htmlLabels: true, curve: 'basis' },
    });
  }
}

function isColorDark(color: string): boolean {
  // Parse hex color (#rrggbb or #rgb)
  let r = 0, g = 0, b = 0;
  if (color.startsWith('#')) {
    const hex = color.slice(1);
    if (hex.length === 3) {
      r = parseInt(hex[0] + hex[0], 16);
      g = parseInt(hex[1] + hex[1], 16);
      b = parseInt(hex[2] + hex[2], 16);
    } else if (hex.length >= 6) {
      r = parseInt(hex.slice(0, 2), 16);
      g = parseInt(hex.slice(2, 4), 16);
      b = parseInt(hex.slice(4, 6), 16);
    }
  }
  // Relative luminance
  const luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255;
  return luminance < 0.5;
}

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
        syncMermaidTheme();
        const id = `mermaid-render-${++renderCounter}`;
        try {
          const { svg } = await mermaid.render(id, code);
          preview.innerHTML = svg;
          preview.style.display = '';
        } catch (err: any) {
          preview.innerHTML = '';
          errorArea.textContent = err.message || 'Mermaid syntax error';
          errorArea.style.display = '';
        } finally {
          // Mermaid v11 leaves temporary render/error elements in document.body —
          // remove them to prevent the error overlay from covering the editor.
          // Only remove elements that are direct children of body (not our preview SVG).
          document.querySelectorAll(`body > #${CSS.escape(id)}`).forEach(el => el.remove());
        }
      };

      // Zoom support: Ctrl+wheel to zoom, double-click to reset
      let zoomLevel = 1;
      preview.style.transformOrigin = 'center top';

      preview.addEventListener('wheel', (e) => {
        if (!e.ctrlKey) return;
        e.preventDefault();
        e.stopPropagation();
        const delta = e.deltaY > 0 ? -0.1 : 0.1;
        zoomLevel = Math.min(3, Math.max(0.3, zoomLevel + delta));
        preview.style.transform = zoomLevel === 1 ? '' : `scale(${zoomLevel})`;
      }, { passive: false });

      preview.addEventListener('dblclick', (e) => {
        if (codeArea.style.display !== 'none') return;
        e.stopPropagation();
        zoomLevel = 1;
        preview.style.transform = '';
      });

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
