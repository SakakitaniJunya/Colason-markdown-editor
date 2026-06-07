#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include <QList>

class EditorPane;
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
    void openFileFromExplorer(const QString& path);
    void saveFile();
    void saveFileAs();
    void toggleSidebar();
    void toggleSourceMode();
    void toggleFocusMode();
    void toggleTypewriterMode();
    void toggleWideMode();
    void openInNewWindow();
    void splitRight();
    void splitDown();
    void closeActivePane();
    void focusNextPane();
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
    void setupConnections();
    void setupManagers();
    void updateTitle();
    void setActivePaneMarkdown(const QString& markdown);
    void setActivePaneHtml(const QString& html);
    QString editorContent() const;
    void applyTheme(const QString& themeName, const QString& css, const QString& qss);
    void injectThemeCSS(const QString& css);
    void checkDraftRecovery();
    void applyPreferences();

    // --- Pane management (VSCode-style in-window split) ---
    EditorPane* createPane();                  // builds + wires bridges, does NOT add to layout
    void wirePane(EditorPane* pane);            // connects per-pane bridge signals
    void loadEditorIntoPane(EditorPane* pane);  // sets URL of the editor HTML
    void setActivePane(EditorPane* pane);
    void splitActive(Qt::Orientation orientation);
    void removePane(EditorPane* pane);

    QSplitter* m_splitter = nullptr;            // sidebar | paneSplitter
    QSplitter* m_paneSplitter = nullptr;        // holds 1..N EditorPane widgets
    QList<EditorPane*> m_panes;
    EditorPane* m_activePane = nullptr;

    SidebarContainer* m_sidebar = nullptr;

#ifndef HAS_WEBENGINE
    QWidget* m_editorPlaceholder = nullptr;
#endif

    MenuBarManager* m_menuBarManager = nullptr;
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
    bool m_wideMode = false;
};
