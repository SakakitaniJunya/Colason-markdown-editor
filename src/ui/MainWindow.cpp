#include "MainWindow.h"
#include "MenuBarManager.h"
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

#ifdef HAS_WEBENGINE
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_docManager = new DocumentManager(this);
    m_prefs = new PreferencesManager(this);

    setupUI();
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
    m_themeManager = new ThemeManager(this);
    setupWebEngine();
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

    // Restore title bar auto-hide state
#ifdef Q_OS_WIN
    if (settings.value("window/titleBarAutoHide", false).toBool()) {
        setTitleBarAutoHide(true);
    }
#else
    settings.setValue("window/titleBarAutoHide", false);
#endif

    // Apply saved theme or auto-detect
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

void MainWindow::setupUI()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_splitter);

    // Sidebar
    m_sidebar = new SidebarContainer(this);
    m_splitter->addWidget(m_sidebar);

    // Editor area
#ifdef HAS_WEBENGINE
    m_editorView = new QWebEngineView(this);
    m_splitter->addWidget(m_editorView);
#else
    m_editorPlaceholder = new QWidget(this);
    auto* label = new QLabel(
        QString::fromUtf8("QWebEngine \343\201\214\345\210\251\347\224\250\343\201\247\343\201\215\343\201\276\343\201\233\343\202\223\343\200\202\n"  // "QWebEngine が利用できません。\n"
                          "qtwebengine \343\202\222\343\203\223\343\203\253\343\203\211\343\201\227\343\201\246\343\202\250\343\203\207\343\202\243\343\202\277\343\202\222\346\234\211\345\212\271\343\201\253\343\201\227\343\201\246\343\201\217\343\201\240\343\201\225\343\201\204\343\200\202"),  // "qtwebengine をビルドしてエディタを有効にしてください。"
        m_editorPlaceholder);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 14px;");
    auto* layout = new QVBoxLayout(m_editorPlaceholder);
    layout->addWidget(label);
    m_splitter->addWidget(m_editorPlaceholder);
#endif

    // Splitter sizes: sidebar 240px, editor rest
    m_splitter->setSizes({240, 1000});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setCollapsible(0, true);   // Sidebar collapsible
    m_splitter->setCollapsible(1, false);  // Editor not collapsible
    m_splitter->setHandleWidth(1);         // 1px border per design

    // Minimum sizes
    m_sidebar->setMinimumWidth(160);
    m_sidebar->setMaximumWidth(400);

    // Panel switching shortcuts: 1=Files, 2=Outline, 3=Documents
    auto* shortcut1 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_1), this);
    connect(shortcut1, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(0);  // Files
    });

    auto* shortcut2 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_2), this);
    connect(shortcut2, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(1);  // Outline
    });

    auto* shortcut3 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_3), this);
    connect(shortcut3, &QShortcut::activated, this, [this]() {
        if (!m_sidebarVisible) toggleSidebar();
        m_sidebar->switchToPanel(2);  // Documents
    });

    // QuickOpen shortcut
    auto* quickOpenShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), this);
    connect(quickOpenShortcut, &QShortcut::activated, this, &MainWindow::quickOpen);
}

void MainWindow::setupMenuBar()
{
    m_menuBarManager = new MenuBarManager(this);

    // Create a standalone QMenuBar (not the default one from QMainWindow)
    auto* menuBar = new QMenuBar(nullptr);

    // Wrap it in a container widget with hamburger + pin + window control buttons
    m_menuWidget = m_menuBarManager->createMenuWidget(menuBar);

    // Replace QMainWindow's default menu bar area with our custom widget
    setMenuWidget(m_menuWidget);
}

