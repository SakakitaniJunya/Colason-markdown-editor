#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QLabel>
#include <QTimer>

#ifdef HAS_WEBENGINE
#include <QWebEngineView>
#include <QWebChannel>
#endif

class EditorBridge;
class OutlineBridge;
class SearchBridge;
class ThemeBridge;
class SidebarContainer;
class MenuBarManager;
class DocumentManager;
class AutoSaveManager;
class DraftRecoveryManager;
class RecentFilesManager;
class GlobalSearchManager;
class ImageManager;
class ExportManager;
class ThemeManager;
class PreferencesManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    void newDocument();
    void openFile();
    void openFolder();
    void saveFile();
    void saveFileAs();
    void toggleSidebar();
    void toggleSourceMode();
    void toggleFocusMode();
    void toggleTypewriterMode();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void executeEditorCommand(const QString& command, const QString& argsJson = {});
    void quickOpen();
    void showExportPdfDialog();
    void showExportHtmlDialog();
    void showThemeMenu();
    void changeTheme(const QString& themeName);
    void setTitleBarAutoHide(bool enabled);
    bool titleBarAutoHide() const { return m_titleBarAutoHide; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private:
    void setupUI();
    void setupMenuBar();
    void setupFramelessWindow();
    void setupWebEngine();
    void setupConnections();
    void setupManagers();
    void loadEditorPage();
    void updateTitle();
    void setEditorMarkdown(const QString& markdown);
    void setEditorHtml(const QString& html);
    QString editorContent() const;
    void applyTheme(const QString& themeName, const QString& css, const QString& qss);
    void injectThemeCSS(const QString& css);
    void checkDraftRecovery();
    void applyPreferences();

    QSplitter* m_splitter = nullptr;
    SidebarContainer* m_sidebar = nullptr;

#ifdef HAS_WEBENGINE
    QWebEngineView* m_editorView = nullptr;
    QWebChannel* m_channel = nullptr;
    EditorBridge* m_editorBridge = nullptr;
    OutlineBridge* m_outlineBridge = nullptr;
    SearchBridge* m_searchBridge = nullptr;
    ThemeBridge* m_themeBridge = nullptr;
#else
    QWidget* m_editorPlaceholder = nullptr;
#endif

    MenuBarManager* m_menuBarManager = nullptr;
    DocumentManager* m_docManager = nullptr;
    AutoSaveManager* m_autoSaveManager = nullptr;
    DraftRecoveryManager* m_draftManager = nullptr;
    RecentFilesManager* m_recentFiles = nullptr;
    GlobalSearchManager* m_globalSearch = nullptr;
    ImageManager* m_imageManager = nullptr;
    ExportManager* m_exportManager = nullptr;
    ThemeManager* m_themeManager = nullptr;
    PreferencesManager* m_prefs = nullptr;

    QWidget* m_menuWidget = nullptr;
    bool m_sidebarVisible = true;
    int m_zoomPercent = 100;

    QTimer* m_titleBarHideTimer = nullptr;
    bool m_titleBarAutoHide = false;
    bool m_titleBarShown = true;
};
