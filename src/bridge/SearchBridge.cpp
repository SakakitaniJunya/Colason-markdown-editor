#include "SearchBridge.h"

SearchBridge::SearchBridge(QObject* parent)
    : QObject(parent)
{
}

void SearchBridge::requestFind(const QString& query, bool caseSensitive, bool regex)
{
    emit findRequested(query, caseSensitive, regex);
}

void SearchBridge::requestReplace(const QString& from, const QString& to, bool all)
{
    emit replaceRequested(from, to, all);
}

void SearchBridge::requestFindNext()
{
    emit findNextRequested();
}

void SearchBridge::requestFindPrevious()
{
    emit findPreviousRequested();
}
