#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QStackedWidget>

class FileExplorerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileExplorerPanel(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString rootPath() const;
    void setCurrentFile(const QString& filePath);

signals:
    void fileSelected(const QString& filePath);
    void openFolderRequested();
    void newFileCreated(const QString& filePath);

private slots:
    void onItemClicked(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);

private:
    void setupUI();
    void createNewFile();
    void createNewFolder();
    void renameCurrentItem();
    void deleteCurrentItem();

    QStackedWidget* m_stack = nullptr;
    QWidget* m_placeholder = nullptr;
    QWidget* m_explorerPage = nullptr;
    QLabel* m_folderLabel = nullptr;
    QTreeView* m_treeView = nullptr;
    QFileSystemModel* m_model = nullptr;

    QString m_pendingNewFilePath; // path of file being named inline
};
