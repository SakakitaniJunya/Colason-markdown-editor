#include "MainWindow.h"
#include "MenuBarManager.h"
#include "EditorPane.h"
#include "QuickOpenDialog.h"
#include "sidebar/SidebarContainer.h"
#include "bridge/EditorBridge.h"
#include "bridge/OutlineBridge.h"
#include "bridge/SearchBridge.h"
#include "bridge/ThemeBridge.h"
#include "core/DocumentManager.h"
#include "core/AutoSaveManager.h"
#include "core/DraftRecoveryManager.h"
#include "core/RecentFilesManager.h"
#include "core/GlobalSearchManager.h"
#include "core/ImageManager.h"
#include "core/ExportManager.h"
#include "core/ThemeManager.h"
#include "core/PreferencesManager.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QSettings>
#include <QShortcut>
#include <QImage>
#include <QClipboard>
#include <QStyleHints>
#include <QGuiApplication>
#include <QStatusBar>
#include <QMenuBar>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QScreen>

#ifdef HAS_WEBENGINE
#include <QWebEngineView>
#include <QWebChannel>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QJsonDocument>
#include <QJsonArray>
#endif

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windowsx.h>
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#endif

namespace {

// Helper: JSON-quote a string for JS injection
QString jsQuote(const QString& s) {
    QJsonArray arr;
    arr.append(s);
    QString json = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    return json.mid(1, json.size() - 2);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_prefs = new PreferencesManager(this);
    m_themeManager = new ThemeManager(this);

    setupUI();

    // Initial pane
#ifdef HAS_WEBENGINE
    auto* initial = createPane();
    m_paneSplitter->addWidget(initial);
    m_panes.append(initial);
    loadEditorIntoPane(initial);
    setActivePane(initial);
#endif

    setupMenuBar();
    setupFramelessWindow();
    statusBar()->hide();

    // Title bar auto-hide timer
    m_titleBarHideTimer = new QTimer(this);
    m_titleBarHideTimer->setSingleShot(true);
    connect(m_titleBarHideTimer, &QTimer::timeout, this, [this]() {
#ifdef Q_OS_WIN
        if (m_titleBarAutoHide && m_titleBarShown) {
            m_menuWidget->setVisible(false);
            m_titleBarShown = false;
        }
#endif
    });

    setupManagers();
    setupConnections();

    // Restore window geometry
    QSettings settings;
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());
    if (size().isEmpty()) {
        resize(1280, 800);
    }

    setAcceptDrops(true);
    updateTitle();

#ifdef Q_OS_WIN
    if (settings.value("window/titleBarAutoHide", false).toBool()) {
        setTitleBarAutoHide(true);
    }
#else
    settings.setValue("window/titleBarAutoHide", false);
#endif

    applyPreferences();

    // Restore last opened folder
    const auto folders = m_recentFiles->recentFolders();
    if (!folders.isEmpty() && QDir(folders.first()).exists()) {
        m_sidebar->setRootPath(folders.first());
    }

    // Check for recoverable drafts
    QTimer::singleShot(500, this, &MainWindow::checkDraftRecovery);
}

MainWindow::~MainWindow() = default;

// ----------------------------------------------------------------------
// UI / Pane scaffolding
// ----------------------------------------------------------------------

void MainWindow::setupUI()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_splitter);

    m_sidebar = new SidebarContainer(this);
    m_splitter->addWidget(m_sidebar);

#ifdef HAS_WEBENGINE
    m_paneSplitter = new QSplitter(Qt::Horizontal, this);
    m_paneSplitter->setHandleWidth(2);
    m_paneSplitter->setChildrenCollapsible(false);
    m_splitter->addWidget(m_paneSplitter);
#else
    m_editorPlaceholder = new QWidget(this);
    auto* label = new QLabel(
        QString::fromUtf8("QWebEngine \343\201\214\345\210\251\347\224\250\343\201\247\343\201\215\343\201\276\343\201\233\343\202\223\343\200\202\n"
                          "qtwebengine \343\202\222\343\203\223\343\203\253\343\203\211\343\201\227\343\201\246\343\202\250\343\203\207\343\202\243\343\202\277\343\202\222\346\234\211\345\212\271\343\201\253\343\201\227\343\201\246\343\201\217\343\201\240\343\201\225\343\201\204\343\200\202"),
        m_editorPlaceholder);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 14px;");
    auto* layout = new QVBoxLayout(m_editorPlaceholder);
    layout->addWidget(label);
    m_splitter->addWidget(m_editorPlaceholder);
