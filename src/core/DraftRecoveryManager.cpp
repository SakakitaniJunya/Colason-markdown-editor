#include "DraftRecoveryManager.h"
#include "DocumentManager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

DraftRecoveryManager::DraftRecoveryManager(DocumentManager* docManager, QObject* parent)
    : QObject(parent)
    , m_docManager(docManager)
{
    ensureDraftDir();

    m_draftTimer = new QTimer(this);
    m_draftTimer->setInterval(5000); // 5 seconds
    connect(m_draftTimer, &QTimer::timeout, this, &DraftRecoveryManager::onDraftTimerTick);
    m_draftTimer->start();
}

QString DraftRecoveryManager::draftDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/drafts";
}

void DraftRecoveryManager::ensureDraftDir()
{
    QDir dir(draftDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString DraftRecoveryManager::draftFileName(const QString& originalPath) const
{
    QFileInfo fi(originalPath);
    QString base = fi.completeBaseName();
    if (base.isEmpty()) base = "untitled";
    return base + "-" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".md.draft";
}

bool DraftRecoveryManager::hasRecoverableDrafts() const
{
    QDir dir(draftDir());
    return !dir.entryList({"*.draft"}, QDir::Files).isEmpty();
}

QStringList DraftRecoveryManager::recoverableDraftPaths() const
{
    QDir dir(draftDir());
    QStringList result;
    for (const auto& entry : dir.entryInfoList({"*.draft"}, QDir::Files, QDir::Time)) {
        result.append(entry.absoluteFilePath());
    }
    return result;
}

QString DraftRecoveryManager::readDraft(const QString& draftPath) const
{
    QFile file(draftPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

QString DraftRecoveryManager::originalPathForDraft(const QString& draftPath) const
{
    QString metaPath = draftPath + ".meta";
    QFile file(metaPath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object()["originalPath"].toString();
}

void DraftRecoveryManager::deleteDraft(const QString& draftPath)
{
    QFile::remove(draftPath);
    QFile::remove(draftPath + ".meta");
}

void DraftRecoveryManager::deleteAllDrafts()
{
    QDir dir(draftDir());
    for (const auto& entry : dir.entryInfoList({"*.draft", "*.meta"}, QDir::Files)) {
        QFile::remove(entry.absoluteFilePath());
    }
}

void DraftRecoveryManager::cleanupOldDrafts(int maxAgeDays)
{
    QDir dir(draftDir());
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-maxAgeDays);

    for (const auto& entry : dir.entryInfoList({"*.draft"}, QDir::Files)) {
        if (entry.lastModified() < cutoff) {
            QFile::remove(entry.absoluteFilePath());
            QFile::remove(entry.absoluteFilePath() + ".meta");
        }
    }
}

void DraftRecoveryManager::onDraftTimerTick()
{
    if (!m_docManager->isDirty()) return;
    if (m_waitingForContent) return;

    m_waitingForContent = true;
    emit getContentRequested();
}

void DraftRecoveryManager::saveDraft(const QString& content)
{
    m_waitingForContent = false;

    if (content.isEmpty()) return;

    ensureDraftDir();

    // Delete old drafts for same file first
    QString originalPath = m_docManager->currentFilePath();
    QDir dir(draftDir());
    for (const auto& entry : dir.entryInfoList({"*.draft"}, QDir::Files)) {
        QString existingOriginal = originalPathForDraft(entry.absoluteFilePath());
        if (existingOriginal == originalPath) {
            deleteDraft(entry.absoluteFilePath());
        }
    }

    // Save new draft
    QString draftPath = draftDir() + "/" + draftFileName(originalPath);

    QFile draftFile(draftPath);
    if (draftFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&draftFile);
        out.setEncoding(QStringConverter::Utf8);
        out << content;
    }

    // Save metadata
    QFile metaFile(draftPath + ".meta");
    if (metaFile.open(QIODevice::WriteOnly)) {
        QJsonObject meta;
        meta["originalPath"] = originalPath;
        meta["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metaFile.write(QJsonDocument(meta).toJson(QJsonDocument::Compact));
    }
}
