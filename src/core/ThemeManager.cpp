#include "ThemeManager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStyleHints>
#include <QGuiApplication>
#include <QPalette>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    registerBuiltinThemes();
}

void ThemeManager::registerBuiltinThemes()
{
    // ── Light theme ──
    m_themes["light"] = {
        "Light",
        // editorCSS — only CSS custom property overrides (base.css defaults are already light)
        ":root {"
        "  --bg: #ffffff; --fg: #333333; --fg-muted: #586069; --fg-subtle: #6a737d;"
        "  --border: #e1e4e8; --border-light: #eee; --surface: #f6f8fa; --surface-alt: #f8f9fa;"
        "  --link: #4183c4; --heading: inherit; --heading-border: #eee;"
        "  --code-bg: #f6f8fa; --code-border: #e1e4e8; --code-fg: #e83e8c;"
        "  --pre-bg: #f6f8fa; --pre-border: #e1e4e8; --pre-fg: inherit;"
        "  --bq-border: #dfe2e5; --bq-fg: #6a737d;"
        "  --th-bg: #f6f8fa; --td-border: #dfe2e5; --tr-alt-bg: #f8f9fa;"
        "  --mark-bg: #fff3a3; --mark-fg: inherit; --hr-border: #e1e4e8; --strike-fg: #999;"
        "  --selection-bg: #b4d5fe; --placeholder-fg: #adb5bd;"
        "  --scrollbar-thumb: #ccc; --scrollbar-thumb-hover: #999;"
        "  --hljs-fg: #24292e; --hljs-keyword: #d73a49; --hljs-string: #032f62;"
        "  --hljs-comment: #6a737d; --hljs-number: #005cc5; --hljs-function: #6f42c1; --hljs-builtin: #e36209;"
        "  --mermaid-border: #e1e4e8; --mermaid-bg: #fafbfc; --mermaid-code-bg: #f6f8fa; --mermaid-code-fg: #333;"
        "  --mermaid-err-bg: #ffeef0; --mermaid-err-fg: #cb2431; --mermaid-err-border: #fdb8c0;"
        "  --katex-border: #e1e4e8; --katex-bg: #fafbfc; --katex-code-bg: #f6f8fa; --katex-code-fg: #333;"
        "  --katex-hover-bg: #f0f0f0; --task-done-fg: #999;"
        "}",
        // appQSS
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #fafbfc; }"

        "QMenuBar { background: #fafbfc; color: #24292e; border-bottom: 1px solid #e1e4e8; padding: 2px 0; }"
        "QMenuBar::item { padding: 4px 10px; background: transparent; }"
        "QMenuBar::item:selected { background: #e8e8e8; border-radius: 4px; }"
        "QMenu { background: #ffffff; color: #24292e; border: 1px solid #d0d7de; border-radius: 6px; padding: 4px 0; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #e8f0fe; }"
        "QMenu::separator { height: 1px; background: #e1e4e8; margin: 4px 8px; }"

        "QStatusBar { background: #fafbfc; border-top: 1px solid #e1e4e8; padding: 2px 0; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { color: #586069; font-size: 12px; padding: 0 8px; }"

        "QSplitter::handle { background: #e1e4e8; width: 1px; }"

        "QTabWidget::pane { border: none; border-right: 1px solid #e1e4e8; }"
        "QTabBar { background: #fafbfc; }"
        "QTabBar::tab { padding: 5px 7px; border: none; border-bottom: 2px solid transparent;"
        "  color: #586069; font-size: 11px; font-weight: 600; text-transform: uppercase; }"
        "QTabBar::tab:selected { color: #24292e; border-bottom-color: #24292e; }"
        "QTabBar::tab:hover { color: #24292e; }"

        "QTreeView { background: #ffffff; color: #24292e; border: none; outline: none; font-size: 12px; }"
        "QTreeView::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover { background: #f6f8fa; }"
        "QTreeView::item:selected { background: #e8f0fe; color: #24292e; }"
        "QTreeView::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed { image: url(:/icons/chevron-right-dark.svg); }"
        "QTreeView::branch:has-children:open { image: url(:/icons/chevron-down-dark.svg); }"
        "QTreeWidget { background: #ffffff; color: #24292e; border: none; outline: none; font-size: 12px; }"
        "QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeWidget::item:hover { background: #f6f8fa; }"
        "QTreeWidget::item:selected { background: #e8f0fe; color: #24292e; }"
        "QTreeWidget::branch { background: transparent; }"
        "QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-dark.svg); }"
        "QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-dark.svg); }"

        "QListWidget { background: #ffffff; border: none; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #f0f0f0; }"
        "QListWidget::item:hover { background: #f6f8fa; }"
        "QListWidget::item:selected { background: #e8f0fe; }"

        "#folderHeader { color: #586069; font-size: 11px; font-weight: 700; letter-spacing: 1px;"
        "  border-bottom: 1px solid #e1e4e8; padding: 5px 7px; background: #fafbfc; }"
        "#filePlaceholder { color: #8b949e; font-size: 12px; }"
        "#openFolderBtn { background: #f6f8fa; color: #24292e; border: 1px solid #d0d7de;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "#openFolderBtn:hover { background: #e8f0fe; border-color: #0969da; }"

        "#docIndex { color: #586069; font-size: 12px; font-weight: 600; }"
        "#docExt { color: #8b949e; font-size: 11px; }"
        "#docTitle { color: #24292e; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #586069; font-size: 11px; line-height: 1.3; }"

        "QLineEdit { background: #ffffff; color: #24292e; border: 1px solid #d0d7de;"
        "  border-radius: 6px; padding: 4px 8px; }"

        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #c8c8c8; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #999; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }",
        false
    };

    // ── Dark theme (Typora-like) ──
    m_themes["dark"] = {
        "Dark",
        // editorCSS — CSS custom property overrides only
        ":root {"
        "  --bg: #2b2b2b; --fg: #c8c8c8; --fg-muted: #999; --fg-subtle: #8b949e;"
        "  --border: #404040; --border-light: #404040; --surface: #1e1e1e; --surface-alt: #2f2f2f;"
        "  --link: #6cb6ff; --heading: #e0e0e0; --heading-border: #404040;"
        "  --code-bg: #363636; --code-border: #4a4a4a; --code-fg: #f97583;"
        "  --pre-bg: #1e1e1e; --pre-border: #404040; --pre-fg: #d4d4d4;"
        "  --bq-border: #505050; --bq-fg: #999;"
        "  --th-bg: #333; --td-border: #4a4a4a; --tr-alt-bg: #2f2f2f;"
        "  --mark-bg: #5c4b00; --mark-fg: #e0e0e0; --hr-border: #404040; --strike-fg: #666;"
        "  --selection-bg: #264f78; --placeholder-fg: #555;"
        "  --scrollbar-thumb: #555; --scrollbar-thumb-hover: #777;"
        "  --hljs-fg: #d4d4d4; --hljs-keyword: #c586c0; --hljs-string: #ce9178;"
        "  --hljs-comment: #6a9955; --hljs-number: #b5cea8; --hljs-function: #dcdcaa; --hljs-builtin: #4ec9b0;"
        "  --mermaid-border: #404040; --mermaid-bg: #1e1e1e; --mermaid-code-bg: #1e1e1e; --mermaid-code-fg: #d4d4d4;"
        "  --mermaid-err-bg: #3c1f1f; --mermaid-err-fg: #f48771; --mermaid-err-border: #5a2020;"
        "  --katex-border: #404040; --katex-bg: #1e1e1e; --katex-code-bg: #1e1e1e; --katex-code-fg: #d4d4d4;"
        "  --katex-hover-bg: #333; --task-done-fg: #666;"
        "}",

        // appQSS — comprehensive Typora-like dark styling
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #2b2b2b; }"

        // Title bar area
        "QMenuBar { background: #2b2b2b; color: #bbb; border: none; padding: 2px 0; }"
        "QMenuBar::item { padding: 4px 10px; background: transparent; border-radius: 4px; }"
        "QMenuBar::item:selected { background: #3c3c3c; color: #e0e0e0; }"
        "QMenu { background: #252526; color: #ccc; border: 1px solid #3c3c3c; border-radius: 6px; padding: 4px 0; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #094771; color: #fff; }"
        "QMenu::separator { height: 1px; background: #3c3c3c; margin: 4px 8px; }"
        "QMenu::item:disabled { color: #666; }"

        // Status bar
        "QStatusBar { background: #252526; border-top: 1px solid #333; padding: 2px 0; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { color: #888; font-size: 12px; padding: 0 8px; }"

        // Splitter handle
        "QSplitter::handle { background: #333; width: 1px; }"

        // Tab widget (sidebar tabs)
        "QTabWidget::pane { border: none; border-right: 1px solid #333; }"
        "QTabBar { background: #2b2b2b; }"
        "QTabBar::tab { padding: 5px 7px; border: none; border-bottom: 2px solid transparent;"
        "  color: #888; font-size: 11px; font-weight: 600; text-transform: uppercase;"
        "  background: transparent; }"
        "QTabBar::tab:selected { color: #e0e0e0; border-bottom-color: #e0e0e0; }"
        "QTabBar::tab:hover { color: #ccc; }"

        // File tree
        "QTreeView { background: #2b2b2b; color: #bbb; border: none; outline: none; font-size: 12px; }"
        "QTreeView::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover { background: #333; }"
        "QTreeView::item:selected { background: #37373d; color: #e0e0e0; }"
        "QTreeView::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed { image: url(:/icons/chevron-right-light.svg); }"
        "QTreeView::branch:has-children:open { image: url(:/icons/chevron-down-light.svg); }"
        "QTreeWidget { background: #2b2b2b; color: #bbb; border: none; outline: none; font-size: 12px; }"
        "QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeWidget::item:hover { background: #333; }"
        "QTreeWidget::item:selected { background: #37373d; color: #e0e0e0; }"
        "QTreeWidget::branch { background: transparent; }"
        "QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-light.svg); }"
        "QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-light.svg); }"

        // Document list
        "QListWidget { background: #2b2b2b; border: none; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #333; }"
        "QListWidget::item:hover { background: #333; }"
        "QListWidget::item:selected { background: #37373d; }"

        "#folderHeader { color: #888; font-size: 11px; font-weight: 700; letter-spacing: 1px;"
        "  border-bottom: 1px solid #333; padding: 5px 7px; background: #252526; }"
        "#filePlaceholder { color: #666; font-size: 12px; }"
        "#openFolderBtn { background: #3c3c3c; color: #ccc; border: 1px solid #555;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "#openFolderBtn:hover { background: #094771; border-color: #007acc; color: #fff; }"

        "#docIndex { color: #888; font-size: 12px; font-weight: 600; }"
        "#docExt { color: #666; font-size: 11px; }"
        "#docTitle { color: #ddd; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #888; font-size: 11px; line-height: 1.3; }"

        // Input
        "QLineEdit { background: #3c3c3c; color: #ccc; border: 1px solid #555; border-radius: 6px; padding: 4px 8px; }"
        "QLineEdit:focus { border-color: #007acc; }"

        // Scrollbar
        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #555; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #777; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal { height: 8px; background: transparent; }"
        "QScrollBar::handle:horizontal { background: #555; border-radius: 4px; min-width: 30px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"

        // Tooltips
        "QToolTip { background: #333; color: #ddd; border: 1px solid #555; padding: 4px 8px; }",
        true
    };

    // ── GitHub Light theme ──
    m_themes["github-light"] = {
        "GitHub Light",
        ":root {"
        "  --bg: #ffffff; --fg: #24292f; --fg-muted: #57606a; --fg-subtle: #6e7781;"
        "  --border: #d0d7de; --border-light: #d0d7de; --surface: #f6f8fa; --surface-alt: #f6f8fa;"
        "  --link: #0969da; --heading: inherit; --heading-border: #d0d7de;"
        "  --code-bg: rgba(175,184,193,0.2); --code-border: #d0d7de; --code-fg: #24292f;"
        "  --pre-bg: #f6f8fa; --pre-border: #d0d7de; --pre-fg: #24292f;"
        "  --bq-border: #d0d7de; --bq-fg: #57606a;"
        "  --th-bg: #f6f8fa; --td-border: #d0d7de; --tr-alt-bg: #f6f8fa;"
        "  --mark-bg: #fff8c5; --mark-fg: inherit; --hr-border: #d0d7de; --strike-fg: #8b949e;"
        "  --selection-bg: #b4d5fe; --placeholder-fg: #8b949e;"
        "  --scrollbar-thumb: #c8c8c8; --scrollbar-thumb-hover: #999;"
        "  --hljs-fg: #24292f; --hljs-keyword: #cf222e; --hljs-string: #0a3069;"
        "  --hljs-comment: #6e7781; --hljs-number: #0550ae; --hljs-function: #8250df; --hljs-builtin: #953800;"
        "  --mermaid-border: #d0d7de; --mermaid-bg: #f6f8fa; --mermaid-code-bg: #f6f8fa; --mermaid-code-fg: #24292f;"
        "  --mermaid-err-bg: #ffebe9; --mermaid-err-fg: #cf222e; --mermaid-err-border: #ff8182;"
        "  --katex-border: #d0d7de; --katex-bg: #f6f8fa; --katex-code-bg: #f6f8fa; --katex-code-fg: #24292f;"
        "  --katex-hover-bg: #eef2f6; --task-done-fg: #8b949e;"
        "}",
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #f6f8fa; }"
        "QMenuBar { background: #f6f8fa; color: #24292f; border-bottom: 1px solid #d0d7de; padding: 2px 0; }"
        "QMenuBar::item { padding: 4px 10px; background: transparent; }"
        "QMenuBar::item:selected { background: #eef2f6; border-radius: 4px; }"
        "QMenu { background: #ffffff; color: #24292f; border: 1px solid #d0d7de; border-radius: 6px; padding: 4px 0; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #0969da; color: white; }"
        "QMenu::separator { height: 1px; background: #d0d7de; margin: 4px 8px; }"
        "QStatusBar { background: #f6f8fa; border-top: 1px solid #d0d7de; padding: 2px 0; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { color: #57606a; font-size: 12px; padding: 0 8px; }"
        "QSplitter::handle { background: #d0d7de; width: 1px; }"
        "QTabWidget::pane { border: none; border-right: 1px solid #d0d7de; }"
        "QTabBar { background: #f6f8fa; }"
        "QTabBar::tab { padding: 5px 7px; border: none; color: #57606a; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #24292f; border-bottom-color: #fd8c73; }"
        "QTabBar::tab:hover { color: #24292f; }"
        "QTreeView, QTreeWidget { background: #f6f8fa; color: #24292f; border: none; outline: none; font-size: 12px; }"
        "QTreeView::item, QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover, QTreeWidget::item:hover { background: #eef2f6; }"
        "QTreeView::item:selected, QTreeWidget::item:selected { background: #ddf4ff; color: #24292f; }"
        "QTreeView::branch, QTreeWidget::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed, QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-dark.svg); }"
        "QTreeView::branch:has-children:open, QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-dark.svg); }"
        "QListWidget { background: #f6f8fa; border: none; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #eef2f6; }"
        "QListWidget::item:hover { background: #eef2f6; }"
        "QListWidget::item:selected { background: #ddf4ff; }"
        "#folderHeader { color: #57606a; font-size: 11px; font-weight: 700; letter-spacing: 1px;"
        "  border-bottom: 1px solid #d0d7de; padding: 5px 7px; background: #f6f8fa; }"
        "#filePlaceholder { color: #8b949e; font-size: 12px; }"
        "#openFolderBtn { background: #f6f8fa; color: #24292f; border: 1px solid #d0d7de;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "#openFolderBtn:hover { background: #ddf4ff; border-color: #0969da; }"
        "#docIndex { color: #57606a; font-size: 12px; font-weight: 600; }"
        "#docExt { color: #8b949e; font-size: 11px; }"
        "#docTitle { color: #24292f; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #57606a; font-size: 11px; }"
        "QLineEdit { background: #ffffff; color: #24292f; border: 1px solid #d0d7de;"
        "  border-radius: 6px; padding: 4px 8px; }"
        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #c8c8c8; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #999; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }",
        false
    };

    // ── GitHub Dark theme ──
    m_themes["github-dark"] = {
        "GitHub Dark",
        ":root {"
        "  --bg: #0d1117; --fg: #e6edf3; --fg-muted: #8b949e; --fg-subtle: #8b949e;"
        "  --border: #30363d; --border-light: #30363d; --surface: #161b22; --surface-alt: #161b22;"
        "  --link: #58a6ff; --heading: #e6edf3; --heading-border: #30363d;"
        "  --code-bg: rgba(110,118,129,0.4); --code-border: #30363d; --code-fg: #e6edf3;"
        "  --pre-bg: #161b22; --pre-border: #30363d; --pre-fg: #e6edf3;"
        "  --bq-border: #30363d; --bq-fg: #8b949e;"
        "  --th-bg: #161b22; --td-border: #30363d; --tr-alt-bg: #161b22;"
        "  --mark-bg: rgba(187,128,9,0.4); --mark-fg: #e6edf3; --hr-border: #30363d; --strike-fg: #8b949e;"
        "  --selection-bg: #264f78; --placeholder-fg: #484f58;"
        "  --scrollbar-thumb: #484f58; --scrollbar-thumb-hover: #6e7681;"
        "  --hljs-fg: #e6edf3; --hljs-keyword: #ff7b72; --hljs-string: #a5d6ff;"
        "  --hljs-comment: #8b949e; --hljs-number: #79c0ff; --hljs-function: #d2a8ff; --hljs-builtin: #ffa657;"
        "  --mermaid-border: #30363d; --mermaid-bg: #161b22; --mermaid-code-bg: #161b22; --mermaid-code-fg: #e6edf3;"
        "  --mermaid-err-bg: #2d1117; --mermaid-err-fg: #ff7b72; --mermaid-err-border: #3d1c20;"
        "  --katex-border: #30363d; --katex-bg: #161b22; --katex-code-bg: #161b22; --katex-code-fg: #e6edf3;"
        "  --katex-hover-bg: #161b22; --task-done-fg: #8b949e;"
        "}",
        // appQSS
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #0d1117; }"
        "QMenuBar { background: #161b22; color: #e6edf3; border-bottom: 1px solid #30363d; padding: 2px 0; }"
        "QMenuBar::item { padding: 4px 10px; background: transparent; border-radius: 4px; }"
        "QMenuBar::item:selected { background: #30363d; color: #e6edf3; }"
        "QMenu { background: #161b22; color: #e6edf3; border: 1px solid #30363d; border-radius: 6px; padding: 4px 0; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #1f6feb; color: #ffffff; }"
        "QMenu::separator { height: 1px; background: #30363d; margin: 4px 8px; }"
        "QMenu::item:disabled { color: #484f58; }"
        "QStatusBar { background: #161b22; border-top: 1px solid #30363d; padding: 2px 0; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { color: #8b949e; font-size: 12px; padding: 0 8px; }"
        "QSplitter::handle { background: #30363d; width: 1px; }"
        "QTabWidget::pane { border: none; border-right: 1px solid #30363d; }"
        "QTabBar { background: #0d1117; }"
        "QTabBar::tab { padding: 5px 7px; border: none; border-bottom: 2px solid transparent;"
        "  color: #8b949e; font-size: 11px; font-weight: 600; text-transform: uppercase;"
        "  background: transparent; }"
        "QTabBar::tab:selected { color: #e6edf3; border-bottom-color: #f78166; }"
        "QTabBar::tab:hover { color: #e6edf3; }"
        "QTreeView, QTreeWidget { background: #0d1117; color: #e6edf3; border: none; outline: none; font-size: 12px; }"
        "QTreeView::item, QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover, QTreeWidget::item:hover { background: #161b22; }"
        "QTreeView::item:selected, QTreeWidget::item:selected { background: #1f2937; color: #e6edf3; }"
        "QTreeView::branch, QTreeWidget::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed, QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-light.svg); }"
        "QTreeView::branch:has-children:open, QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-light.svg); }"
        "QListWidget { background: #0d1117; border: none; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #161b22; }"
        "QListWidget::item:hover { background: #161b22; }"
        "QListWidget::item:selected { background: #1f2937; }"
        "#folderHeader { color: #8b949e; font-size: 11px; font-weight: 700; letter-spacing: 1px;"
        "  border-bottom: 1px solid #30363d; padding: 5px 7px; background: #161b22; }"
        "#filePlaceholder { color: #484f58; font-size: 12px; }"
        "#openFolderBtn { background: #21262d; color: #e6edf3; border: 1px solid #30363d;"
        "  border-radius: 6px; padding: 5px 12px; font-size: 12px; }"
        "#openFolderBtn:hover { background: #1f6feb; border-color: #58a6ff; color: #ffffff; }"
        "#docIndex { color: #8b949e; font-size: 12px; font-weight: 600; }"
        "#docExt { color: #484f58; font-size: 11px; }"
        "#docTitle { color: #e6edf3; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #8b949e; font-size: 11px; line-height: 1.3; }"
        "QLineEdit { background: #0d1117; color: #e6edf3; border: 1px solid #30363d;"
        "  border-radius: 6px; padding: 4px 8px; }"
        "QLineEdit:focus { border-color: #58a6ff; }"
        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #484f58; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #6e7681; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal { height: 8px; background: transparent; }"
        "QScrollBar::handle:horizontal { background: #484f58; border-radius: 4px; min-width: 30px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"
        "QToolTip { background: #161b22; color: #e6edf3; border: 1px solid #30363d; padding: 4px 8px; }",
        true
    };

    // ── Sepia theme ──
    m_themes["sepia"] = {
        "Sepia",
        ":root {"
        "  --bg: #f4ecd8; --fg: #5b4636; --fg-muted: #7a6652; --fg-subtle: #9e8c75;"
        "  --border: #d6c9a8; --border-light: #d6c9a8; --surface: #ece0c8; --surface-alt: #ede2ce;"
        "  --link: #8b6914; --heading: #3d2b1f; --heading-border: #d6c9a8;"
        "  --code-bg: #ece0c8; --code-border: #d6c9a8; --code-fg: #8b4513;"
        "  --pre-bg: #ece0c8; --pre-border: #d6c9a8; --pre-fg: #5b4636;"
        "  --bq-border: #c4b08c; --bq-fg: #7a6652;"
        "  --th-bg: #ece0c8; --td-border: #d6c9a8; --tr-alt-bg: #ede2ce;"
        "  --mark-bg: #f0d880; --mark-fg: inherit; --hr-border: #d6c9a8; --strike-fg: #9e8c75;"
        "  --selection-bg: #d6c9a8; --placeholder-fg: #9e8c75;"
        "  --scrollbar-thumb: #c4b08c; --scrollbar-thumb-hover: #a89070;"
        "  --hljs-fg: #5b4636; --hljs-keyword: #8b4513; --hljs-string: #6b4226;"
        "  --hljs-comment: #9e8c75; --hljs-number: #8b6914; --hljs-function: #7a5230; --hljs-builtin: #a0522d;"
        "  --mermaid-border: #d6c9a8; --mermaid-bg: #ece0c8; --mermaid-code-bg: #ece0c8; --mermaid-code-fg: #5b4636;"
        "  --mermaid-err-bg: #f0d8c0; --mermaid-err-fg: #8b0000; --mermaid-err-border: #c4a080;"
        "  --katex-border: #d6c9a8; --katex-bg: #ece0c8; --katex-code-bg: #ece0c8; --katex-code-fg: #5b4636;"
        "  --katex-hover-bg: #e0d0b0; --task-done-fg: #9e8c75;"
        "}",
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #ece0c8; }"
        "QMenuBar { background: #d6c9a8; color: #3d2b1f; }"
        "QStatusBar { background: #d6c9a8; color: #3d2b1f; }"
        "QSplitter::handle { background: #c4b08c; width: 1px; }"
        "QTabWidget::pane { border-right: 1px solid #c4b08c; }"
        "QTabBar::tab { padding: 5px 7px; color: #7a6652; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #3d2b1f; border-bottom-color: #8b6914; }"
        "QTreeView, QTreeWidget { background: #f4ecd8; color: #5b4636; border: none; font-size: 12px; }"
        "QTreeView::item, QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover, QTreeWidget::item:hover { background: #ece0c8; }"
        "QTreeView::item:selected, QTreeWidget::item:selected { background: #e0d0a8; }"
        "QTreeView::branch, QTreeWidget::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed, QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-dark.svg); }"
        "QTreeView::branch:has-children:open, QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-dark.svg); }"
        "QListWidget { background: #f4ecd8; border: none; }"
        "QListWidget::item:selected { background: #e0d0a8; }"
        "#docTitle { color: #3d2b1f; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #7a6652; font-size: 11px; }",
        false
    };

    // ── Nord theme ──
    m_themes["nord"] = {
        "Nord",
        ":root {"
        "  --bg: #2e3440; --fg: #d8dee9; --fg-muted: #7b88a1; --fg-subtle: #616e88;"
        "  --border: #434c5e; --border-light: #3b4252; --surface: #3b4252; --surface-alt: #353b49;"
        "  --link: #88c0d0; --heading: #eceff4; --heading-border: #3b4252;"
        "  --code-bg: #3b4252; --code-border: #434c5e; --code-fg: #bf616a;"
        "  --pre-bg: #3b4252; --pre-border: #434c5e; --pre-fg: #d8dee9;"
        "  --bq-border: #4c566a; --bq-fg: #a3be8c;"
        "  --th-bg: #3b4252; --td-border: #434c5e; --tr-alt-bg: #353b49;"
        "  --mark-bg: #ebcb8b; --mark-fg: #2e3440; --hr-border: #434c5e; --strike-fg: #616e88;"
        "  --selection-bg: #434c5e; --placeholder-fg: #4c566a;"
        "  --scrollbar-thumb: #4c566a; --scrollbar-thumb-hover: #616e88;"
        "  --hljs-fg: #d8dee9; --hljs-keyword: #81a1c1; --hljs-string: #a3be8c;"
        "  --hljs-comment: #616e88; --hljs-number: #b48ead; --hljs-function: #88c0d0; --hljs-builtin: #8fbcbb;"
        "  --mermaid-border: #434c5e; --mermaid-bg: #3b4252; --mermaid-code-bg: #3b4252; --mermaid-code-fg: #d8dee9;"
        "  --mermaid-err-bg: #3b2c2c; --mermaid-err-fg: #bf616a; --mermaid-err-border: #4c3030;"
        "  --katex-border: #434c5e; --katex-bg: #3b4252; --katex-code-bg: #3b4252; --katex-code-fg: #d8dee9;"
        "  --katex-hover-bg: #3b4252; --task-done-fg: #616e88;"
        "}",
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #2e3440; }"
        "QMenuBar { background: #2e3440; color: #d8dee9; border: none; }"
        "QMenuBar::item:selected { background: #3b4252; }"
        "QMenu { background: #3b4252; color: #d8dee9; border: 1px solid #434c5e; }"
        "QMenu::item:selected { background: #434c5e; }"
        "QStatusBar { background: #3b4252; color: #d8dee9; }"
        "QStatusBar QLabel { color: #d8dee9; }"
        "QSplitter::handle { background: #3b4252; width: 1px; }"
        "QTabWidget::pane { border-right: 1px solid #3b4252; }"
        "QTabBar::tab { padding: 5px 7px; color: #7b88a1; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #eceff4; border-bottom-color: #88c0d0; }"
        "QTreeView, QTreeWidget { background: #2e3440; color: #d8dee9; border: none; font-size: 12px; }"
        "QTreeView::item, QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover, QTreeWidget::item:hover { background: #3b4252; }"
        "QTreeView::item:selected, QTreeWidget::item:selected { background: #434c5e; }"
        "QTreeView::branch, QTreeWidget::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed, QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-light.svg); }"
        "QTreeView::branch:has-children:open, QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-light.svg); }"
        "QListWidget { background: #2e3440; border: none; }"
        "QListWidget::item:hover { background: #3b4252; }"
        "QListWidget::item:selected { background: #434c5e; }"
        "#docTitle { color: #eceff4; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #7b88a1; font-size: 11px; }"
        "QLineEdit { background: #3b4252; color: #d8dee9; border: 1px solid #434c5e; }"
        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #4c566a; border-radius: 4px; }"
        "QToolTip { background: #3b4252; color: #d8dee9; border: 1px solid #434c5e; }",
        true
    };

    // ── Dracula theme ──
    m_themes["dracula"] = {
        "Dracula",
        ":root {"
        "  --bg: #282a36; --fg: #f8f8f2; --fg-muted: #6272a4; --fg-subtle: #6272a4;"
        "  --border: #6272a4; --border-light: #44475a; --surface: #44475a; --surface-alt: #313340;"
        "  --link: #8be9fd; --heading: #f8f8f2; --heading-border: #44475a;"
        "  --code-bg: #44475a; --code-border: #6272a4; --code-fg: #ff79c6;"
        "  --pre-bg: #44475a; --pre-border: #6272a4; --pre-fg: #f8f8f2;"
        "  --bq-border: #6272a4; --bq-fg: #bd93f9;"
        "  --th-bg: #44475a; --td-border: #6272a4; --tr-alt-bg: #313340;"
        "  --mark-bg: #f1fa8c; --mark-fg: #282a36; --hr-border: #6272a4; --strike-fg: #6272a4;"
        "  --selection-bg: #44475a; --placeholder-fg: #6272a4;"
        "  --scrollbar-thumb: #6272a4; --scrollbar-thumb-hover: #7b8ab8;"
        "  --hljs-fg: #f8f8f2; --hljs-keyword: #ff79c6; --hljs-string: #f1fa8c;"
        "  --hljs-comment: #6272a4; --hljs-number: #bd93f9; --hljs-function: #50fa7b; --hljs-builtin: #8be9fd;"
        "  --mermaid-border: #6272a4; --mermaid-bg: #44475a; --mermaid-code-bg: #44475a; --mermaid-code-fg: #f8f8f2;"
        "  --mermaid-err-bg: #3c1f2a; --mermaid-err-fg: #ff5555; --mermaid-err-border: #5a2030;"
        "  --katex-border: #6272a4; --katex-bg: #44475a; --katex-code-bg: #44475a; --katex-code-fg: #f8f8f2;"
        "  --katex-hover-bg: #44475a; --task-done-fg: #6272a4;"
        "}",
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #282a36; }"
        "QMenuBar { background: #21222c; color: #f8f8f2; border: none; }"
        "QMenuBar::item:selected { background: #44475a; }"
        "QMenu { background: #282a36; color: #f8f8f2; border: 1px solid #44475a; }"
        "QMenu::item:selected { background: #44475a; }"
        "QStatusBar { background: #21222c; color: #f8f8f2; }"
        "QStatusBar QLabel { color: #f8f8f2; }"
        "QSplitter::handle { background: #44475a; width: 1px; }"
        "QTabWidget::pane { border-right: 1px solid #44475a; }"
        "QTabBar::tab { padding: 5px 7px; color: #6272a4; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #f8f8f2; border-bottom-color: #bd93f9; }"
        "QTreeView, QTreeWidget { background: #282a36; color: #f8f8f2; border: none; font-size: 12px; }"
        "QTreeView::item, QTreeWidget::item { padding: 2px 0; margin: 0; }"
        "QTreeView::item:hover, QTreeWidget::item:hover { background: #44475a; }"
        "QTreeView::item:selected, QTreeWidget::item:selected { background: #6272a4; }"
        "QTreeView::branch, QTreeWidget::branch { background: transparent; }"
        "QTreeView::branch:has-children:closed, QTreeWidget::branch:has-children:closed { image: url(:/icons/chevron-right-light.svg); }"
        "QTreeView::branch:has-children:open, QTreeWidget::branch:has-children:open { image: url(:/icons/chevron-down-light.svg); }"
        "QListWidget { background: #282a36; border: none; }"
        "QListWidget::item:hover { background: #44475a; }"
        "QListWidget::item:selected { background: #6272a4; }"
        "#docTitle { color: #f8f8f2; font-size: 13px; font-weight: 600; }"
        "#docPreview { color: #6272a4; font-size: 11px; }"
        "QLineEdit { background: #44475a; color: #f8f8f2; border: 1px solid #6272a4; }"
        "QScrollBar:vertical { width: 8px; background: transparent; }"
        "QScrollBar::handle:vertical { background: #6272a4; border-radius: 4px; }"
        "QToolTip { background: #282a36; color: #f8f8f2; border: 1px solid #6272a4; }",
        true
    };
}