void MainWindow::setupFramelessWindow()
{
#ifdef Q_OS_WIN
    // Force native window handle creation, then extend DWM frame for shadow
    HWND hwnd = reinterpret_cast<HWND>(winId());
    MARGINS margins = {0, 0, 0, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Notify Windows that the non-client area calculation has changed
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

void MainWindow::setupWebEngine()
{
#ifdef HAS_WEBENGINE
    // Setup QWebChannel with all bridges
    m_channel = new QWebChannel(this);
    m_editorBridge = new EditorBridge(this);
    m_outlineBridge = new OutlineBridge(this);
    m_searchBridge = new SearchBridge(this);
    m_themeBridge = new ThemeBridge(this);

    m_channel->registerObject("editor", m_editorBridge);
    m_channel->registerObject("outline", m_outlineBridge);
    m_channel->registerObject("search", m_searchBridge);
    m_channel->registerObject("theme", m_themeBridge);

    m_editorView->page()->setWebChannel(m_channel);

    // qwebchannel.js is bundled in the TypeScript editor (editor/src/qwebchannel.ts)
    // so no C++ injection is needed

    // WebEngine settings
    auto* settings = m_editorView->page()->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

    // Determine initial theme and inject CSS BEFORE page loads to prevent FOUC
    QString themeName = m_prefs->autoDetectTheme()
        ? m_themeManager->detectSystemThemeName()
        : m_prefs->theme();
    QString themeCss = m_themeManager->themeCSS(themeName);
    bool isDark = m_themeManager->isDarkTheme(themeName);

    // Set page background color to match theme (prevents white flash)
    m_editorView->page()->setBackgroundColor(isDark ? QColor("#1e1e1e") : QColor("#ffffff"));

    // Inject theme CSS at DocumentReady — DOM exists, runs before page display
    if (!themeCss.isEmpty()) {
        QJsonArray arr;
        arr.append(themeCss);
        QString jsonCss = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        QString jsonStr = jsonCss.mid(1, jsonCss.size() - 2);

        QString jsSource = QString(
            "(function(){"
            "var t=document.head||document.documentElement;"
            "if(!t) return;"
            "var el=document.createElement('style');"
            "el.id='colason-theme';"
            "el.textContent=%1;"
            "t.appendChild(el);"
            "})()"
        ).arg(jsonStr);

        QWebEngineScript script;
        script.setName("colason-theme-init");
        script.setSourceCode(jsSource);
        script.setInjectionPoint(QWebEngineScript::DocumentReady);
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setRunsOnSubFrames(false);
        m_editorView->page()->scripts().insert(script);
    }

    loadEditorPage();
#endif
}

void MainWindow::setupManagers()
{
    m_recentFiles = new RecentFilesManager(this);
    m_autoSaveManager = new AutoSaveManager(m_docManager, this);
    m_draftManager = new DraftRecoveryManager(m_docManager, this);
    m_globalSearch = new GlobalSearchManager(this);
    m_imageManager = new ImageManager(this);
    m_exportManager = new ExportManager(this);
    // m_themeManager is created earlier (before setupWebEngine) for early CSS injection

    // AutoSave settings from preferences
    m_autoSaveManager->setEnabled(m_prefs->autoSaveEnabled());
    m_autoSaveManager->setInterval(m_prefs->autoSaveIntervalMs());
    m_imageManager->setAssetsSubfolder(m_prefs->imageAssetSubfolder());
}

void MainWindow::loadEditorPage()
{
#ifdef HAS_WEBENGINE
    QString editorDistDir = QString::fromUtf8(EDITOR_DIST_DIR);
    QString indexPath = editorDistDir + "/index.html";

    if (QFile::exists(indexPath)) {
        m_editorView->setUrl(QUrl::fromLocalFile(indexPath));
    } else {
        m_editorView->setUrl(QUrl("qrc:/editor/index.html"));
    }
#endif
}

void MainWindow::setupConnections()
{
#ifdef HAS_WEBENGINE
    // EditorBridge signals
    connect(m_editorBridge, &EditorBridge::contentChanged, this, [this](const QString& html) {
        m_docManager->setContent(html);
        updateTitle();
    });

    connect(m_editorBridge, &EditorBridge::wordCountChanged, this,
            [this](int, int) {
                // Word count display removed (no status bar)
            });

    connect(m_editorBridge, &EditorBridge::documentDirty, this, [this](bool dirty) {
        m_docManager->setDirty(dirty);
        updateTitle();
    });

    connect(m_outlineBridge, &OutlineBridge::headingsChanged, this,
            [this](const QString& json) {
                m_sidebar->updateOutline(json);
            });

    // OutlineBridge: active heading sync
    connect(m_outlineBridge, &OutlineBridge::activeHeadingChanged, this,
            [this](const QString& id) {
                m_sidebar->setActiveHeading(id);
            });

    // AutoSave: request content from editor
    connect(m_autoSaveManager, &AutoSaveManager::getContentRequested, this, [this]() {
        m_editorView->page()->runJavaScript("colasonAPI.getContent()", [this](const QVariant& result) {
            m_autoSaveManager->onContentReceived(result.toString());
        });
    });

    connect(m_autoSaveManager, &AutoSaveManager::autoSaved, this, [this]() {
        statusBar()->showMessage(tr("Auto-saved"), 3000);
        updateTitle();
    });

    // Draft recovery: request content
    connect(m_draftManager, &DraftRecoveryManager::getContentRequested, this, [this]() {
        m_editorView->page()->runJavaScript("colasonAPI.getContent()", [this](const QVariant& result) {
            m_draftManager->saveDraft(result.toString());
        });
    });

    // Export signals
    connect(m_exportManager, &ExportManager::exportFinished, this, [this](bool success, const QString& path) {
        if (success) {
            statusBar()->showMessage(tr("Exported to: %1").arg(path), 5000);
        }
    });

    connect(m_exportManager, &ExportManager::exportError, this, [this](const QString& error) {
        QMessageBox::warning(this, tr("Export Error"), error);
    });

    // Theme changes
    connect(m_themeManager, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);

    // Preferences changes
    connect(m_prefs, &PreferencesManager::autoSaveChanged, this, [this](bool enabled, int intervalMs) {
        m_autoSaveManager->setEnabled(enabled);
        m_autoSaveManager->setInterval(intervalMs);
    });

    connect(m_prefs, &PreferencesManager::keybindingChanged, this, [this](const QString& keybinding) {
        QString js = QString("colasonAPI.setKeybinding && colasonAPI.setKeybinding('%1')").arg(keybinding);
        m_editorView->page()->runJavaScript(js);
    });

    connect(m_prefs, &PreferencesManager::themeChanged, this, [this](const QString& theme) {
        m_themeManager->setTheme(theme);
    });
#endif

    // Sidebar -> Editor (outline click)
    connect(m_sidebar, &SidebarContainer::headingClicked, this,
            [this](const QString& id) {
#ifdef HAS_WEBENGINE
                QString js = QString("colasonAPI.scrollToHeading('%1')").arg(id);
                m_editorView->page()->runJavaScript(js);
#endif
            });

    // Sidebar -> Open folder (from placeholder button)
    connect(m_sidebar, &SidebarContainer::openFolderRequested, this, &MainWindow::openFolder);

    // Sidebar -> Open file
    connect(m_sidebar, &SidebarContainer::fileSelected, this,
            [this](const QString& path) {
                qDebug() << "[fileSelected] path:" << path;
                bool ok = m_docManager->openDocument(path);
                qDebug() << "[fileSelected] openDocument:" << ok;
                m_recentFiles->addFile(path);
                QString content = m_docManager->currentContent();
                qDebug() << "[fileSelected] content length:" << content.length();
                setEditorMarkdown(content);
                m_sidebar->setCurrentFile(path);
                updateTitle();
            });

    // OS theme change detection
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            [this](Qt::ColorScheme scheme) {
                if (m_prefs->autoDetectTheme()) {
                    m_themeManager->setTheme(scheme == Qt::ColorScheme::Dark ? "dark" : "light");
                }
            });
#endif
}

void MainWindow::newDocument()
{
    m_docManager->newDocument();
    setEditorHtml("<p></p>");
    updateTitle();
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(),
        tr("Markdown Files (*.md *.markdown *.txt);;All Files (*)"));

    if (path.isEmpty()) return;

    m_docManager->openDocument(path);
    m_recentFiles->addFile(path);
    setEditorMarkdown(m_docManager->currentContent());
    m_sidebar->setCurrentFile(path);
    updateTitle();
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Folder"), QString());

    if (dir.isEmpty()) return;
    m_recentFiles->addFolder(dir);
    m_sidebar->setRootPath(dir);
}

