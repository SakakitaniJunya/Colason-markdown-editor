#include "DocumentListPanel.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QScrollBar>

DocumentListPanel::DocumentListPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &DocumentListPanel::onDirectoryChanged);
}

void DocumentListPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(this);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setSpacing(0);

    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemClicked,
            this, &DocumentListPanel::onItemClicked);
}

void DocumentListPanel::setRootPath(const QString& path)
{
    if (m_rootPath == path) return;

    // Remove old watched path
    if (!m_rootPath.isEmpty()) {
        m_watcher->removePath(m_rootPath);
    }

    m_rootPath = path;

    if (!path.isEmpty()) {
        m_watcher->addPath(path);
    }

    scanDocuments();
}

QString DocumentListPanel::rootPath() const
{
    return m_rootPath;
}

void DocumentListPanel::setCurrentFile(const QString& filePath)
{
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto* item = m_listWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            m_listWidget->setCurrentItem(item);
            m_listWidget->scrollToItem(item);
            return;
        }
    }
}

void DocumentListPanel::refresh()
{
    scanDocuments();
}

void DocumentListPanel::scanDocuments()
{
    m_listWidget->clear();
    m_documentPaths.clear();

    if (m_rootPath.isEmpty()) return;

    QDir dir(m_rootPath);
    QStringList filters = {"*.md", "*.markdown", "*.txt"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    int index = 1;
    for (const auto& fi : files) {
        QString filePath = fi.absoluteFilePath();
        QString title = extractTitle(filePath);
        QString preview = extractPreview(filePath);

        if (title.isEmpty()) {
            title = fi.completeBaseName();
        }

        m_documentPaths.append(filePath);

        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, filePath);
        item->setSizeHint(QSize(0, 64));
        m_listWidget->addItem(item);

        auto* widget = createDocumentItem(index, filePath, title, preview);
        m_listWidget->setItemWidget(item, widget);

        ++index;
    }
}

QString DocumentListPanel::extractTitle(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream in(&file);
    // Read first 20 lines to find a heading
    for (int i = 0; i < 20 && !in.atEnd(); ++i) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // Markdown heading
        static QRegularExpression headingRe(R"(^#{1,6}\s+(.+))");
        auto match = headingRe.match(line);
        if (match.hasMatch()) {
            return match.captured(1).trimmed();
        }

        // First non-empty line as title fallback
        if (i == 0 || (i == 1 && line.startsWith("==="))) {
            // Setext heading (=== underline)
            if (line.startsWith("===")) continue;
            return line;
        }
    }
    return {};
}

QString DocumentListPanel::extractPreview(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream in(&file);
    QStringList lines;
    bool pastTitle = false;

    for (int i = 0; i < 30 && !in.atEnd(); ++i) {
        QString line = in.readLine().trimmed();

        // Skip heading lines
        if (!pastTitle) {
            if (line.isEmpty()) continue;
            if (line.startsWith('#') || line.startsWith("===") || line.startsWith("---")) {
                pastTitle = true;
                continue;
            }
            pastTitle = true;
        }

        if (line.isEmpty()) continue;
        // Skip markdown syntax
        if (line.startsWith("```") || line.startsWith("---") || line.startsWith("===")) continue;

        // Clean markdown formatting
        line.remove(QRegularExpression(R"(\*{1,2}|_{1,2}|`|!\[|\]\([^)]*\)|\[|\])"));
        line = line.trimmed();

        if (!line.isEmpty()) {
            lines.append(line);
            if (lines.size() >= 2) break;
        }
    }

    return lines.join(" ").left(120);
}

QWidget* DocumentListPanel::createDocumentItem(int index, const QString& filePath,
                                                const QString& title, const QString& preview)
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(2);

    // Top row: index + extension
    QFileInfo fi(filePath);
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(6);

    auto* indexLabel = new QLabel(QString::number(index));
    indexLabel->setObjectName("docIndex");

    auto* extLabel = new QLabel(fi.suffix().toLower());
    extLabel->setObjectName("docExt");

    topRow->addWidget(indexLabel);
    topRow->addWidget(extLabel);
    topRow->addStretch();

    // Title
    auto* titleLabel = new QLabel(title);
    titleLabel->setObjectName("docTitle");
    titleLabel->setWordWrap(false);
    titleLabel->setTextFormat(Qt::PlainText);

    // Preview
    auto* previewLabel = new QLabel(preview);
    previewLabel->setObjectName("docPreview");
    previewLabel->setWordWrap(true);
    previewLabel->setTextFormat(Qt::PlainText);
    previewLabel->setMaximumHeight(34);

    layout->addLayout(topRow);
    layout->addWidget(titleLabel);
    if (!preview.isEmpty()) {
        layout->addWidget(previewLabel);
    }

    return widget;
}

void DocumentListPanel::onItemClicked(QListWidgetItem* item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        emit fileSelected(filePath);
    }
}

void DocumentListPanel::onDirectoryChanged(const QString& /*path*/)
{
    scanDocuments();
}