#endif

    m_splitter->setSizes({240, 1000});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setCollapsible(0, true);
    m_splitter->setCollapsible(1, false);
    m_splitter->setHandleWidth(1);

    m_sidebar->setMinimumWidth(160);
    m_sidebar->setMaximumWidth(400);

    auto* shortcut1 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_1), this);
    connect(shortcut1, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(0);
    });
    auto* shortcut2 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_2), this);
    connect(shortcut2, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(1);
    });
    auto* shortcut3 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_3), this);
    connect(shortcut3, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(2);
    });

    auto* quickOpenShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this);
    connect(quickOpenShortcut, &QShortcut::activated, this, &MainWindow::quickOpen);

    // Cycle to next pane (Ctrl+`)
    auto* cyclePaneShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), this);
    connect(cyclePaneShortcut, &QShortcut::activated, this, &MainWindow::focusNextPane);
}

void MainWindow::setupMenuBar()
{
    m_menuBarManager = new MenuBarManager(this);
    auto* menuBar = new QMenuBar(nullptr);
    m_menuWidget = m_menuBarManager->createMenuWidget(menuBar);
    setMenuWidget(m_menuWidget);
}

void MainWindow::setupFramelessWindow()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    MARGINS margins = {0, 0, 0, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

EditorPane* MainWindow::createPane()
{
#ifdef HAS_WEBENGINE
    auto* pane = new EditorPane();
    wirePane(pane);
    return pane;
#else
    return nullptr;
#endif
}

void MainWindow::wirePane(EditorPane* pane)
{
#ifdef HAS_WEBENGINE
    auto* p = pane;

    connect(p, &EditorPane::focusRequested, this, &MainWindow::setActivePane);

    // contentChanged: store in this pane's DocumentManager + refresh title if active
    connect(p->editorBridge(), &EditorBridge::contentChanged, this, [this, p](const QString& html) {
        p->docManager()->setContent(html);
        if (p == m_activePane) updateTitle();
    });

    connect(p->editorBridge(), &EditorBridge::documentDirty, this, [this, p](bool dirty) {
        p->docManager()->setDirty(dirty);
        if (p == m_activePane) updateTitle();
    });

    // Cmd/Ctrl-click on a link → open externally
    connect(p->editorBridge(), &EditorBridge::openLinkRequested, this, [](const QString& url) {
        QUrl u(url);
        if (!u.isValid() || u.isRelative()) return;
        const QString scheme = u.scheme().toLower();
        if (scheme == "http" || scheme == "https" || scheme == "mailto" || scheme == "file") {
            QDesktopServices::openUrl(u);
        }
    });

    // Outline updates: only if firing pane is active
    connect(p->outlineBridge(), &OutlineBridge::headingsChanged, this, [this, p](const QString& json) {
        if (p == m_activePane) m_sidebar->updateOutline(json);
    });
    connect(p->outlineBridge(), &OutlineBridge::activeHeadingChanged, this, [this, p](const QString& id) {
        if (p == m_activePane) m_sidebar->setActiveHeading(id);
    });

    // Pre-inject theme CSS at DocumentReady so the very first paint is themed
    QString themeName = m_prefs->autoDetectTheme()
        ? m_themeManager->detectSystemThemeName()
        : m_prefs->theme();
    QString themeCss = m_themeManager->themeCSS(themeName);
    bool isDark = m_themeManager->isDarkTheme(themeName);
    p->page()->setBackgroundColor(isDark ? QColor("#1e1e1e") : QColor("#ffffff"));
    if (!themeCss.isEmpty()) {
        QString jsSource = QString(
            "(function(){"
            "var t=document.head||document.documentElement;"
            "if(!t) return;"
            "var el=document.createElement('style');"
            "el.id='colason-theme';"
            "el.textContent=%1;"
            "t.appendChild(el);"
            "})()"
        ).arg(jsQuote(themeCss));

        QWebEngineScript script;
        script.setName("colason-theme-init");
        script.setSourceCode(jsSource);
        script.setInjectionPoint(QWebEngineScript::DocumentReady);
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setRunsOnSubFrames(false);
        p->page()->scripts().insert(script);
    }

    // Re-apply theme + wide mode + keybinding when the editor page finishes loading.
    connect(p->page(), &QWebEnginePage::loadFinished, this, [this, p](bool ok) {
        if (!ok) return;
        QString themeName = m_themeManager->currentThemeName();
        QString css = m_themeManager->themeCSS(themeName);
        if (!css.isEmpty()) {
            QString js = QString(
                "(function(){"
                "if(!document.head) return;"
                "var el=document.getElementById('colason-theme');"
                "if(!el){el=document.createElement('style');el.id='colason-theme';}"
                "el.textContent=%1;"
                "document.head.appendChild(el);"
                "})()"
            ).arg(jsQuote(css));
            p->page()->runJavaScript(js);
        }
        p->page()->runJavaScript(
            QString("colasonAPI.setWideMode(%1)").arg(m_wideMode ? "true" : "false"));
        QString keybinding = m_prefs->keybinding();
        if (keybinding != "default") {
            QString js = QString("colasonAPI.setKeybinding && colasonAPI.setKeybinding(%1)").arg(jsQuote(keybinding));
            p->page()->runJavaScript(js);
        }
        if (m_zoomPercent != 100) {
            p->page()->runJavaScript(QString("colasonAPI.setZoom(%1)").arg(m_zoomPercent));
        }
    });
#else
    Q_UNUSED(pane);
#endif
}

void MainWindow::loadEditorIntoPane(EditorPane* pane)
{
#ifdef HAS_WEBENGINE
    QString editorDistDir = QString::fromUtf8(EDITOR_DIST_DIR);
    QString indexPath = editorDistDir + "/index.html";
    pane->loadEditorPage(indexPath, "qrc:/editor/index.html");
#else
    Q_UNUSED(pane);
#endif
}

void MainWindow::setActivePane(EditorPane* pane)
{
    if (!pane) return;
    if (pane == m_activePane) return;
    if (m_activePane) m_activePane->setActive(false);
    m_activePane = pane;
    pane->setActive(true);
    // Refresh title + active file for sidebar
    updateTitle();
    if (m_sidebar) m_sidebar->setCurrentFile(pane->docManager()->currentFilePath());
}

void MainWindow::splitActive(Qt::Orientation orientation)
{
#ifdef HAS_WEBENGINE
    if (!m_activePane) return;
    auto* current = m_activePane;
    auto* parentSplitter = qobject_cast<QSplitter*>(current->parentWidget());
    if (!parentSplitter) return;

    int idx = parentSplitter->indexOf(current);

    auto* newPane = createPane();
    if (!newPane) return;
    m_panes.append(newPane);
    loadEditorIntoPane(newPane);

    if (parentSplitter->orientation() == orientation) {
        parentSplitter->insertWidget(idx + 1, newPane);
        // Equalize sizes
        QList<int> sizes;
        const int extent = (orientation == Qt::Horizontal) ? parentSplitter->width() : parentSplitter->height();
        const int n = parentSplitter->count();
        const int each = n > 0 ? extent / n : 200;
        for (int i = 0; i < n; ++i) sizes.append(each);
        parentSplitter->setSizes(sizes);
    } else {
        // Different orientation: nest a new splitter in place of `current`
        auto* nested = new QSplitter(orientation);
        nested->setHandleWidth(2);
        nested->setChildrenCollapsible(false);
        const int extent = (orientation == Qt::Horizontal) ? current->width() : current->height();
        // Detach `current` from its parent so we can re-parent it into the nested splitter
        current->setParent(nullptr);
        nested->addWidget(current);
        nested->addWidget(newPane);
        const int half = extent > 0 ? extent / 2 : 200;
        nested->setSizes({half, extent - half});
        parentSplitter->insertWidget(idx, nested);
    }

    setActivePane(newPane);
#else
    Q_UNUSED(orientation);
#endif
}

void MainWindow::splitRight() { splitActive(Qt::Horizontal); }
void MainWindow::splitDown()  { splitActive(Qt::Vertical); }

void MainWindow::closeActivePane()
{
    if (!m_activePane) return;
    if (m_panes.size() <= 1) return;  // never close the last pane

    auto* current = m_activePane;
    auto* parentSplitter = qobject_cast<QSplitter*>(current->parentWidget());
    if (!parentSplitter) return;

    // Pick a successor before deletion
    EditorPane* successor = nullptr;
    for (auto* p : m_panes) {
        if (p != current) { successor = p; break; }
    }

    m_panes.removeAll(current);
    m_activePane = nullptr;
    current->deleteLater();

    // Collapse trivial splitters: if parentSplitter ended up with only 1 child
    // and parentSplitter itself is nested in another QSplitter, hoist that
    // remaining child up one level. (Keeps the layout tight, VSCode-style.)
    if (parentSplitter != m_paneSplitter && parentSplitter->count() == 1) {
        auto* grandparent = qobject_cast<QSplitter*>(parentSplitter->parentWidget());
        if (grandparent) {
            int idx = grandparent->indexOf(parentSplitter);
            QWidget* survivor = parentSplitter->widget(0);
            survivor->setParent(nullptr);
            grandparent->insertWidget(idx, survivor);
            parentSplitter->deleteLater();
        }
    }

    if (successor) setActivePane(successor);
}

void MainWindow::focusNextPane()
{
    if (m_panes.size() <= 1) return;
    int idx = m_panes.indexOf(m_activePane);
    EditorPane* next = m_panes.value((idx + 1) % m_panes.size());
    if (next) {
        setActivePane(next);
        next->setFocus(Qt::OtherFocusReason);
    }
}

// ----------------------------------------------------------------------
// Managers / connections
// ----------------------------------------------------------------------

void MainWindow::setupManagers()
{
    m_recentFiles = new RecentFilesManager(this);
    // AutoSave/Draft are tied to the *initial* pane's DocumentManager.
    // (Saving any other pane is via Ctrl+S which always uses the active pane.)
    auto* primaryDoc = m_panes.isEmpty() ? nullptr : m_panes.first()->docManager();
    m_autoSaveManager = new AutoSaveManager(primaryDoc, this);
    m_draftManager = new DraftRecoveryManager(primaryDoc, this);
    m_globalSearch = new GlobalSearchManager(this);
    m_imageManager = new ImageManager(this);
    m_exportManager = new ExportManager(this);

    m_autoSaveManager->setEnabled(m_prefs->autoSaveEnabled());
    m_autoSaveManager->setInterval(m_prefs->autoSaveIntervalMs());
    m_imageManager->setAssetsSubfolder(m_prefs->imageAssetSubfolder());
}

void MainWindow::setupConnections()
{
#ifdef HAS_WEBENGINE
    // AutoSave: route content request to the *primary* pane (matches how the
    // manager was constructed). Active pane saves go through saveFile() instead.
    connect(m_autoSaveManager, &AutoSaveManager::getContentRequested, this, [this]() {
        if (m_panes.isEmpty()) return;
        m_panes.first()->page()->runJavaScript("colasonAPI.getContent()", [this](const QVariant& result) {
            m_autoSaveManager->onContentReceived(result.toString());
        });
    });
    connect(m_autoSaveManager, &AutoSaveManager::autoSaved, this, [this]() {
        statusBar()->showMessage(tr("Auto-saved"), 3000);
        updateTitle();
    });

    connect(m_draftManager, &DraftRecoveryManager::getContentRequested, this, [this]() {
        if (m_panes.isEmpty()) return;
        m_panes.first()->page()->runJavaScript("colasonAPI.getContent()", [this](const QVariant& result) {
            m_draftManager->saveDraft(result.toString());
        });
    });

    connect(m_exportManager, &ExportManager::exportFinished, this, [this](bool success, const QString& path) {
        if (success) {
            statusBar()->showMessage(tr("Exported to: %1").arg(path), 5000);
        }
    });
    connect(m_exportManager, &ExportManager::exportError, this, [this](const QString& error) {
        QMessageBox::warning(this, tr("Export Error"), error);
    });

    connect(m_themeManager, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);

    connect(m_prefs, &PreferencesManager::autoSaveChanged, this, [this](bool enabled, int intervalMs) {
        m_autoSaveManager->setEnabled(enabled);
        m_autoSaveManager->setInterval(intervalMs);
    });
    connect(m_prefs, &PreferencesManager::keybindingChanged, this, [this](const QString& keybinding) {
        QString js = QString("colasonAPI.setKeybinding && colasonAPI.setKeybinding(%1)").arg(jsQuote(keybinding));
        for (auto* p : m_panes) p->page()->runJavaScript(js);
    });
    connect(m_prefs, &PreferencesManager::themeChanged, this, [this](const QString& theme) {
        m_themeManager->setTheme(theme);
    });
#endif

    connect(m_sidebar, &SidebarContainer::headingClicked, this, [this](const QString& id) {
#ifdef HAS_WEBENGINE
        if (!m_activePane) return;
        QString js = QString("colasonAPI.scrollToHeading(%1)").arg(jsQuote(id));
        m_activePane->page()->runJavaScript(js);
#endif
    });

    connect(m_sidebar, &SidebarContainer::openFolderRequested, this, &MainWindow::openFolder);

    // Sidebar -> Open file in active pane
    connect(m_sidebar, &SidebarContainer::fileSelected, this, [this](const QString& path) {
        if (!m_activePane) return;
        bool ok = m_activePane->docManager()->openDocument(path);
        if (!ok) return;
        m_recentFiles->addFile(path);
        QString content = m_activePane->docManager()->currentContent();
        setActivePaneMarkdown(content);
        m_sidebar->setCurrentFile(path);
        updateTitle();
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        if (m_prefs->autoDetectTheme()) {
            m_themeManager->setTheme(scheme == Qt::ColorScheme::Dark ? "dark" : "light");
        }
    });
#endif
}

// ----------------------------------------------------------------------
// File operations (active pane)
// ----------------------------------------------------------------------

void MainWindow::newDocument()
{
    if (!m_activePane) return;
    m_activePane->docManager()->newDocument();
    setActivePaneHtml("<p></p>");
    updateTitle();
}

void MainWindow::openFile()
{
    if (!m_activePane) return;
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(),
        tr("Markdown Files (*.md *.markdown *.txt);;All Files (*)"));
    if (path.isEmpty()) return;

    m_activePane->docManager()->openDocument(path);
    m_recentFiles->addFile(path);
    setActivePaneMarkdown(m_activePane->docManager()->currentContent());
    m_sidebar->setCurrentFile(path);
    updateTitle();
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder"), QString());
    if (dir.isEmpty()) return;
    m_recentFiles->addFolder(dir);
    m_sidebar->setRootPath(dir);
}