void MainWindow::saveFile()
{
    if (m_docManager->currentFilePath().isEmpty()) {
        saveFileAs();
        return;
    }

#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.getMarkdown()", [this](const QVariant& result) {
        if (result.isValid() && m_docManager->saveDocument(result.toString())) {
            m_draftManager->deleteAllDrafts();
        } else {
            statusBar()->showMessage(tr("Failed to save file"), 5000);
        }
        updateTitle();
    });
#endif
}

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), QString(),
        tr("Markdown Files (*.md);;All Files (*)"));

    if (path.isEmpty()) return;

#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.getMarkdown()", [this, path](const QVariant& result) {
        if (result.isValid() && m_docManager->saveDocumentAs(path, result.toString())) {
            m_recentFiles->addFile(path);
            m_draftManager->deleteAllDrafts();
        } else {
            statusBar()->showMessage(tr("Failed to save file"), 5000);
        }
        updateTitle();
    });
#endif
}

void MainWindow::toggleSidebar()
{
    m_sidebarVisible = !m_sidebarVisible;
    m_sidebar->setVisible(m_sidebarVisible);
}

void MainWindow::toggleSourceMode()
{
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.toggleSourceMode()");
#endif
}

void MainWindow::toggleFocusMode()
{
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.toggleFocusMode()");
#endif
}

