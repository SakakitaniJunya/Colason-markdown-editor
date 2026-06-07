#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>

class DocumentListPanel;
class FileExplorerPanel;
class OutlinePanel;

class SidebarContainer : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarContainer(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString rootPath() const;
    void updateOutline(const QString& json);
    void setActiveHeading(const QString& id);
    void setCurrentFile(const QString& filePath);
    void switchToPanel(int index);

signals:
    void fileSelected(const QString& filePath);
    void headingClicked(const QString& id);
    void openFolderRequested();
    void newFileCreated(const QString& filePath);

private:
    void setupUI();

    QTabWidget* m_tabWidget = nullptr;
    DocumentListPanel* m_documentList = nullptr;
    FileExplorerPanel* m_fileExplorer = nullptr;
    OutlinePanel* m_outlinePanel = nullptr;
};
