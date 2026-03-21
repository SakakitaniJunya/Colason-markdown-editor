import { Node, mergeAttributes } from '@tiptap/core';
import katex from 'katex';
import 'katex/dist/katex.min.css';

export const KaTeXBlock = Node.create({
  name: 'katexBlock',
  group: 'block',
  atom: true,
  defining: true,

  addAttributes() {
    return {
      code: { default: '' },
    };
  },

  parseHTML() {
    return [
      { tag: 'div[data-type="katex-block"]' },
    ];
  },

  renderHTML({ HTMLAttributes }) {
    return ['div', mergeAttributes(HTMLAttributes, { 'data-type': 'katex-block' })];
  },

  addNodeView() {
    return ({ node, getPos, editor }) => {
      const container = document.createElement('div');
      container.classList.add('katex-block');
      container.setAttribute('data-type', 'katex-block');

      const codeArea = document.createElement('textarea');
      codeArea.classList.add('katex-code');
      codeArea.value = node.attrs.code;
      codeArea.spellcheck = false;
      codeArea.placeholder = 'LaTeX math expression...';

      const preview = document.createElement('div');
      preview.classList.add('katex-preview');

      const errorArea = document.createElement('div');
      errorArea.classList.add('katex-error');

      container.appendChild(codeArea);
      container.appendChild(preview);
      container.appendChild(errorArea);

      const renderKatex = (code: string) => {
        errorArea.textContent = '';
        errorArea.style.display = 'none';
        try {
          preview.innerHTML = katex.renderToString(code, {
            displayMode: true,
            throwOnError: false,
            errorColor: '#cc0000',
          });
          preview.style.display = '';
        } catch (err: any) {
          errorArea.textContent = err.message || 'LaTeX error';
          errorArea.style.display = '';
        }
      };

      renderKatex(node.attrs.code);

      let debounceTimer: ReturnType<typeof setTimeout>;

      codeArea.addEventListener('input', () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
          const newCode = codeArea.value;
          renderKatex(newCode);

          if (typeof getPos === 'function') {
            editor.view.dispatch(
              editor.view.state.tr.setNodeMarkup(getPos(), undefined, {
                ...node.attrs,
                code: newCode,
              })
            );
          }
        }, 200);
      });

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

      codeArea.addEventListener('keydown', (e) => {
        e.stopPropagation();
      });

      return {
        dom: container,
        update(updatedNode) {
          if (updatedNode.type.name !== 'katexBlock') return false;
          if (updatedNode.attrs.code !== codeArea.value) {
            codeArea.value = updatedNode.attrs.code;
            renderKatex(updatedNode.attrs.code);
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