void MainWindow::toggleTypewriterMode()
{
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.toggleTypewriterMode()");
#endif
}

void MainWindow::zoomIn()
{
    m_zoomPercent = qMin(m_zoomPercent + 10, 200);
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript(
        QString("colasonAPI.setZoom(%1)").arg(m_zoomPercent));
#endif
}

void MainWindow::zoomOut()
{
    m_zoomPercent = qMax(m_zoomPercent - 10, 50);
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript(
        QString("colasonAPI.setZoom(%1)").arg(m_zoomPercent));
#endif
}

void MainWindow::resetZoom()
{
    m_zoomPercent = 100;
#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.setZoom(100)");
#endif
}

void MainWindow::executeEditorCommand(const QString& command, const QString& argsJson)
{
#ifdef HAS_WEBENGINE
    QString escaped = argsJson;
    escaped.replace("'", "\\'");
    QString js = QString("colasonAPI.executeCommand('%1', '%2')").arg(command, escaped);
    m_editorView->page()->runJavaScript(js);
#endif
}

void MainWindow::quickOpen()
{
    QString rootDir = m_sidebar->rootPath();
    if (rootDir.isEmpty()) {
        rootDir = QDir::homePath();
    }

    QuickOpenDialog dialog(rootDir, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.selectedFile();
        if (!path.isEmpty()) {
            m_docManager->openDocument(path);
            m_recentFiles->addFile(path);
            setEditorMarkdown(m_docManager->currentContent());
            updateTitle();
        }
    }
}

void MainWindow::showExportPdfDialog()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export PDF"), QString(),
        tr("PDF Files (*.pdf)"));

    if (path.isEmpty()) return;

#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.getContent()", [this, path](const QVariant& result) {
        QString themeCss = m_themeManager->themeCSS(m_themeManager->currentThemeName());
        m_exportManager->exportToPdf(result.toString(), path, themeCss);
    });
#endif
}

void MainWindow::showExportHtmlDialog()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export HTML"), QString(),
        tr("HTML Files (*.html)"));

    if (path.isEmpty()) return;

#ifdef HAS_WEBENGINE
    m_editorView->page()->runJavaScript("colasonAPI.getContent()", [this, path](const QVariant& result) {
        QString themeCss = m_themeManager->themeCSS(m_themeManager->currentThemeName());
        m_exportManager->exportToHtml(result.toString(), path, themeCss, true);
    });
#endif
}

void MainWindow::showThemeMenu()
{
    // Theme selection is handled via menu actions set up in MenuBarManager
}

void MainWindow::changeTheme(const QString& themeName)
{
    m_prefs->setTheme(themeName);
}

