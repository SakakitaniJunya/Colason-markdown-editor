#include "MenuBarManager.h"
#include "MainWindow.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSettings>
#include <QTimer>

MenuBarManager::MenuBarManager(MainWindow* mainWindow)
    : QObject(mainWindow)
    , m_mainWindow(mainWindow)
{
    restorePinState();
}

void MenuBarManager::setupMenuBar(QMenuBar* menuBar)
{
    m_menuBar = menuBar;
    setupFileMenu(menuBar);
    setupEditMenu(menuBar);
    setupParagraphMenu(menuBar);
    setupFormatMenu(menuBar);
    setupViewMenu(menuBar);
    setupHelpMenu(menuBar);
    connectMenuSignals(menuBar);
}

QWidget* MenuBarManager::createMenuWidget(QMenuBar* menuBar)
{
    auto* container = new QWidget(m_mainWindow);
    container->setObjectName("titleBarWidget");
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Hamburger button (left side, shown when menu is hidden)
    m_hamburgerButton = new QPushButton(container);
    m_hamburgerButton->setText(QString::fromUtf8("\xe2\x89\xa1"));  // ≡
    m_hamburgerButton->setFixedSize(46, 32);
    m_hamburgerButton->setFlat(true);
    m_hamburgerButton->setCursor(Qt::PointingHandCursor);
    m_hamburgerButton->setStyleSheet(
        "QPushButton { font-size: 16px; border: none; padding: 0; color: #666; }"
        "QPushButton:hover { background: rgba(128,128,128,0.25); color: #333; }");
    connect(m_hamburgerButton, &QPushButton::clicked, this, &MenuBarManager::showMenuBar);

    // Pin button
    m_pinButton = new QPushButton(container);
    m_pinButton->setFixedSize(32, 32);
    m_pinButton->setFlat(true);
    m_pinButton->setCursor(Qt::PointingHandCursor);
    m_pinButton->setToolTip(tr("Pin menu bar"));
    connect(m_pinButton, &QPushButton::clicked, this, &MenuBarManager::togglePin);

    // Window control buttons (Segoe MDL2 Assets icons)
    const QString winBtnStyle =
        "QPushButton { border: none; background: transparent;"
        "  font-family: 'Segoe MDL2 Assets'; font-size: 10px; }"
        "QPushButton:hover { background: rgba(128,128,128,0.25); }";
    const QString closeBtnStyle =
        "QPushButton { border: none; background: transparent;"
        "  font-family: 'Segoe MDL2 Assets'; font-size: 10px; }"
        "QPushButton:hover { background: #e81123; color: white; }";

    m_minimizeButton = new QPushButton(QString(QChar(0xE921)), container);  // ChromeMinimize
    m_minimizeButton->setFixedSize(46, 32);
    m_minimizeButton->setFlat(true);
    m_minimizeButton->setStyleSheet(winBtnStyle);
    connect(m_minimizeButton, &QPushButton::clicked, m_mainWindow, &QWidget::showMinimized);

    m_maximizeButton = new QPushButton(QString(QChar(0xE922)), container);  // ChromeMaximize
    m_maximizeButton->setFixedSize(46, 32);
    m_maximizeButton->setFlat(true);
    m_maximizeButton->setStyleSheet(winBtnStyle);
    connect(m_maximizeButton, &QPushButton::clicked, m_mainWindow, [this]() {
        if (m_mainWindow->isMaximized())
            m_mainWindow->showNormal();
        else
            m_mainWindow->showMaximized();
    });

    m_closeButton = new QPushButton(QString(QChar(0xE8BB)), container);  // ChromeClose
    m_closeButton->setFixedSize(46, 32);
    m_closeButton->setFlat(true);
    m_closeButton->setStyleSheet(closeBtnStyle);
    connect(m_closeButton, &QPushButton::clicked, m_mainWindow, &QWidget::close);

    // Layout: [hamburger] [menubar] [pin] [drag area] [−] [□] [×]
    layout->addWidget(m_hamburgerButton);
    layout->addWidget(menuBar);
    layout->addWidget(m_pinButton);
    layout->addStretch(1);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    // Setup menu bar (populate menus)
    setupMenuBar(menuBar);

    // Apply initial state
    updateVisibility();

    return container;
}

