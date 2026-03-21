#pragma once

#include <QObject>
#include <QTimer>

class DocumentManager;
class EditorBridge;

class AutoSaveManager : public QObject
{
    Q_OBJECT

public:
    explicit AutoSaveManager(DocumentManager* docManager, QObject* parent = nullptr);

    void setInterval(int ms);
    int interval() const { return m_timer->interval(); }
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

signals:
    void autoSaved();
    void getContentRequested();

public slots:
    void onContentReceived(const QString& content);

private slots:
    void onTimerTick();

private:
    DocumentManager* m_docManager;
    QTimer* m_timer;
    bool m_enabled = true;
    bool m_waitingForContent = false;
};
