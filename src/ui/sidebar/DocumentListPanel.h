#pragma once

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

class DocumentListPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DocumentListPanel(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString rootPath() const;
    void setCurrentFile(const QString& filePath);
    void refresh();

signals:
    void fileSelected(const QString& filePath);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onDirectoryChanged(const QString& path);

private:
    void setupUI();
    void scanDocuments();
    QString extractTitle(const QString& filePath) const;
    QString extractPreview(const QString& filePath) const;
    QWidget* createDocumentItem(int index, const QString& filePath,
                                const QString& title, const QString& preview);

    QListWidget* m_listWidget = nullptr;
    QLabel* m_headerLabel = nullptr;
    QString m_rootPath;
    QFileSystemWatcher* m_watcher = nullptr;
    QStringList m_documentPaths;
};
