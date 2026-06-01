#pragma once

#include <QWidget>
#include <QString>

#ifdef HAS_WEBENGINE
#include <QWebEngineView>
#include <QWebChannel>
#endif

class EditorBridge;
class OutlineBridge;
class SearchBridge;
class ThemeBridge;
class DocumentManager;

// One editor surface — own QWebEngineView + own QWebChannel + own bridges
// + own DocumentManager. Multiple panes coexist inside MainWindow's pane
// splitter so the user can edit/view several documents side-by-side
// (VSCode-style split). Each pane is fully independent except for theme
// + zoom which MainWindow broadcasts to all panes.
class EditorPane : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPane(QWidget* parent = nullptr);
    ~EditorPane() override;

#ifdef HAS_WEBENGINE
    QWebEngineView* view() const { return m_view; }
    QWebEnginePage* page() const { return m_view ? m_view->page() : nullptr; }
#endif

    EditorBridge* editorBridge() const { return m_editorBridge; }
    OutlineBridge* outlineBridge() const { return m_outlineBridge; }
    SearchBridge* searchBridge() const { return m_searchBridge; }
    ThemeBridge* themeBridge() const { return m_themeBridge; }
    DocumentManager* docManager() const { return m_docManager; }

    void setActive(bool active);
    bool isActive() const { return m_active; }

    // Load the bundled editor HTML (path resolved by MainWindow).
    void loadEditorPage(const QString& indexPath, const QString& fallbackQrc);

signals:
    // Emitted when the user clicks/focuses inside this pane — MainWindow
    // listens and promotes the pane to "active".
    void focusRequested(EditorPane* self);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupBridges();
    void applyActiveStyle();

#ifdef HAS_WEBENGINE
    QWebEngineView* m_view = nullptr;
    QWebChannel* m_channel = nullptr;
#endif
    EditorBridge* m_editorBridge = nullptr;
    OutlineBridge* m_outlineBridge = nullptr;
    SearchBridge* m_searchBridge = nullptr;
    ThemeBridge* m_themeBridge = nullptr;
    DocumentManager* m_docManager = nullptr;

    bool m_active = false;
};
