#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>

class PreferencesManager : public QObject
{
    Q_OBJECT

public:
    explicit PreferencesManager(QObject* parent = nullptr);

    void load();
    void save();

    // Editor preferences
    QString keybinding() const { return m_keybinding; } // "default", "vim", "emacs"
    void setKeybinding(const QString& keybinding);
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& family);
    bool wordWrap() const { return m_wordWrap; }
    void setWordWrap(bool wrap);
    bool lineNumbers() const { return m_lineNumbers; }
    void setLineNumbers(bool show);

    // AutoSave
    bool autoSaveEnabled() const { return m_autoSaveEnabled; }
    void setAutoSaveEnabled(bool enabled);
    int autoSaveIntervalMs() const { return m_autoSaveIntervalMs; }
    void setAutoSaveIntervalMs(int ms);

    // Theme
    QString theme() const { return m_theme; }
    void setTheme(const QString& theme);
    bool autoDetectTheme() const { return m_autoDetectTheme; }
    void setAutoDetectTheme(bool detect);

    // Image
    QString imageAssetSubfolder() const { return m_imageAssetSubfolder; }
    void setImageAssetSubfolder(const QString& subfolder);

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

signals:
    void keybindingChanged(const QString& keybinding);
    void fontChanged(const QString& family, int size);
    void wordWrapChanged(bool wrap);
    void lineNumbersChanged(bool show);
    void autoSaveChanged(bool enabled, int intervalMs);
    void themeChanged(const QString& theme);
    void preferencesLoaded();

private:
    QString configFilePath() const;

    QString m_keybinding = "default";
    int m_fontSize = 16;
    QString m_fontFamily = "";
    bool m_wordWrap = true;
    bool m_lineNumbers = true;
    bool m_autoSaveEnabled = true;
    int m_autoSaveIntervalMs = 300000; // 5 minutes
    QString m_theme = "dark";
    bool m_autoDetectTheme = true;
    QString m_imageAssetSubfolder = "assets";
};
