#pragma once

#include <QObject>
#include <QStringList>

class RecentFilesManager : public QObject
{
    Q_OBJECT

public:
    explicit RecentFilesManager(QObject* parent = nullptr, int maxEntries = 10);

    QStringList recentFiles() const;
    QStringList recentFolders() const;
    void addFile(const QString& filePath);
    void addFolder(const QString& folderPath);
    void clearFiles();
    void clearFolders();

signals:
    void recentFilesChanged();
    void recentFoldersChanged();

private:
    void load();
    void saveFiles();
    void saveFolders();

    int m_maxEntries;
    QStringList m_recentFiles;
    QStringList m_recentFolders;
};
