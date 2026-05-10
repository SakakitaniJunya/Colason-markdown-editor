#include "EditorPane.h"

#include "bridge/EditorBridge.h"
#include "bridge/OutlineBridge.h"
#include "bridge/SearchBridge.h"
#include "bridge/ThemeBridge.h"
#include "core/DocumentManager.h"

#include <QVBoxLayout>
#include <QFile>
#include <QUrl>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QEvent>

#ifdef HAS_WEBENGINE
#include <QWebEnginePage>
#include <QWebEngineSettings>
#endif

EditorPane::EditorPane(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("editorPane");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#ifdef HAS_WEBENGINE
    m_view = new QWebEngineView(this);
    m_view->setObjectName("editorPaneView");
    layout->addWidget(m_view);

    // Track focus on the underlying focus-proxy widget so clicks inside
    // the rendered web content also raise focusRequested.
    m_view->installEventFilter(this);
    if (auto* fp = m_view->focusProxy()) {
        fp->installEventFilter(this);
    }
#endif

    m_docManager = new DocumentManager(this);
    setupBridges();
    applyActiveStyle();
}

EditorPane::~EditorPane() = default;

void EditorPane::setupBridges()
{
    m_editorBridge = new EditorBridge(this);
    m_outlineBridge = new OutlineBridge(this);
    m_searchBridge = new SearchBridge(this);
    m_themeBridge = new ThemeBridge(this);

#ifdef HAS_WEBENGINE
    m_channel = new QWebChannel(this);
    m_channel->registerObject("editor", m_editorBridge);
    m_channel->registerObject("outline", m_outlineBridge);
    m_channel->registerObject("search", m_searchBridge);
    m_channel->registerObject("theme", m_themeBridge);

    m_view->page()->setWebChannel(m_channel);

    auto* settings = m_view->page()->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
#endif
}

void EditorPane::loadEditorPage(const QString& indexPath, const QString& fallbackQrc)
{
#ifdef HAS_WEBENGINE
    if (!m_view) return;
    if (QFile::exists(indexPath)) {
        m_view->setUrl(QUrl::fromLocalFile(indexPath));
    } else {
        m_view->setUrl(QUrl(fallbackQrc));
    }
#else
    Q_UNUSED(indexPath);
    Q_UNUSED(fallbackQrc);
#endif
}

void EditorPane::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    applyActiveStyle();
}

void EditorPane::applyActiveStyle()
{
    // Subtle 2px accent bar at the top of the active pane so users can see
    // which pane they are typing in (matches VSCode behavior).
    if (m_active) {
        setStyleSheet("#editorPane { border-top: 2px solid #4a90e2; }"
                      "#editorPane > QWebEngineView { border: none; }");
    } else {
        setStyleSheet("#editorPane { border-top: 2px solid transparent; }"
                      "#editorPane > QWebEngineView { border: none; }");
    }
}

bool EditorPane::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    switch (event->type()) {
    case QEvent::FocusIn:
    case QEvent::MouseButtonPress:
        emit focusRequested(this);
        break;
    default:
        break;
    }
    return false;
}
