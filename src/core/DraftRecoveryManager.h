#pragma once

#include <QObject>
#include <QTimer>
#include <QDir>

class DocumentManager;

class DraftRecoveryManager : public QObject
{
    Q_OBJECT

public:
    explicit DraftRecoveryManager(DocumentManager* docManager, QObject* parent = nullptr);

    bool hasRecoverableDrafts() const;
    QStringList recoverableDraftPaths() const;
    QString readDraft(const QString& draftPath) const;
    QString originalPathForDraft(const QString& draftPath) const;
    void deleteDraft(const QString& draftPath);
    void deleteAllDrafts();
    void cleanupOldDrafts(int maxAgeDays = 7);

signals:
    void getContentRequested();

public slots:
    void saveDraft(const QString& content);

private slots:
    void onDraftTimerTick();

private:
    QString draftDir() const;
    QString draftFileName(const QString& originalPath) const;
    void ensureDraftDir();

    DocumentManager* m_docManager;
    QTimer* m_draftTimer;
    bool m_waitingForContent = false;
};