void MainWindow::saveFile()
{
    if (!m_activePane) return;
    auto* dm = m_activePane->docManager();
    if (dm->currentFilePath().isEmpty()) {
        saveFileAs();
        return;
    }
#ifdef HAS_WEBENGINE
    auto* p = m_activePane;
    p->page()->runJavaScript("colasonAPI.getMarkdown()", [this, p, dm](const QVariant& result) {
        if (result.isValid() && dm->saveDocument(result.toString())) {
            m_draftManager->deleteAllDrafts();
        } else {
            statusBar()->showMessage(tr("Failed to save file"), 5000);
        }
        if (p == m_activePane) updateTitle();
    });
#endif
}

void MainWindow::saveFileAs()
{
    if (!m_activePane) return;
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), QString(),
        tr("Markdown Files (*.md);;All Files (*)"));
    if (path.isEmpty()) return;

#ifdef HAS_WEBENGINE
    auto* p = m_activePane;
    auto* dm = p->docManager();
    p->page()->runJavaScript("colasonAPI.getMarkdown()", [this, p, dm, path](const QVariant& result) {
        if (result.isValid() && dm->saveDocumentAs(path, result.toString())) {
            m_recentFiles->addFile(path);
            m_draftManager->deleteAllDrafts();
        } else {
            statusBar()->showMessage(tr("Failed to save file"), 5000);
        }
        if (p == m_activePane) updateTitle();
    });
