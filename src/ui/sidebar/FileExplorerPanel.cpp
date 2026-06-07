#include "FileExplorerPanel.h"
#include "FileIconProvider.h"

#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QAbstractItemDelegate>

FileExplorerPanel::FileExplorerPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
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

    // File system model — include hidden files (e.g. .claude, .git) and allow rename
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    m_model->setIconProvider(new FileIconProvider());
    m_model->setReadOnly(false); // enable rename via edit()

    // Tree view
    m_treeView = new QTreeView(m_explorerPage);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(12);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // only programmatic edit
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setFocusPolicy(Qt::StrongFocus);
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

    // Track inline edit completion for newly created files
    connect(m_treeView->itemDelegate(), &QAbstractItemDelegate::closeEditor, this,
        [this](QWidget*, QAbstractItemDelegate::EndEditHint hint) {
            if (m_pendingNewFilePath.isEmpty()) return;

            if (hint == QAbstractItemDelegate::RevertModelCache) {
                // User pressed Escape — open the placeholder file as-is
                QString path = m_pendingNewFilePath;
                m_pendingNewFilePath.clear();
                emit newFileCreated(path);
            } else {
                // User committed — wait a tick for QFileSystemModel::fileRenamed to fire.
                // If it doesn't fire (user accepted the placeholder name unchanged),
                // open the original path.
                QTimer::singleShot(80, this, [this]() {
                    if (!m_pendingNewFilePath.isEmpty()) {
                        QString path = m_pendingNewFilePath;
                        m_pendingNewFilePath.clear();
                        emit newFileCreated(path);
                    }
                });
            }
        });

    // When QFileSystemModel actually renames the file, open the final path
    connect(m_model, &QFileSystemModel::fileRenamed, this,
        [this](const QString& dir, const QString& /*oldName*/, const QString& newName) {
            if (m_pendingNewFilePath.isEmpty()) return;
            m_pendingNewFilePath.clear(); // prevent the timer from firing
            emit newFileCreated(dir + QDir::separator() + newName);
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
    QModelIndex index = m_treeView->indexAt(pos);
    QMenu menu;

    menu.addAction(tr("New File"), this, &FileExplorerPanel::createNewFile);
    menu.addAction(tr("New Folder"), this, &FileExplorerPanel::createNewFolder);

    if (index.isValid()) {
        menu.addSeparator();
        menu.addAction(tr("Rename"), this, &FileExplorerPanel::renameCurrentItem);
        menu.addAction(tr("Delete"), this, &FileExplorerPanel::deleteCurrentItem);
        menu.addSeparator();
        menu.addAction(tr("Open in Finder"), this, [this, index]() {
            QString path = m_model->isDir(index)
                ? m_model->filePath(index)
                : m_model->fileInfo(index).absolutePath();
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        menu.addAction(tr("Copy Path"), this, [this, index]() {
            QApplication::clipboard()->setText(m_model->filePath(index));
        });
    } else {
        menu.addSeparator();
        menu.addAction(tr("Open in Finder"), this, [this]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_model->rootPath()));
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void FileExplorerPanel::createNewFile()
{
    QModelIndex index = m_treeView->currentIndex();
    QString dirPath = m_model->rootPath();

    if (index.isValid()) {
        if (m_model->isDir(index)) {
            dirPath = m_model->filePath(index);
            m_treeView->expand(index);
        } else {
            dirPath = m_model->fileInfo(index).absolutePath();
        }
    }

    // Find unique placeholder name
    QString filePath = dirPath + "/untitled.md";
    int n = 1;
    while (QFile::exists(filePath)) {
        filePath = QString("%1/untitled-%2.md").arg(dirPath).arg(n++);
    }

    // Create the file on disk
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.close();

    m_pendingNewFilePath = filePath;

    // Wait for QFileSystemModel to detect the new file, then start inline rename
    QTimer::singleShot(150, this, [this, filePath]() {
        QModelIndex newIdx = m_model->index(filePath);
        if (newIdx.isValid()) {
            m_treeView->setCurrentIndex(newIdx);
            m_treeView->scrollTo(newIdx);
            m_treeView->edit(newIdx);
        } else {
            // Model hasn't refreshed yet — open directly
            QString path = m_pendingNewFilePath;
            m_pendingNewFilePath.clear();
            emit newFileCreated(path);
        }
    });
}

void FileExplorerPanel::createNewFolder()
{
    QModelIndex index = m_treeView->currentIndex();
    QString dirPath = m_model->rootPath();

    if (index.isValid()) {
        dirPath = m_model->isDir(index)
            ? m_model->filePath(index)
            : m_model->fileInfo(index).absolutePath();
        if (m_model->isDir(index)) m_treeView->expand(index);
    }

    QString folderPath = dirPath + "/untitled folder";
    int n = 1;
    while (QDir(folderPath).exists()) {
        folderPath = QString("%1/untitled folder %2").arg(dirPath).arg(n++);
    }

    if (!QDir().mkdir(folderPath)) return;

    QTimer::singleShot(150, this, [this, folderPath]() {
        QModelIndex newIdx = m_model->index(folderPath);
        if (newIdx.isValid()) {
            m_treeView->setCurrentIndex(newIdx);
            m_treeView->scrollTo(newIdx);
            m_treeView->edit(newIdx);
        }
    });
}

void FileExplorerPanel::renameCurrentItem()
{
    QModelIndex index = m_treeView->currentIndex();
    if (index.isValid()) {
        m_treeView->edit(index);
    }
}

void FileExplorerPanel::deleteCurrentItem()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    QString path = m_model->filePath(index);
    bool isDir = m_model->isDir(index);
    QString name = m_model->fileName(index);

    auto ret = QMessageBox::question(
        this, tr("Delete"),
        tr("Delete \"%1\"? This cannot be undone.").arg(name),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) return;

    if (isDir) {
        QDir(path).removeRecursively();
    } else {
        QFile::remove(path);
    }
}