QStringList ThemeManager::availableThemes() const
{
    return m_themes.keys();
}

QString ThemeManager::themeCSS(const QString& themeName) const
{
    if (m_themes.contains(themeName)) {
        return m_themes[themeName].editorCSS;
    }
    return {};
}

QString ThemeManager::themeQSS(const QString& themeName) const
{
    if (m_themes.contains(themeName)) {
        return m_themes[themeName].appQSS;
    }
    return {};
}

void ThemeManager::setTheme(const QString& themeName)
{
    if (!m_themes.contains(themeName)) return;
    m_currentTheme = themeName;
    const auto& theme = m_themes[themeName];
    emit themeChanged(themeName, theme.editorCSS, theme.appQSS);
}

void ThemeManager::loadCustomTheme(const QString& cssFilePath)
{
    QString css = readFile(cssFilePath);
    if (css.isEmpty()) return;

    QFileInfo fi(cssFilePath);
    QString name = "custom-" + fi.completeBaseName();

    m_themes[name] = {
        fi.completeBaseName(),
        css,
        {}, // No QSS for custom themes
        false
    };

    setTheme(name);
}

void ThemeManager::detectSystemTheme()
{
    setTheme(detectSystemThemeName());
}

QString ThemeManager::detectSystemThemeName() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto colorScheme = QGuiApplication::styleHints()->colorScheme();
    return (colorScheme == Qt::ColorScheme::Dark) ? "dark" : "light";
#else
    // Fallback for Qt versions without QStyleHints::colorScheme.
    const auto windowColor = QGuiApplication::palette().color(QPalette::Window);
    return (windowColor.lightness() < 128) ? "dark" : "light";
#endif
}

bool ThemeManager::isDarkTheme(const QString& themeName) const
{
    if (m_themes.contains(themeName)) {
        return m_themes[themeName].isDark;
    }
    return false;
}

QString ThemeManager::readFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&file);
    return in.readAll();
}