#endif
}

// ----------------------------------------------------------------------
// View / mode toggles (active pane unless noted)
// ----------------------------------------------------------------------

void MainWindow::toggleSidebar()
{
    m_sidebarVisible = !m_sidebarVisible;
    m_sidebar->setVisible(m_sidebarVisible);
}

#define ACTIVE_JS(call) do { \
    if (m_activePane) m_activePane->page()->runJavaScript(call); \
} while (0)

void MainWindow::toggleSourceMode()    { ACTIVE_JS("colasonAPI.toggleSourceMode()"); }
void MainWindow::toggleFocusMode()     { ACTIVE_JS("colasonAPI.toggleFocusMode()"); }
void MainWindow::toggleTypewriterMode(){ ACTIVE_JS("colasonAPI.toggleTypewriterMode()"); }

void MainWindow::toggleWideMode()
{
    m_wideMode = !m_wideMode;
    QSettings().setValue("view/wideMode", m_wideMode);
#ifdef HAS_WEBENGINE
    QString js = QString("colasonAPI.setWideMode(%1)").arg(m_wideMode ? "true" : "false");
    for (auto* p : m_panes) p->page()->runJavaScript(js);
#endif
}

void MainWindow::openInNewWindow()
{
    auto* w = new MainWindow();
    w->setAttribute(Qt::WA_DeleteOnClose, true);
    QRect g = geometry();
    QScreen* scr = screen();
    QRect avail = scr ? scr->availableGeometry() : QRect(0, 0, 2560, 1440);
    int x = qMin(g.right() + 12, avail.right() - g.width());
    int y = g.top();
    w->setGeometry(QRect(QPoint(x, y), g.size()));
    w->show();
    w->raise();
    w->activateWindow();
}

