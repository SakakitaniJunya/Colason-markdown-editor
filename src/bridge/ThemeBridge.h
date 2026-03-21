#pragma once

#include <QObject>
#include <QString>

class ThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY themeChanged)

public:
    explicit ThemeBridge(QObject* parent = nullptr);

    QString currentTheme() const { return m_currentTheme; }
    Q_INVOKABLE void requestSetTheme(const QString& css);

signals:
    void setThemeRequested(const QString& css);
    void themeChanged(const QString& themeName);

private:
    QString m_currentTheme;
};