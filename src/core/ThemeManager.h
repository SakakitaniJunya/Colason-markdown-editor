#pragma once

#include <QObject>
#include <QString>
#include <QMap>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    explicit ThemeManager(QObject* parent = nullptr);

    QStringList availableThemes() const;
    QString currentThemeName() const { return m_currentTheme; }
    QString themeCSS(const QString& themeName) const;
    QString themeQSS(const QString& themeName) const;
    void setTheme(const QString& themeName);
    void loadCustomTheme(const QString& cssFilePath);
    void detectSystemTheme();
    QString detectSystemThemeName() const;
    bool isDarkTheme(const QString& themeName) const;

signals:
    void themeChanged(const QString& themeName, const QString& css, const QString& qss);

private:
    void registerBuiltinThemes();
    QString readFile(const QString& path) const;

    struct ThemeData {
        QString displayName;
        QString editorCSS;
        QString appQSS;
        bool isDark;
    };

    QMap<QString, ThemeData> m_themes;
    QString m_currentTheme = "dark";
};
