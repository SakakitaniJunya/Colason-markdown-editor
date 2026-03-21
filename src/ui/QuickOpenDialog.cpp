#include "QuickOpenDialog.h"

#include <QDirIterator>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDir>
#include <algorithm>

QuickOpenDialog::QuickOpenDialog(const QString& rootDir, QWidget* parent)
    : QDialog(parent)
    , m_rootDir(rootDir)
{
    setWindowTitle(tr("Quick Open"));
    setMinimumSize(500, 400);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Type to search files..."));
    m_filterEdit->setStyleSheet(
        "QLineEdit { border: none; border-bottom: 1px solid #0078d4; padding: 10px 16px; "
        "font-size: 14px; background: #f5f5f5; }");
    layout->addWidget(m_filterEdit);

    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(
        "QListWidget { border: none; }"
        "QListWidget::item { padding: 6px 16px; }"
        "QListWidget::item:hover { background: #e8e8e8; }"
        "QListWidget::item:selected { background: #0078d4; color: white; }");
    layout->addWidget(m_listWidget);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &QuickOpenDialog::onFilterChanged);
    connect(m_listWidget, &QListWidget::itemActivated, this, &QuickOpenDialog::onItemActivated);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &QuickOpenDialog::onItemActivated);

    setStyleSheet("QDialog { border: 1px solid #ccc; }");

    scanFiles();
    filterFiles("");
    m_filterEdit->setFocus();
}

void QuickOpenDialog::scanFiles()
{
    m_allFiles.clear();

    if (m_rootDir.isEmpty()) return;

    QStringList nameFilters = {"*.md", "*.markdown", "*.txt", "*.html"};
    QDirIterator it(m_rootDir, nameFilters, QDir::Files, QDirIterator::Subdirectories);

    QDir root(m_rootDir);
    while (it.hasNext()) {
        QString path = it.next();
        m_allFiles.append(root.relativeFilePath(path));
    }

    std::sort(m_allFiles.begin(), m_allFiles.end());
}

void QuickOpenDialog::filterFiles(const QString& query)
{
    m_listWidget->clear();

    if (query.isEmpty()) {
        // Show all files (limited)
        int count = qMin(m_allFiles.size(), 50);
        for (int i = 0; i < count; ++i) {
            auto* item = new QListWidgetItem(m_allFiles[i]);
            item->setData(Qt::UserRole, m_rootDir + "/" + m_allFiles[i]);
            m_listWidget->addItem(item);
        }
        return;
    }

    // Fuzzy match and sort by score
    struct ScoredFile {
        QString path;
        int score;
    };

    QVector<ScoredFile> scored;
    for (const auto& file : m_allFiles) {
        int score = fuzzyScore(query.toLower(), file.toLower());
        if (score > 0) {
            scored.append({file, score});
        }
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredFile& a, const ScoredFile& b) {
        return a.score > b.score;
    });

    int count = qMin(scored.size(), 50);
    for (int i = 0; i < count; ++i) {
        auto* item = new QListWidgetItem(scored[i].path);
        item->setData(Qt::UserRole, m_rootDir + "/" + scored[i].path);
        m_listWidget->addItem(item);
    }
}

int QuickOpenDialog::fuzzyScore(const QString& query, const QString& target) const
{
    if (query.isEmpty()) return 1;

    int score = 0;
    int qi = 0;
    int consecutive = 0;

    for (int ti = 0; ti < target.length() && qi < query.length(); ++ti) {
        if (target[ti] == query[qi]) {
            score += 1 + consecutive;
            consecutive++;
            qi++;

            // Bonus for matching at word boundaries
            if (ti == 0 || target[ti - 1] == '/' || target[ti - 1] == '\\' || target[ti - 1] == '-' || target[ti - 1] == '_') {
                score += 5;
            }
        } else {
            consecutive = 0;
        }
    }

    return qi == query.length() ? score : 0;
}

void QuickOpenDialog::onFilterChanged(const QString& text)
{
    filterFiles(text);
}

void QuickOpenDialog::onItemActivated(QListWidgetItem* item)
{
    m_selectedFile = item->data(Qt::UserRole).toString();
    accept();
}

void QuickOpenDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }

    if (event->key() == Qt::Key_Down) {
        int row = m_listWidget->currentRow();
        if (row < m_listWidget->count() - 1) {
            m_listWidget->setCurrentRow(row + 1);
        }
        return;
    }

    if (event->key() == Qt::Key_Up) {
        int row = m_listWidget->currentRow();
        if (row > 0) {
            m_listWidget->setCurrentRow(row - 1);
        }
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        auto* item = m_listWidget->currentItem();
        if (item) {
            onItemActivated(item);
        }
        return;
    }

    QDialog::keyPressEvent(event);
}
