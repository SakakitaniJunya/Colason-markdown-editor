#include "ThemeManager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStyleHints>
#include <QGuiApplication>

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
        // editorCSS
        "body { background: #ffffff; color: #333333; }"
        ".tiptap h1, .tiptap h2 { border-bottom-color: #eee; }"
        ".tiptap a { color: #4183c4; }"
        ".tiptap code { background: #f6f8fa; border-color: #e1e4e8; color: #e83e8c; }"
        ".tiptap pre { background: #f6f8fa; border-color: #e1e4e8; }"
        ".tiptap blockquote { border-left-color: #dfe2e5; color: #6a737d; }"
        ".tiptap th { background: #f6f8fa; }"
        ".tiptap mark { background-color: #fff3a3; }"
        ".tiptap pre code { color: #333; }"
        ".hljs { color: #333; }"
        ".hljs-keyword { color: #d73a49; }"
        ".hljs-string { color: #032f62; }"
        ".hljs-comment { color: #6a737d; }"
        ".hljs-number { color: #005cc5; }"
        ".hljs-function, .hljs-title { color: #6f42c1; }"
        ".hljs-built_in { color: #e36209; }"
        ".mermaid-block { border-color: #e1e4e8; background: #f6f8fa; }"
        ".mermaid-block .mermaid-code { background: #f6f8fa; border-bottom-color: #e1e4e8; color: #333; }"
        ".mermaid-block .mermaid-error { background: #ffeef0; color: #cb2431; border-top-color: #fdb8c0; }"
        ".katex-block { border-color: #e1e4e8; background: #f6f8fa; }"
        ".katex-block .katex-code { background: #f6f8fa; border-bottom-color: #e1e4e8; color: #333; }"
        ".katex-inline:hover { background: #f0f0f0; }",
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
        "QTabBar::tab { padding: 8px 16px; border: none; border-bottom: 2px solid transparent;"
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
        "  border-bottom: 1px solid #e1e4e8; padding: 6px 10px; background: #fafbfc; }"
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
        // editorCSS
        "body { background: #2b2b2b; color: #c8c8c8; }"
        ".tiptap h1 { color: #e0e0e0; border-bottom-color: #404040; }"
        ".tiptap h2 { color: #e0e0e0; border-bottom-color: #404040; }"
        ".tiptap h3, .tiptap h4, .tiptap h5 { color: #d4d4d4; }"
        ".tiptap h6 { color: #8b949e; }"
        ".tiptap a { color: #6cb6ff; }"
        ".tiptap code { background: #363636; border-color: #4a4a4a; color: #f97583; }"
        ".tiptap pre { background: #1e1e1e; border-color: #404040; }"
        ".tiptap pre code { color: #d4d4d4; }"
        ".tiptap blockquote { border-left-color: #505050; color: #999; }"
        ".tiptap th { background: #333; border-color: #4a4a4a; }"
        ".tiptap td { border-color: #4a4a4a; }"
        ".tiptap tr:nth-child(2n) { background: #2f2f2f; }"
        ".tiptap mark { background-color: #5c4b00; color: #e0e0e0; }"
        ".tiptap hr { border-top-color: #404040; }"
        ".tiptap s { color: #666; }"
        ".tiptap img { opacity: 0.9; }"
        "::selection { background: #264f78; }"
        "::-webkit-scrollbar { width: 8px; }"
        "::-webkit-scrollbar-track { background: transparent; }"
        "::-webkit-scrollbar-thumb { background: #555; border-radius: 4px; }"
        "::-webkit-scrollbar-thumb:hover { background: #777; }"
        ".hljs { color: #d4d4d4; }"
        ".hljs-keyword { color: #c586c0; }"
        ".hljs-string { color: #ce9178; }"
        ".hljs-comment { color: #6a9955; }"
        ".hljs-number { color: #b5cea8; }"
        ".hljs-function, .hljs-title { color: #dcdcaa; }"
        ".hljs-built_in { color: #4ec9b0; }"
        ".mermaid-block { border-color: #404040; background: #1e1e1e; }"
        ".mermaid-block .mermaid-code { background: #1e1e1e; border-bottom-color: #404040; color: #d4d4d4; }"
        ".mermaid-block .mermaid-error { background: #3c1f1f; color: #f48771; border-top-color: #5a2020; }"
        ".katex-block { border-color: #404040; background: #1e1e1e; }"
        ".katex-block .katex-code { background: #1e1e1e; border-bottom-color: #404040; color: #d4d4d4; }"
        ".katex-inline:hover { background: #333; }"
        ".tiptap p.is-editor-empty:first-child::before { color: #555; }"
        ".tiptap ul[data-type='taskList'] li[data-checked='true'] > div { color: #666; }",

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
        "QTabBar::tab { padding: 8px 14px; border: none; border-bottom: 2px solid transparent;"
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
        "  border-bottom: 1px solid #333; padding: 6px 10px; background: #252526; }"
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
        "body { background: #ffffff; color: #24292f; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Noto Sans, Helvetica, Arial, sans-serif; }"
        ".tiptap h1 { border-bottom: 1px solid #d0d7de; }"
        ".tiptap h2 { border-bottom: 1px solid #d0d7de; }"
        ".tiptap a { color: #0969da; }"
        ".tiptap code { background: rgba(175,184,193,0.2); color: #24292f; }"
        ".tiptap pre { background: #f6f8fa; }"
        ".tiptap blockquote { border-left-color: #d0d7de; color: #57606a; }"
        ".tiptap mark { background-color: #fff8c5; }"
        ".tiptap pre code { color: #24292f; }"
        ".hljs { color: #24292f; }"
        ".hljs-keyword { color: #cf222e; }"
        ".hljs-string { color: #0a3069; }"
        ".hljs-comment { color: #6e7781; }"
        ".hljs-number { color: #0550ae; }"
        ".hljs-function, .hljs-title { color: #8250df; }"
        ".hljs-built_in { color: #953800; }"
        ".mermaid-block { border-color: #d0d7de; background: #f6f8fa; }"
        ".mermaid-block .mermaid-code { background: #f6f8fa; border-bottom-color: #d0d7de; color: #24292f; }"
        ".mermaid-block .mermaid-error { background: #ffebe9; color: #cf222e; border-top-color: #ff8182; }"
        ".katex-block { border-color: #d0d7de; background: #f6f8fa; }"
        ".katex-block .katex-code { background: #f6f8fa; border-bottom-color: #d0d7de; color: #24292f; }"
        ".katex-inline:hover { background: #eef2f6; }",
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
        "QTabBar::tab { padding: 8px 14px; border: none; color: #57606a; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
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
        "  border-bottom: 1px solid #d0d7de; padding: 6px 10px; background: #f6f8fa; }"
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
        "body { background: #0d1117; color: #e6edf3; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Noto Sans, Helvetica, Arial, sans-serif; }"
        ".tiptap h1 { border-bottom: 1px solid #30363d; color: #e6edf3; }"
        ".tiptap h2 { border-bottom: 1px solid #30363d; color: #e6edf3; }"
        ".tiptap h3, .tiptap h4, .tiptap h5 { color: #e6edf3; }"
        ".tiptap h6 { color: #8b949e; }"
        ".tiptap a { color: #58a6ff; }"
        ".tiptap code { background: rgba(110,118,129,0.4); color: #e6edf3; }"
        ".tiptap pre { background: #161b22; border-color: #30363d; }"
        ".tiptap pre code { color: #e6edf3; }"
        ".tiptap blockquote { border-left-color: #30363d; color: #8b949e; }"
        ".tiptap th { background: #161b22; border-color: #30363d; }"
        ".tiptap td { border-color: #30363d; }"
        ".tiptap tr:nth-child(2n) { background: #161b22; }"
        ".tiptap mark { background-color: rgba(187,128,9,0.4); color: #e6edf3; }"
        ".tiptap hr { border-top-color: #30363d; }"
        ".tiptap s { color: #8b949e; }"
        ".tiptap img { opacity: 0.9; }"
        "::selection { background: #264f78; }"
        "::-webkit-scrollbar { width: 8px; }"
        "::-webkit-scrollbar-track { background: transparent; }"
        "::-webkit-scrollbar-thumb { background: #484f58; border-radius: 4px; }"
        "::-webkit-scrollbar-thumb:hover { background: #6e7681; }"
        ".hljs { color: #e6edf3; }"
        ".hljs-keyword { color: #ff7b72; }"
        ".hljs-string { color: #a5d6ff; }"
        ".hljs-comment { color: #8b949e; }"
        ".hljs-number { color: #79c0ff; }"
        ".hljs-function, .hljs-title { color: #d2a8ff; }"
        ".hljs-built_in { color: #ffa657; }"
        ".mermaid-block { border-color: #30363d; background: #161b22; }"
        ".mermaid-block .mermaid-code { background: #161b22; border-bottom-color: #30363d; color: #e6edf3; }"
        ".mermaid-block .mermaid-error { background: #2d1117; color: #ff7b72; border-top-color: #3d1c20; }"
        ".katex-block { border-color: #30363d; background: #161b22; }"
        ".katex-block .katex-code { background: #161b22; border-bottom-color: #30363d; color: #e6edf3; }"
        ".katex-inline:hover { background: #161b22; }"
        ".tiptap p.is-editor-empty:first-child::before { color: #484f58; }"
        ".tiptap ul[data-type='taskList'] li[data-checked='true'] > div { color: #8b949e; }",
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
        "QTabBar::tab { padding: 8px 14px; border: none; border-bottom: 2px solid transparent;"
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
        "  border-bottom: 1px solid #30363d; padding: 6px 10px; background: #161b22; }"
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
        "body { background: #f4ecd8; color: #5b4636; }"
        ".tiptap h1, .tiptap h2 { border-bottom-color: #d6c9a8; color: #3d2b1f; }"
        ".tiptap a { color: #8b6914; }"
        ".tiptap code { background: #ece0c8; border-color: #d6c9a8; color: #8b4513; }"
        ".tiptap pre { background: #ece0c8; border-color: #d6c9a8; }"
        ".tiptap blockquote { border-left-color: #c4b08c; color: #7a6652; }"
        ".tiptap th { background: #ece0c8; }"
        ".tiptap td, .tiptap th { border-color: #d6c9a8; }"
        ".tiptap tr:nth-child(2n) { background: #ede2ce; }"
        ".tiptap mark { background-color: #f0d880; }"
        ".tiptap pre code { color: #5b4636; }"
        ".hljs { color: #5b4636; }"
        ".hljs-keyword { color: #8b4513; }"
        ".hljs-string { color: #6b4226; }"
        ".hljs-comment { color: #9e8c75; }"
        ".hljs-number { color: #8b6914; }"
        ".hljs-function, .hljs-title { color: #7a5230; }"
        ".hljs-built_in { color: #a0522d; }"
        ".mermaid-block { border-color: #d6c9a8; background: #ece0c8; }"
        ".mermaid-block .mermaid-code { background: #ece0c8; border-bottom-color: #d6c9a8; color: #5b4636; }"
        ".mermaid-block .mermaid-error { background: #f0d8c0; color: #8b0000; border-top-color: #c4a080; }"
        ".katex-block { border-color: #d6c9a8; background: #ece0c8; }"
        ".katex-block .katex-code { background: #ece0c8; border-bottom-color: #d6c9a8; color: #5b4636; }"
        ".katex-inline:hover { background: #e0d0b0; }",
        "* { font-family: 'Segoe UI', 'Yu Gothic UI', sans-serif; }"
        "QMainWindow { background: #ece0c8; }"
        "QMenuBar { background: #d6c9a8; color: #3d2b1f; }"
        "QStatusBar { background: #d6c9a8; color: #3d2b1f; }"
        "QSplitter::handle { background: #c4b08c; width: 1px; }"
        "QTabWidget::pane { border-right: 1px solid #c4b08c; }"
        "QTabBar::tab { padding: 8px 14px; color: #7a6652; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
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
        "body { background: #2e3440; color: #d8dee9; }"
        ".tiptap h1, .tiptap h2 { border-bottom-color: #3b4252; color: #eceff4; }"
        ".tiptap a { color: #88c0d0; }"
        ".tiptap code { background: #3b4252; border-color: #434c5e; color: #bf616a; }"
        ".tiptap pre { background: #3b4252; border-color: #434c5e; }"
        ".tiptap blockquote { border-left-color: #4c566a; color: #a3be8c; }"
        ".tiptap th { background: #3b4252; }"
        ".tiptap td, .tiptap th { border-color: #434c5e; }"
        ".tiptap tr:nth-child(2n) { background: #353b49; }"
        ".tiptap mark { background-color: #ebcb8b; color: #2e3440; }"
        ".tiptap hr { border-top-color: #434c5e; }"
        "::selection { background: #434c5e; }"
        ".tiptap pre code { color: #d8dee9; }"
        ".hljs { color: #d8dee9; }"
        ".hljs-keyword { color: #81a1c1; }"
        ".hljs-string { color: #a3be8c; }"
        ".hljs-comment { color: #616e88; }"
        ".hljs-number { color: #b48ead; }"
        ".hljs-function, .hljs-title { color: #88c0d0; }"
        ".hljs-built_in { color: #8fbcbb; }"
        ".mermaid-block { border-color: #434c5e; background: #3b4252; }"
        ".mermaid-block .mermaid-code { background: #3b4252; border-bottom-color: #434c5e; color: #d8dee9; }"
        ".mermaid-block .mermaid-error { background: #3b2c2c; color: #bf616a; border-top-color: #4c3030; }"
        ".katex-block { border-color: #434c5e; background: #3b4252; }"
        ".katex-block .katex-code { background: #3b4252; border-bottom-color: #434c5e; color: #d8dee9; }"
        ".katex-inline:hover { background: #3b4252; }",
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
        "QTabBar::tab { padding: 8px 14px; color: #7b88a1; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
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
        "body { background: #282a36; color: #f8f8f2; }"
        ".tiptap h1, .tiptap h2 { border-bottom-color: #44475a; color: #f8f8f2; }"
        ".tiptap a { color: #8be9fd; }"
        ".tiptap code { background: #44475a; border-color: #6272a4; color: #ff79c6; }"
        ".tiptap pre { background: #44475a; border-color: #6272a4; }"
        ".tiptap blockquote { border-left-color: #6272a4; color: #bd93f9; }"
        ".tiptap th { background: #44475a; }"
        ".tiptap td, .tiptap th { border-color: #6272a4; }"
        ".tiptap tr:nth-child(2n) { background: #313340; }"
        ".tiptap mark { background-color: #f1fa8c; color: #282a36; }"
        ".tiptap hr { border-top-color: #6272a4; }"
        "::selection { background: #44475a; }"
        ".tiptap pre code { color: #f8f8f2; }"
        ".hljs { color: #f8f8f2; }"
        ".hljs-keyword { color: #ff79c6; }"
        ".hljs-string { color: #f1fa8c; }"
        ".hljs-comment { color: #6272a4; }"
        ".hljs-number { color: #bd93f9; }"
        ".hljs-function, .hljs-title { color: #50fa7b; }"
        ".hljs-built_in { color: #8be9fd; }"
        ".mermaid-block { border-color: #6272a4; background: #44475a; }"
        ".mermaid-block .mermaid-code { background: #44475a; border-bottom-color: #6272a4; color: #f8f8f2; }"
        ".mermaid-block .mermaid-error { background: #3c1f2a; color: #ff5555; border-top-color: #5a2030; }"
        ".katex-block { border-color: #6272a4; background: #44475a; }"
        ".katex-block .katex-code { background: #44475a; border-bottom-color: #6272a4; color: #f8f8f2; }"
        ".katex-inline:hover { background: #44475a; }",
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
        "QTabBar::tab { padding: 8px 14px; color: #6272a4; font-size: 11px; font-weight: 600; text-transform: uppercase; border-bottom: 2px solid transparent; }"
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
    auto colorScheme = QGuiApplication::styleHints()->colorScheme();
    if (colorScheme == Qt::ColorScheme::Dark) {
        setTheme("dark");
    } else {
        setTheme("light");
    }
}

QString ThemeManager::readFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&file);
    return in.readAll();
}
