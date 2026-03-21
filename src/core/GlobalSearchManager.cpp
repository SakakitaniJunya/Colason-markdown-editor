#include "GlobalSearchManager.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QThread>

GlobalSearchManager::GlobalSearchManager(QObject* parent)
    : QObject(parent)
{
}

void GlobalSearchManager::search(const QString& rootDir, const QString& query,
                                  const QString& fileFilter, bool caseSensitive, bool useRegex)
{
    if (m_searching) return;
    m_cancelled = false;

    auto* thread = QThread::create([this, rootDir, query, fileFilter, caseSensitive, useRegex]() {
        doSearch(rootDir, query, fileFilter, caseSensitive, useRegex);
    });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void GlobalSearchManager::cancel()
{
    m_cancelled = true;
}

void GlobalSearchManager::doSearch(const QString& rootDir, const QString& query,
                                    const QString& fileFilter, bool caseSensitive, bool useRegex)
{
    m_searching = true;
    QMetaObject::invokeMethod(this, "searchStarted", Qt::QueuedConnection);

    int totalMatches = 0;
    QStringList filters = fileFilter.split(';', Qt::SkipEmptyParts);
    if (filters.isEmpty()) filters << "*.md";

    QRegularExpression regex;
    if (useRegex) {
        auto options = caseSensitive ? QRegularExpression::NoPatternOption
                                     : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(query, options);
    } else {
        auto options = caseSensitive ? QRegularExpression::NoPatternOption
                                     : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(QRegularExpression::escape(query), options);
    }

    if (!regex.isValid()) {
        m_searching = false;
        QMetaObject::invokeMethod(this, [this]() { emit searchFinished(0); }, Qt::QueuedConnection);
        return;
    }

    QDirIterator it(rootDir, filters, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !m_cancelled) {
        QString filePath = it.next();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        int lineNumber = 0;
        while (!in.atEnd() && !m_cancelled) {
            ++lineNumber;
            QString line = in.readLine();

            QRegularExpressionMatchIterator matchIt = regex.globalMatch(line);
            while (matchIt.hasNext()) {
                auto match = matchIt.next();
                SearchResult result;
                result.filePath = filePath;
                result.lineNumber = lineNumber;
                result.lineText = line.trimmed();
                result.matchStart = match.capturedStart();
                result.matchLength = match.capturedLength();

                ++totalMatches;

                QMetaObject::invokeMethod(this, [this, result]() {
                    emit resultFound(result);
                }, Qt::QueuedConnection);
            }
        }
    }

    m_searching = false;
    int total = totalMatches;
    QMetaObject::invokeMethod(this, [this, total]() { emit searchFinished(total); }, Qt::QueuedConnection);
}