void MainWindow::zoomIn()
{
    m_zoomPercent = qMin(m_zoomPercent + 10, 200);
#ifdef HAS_WEBENGINE
    QString js = QString("colasonAPI.setZoom(%1)").arg(m_zoomPercent);
    for (auto* p : m_panes) p->page()->runJavaScript(js);
#endif
}

void MainWindow::zoomOut()
{
    m_zoomPercent = qMax(m_zoomPercent - 10, 50);
#ifdef HAS_WEBENGINE
    QString js = QString("colasonAPI.setZoom(%1)").arg(m_zoomPercent);
    for (auto* p : m_panes) p->page()->runJavaScript(js);
#endif
}

void MainWindow::resetZoom()
{
    m_zoomPercent = 100;
#ifdef HAS_WEBENGINE
    for (auto* p : m_panes) p->page()->runJavaScript("colasonAPI.setZoom(100)");
#endif
}

void MainWindow::executeEditorCommand(const QString& command, const QString& argsJson)
{
#ifdef HAS_WEBENGINE
    if (!m_activePane) return;
    QString js = QString("colasonAPI.executeCommand(%1, %2)").arg(jsQuote(command), jsQuote(argsJson));
    m_activePane->page()->runJavaScript(js);
#else
    Q_UNUSED(command);
    Q_UNUSED(argsJson);
#endif
}

