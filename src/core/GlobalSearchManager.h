#pragma once

#include <QObject>
#include <QVector>

struct SearchResult {
    QString filePath;
    int lineNumber;
    QString lineText;
    int matchStart;
    int matchLength;
};

class GlobalSearchManager : public QObject
{
    Q_OBJECT

public:
    explicit GlobalSearchManager(QObject* parent = nullptr);

    void search(const QString& rootDir, const QString& query,
                const QString& fileFilter = "*.md", bool caseSensitive = false, bool useRegex = false);
    void cancel();
    bool isSearching() const { return m_searching; }

signals:
    void searchStarted();
    void resultFound(const SearchResult& result);
    void searchFinished(int totalMatches);

private:
    void doSearch(const QString& rootDir, const QString& query,
                  const QString& fileFilter, bool caseSensitive, bool useRegex);

    bool m_searching = false;
    bool m_cancelled = false;
};