void MainWindow::setTitleBarAutoHide(bool enabled)
{
#ifndef Q_OS_WIN
    Q_UNUSED(enabled);
    m_titleBarAutoHide = false;
    QSettings settings;
    settings.setValue("window/titleBarAutoHide", false);
    if (m_titleBarHideTimer) {
        m_titleBarHideTimer->stop();
    }
    if (m_menuWidget) {
        m_menuWidget->setVisible(true);
    }
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

void MainWindow::setEditorMarkdown(const QString& markdown)
{
#ifdef HAS_WEBENGINE
    // Use runJavaScript directly for reliability (bypasses QWebChannel signal timing)
    QJsonArray arr;
    arr.append(markdown);
    QString jsonMd = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    QString jsonStr = jsonMd.mid(1, jsonMd.size() - 2); // extract JSON string

    QString js = QString(
        "if(typeof colasonAPI!=='undefined' && colasonAPI.setMarkdown){"
        "colasonAPI.setMarkdown(%1);"
        "} else { console.error('[Colason] colasonAPI.setMarkdown not available'); }"
    ).arg(jsonStr);
    m_editorView->page()->runJavaScript(js);
#endif
}

void MainWindow::setEditorHtml(const QString& html)
{
#ifdef HAS_WEBENGINE
    QJsonArray arr;
    arr.append(html);
    QString jsonHtml = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    QString jsonStr = jsonHtml.mid(1, jsonHtml.size() - 2);

    QString js = QString(
        "if(typeof colasonAPI!=='undefined' && colasonAPI.setContent){"
        "colasonAPI.setContent(%1);"
        "} else { console.error('[Colason] colasonAPI.setContent not available'); }"
    ).arg(jsonStr);
    m_editorView->page()->runJavaScript(js);
#endif
}

QString MainWindow::editorContent() const
{
    return m_docManager->currentContent();
}

void MainWindow::updateTitle()
{
    QString title = "untitled";
    QString filePath = m_docManager->currentFilePath();

    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        title = fi.fileName();
    }

    if (m_docManager->isDirty()) {
        title = "* " + title;
    }

    setWindowTitle(title);
}

void MainWindow::applyTheme(const QString& themeName, const QString& css, const QString& qss)
{
    // Apply QSS to native widgets
    qApp->setStyleSheet(qss);

    // Apply title bar color to match theme
#ifdef Q_OS_WIN
    COLORREF captionColor = 0;
    COLORREF textColor = 0;
    BOOL useDarkMode = FALSE;

    if (themeName == "dark") {
        captionColor = RGB(43, 43, 43);       // #2b2b2b
        textColor = RGB(187, 187, 187);       // #bbbbbb
        useDarkMode = TRUE;
    } else if (themeName == "github-light") {
        captionColor = RGB(246, 248, 250);    // #f6f8fa
        textColor = RGB(36, 41, 47);          // #24292f
    } else if (themeName == "github-dark") {
        captionColor = RGB(22, 27, 34);       // #161b22
        textColor = RGB(230, 237, 243);       // #e6edf3
        useDarkMode = TRUE;
    } else if (themeName == "sepia") {
        captionColor = RGB(214, 201, 168);    // #d6c9a8
        textColor = RGB(61, 43, 31);          // #3d2b1f
    } else if (themeName == "nord") {
        captionColor = RGB(46, 52, 64);       // #2e3440
        textColor = RGB(216, 222, 233);       // #d8dee9
        useDarkMode = TRUE;
    } else if (themeName == "dracula") {
        captionColor = RGB(33, 34, 44);       // #21222c
        textColor = RGB(248, 248, 242);       // #f8f8f2
        useDarkMode = TRUE;
    } else { // light
        captionColor = RGB(250, 251, 252);    // #fafbfc
        textColor = RGB(36, 41, 46);          // #24292e
    }

    HWND hwnd = reinterpret_cast<HWND>(winId());
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
#endif

    // Apply CSS to web editor — inject directly into DOM for reliability
#ifdef HAS_WEBENGINE
    injectThemeCSS(css);
#endif
}