void MainWindow::quickOpen()
{
    if (!m_activePane) return;
    QString rootDir = m_sidebar->rootPath();
    if (rootDir.isEmpty()) rootDir = QDir::homePath();

    QuickOpenDialog dialog(rootDir, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.selectedFile();
        if (!path.isEmpty()) {
            m_activePane->docManager()->openDocument(path);
            m_recentFiles->addFile(path);
            setActivePaneMarkdown(m_activePane->docManager()->currentContent());
            updateTitle();
        }
    }
}

void MainWindow::showExportPdfDialog()
{
    if (!m_activePane) return;
    QString path = QFileDialog::getSaveFileName(this, tr("Export PDF"), QString(), tr("PDF Files (*.pdf)"));
    if (path.isEmpty()) return;
#ifdef HAS_WEBENGINE
    m_activePane->page()->runJavaScript("colasonAPI.getContent()", [this, path](const QVariant& result) {
        QString themeCss = m_themeManager->themeCSS(m_themeManager->currentThemeName());
        m_exportManager->exportToPdf(result.toString(), path, themeCss);
    });
#endif
}

void MainWindow::showExportHtmlDialog()
{
    if (!m_activePane) return;
    QString path = QFileDialog::getSaveFileName(this, tr("Export HTML"), QString(), tr("HTML Files (*.html)"));
    if (path.isEmpty()) return;
#ifdef HAS_WEBENGINE
    m_activePane->page()->runJavaScript("colasonAPI.getContent()", [this, path](const QVariant& result) {
        QString themeCss = m_themeManager->themeCSS(m_themeManager->currentThemeName());
        m_exportManager->exportToHtml(result.toString(), path, themeCss, true);
    });
#endif
}

void MainWindow::showThemeMenu() {}
void MainWindow::changeTheme(const QString& themeName) { m_prefs->setTheme(themeName); }

void MainWindow::setTitleBarAutoHide(bool enabled)
{
#ifndef Q_OS_WIN
    Q_UNUSED(enabled);
    m_titleBarAutoHide = false;
    QSettings settings;
    settings.setValue("window/titleBarAutoHide", false);
    if (m_titleBarHideTimer) m_titleBarHideTimer->stop();
    if (m_menuWidget) m_menuWidget->setVisible(true);
    m_titleBarShown = true;
    return;
#else
    m_titleBarAutoHide = enabled;
    QSettings settings;
    settings.setValue("window/titleBarAutoHide", enabled);
    if (!enabled) {
        m_titleBarHideTimer->stop();
        m_menuWidget->setVisible(true);
        m_titleBarShown = true;
    } else {
        m_titleBarHideTimer->start(1500);
    }
#endif
}

// ----------------------------------------------------------------------
// Active-pane content helpers
// ----------------------------------------------------------------------

void MainWindow::setActivePaneMarkdown(const QString& markdown)
{
#ifdef HAS_WEBENGINE
    if (!m_activePane) return;
    QString js = QString(
        "if(typeof colasonAPI!=='undefined' && colasonAPI.setMarkdown){"
        "colasonAPI.setMarkdown(%1);"
        "} else { console.error('[Colason] colasonAPI.setMarkdown not available'); }"
    ).arg(jsQuote(markdown));
    m_activePane->page()->runJavaScript(js);
#else
    Q_UNUSED(markdown);
#endif
}

void MainWindow::setActivePaneHtml(const QString& html)
{
#ifdef HAS_WEBENGINE
    if (!m_activePane) return;
    QString js = QString(
        "if(typeof colasonAPI!=='undefined' && colasonAPI.setContent){"
        "colasonAPI.setContent(%1);"
        "} else { console.error('[Colason] colasonAPI.setContent not available'); }"
    ).arg(jsQuote(html));
    m_activePane->page()->runJavaScript(js);
#else
    Q_UNUSED(html);
#endif
}

QString MainWindow::editorContent() const
{
    return m_activePane ? m_activePane->docManager()->currentContent() : QString();
}

