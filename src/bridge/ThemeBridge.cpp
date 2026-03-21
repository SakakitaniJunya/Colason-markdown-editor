#include "ThemeBridge.h"

ThemeBridge::ThemeBridge(QObject* parent)
    : QObject(parent)
{
}

void ThemeBridge::requestSetTheme(const QString& css)
{
    emit setThemeRequested(css);
}