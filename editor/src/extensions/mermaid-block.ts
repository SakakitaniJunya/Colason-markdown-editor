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

      // Zoom toolbar (always present, fades in on hover or while zoomed)
      const zoomBar = document.createElement('div');
      zoomBar.classList.add('mermaid-zoom-controls');
      zoomBar.contentEditable = 'false';
      zoomBar.setAttribute('data-no-edit', '');
      const mkBtn = (label: string, title: string, onClick: (e: MouseEvent) => void) => {
        const b = document.createElement('button');
        b.type = 'button';
        b.textContent = label;
        b.title = title;
        b.addEventListener('mousedown', (e) => e.preventDefault());
        b.addEventListener('click', (e) => { e.stopPropagation(); onClick(e); });
        return b;
      };
      const zoomLabel = document.createElement('span');
      zoomLabel.classList.add('mermaid-zoom-label');
      zoomLabel.textContent = '100%';

      container.appendChild(codeArea);
      container.appendChild(preview);
      preview.appendChild(zoomBar);
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

      // Zoom + pan support. The diagram is wrapped in an <svg>; we transform the
      // preview's content via CSS transform. Pan = translate, zoom = scale.
      // Overlay buttons + Esc keep the user from getting stuck when zoomed in.
      let zoomLevel = 1;
      let panX = 0;
      let panY = 0;
      preview.style.transformOrigin = '0 0';

      const applyTransform = () => {
        if (zoomLevel === 1 && panX === 0 && panY === 0) {
          preview.style.transform = '';
          preview.classList.remove('is-zoomed');
        } else {
          preview.style.transform = `translate(${panX}px, ${panY}px) scale(${zoomLevel})`;
          preview.classList.add('is-zoomed');
        }
        zoomLabel.textContent = `${Math.round(zoomLevel * 100)}%`;
      };

      const applyZoom = (next: number) => {
        const prev = zoomLevel;
        zoomLevel = Math.min(4, Math.max(0.25, Math.round(next * 100) / 100));
        // When zooming back to 1×, also reset pan so content snaps to origin.
        if (zoomLevel === 1) { panX = 0; panY = 0; }
        // Keep the visual center stable across zoom changes (zoom around center).
        const rect = preview.getBoundingClientRect();
        const cx = rect.width / 2;
        const cy = rect.height / 2;
        panX = cx - ((cx - panX) * (zoomLevel / prev));
        panY = cy - ((cy - panY) * (zoomLevel / prev));
        if (zoomLevel === 1) { panX = 0; panY = 0; }
        applyTransform();
      };

      const fitToWidth = () => {
        const svg = preview.querySelector('svg') as SVGSVGElement | null;
        if (!svg) { applyZoom(1); return; }
        const containerWidth = preview.clientWidth - 32;
        const svgWidth = svg.getBoundingClientRect().width / zoomLevel;
        if (svgWidth > 0) {
          panX = 0; panY = 0;
          applyZoom(containerWidth / svgWidth);
        }
      };

      // Build toolbar buttons (children appended to existing zoomBar)
      zoomBar.appendChild(mkBtn('−', 'Zoom out', () => applyZoom(zoomLevel - 0.1)));
      zoomBar.appendChild(zoomLabel);
      zoomBar.appendChild(mkBtn('+', 'Zoom in', () => applyZoom(zoomLevel + 0.1)));
      zoomBar.appendChild(mkBtn('⤢', 'Fit width', () => fitToWidth()));
      zoomBar.appendChild(mkBtn('1×', 'Reset zoom (Esc)', () => applyZoom(1)));

      preview.addEventListener('wheel', (e) => {
        if (!(e.ctrlKey || e.metaKey)) return;
        e.preventDefault();
        e.stopPropagation();
        const delta = e.deltaY > 0 ? -0.1 : 0.1;
        applyZoom(zoomLevel + delta);
      }, { passive: false });

      // Drag to pan while zoomed in (mouse) — never hijacks normal click.
      let dragState: { startX: number; startY: number; baseX: number; baseY: number } | null = null;
      preview.addEventListener('mousedown', (e) => {
        if (zoomLevel === 1) return;          // no pan when not zoomed
        if (e.button !== 0) return;           // primary button only
        const target = e.target as HTMLElement;
        if (target.closest('.mermaid-zoom-controls')) return;  // toolbar clicks
        dragState = { startX: e.clientX, startY: e.clientY, baseX: panX, baseY: panY };
        e.preventDefault();
        e.stopPropagation();
      });
      const onMove = (e: MouseEvent) => {
        if (!dragState) return;
        panX = dragState.baseX + (e.clientX - dragState.startX);
        panY = dragState.baseY + (e.clientY - dragState.startY);
        applyTransform();
      };
      const onUp = () => { dragState = null; };
      document.addEventListener('mousemove', onMove);
      document.addEventListener('mouseup', onUp);
      (preview as any)._mermaidPanHandlers = { onMove, onUp };

      preview.addEventListener('dblclick', (e) => {
        if (zoomLevel === 1) return;
        e.stopPropagation();
        e.preventDefault();
        panX = 0; panY = 0;
        applyZoom(1);
      });

      // ESC key resets zoom whenever the preview is in view
      const onKey = (e: KeyboardEvent) => {
        if (e.key === 'Escape' && zoomLevel !== 1 && preview.matches(':hover')) {
          applyZoom(1);
        }
      };
      document.addEventListener('keydown', onKey);
      (preview as any)._mermaidEscHandler = onKey;

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
          const handler = (preview as any)._mermaidEscHandler;
          if (handler) document.removeEventListener('keydown', handler);
          const pan = (preview as any)._mermaidPanHandlers;
          if (pan) {
            document.removeEventListener('mousemove', pan.onMove);
            document.removeEventListener('mouseup', pan.onUp);
          }
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