void MenuBarManager::showMenuBar()
{
    m_menuBar->setVisible(true);
    m_pinButton->setVisible(true);
    m_hamburgerButton->setVisible(false);
}

void MenuBarManager::hideMenuBar()
{
    if (m_pinned) return;
    m_menuBar->setVisible(false);
    m_pinButton->setVisible(false);
    m_hamburgerButton->setVisible(true);
}

void MenuBarManager::togglePin()
{
    m_pinned = !m_pinned;
    savePinState();
    updateVisibility();
}

void MenuBarManager::updateVisibility()
{
    if (!m_pinButton || !m_hamburgerButton || !m_menuBar) return;

    // Update pin button appearance (monochrome icons)
    if (m_pinned) {
        // Pinned: filled look with darker color
        m_pinButton->setText(QString::fromUtf8("\xe2\x97\x8f"));  // ● (filled circle = pinned)
        m_pinButton->setStyleSheet(
            "QPushButton { font-size: 10px; border: none; padding: 0; margin: 2px; color: #333; background: rgba(0,0,0,0.08); border-radius: 3px; }"
            "QPushButton:hover { background: rgba(0,0,0,0.15); }");
        m_pinButton->setToolTip(tr("Unpin menu bar"));
        m_menuBar->setVisible(true);
        m_pinButton->setVisible(true);
        m_hamburgerButton->setVisible(false);
    } else {
        // Unpinned: outline style, subtle
        m_pinButton->setText(QString::fromUtf8("\xe2\x97\x8b"));  // ○ (empty circle = unpinned)
        m_pinButton->setStyleSheet(
            "QPushButton { font-size: 10px; border: none; padding: 0; margin: 2px; color: #999; }"
            "QPushButton:hover { background: rgba(0,0,0,0.08); border-radius: 3px; color: #666; }");
        m_pinButton->setToolTip(tr("Pin menu bar"));
        m_menuBar->setVisible(false);
        m_pinButton->setVisible(false);
        m_hamburgerButton->setVisible(true);
    }
}

void MenuBarManager::savePinState()
{
    QSettings settings;
    settings.setValue("menuBar/pinned", m_pinned);
}

void MenuBarManager::restorePinState()
{
    QSettings settings;
    m_pinned = settings.value("menuBar/pinned", false).toBool();
}

void MenuBarManager::connectMenuSignals(QMenuBar* menuBar)
{
    // When any menu action is triggered, auto-hide if not pinned
    for (auto* action : menuBar->actions()) {
        auto* menu = action->menu();
        if (menu) {
            connect(menu, &QMenu::triggered, this, [this]() {
                // Delay slightly so the menu closes first
                QTimer::singleShot(100, this, &MenuBarManager::hideMenuBar);
            });
        }
    }
}

void MenuBarManager::setupFileMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("&File"));

    menu->addAction(tr("&New"), m_mainWindow, &MainWindow::newDocument,
                    QKeySequence::New);
    menu->addAction(tr("&Open File..."), m_mainWindow, &MainWindow::openFile,
                    QKeySequence::Open);
    menu->addAction(tr("Open &Folder..."), m_mainWindow, &MainWindow::openFolder);

    menu->addSeparator();

    menu->addAction(tr("&Save"), m_mainWindow, &MainWindow::saveFile,
                    QKeySequence::Save);
    menu->addAction(tr("Save &As..."), m_mainWindow, &MainWindow::saveFileAs,
                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    menu->addSeparator();

    // Export submenu
    auto* exportMenu = menu->addMenu(tr("&Export"));
    exportMenu->addAction(tr("Export as &PDF..."), m_mainWindow, &MainWindow::showExportPdfDialog);
    exportMenu->addAction(tr("Export as &HTML..."), m_mainWindow, &MainWindow::showExportHtmlDialog);

    menu->addSeparator();

    menu->addAction(tr("E&xit"), qApp, &QApplication::quit,
                    QKeySequence(Qt::ALT | Qt::Key_F4));
}