void MainWindow::updateTitle()
{
    QString title = "untitled";
    QString filePath;
    bool dirty = false;
    if (m_activePane) {
        filePath = m_activePane->docManager()->currentFilePath();
        dirty = m_activePane->docManager()->isDirty();
    }
    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        title = fi.fileName();
    }
    if (m_panes.size() > 1) {
        title += QString(" — %1 panes").arg(m_panes.size());
    }
    if (dirty) title = "* " + title;
    setWindowTitle(title);
}

// ----------------------------------------------------------------------
// Theme
// ----------------------------------------------------------------------

void MainWindow::applyTheme(const QString& themeName, const QString& css, const QString& qss)
{
    qApp->setStyleSheet(qss);

#ifdef Q_OS_WIN
    COLORREF captionColor = 0;
    COLORREF textColor = 0;
    BOOL useDarkMode = FALSE;
    if (themeName == "dark") { captionColor = RGB(43,43,43); textColor = RGB(187,187,187); useDarkMode = TRUE; }
    else if (themeName == "github-light") { captionColor = RGB(246,248,250); textColor = RGB(36,41,47); }
    else if (themeName == "github-dark")  { captionColor = RGB(22,27,34); textColor = RGB(230,237,243); useDarkMode = TRUE; }
    else if (themeName == "sepia")        { captionColor = RGB(214,201,168); textColor = RGB(61,43,31); }
    else if (themeName == "nord")         { captionColor = RGB(46,52,64); textColor = RGB(216,222,233); useDarkMode = TRUE; }
    else if (themeName == "dracula")      { captionColor = RGB(33,34,44); textColor = RGB(248,248,242); useDarkMode = TRUE; }
    else                                  { captionColor = RGB(250,251,252); textColor = RGB(36,41,46); }

    HWND hwnd = reinterpret_cast<HWND>(winId());
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
#else
    Q_UNUSED(themeName);
#endif

#ifdef HAS_WEBENGINE
    injectThemeCSS(css);
#else
    Q_UNUSED(css);
#endif
}

void MainWindow::injectThemeCSS(const QString& css)
{
#ifdef HAS_WEBENGINE
    // Map the Qt theme name to the web-side `data-theme` token so the
    // tokens.css `html[data-theme='...']` overrides do not outrank
    // the `:root` rules we inject here.
    const QString themeName = m_themeManager->currentThemeName();
    QString webTheme;
    if (themeName == "light" || themeName == "github-light") {
        webTheme = "light";
    } else if (themeName == "sepia") {
        webTheme = "sepia";
    } else {
        webTheme = "dark"; // dark / github-dark / nord / dracula / custom
    }

    QString js = QString(
        "(function(){"
        "if(!document.head) return;"
        "var el=document.getElementById('colason-theme');"
        "if(!el){el=document.createElement('style');el.id='colason-theme';document.head.appendChild(el);}"
        "el.textContent=%1;"
        "document.documentElement.setAttribute('data-theme', %2);"
        "document.documentElement.setAttribute('data-theme-mode', %2);"
        "document.documentElement.style.colorScheme = (%2 === 'dark') ? 'dark' : 'light';"
        "try { localStorage.setItem('colason.theme', %2); } catch(e) {}"
        "document.dispatchEvent(new CustomEvent('colason:themechange', { detail: { mode: %2, resolved: %2 } }));"
        "})()"
    ).arg(jsQuote(css), jsQuote(webTheme));
    for (auto* p : m_panes) p->page()->runJavaScript(js);
#else
    Q_UNUSED(css);
#endif
}

