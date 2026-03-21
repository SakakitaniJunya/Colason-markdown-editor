#include "RecentFilesManager.h"

#include <QSettings>
#include <QFileInfo>

RecentFilesManager::RecentFilesManager(QObject* parent, int maxEntries)
    : QObject(parent)
    , m_maxEntries(maxEntries)
{
    load();
}

void RecentFilesManager::load()
{
    QSettings settings;
    m_recentFiles = settings.value("recent/files").toStringList();
    m_recentFolders = settings.value("recent/folders").toStringList();
}

void RecentFilesManager::saveFiles()
{
    QSettings settings;
    settings.setValue("recent/files", m_recentFiles);
}

void RecentFilesManager::saveFolders()
{
    QSettings settings;
    settings.setValue("recent/folders", m_recentFolders);
}

QStringList RecentFilesManager::recentFiles() const
{
    return m_recentFiles;
}

QStringList RecentFilesManager::recentFolders() const
{
    return m_recentFolders;
}

void RecentFilesManager::addFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    QString canonical = QFileInfo(filePath).absoluteFilePath();

    m_recentFiles.removeAll(canonical);
    m_recentFiles.prepend(canonical);

    while (m_recentFiles.size() > m_maxEntries) {
        m_recentFiles.removeLast();
    }

    saveFiles();
    emit recentFilesChanged();
}

void RecentFilesManager::addFolder(const QString& folderPath)
{
    if (folderPath.isEmpty()) return;
    QString canonical = QFileInfo(folderPath).absoluteFilePath();

    m_recentFolders.removeAll(canonical);
    m_recentFolders.prepend(canonical);

    while (m_recentFolders.size() > m_maxEntries) {
        m_recentFolders.removeLast();
    }

    saveFolders();
    emit recentFoldersChanged();
}

void RecentFilesManager::clearFiles()
{
    m_recentFiles.clear();
    saveFiles();
    emit recentFilesChanged();
}

void RecentFilesManager::clearFolders()
{
    m_recentFolders.clear();
    saveFolders();
    emit recentFoldersChanged();
}
