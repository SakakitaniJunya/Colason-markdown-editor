#pragma once

#include <QObject>
#include <QString>

class SearchBridge : public QObject
{
    Q_OBJECT

public:
    explicit SearchBridge(QObject* parent = nullptr);

    Q_INVOKABLE void requestFind(const QString& query, bool caseSensitive, bool regex);
    Q_INVOKABLE void requestReplace(const QString& from, const QString& to, bool all);
    Q_INVOKABLE void requestFindNext();
    Q_INVOKABLE void requestFindPrevious();

signals:
    void findRequested(const QString& query, bool caseSensitive, bool regex);
    void replaceRequested(const QString& from, const QString& to, bool all);
    void findNextRequested();
    void findPreviousRequested();
    void searchResultsChanged(int matchCount, int currentIndex);
};