void MainWindow::checkDraftRecovery()
{
    if (!m_draftManager->hasRecoverableDrafts()) return;
    auto ret = QMessageBox::question(
        this, tr("Recover Draft"),
        tr("Unsaved drafts were found. Would you like to recover them?"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore);

    if (ret == QMessageBox::Yes) {
        QStringList drafts = m_draftManager->recoverableDraftPaths();
        if (!drafts.isEmpty() && m_activePane) {
            QString content = m_draftManager->readDraft(drafts.first());
            setActivePaneHtml(content);
            m_activePane->docManager()->setDirty(true);
            updateTitle();
        }
    } else if (ret == QMessageBox::No) {
        m_draftManager->deleteAllDrafts();
    }
}

void MainWindow::applyPreferences()
{
    if (m_prefs->autoDetectTheme()) {
        m_themeManager->detectSystemTheme();
    } else {
        m_themeManager->setTheme(m_prefs->theme());
    }

    m_wideMode = QSettings().value("view/wideMode", false).toBool();

    // The per-pane loadFinished handler (wirePane) re-applies theme/wide/keybinding,
    // so nothing else is needed here.
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        m_menuBarManager->updateMaximizeIcon(isMaximized());
    }
    QMainWindow::changeEvent(event);
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    auto* msg = static_cast<MSG*>(message);

    switch (msg->message) {
    case WM_NCCALCSIZE: {
        if (msg->wParam == TRUE) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
            if (IsZoomed(msg->hwnd)) {
                HMONITOR mon = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = {sizeof(mi)};
                if (GetMonitorInfo(mon, &mi)) {
                    params->rgrc[0] = mi.rcWork;
                }
            }
            *result = 0;
            return true;
        }
        break;
    }
    case WM_NCHITTEST: {
        const int x = GET_X_LPARAM(msg->lParam);
        const int y = GET_Y_LPARAM(msg->lParam);
        RECT rect;
        GetWindowRect(msg->hwnd, &rect);
        if (!IsZoomed(msg->hwnd)) {
            const int bw = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            const int bh = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            const bool left   = (x - rect.left)   < bw;
            const bool right  = (rect.right - x)  < bw;
            const bool top    = (y - rect.top)     < bh;
            const bool bottom = (rect.bottom - y)  < bh;
            if (top && left)     { *result = HTTOPLEFT;     return true; }
            if (top && right)    { *result = HTTOPRIGHT;    return true; }
            if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
            if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
            if (left)            { *result = HTLEFT;        return true; }
            if (right)           { *result = HTRIGHT;       return true; }
            if (top)             { *result = HTTOP;         return true; }
            if (bottom)          { *result = HTBOTTOM;      return true; }
        }
        const QPoint localPos = mapFromGlobal(QPoint(x, y));
        if (m_titleBarAutoHide) {
            const int activeTbHeight = (m_titleBarShown && m_menuWidget)
                                           ? qMax(m_menuWidget->height(), 32) : 0;
            const int hoverZone = m_titleBarShown ? activeTbHeight : 5;
            if (localPos.y() >= 0 && localPos.y() < hoverZone) {
                if (!m_titleBarShown) {
                    m_menuWidget->setVisible(true);
                    m_titleBarShown = true;
                }
                m_titleBarHideTimer->stop();
            } else if (m_titleBarShown) {
                if (!m_titleBarHideTimer->isActive()) {
                    m_titleBarHideTimer->start(500);
                }
            }
        }
        const int tbHeight = (m_menuWidget && m_menuWidget->isVisible())
                                 ? qMax(m_menuWidget->height(), 32) : 0;
        if (tbHeight > 0 && localPos.y() >= 0 && localPos.y() < tbHeight) {
            QWidget* child = childAt(localPos);
            bool interactive = false;
            QWidget* w = child;
            while (w && w != this) {
                if (qobject_cast<QPushButton*>(w) || qobject_cast<QMenuBar*>(w)) {
                    interactive = true;
                    break;
                }
                w = w->parentWidget();
            }
            if (!interactive) {
                *result = HTCAPTION;
                return true;
            }
        }
        if (m_titleBarAutoHide && !m_titleBarShown && localPos.y() >= 0 && localPos.y() < 5) {
            *result = HTCAPTION;
            return true;
        }
        *result = HTCLIENT;
        return true;
    }
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        HMONITOR mon = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {sizeof(mi)};
        if (GetMonitorInfo(mon, &mi)) {
            mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
            mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
            mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
            mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
        }
        *result = 0;
        return true;
    }
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Check any dirty pane
    bool anyDirty = false;
    for (auto* p : m_panes) {
        if (p->docManager()->isDirty()) { anyDirty = true; break; }
    }
    if (anyDirty) {
        auto ret = QMessageBox::question(
            this, tr("Unsaved Changes"),
            tr("Do you want to save changes before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            saveFile();  // saves active pane only
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());

    m_draftManager->deleteAllDrafts();
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!m_activePane) return;
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString path = urls.first().toLocalFile();

    QStringList imageExts = {"png", "jpg", "jpeg", "gif", "svg", "bmp", "webp"};
    QFileInfo fi(path);
    if (imageExts.contains(fi.suffix().toLower())) {
        QString docDir = QFileInfo(m_activePane->docManager()->currentFilePath()).absolutePath();
        if (!docDir.isEmpty()) {
            QString relativePath = m_imageManager->handleImageDrop(path, docDir);
            executeEditorCommand("insertImage",
                QString(R"({"src":"%1","alt":"%2"})").arg(relativePath, fi.baseName()));
        }
        return;
    }

    if (path.endsWith(".md") || path.endsWith(".markdown") || path.endsWith(".txt")) {
        m_activePane->docManager()->openDocument(path);
        m_recentFiles->addFile(path);
        setActivePaneMarkdown(m_activePane->docManager()->currentContent());
        updateTitle();
    }
}