void MainWindow::injectThemeCSS(const QString& css)
{
#ifdef HAS_WEBENGINE
    // JSON-encode the CSS string for safe JavaScript injection
    QJsonArray arr;
    arr.append(css);
    QString jsonStr = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    // jsonStr is like ["css content here"] — extract just the quoted string
    QString jsonCss = jsonStr.mid(1, jsonStr.size() - 2);

    // Inject directly into DOM — no dependency on colasonAPI
    // appendChild moves existing element to end of <head>, ensuring highest cascade priority
    QString js = QString(
        "(function(){"
        "if(!document.head) return;"
        "var el=document.getElementById('colason-theme');"
        "if(!el){el=document.createElement('style');el.id='colason-theme';}"
        "el.textContent=%1;"
        "document.head.appendChild(el);"
        "})()"
    ).arg(jsonCss);

    m_editorView->page()->runJavaScript(js);
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
        if (!drafts.isEmpty()) {
            QString content = m_draftManager->readDraft(drafts.first());
            setEditorHtml(content);
            m_docManager->setDirty(true);
            updateTitle();
        }
    } else if (ret == QMessageBox::No) {
        m_draftManager->deleteAllDrafts();
    }
    // Ignore: leave drafts for next launch
}

void MainWindow::applyPreferences()
{
    // Apply theme
    if (m_prefs->autoDetectTheme()) {
        m_themeManager->detectSystemTheme();
    } else {
        m_themeManager->setTheme(m_prefs->theme());
    }

    // Apply theme CSS and keybinding once the editor page finishes loading
#ifdef HAS_WEBENGINE
    connect(m_editorView->page(), &QWebEnginePage::loadFinished, this, [this](bool ok) {
        if (!ok) return;

        // Re-apply theme CSS to WebEngine
        QString themeName = m_themeManager->currentThemeName();
        QString css = m_themeManager->themeCSS(themeName);
        if (!css.isEmpty()) {
            injectThemeCSS(css);
        }

        // Apply keybinding
        QString keybinding = m_prefs->keybinding();
        if (keybinding != "default") {
            QString js = QString("colasonAPI.setKeybinding && colasonAPI.setKeybinding('%1')").arg(keybinding);
            m_editorView->page()->runJavaScript(js);
        }
    }, Qt::SingleShotConnection);
#endif
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
            // When maximized, constrain to work area so we don't cover the taskbar
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

        // Resize borders (skip when maximized)
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

        // Auto-hide title bar: show/hide based on mouse position
        if (m_titleBarAutoHide) {
            const int activeTbHeight = (m_titleBarShown && m_menuWidget)
                                           ? qMax(m_menuWidget->height(), 32) : 0;
            const int hoverZone = m_titleBarShown ? activeTbHeight : 5;

            if (localPos.y() >= 0 && localPos.y() < hoverZone) {
                // Mouse in title bar area or trigger zone at top edge
                if (!m_titleBarShown) {
                    m_menuWidget->setVisible(true);
                    m_titleBarShown = true;
                }
                m_titleBarHideTimer->stop();
            } else if (m_titleBarShown) {
                // Mouse moved below title bar - schedule hide
                if (!m_titleBarHideTimer->isActive()) {
                    m_titleBarHideTimer->start(500);
                }
            }
        }

        // Title bar (menu widget) area: draggable unless over an interactive widget
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

        // When title bar is hidden, top 5px is still draggable
        if (m_titleBarAutoHide && !m_titleBarShown
            && localPos.y() >= 0 && localPos.y() < 5) {
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
    if (m_docManager->isDirty()) {
        auto ret = QMessageBox::question(
            this, tr("Unsaved Changes"),
            tr("Do you want to save changes before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            saveFile();
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    // Save window geometry
    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());

    // Clean up drafts on normal exit
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
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString path = urls.first().toLocalFile();

    // Image files: handle via ImageManager
    QStringList imageExts = {"png", "jpg", "jpeg", "gif", "svg", "bmp", "webp"};
    QFileInfo fi(path);
    if (imageExts.contains(fi.suffix().toLower())) {
        QString docDir = QFileInfo(m_docManager->currentFilePath()).absolutePath();
        if (!docDir.isEmpty()) {
            QString relativePath = m_imageManager->handleImageDrop(path, docDir);
            executeEditorCommand("insertImage",
                QString(R"({"src":"%1","alt":"%2"})").arg(relativePath, fi.baseName()));
        }
        return;
    }

    // Markdown files: open
    if (path.endsWith(".md") || path.endsWith(".markdown") || path.endsWith(".txt")) {
        m_docManager->openDocument(path);
        m_recentFiles->addFile(path);
        setEditorMarkdown(m_docManager->currentContent());
        updateTitle();
    }
}
