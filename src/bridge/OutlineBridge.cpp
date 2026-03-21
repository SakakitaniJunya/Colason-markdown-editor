#include "OutlineBridge.h"

OutlineBridge::OutlineBridge(QObject* parent)
    : QObject(parent)
{
}

void OutlineBridge::requestScrollToHeading(const QString& headingId)
{
    emit scrollToHeadingRequested(headingId);
}