void MenuBarManager::setupEditMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("&Edit"));

    auto* undoAction = menu->addAction(tr("&Undo"), QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("undo");
    });

    auto* redoAction = menu->addAction(tr("&Redo"), QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("redo");
    });

    menu->addSeparator();

    menu->addAction(tr("Cu&t"), QKeySequence::Cut);
    menu->addAction(tr("&Copy"), QKeySequence::Copy);
    menu->addAction(tr("&Paste"), QKeySequence::Paste);

    menu->addSeparator();

    auto* selectAllAction = menu->addAction(tr("Select &All"), QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("selectAll");
    });

    menu->addSeparator();

    menu->addAction(tr("&Find..."), QKeySequence::Find);
    menu->addAction(tr("Find && &Replace..."), QKeySequence(Qt::CTRL | Qt::Key_H));

    menu->addSeparator();

    menu->addAction(tr("&Quick Open..."), m_mainWindow, &MainWindow::quickOpen,
                    QKeySequence(Qt::CTRL | Qt::Key_P));
}

void MenuBarManager::setupParagraphMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("&Paragraph"));

    for (int level = 1; level <= 6; ++level) {
        auto* action = menu->addAction(
            tr("Heading &%1").arg(level),
            QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + level)));
        connect(action, &QAction::triggered, m_mainWindow, [this, level]() {
            m_mainWindow->executeEditorCommand("setHeading",
                                                QString(R"({"level":%1})").arg(level));
        });
    }

    auto* paraAction = menu->addAction(tr("&Paragraph"), QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(paraAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("setParagraph");
    });

    menu->addSeparator();

    auto* tableAction = menu->addAction(tr("&Table..."), QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(tableAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("insertTable", R"({"rows":3,"cols":3})");
    });

    auto* codeBlockAction = menu->addAction(tr("Code &Block"),
                                             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
    connect(codeBlockAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("setCodeBlock");
    });

    auto* mathBlockAction = menu->addAction(tr("&Math Block"),
                                             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(mathBlockAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("insertMathBlock");
    });

    auto* mermaidAction = menu->addAction(tr("Mermaid &Diagram"));
    connect(mermaidAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("insertMermaid");
    });

    auto* quoteAction = menu->addAction(tr("&Quote"),
                                         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q));
    connect(quoteAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleBlockquote");
    });

    menu->addSeparator();

    auto* olAction = menu->addAction(tr("&Ordered List"),
                                      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketRight));
    connect(olAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleOrderedList");
    });

    auto* ulAction = menu->addAction(tr("&Unordered List"),
                                      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketLeft));
    connect(ulAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleBulletList");
    });

    auto* taskAction = menu->addAction(tr("Tas&k List"));
    connect(taskAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleTaskList");
    });

    menu->addSeparator();

    auto* hrAction = menu->addAction(tr("&Horizontal Rule"),
                                      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus));
    connect(hrAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("insertHorizontalRule");
    });
}

