#include "FileExplorerPanel.h"
#include "FileIconProvider.h"

#include <QHeaderView>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

FileExplorerPanel::FileExplorerPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupContextMenu();
}

void FileExplorerPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);

    // ── Page 0: Placeholder (no folder opened) ──
    m_placeholder = new QWidget(this);
    auto* phLayout = new QVBoxLayout(m_placeholder);
    phLayout->setContentsMargins(20, 40, 20, 20);
    phLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto* phLabel = new QLabel(tr("No folder opened"), m_placeholder);
    phLabel->setObjectName("filePlaceholder");
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setWordWrap(true);
    phLayout->addWidget(phLabel);

    auto* openBtn = new QPushButton(tr("Open Folder..."), m_placeholder);
    openBtn->setObjectName("openFolderBtn");
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setFixedWidth(140);
    phLayout->addSpacing(12);
    phLayout->addWidget(openBtn, 0, Qt::AlignCenter);
    phLayout->addStretch();

    connect(openBtn, &QPushButton::clicked, this, &FileExplorerPanel::openFolderRequested);

    m_stack->addWidget(m_placeholder);

    // ── Page 1: File explorer ──
    m_explorerPage = new QWidget(this);
    auto* expLayout = new QVBoxLayout(m_explorerPage);
    expLayout->setContentsMargins(0, 0, 0, 0);
    expLayout->setSpacing(0);

    // Folder name header
    m_folderLabel = new QLabel(m_explorerPage);
    m_folderLabel->setObjectName("folderHeader");
    m_folderLabel->setContentsMargins(10, 6, 10, 6);
    expLayout->addWidget(m_folderLabel);

    // File system model with custom icons
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setIconProvider(new FileIconProvider());

    // Tree view
    m_treeView = new QTreeView(m_explorerPage);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(12);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setFocusPolicy(Qt::NoFocus);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setUniformRowHeights(true);

    // Show only name column
    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);

    expLayout->addWidget(m_treeView);

    m_stack->addWidget(m_explorerPage);

    // Start with placeholder
    m_stack->setCurrentWidget(m_placeholder);

    layout->addWidget(m_stack);

    connect(m_treeView, &QTreeView::clicked, this, &FileExplorerPanel::onItemClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileExplorerPanel::onContextMenu);
}

void FileExplorerPanel::setupContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_contextMenu->addAction(tr("Open in File Manager"), this, [this]() {
        QModelIndex index = m_treeView->currentIndex();
        if (index.isValid()) {
            QString path = m_model->isDir(index) ? m_model->filePath(index) : m_model->fileInfo(index).absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    m_contextMenu->addAction(tr("Copy Path"), this, [this]() {
        QModelIndex index = m_treeView->currentIndex();
        if (index.isValid()) {
            QApplication::clipboard()->setText(m_model->filePath(index));
        }
    });
}

void FileExplorerPanel::setRootPath(const QString& path)
{
    QModelIndex rootIndex = m_model->setRootPath(path);
    m_treeView->setRootIndex(rootIndex);

    // Show folder name in header
    QDir dir(path);
    m_folderLabel->setText(dir.dirName().toUpper());

    // Switch to explorer page
    m_stack->setCurrentWidget(m_explorerPage);
}

QString FileExplorerPanel::rootPath() const
{
    return m_model->rootPath();
}

void FileExplorerPanel::setCurrentFile(const QString& filePath)
{
    if (!filePath.isEmpty()) {
        QModelIndex index = m_model->index(filePath);
        if (index.isValid()) {
            m_treeView->setCurrentIndex(index);
            m_treeView->scrollTo(index);
        }
    }
}

void FileExplorerPanel::onItemClicked(const QModelIndex& index)
{
    if (!m_model->isDir(index)) {
        emit fileSelected(m_model->filePath(index));
    }
}

void FileExplorerPanel::onContextMenu(const QPoint& pos)
{
    m_contextMenu->exec(m_treeView->viewport()->mapToGlobal(pos));
}
