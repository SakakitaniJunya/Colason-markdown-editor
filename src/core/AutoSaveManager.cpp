#include "AutoSaveManager.h"
#include "DocumentManager.h"

AutoSaveManager::AutoSaveManager(DocumentManager* docManager, QObject* parent)
    : QObject(parent)
    , m_docManager(docManager)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(5 * 60 * 1000); // Default: 5 minutes
    connect(m_timer, &QTimer::timeout, this, &AutoSaveManager::onTimerTick);
    m_timer->start();
}

void AutoSaveManager::setInterval(int ms)
{
    m_timer->setInterval(ms);
}

void AutoSaveManager::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        m_timer->start();
    } else {
        m_timer->stop();
    }
}

void AutoSaveManager::onTimerTick()
{
    if (!m_enabled) return;
    if (!m_docManager->isDirty()) return;
    if (m_docManager->currentFilePath().isEmpty()) return;
    if (m_waitingForContent) return;

    m_waitingForContent = true;
    emit getContentRequested();
}

void AutoSaveManager::onContentReceived(const QString& content)
{
    if (!m_waitingForContent) return;
    m_waitingForContent = false;

    if (m_docManager->saveDocument(content)) {
        emit autoSaved();
    }
}
