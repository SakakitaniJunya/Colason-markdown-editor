import { Node, mergeAttributes } from '@tiptap/core';
import { InputRule } from '@tiptap/core';
import katex from 'katex';

export const KaTeXInline = Node.create({
  name: 'katexInline',
  group: 'inline',
  inline: true,
  atom: true,

  addAttributes() {
    return {
      code: { default: '' },
    };
  },

  parseHTML() {
    return [
      { tag: 'span[data-type="katex-inline"]' },
    ];
  },

  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes, { 'data-type': 'katex-inline' })];
  },

  addInputRules() {
    return [
      // Match $...$ but not $$...$$
      new InputRule({
        find: /(?<!\$)\$([^$\n]+)\$$/,
        handler: ({ state, range, match }) => {
          const code = match[1];
          if (!code) return;

          const { tr } = state;
          tr.replaceWith(
            range.from,
            range.to,
            this.type.create({ code })
          );
        },
      }),
    ];
  },

  addNodeView() {
    return ({ node, getPos, editor }) => {
      const span = document.createElement('span');
      span.classList.add('katex-inline');
      span.setAttribute('data-type', 'katex-inline');

      const renderKatex = (code: string) => {
        try {
          span.innerHTML = katex.renderToString(code, {
            displayMode: false,
            throwOnError: false,
            errorColor: '#cc0000',
          });
        } catch {
          span.textContent = `$${code}$`;
        }
      };

      renderKatex(node.attrs.code);

      span.addEventListener('click', () => {
        const newCode = prompt('Edit LaTeX:', node.attrs.code);
        if (newCode !== null && typeof getPos === 'function') {
          editor.view.dispatch(
            editor.view.state.tr.setNodeMarkup(getPos(), undefined, {
              ...node.attrs,
              code: newCode,
            })
          );
        }
      });

      return {
        dom: span,
        update(updatedNode) {
          if (updatedNode.type.name !== 'katexInline') return false;
          renderKatex(updatedNode.attrs.code);
          return true;
        },
        stopEvent() {
          return false;
        },
        ignoreMutation() {
          return true;
        },
      };
    };
  },
});