void MenuBarManager::setupFormatMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("F&ormat"));

    auto* boldAction = menu->addAction(tr("&Bold"), QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleBold");
    });

    auto* italicAction = menu->addAction(tr("&Italic"), QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleItalic");
    });

    auto* underlineAction = menu->addAction(tr("&Underline"), QKeySequence::Underline);
    connect(underlineAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleUnderline");
    });

    auto* strikeAction = menu->addAction(tr("&Strikethrough"),
                                          QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_5));
    connect(strikeAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleStrike");
    });

    menu->addSeparator();

    auto* highlightAction = menu->addAction(tr("H&ighlight"));
    connect(highlightAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleHighlight");
    });

    auto* superAction = menu->addAction(tr("Su&perscript"));
    connect(superAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleSuperscript");
    });

    auto* subAction = menu->addAction(tr("Su&bscript"));
    connect(subAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("toggleSubscript");
    });

    menu->addSeparator();

    auto* clearAction = menu->addAction(tr("Clear F&ormat"),
                                         QKeySequence(Qt::CTRL | Qt::Key_Backslash));
    connect(clearAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->executeEditorCommand("clearFormat");
    });
}

void MenuBarManager::setupViewMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("&View"));

    menu->addAction(tr("Toggle &Sidebar"), m_mainWindow, &MainWindow::toggleSidebar,
                    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));

    menu->addAction(tr("&Source Code Mode"), m_mainWindow, &MainWindow::toggleSourceMode,
                    QKeySequence(Qt::CTRL | Qt::Key_Slash));

    auto* autoHideAction = menu->addAction(tr("Auto-&hide Title Bar"));
    autoHideAction->setCheckable(true);
    autoHideAction->setChecked(m_mainWindow->titleBarAutoHide());
    connect(autoHideAction, &QAction::toggled, m_mainWindow, &MainWindow::setTitleBarAutoHide);

    menu->addSeparator();

    auto* focusModeAction = menu->addAction(tr("&Focus Mode"));
    connect(focusModeAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->toggleFocusMode();
    });

    auto* typewriterAction = menu->addAction(tr("&Typewriter Mode"));
    connect(typewriterAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->toggleTypewriterMode();
    });

    menu->addSeparator();

    // Theme submenu
    auto* themeMenu = menu->addMenu(tr("&Theme"));
    QStringList themes = {"light", "dark", "github-light", "github-dark", "sepia", "nord", "dracula"};
    QStringList themeNames = {"Light", "Dark", "GitHub Light", "GitHub Dark", "Sepia", "Nord", "Dracula"};
    for (int i = 0; i < themes.size(); ++i) {
        auto* action = themeMenu->addAction(themeNames[i]);
        QString themeName = themes[i];
        connect(action, &QAction::triggered, m_mainWindow, [this, themeName]() {
            m_mainWindow->changeTheme(themeName);
        });
    }

    menu->addSeparator();

    auto* zoomInAction = menu->addAction(tr("Zoom &In"),
                                          QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Equal));
    connect(zoomInAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->zoomIn();
    });

    auto* zoomOutAction = menu->addAction(tr("Zoom &Out"),
                                           QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus));
    connect(zoomOutAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->zoomOut();
    });

    auto* resetZoomAction = menu->addAction(tr("&Reset Zoom"),
                                             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_0));
    connect(resetZoomAction, &QAction::triggered, m_mainWindow, [this]() {
        m_mainWindow->resetZoom();
    });
}

void MenuBarManager::setupHelpMenu(QMenuBar* menuBar)
{
    auto* menu = menuBar->addMenu(tr("&Help"));

    auto* aboutAction = menu->addAction(tr("&About Colason"));
    connect(aboutAction, &QAction::triggered, m_mainWindow, [this]() {
        QMessageBox::about(m_mainWindow, tr("About Colason"),
                           tr("<h2>Colason</h2>"
                              "<p>Version 0.1.0</p>"
                              "<p>A Typora-compatible WYSIWYG Markdown editor.</p>"
                              "<p>Built with Qt %1</p>").arg(QT_VERSION_STR));
    });
}

void MenuBarManager::updateMaximizeIcon(bool isMaximized)
{
    if (!m_maximizeButton) return;
    m_maximizeButton->setText(QString(QChar(isMaximized ? 0xE923 : 0xE922)));
}

QAction* MenuBarManager::createAction(const QString& text, const QKeySequence& shortcut)
{
    auto* action = new QAction(text, this);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }
    return action;
}
