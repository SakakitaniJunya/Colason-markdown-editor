#include "SidebarContainer.h"
#include "DocumentListPanel.h"
#include "FileExplorerPanel.h"
#include "OutlinePanel.h"
#include <QTabBar>

SidebarContainer::SidebarContainer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SidebarContainer::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setDocumentMode(true);
    // Tab bar visible for panel switching

    // Document list tab (index 0) - primary tab like Typora
    m_documentList = new DocumentListPanel(this);
    m_tabWidget->addTab(m_documentList, QString::fromUtf8("\346\226\207\346\233\270"));  // "文書"

    // File Explorer tab (index 1)
    m_fileExplorer = new FileExplorerPanel(this);
    m_tabWidget->addTab(m_fileExplorer, tr("Files"));

    // Outline tab (index 2)
    m_outlinePanel = new OutlinePanel(this);
    m_tabWidget->addTab(m_outlinePanel, tr("Outline"));

    layout->addWidget(m_tabWidget);

    // Forward signals from document list and file explorer
    connect(m_documentList, &DocumentListPanel::fileSelected,
            this, &SidebarContainer::fileSelected);
    connect(m_fileExplorer, &FileExplorerPanel::fileSelected,
            this, &SidebarContainer::fileSelected);
    connect(m_outlinePanel, &OutlinePanel::headingClicked,
            this, &SidebarContainer::headingClicked);
    connect(m_fileExplorer, &FileExplorerPanel::openFolderRequested,
            this, &SidebarContainer::openFolderRequested);
}

void SidebarContainer::setRootPath(const QString& path)
{
    m_fileExplorer->setRootPath(path);
    m_documentList->setRootPath(path);
    m_tabWidget->setCurrentWidget(m_documentList);
}

void SidebarContainer::updateOutline(const QString& json)
{
    m_outlinePanel->updateHeadings(json);
}

void SidebarContainer::setActiveHeading(const QString& id)
{
    m_outlinePanel->setActiveHeading(id);
}

QString SidebarContainer::rootPath() const
{
    return m_fileExplorer->rootPath();
}

void SidebarContainer::setCurrentFile(const QString& filePath)
{
    m_fileExplorer->setCurrentFile(filePath);
    m_documentList->setCurrentFile(filePath);
}

void SidebarContainer::switchToPanel(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}
