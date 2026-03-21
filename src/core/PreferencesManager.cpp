#include "PreferencesManager.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

PreferencesManager::PreferencesManager(QObject* parent)
    : QObject(parent)
{
    load();
}

QString PreferencesManager::configFilePath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) dir.mkpath(".");
    return configDir + "/settings.json";
}

void PreferencesManager::load()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        emit preferencesLoaded();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isObject()) {
        fromJson(doc.object());
    }

    emit preferencesLoaded();
}

void PreferencesManager::save()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
}

QJsonObject PreferencesManager::toJson() const
{
    QJsonObject obj;

    QJsonObject editor;
    editor["keybinding"] = m_keybinding;
    editor["fontSize"] = m_fontSize;
    editor["fontFamily"] = m_fontFamily;
    editor["wordWrap"] = m_wordWrap;
    editor["lineNumbers"] = m_lineNumbers;
    obj["editor"] = editor;

    QJsonObject autoSave;
    autoSave["enabled"] = m_autoSaveEnabled;
    autoSave["intervalMs"] = m_autoSaveIntervalMs;
    obj["autoSave"] = autoSave;

    QJsonObject theme;
    theme["name"] = m_theme;
    theme["autoDetect"] = m_autoDetectTheme;
    obj["theme"] = theme;

    QJsonObject image;
    image["assetSubfolder"] = m_imageAssetSubfolder;
    obj["image"] = image;

    return obj;
}

void PreferencesManager::fromJson(const QJsonObject& obj)
{
    if (obj.contains("editor")) {
        QJsonObject editor = obj["editor"].toObject();
        m_keybinding = editor["keybinding"].toString(m_keybinding);
        m_fontSize = editor["fontSize"].toInt(m_fontSize);
        m_fontFamily = editor["fontFamily"].toString(m_fontFamily);
        m_wordWrap = editor["wordWrap"].toBool(m_wordWrap);
        m_lineNumbers = editor["lineNumbers"].toBool(m_lineNumbers);
    }

    if (obj.contains("autoSave")) {
        QJsonObject autoSave = obj["autoSave"].toObject();
        m_autoSaveEnabled = autoSave["enabled"].toBool(m_autoSaveEnabled);
        m_autoSaveIntervalMs = autoSave["intervalMs"].toInt(m_autoSaveIntervalMs);
    }

    if (obj.contains("theme")) {
        QJsonObject theme = obj["theme"].toObject();
        m_theme = theme["name"].toString(m_theme);
        m_autoDetectTheme = theme["autoDetect"].toBool(m_autoDetectTheme);

        // Migrate renamed theme keys
        if (m_theme == "github") {
            m_theme = "github-light";
        }
    }

    if (obj.contains("image")) {
        QJsonObject image = obj["image"].toObject();
        m_imageAssetSubfolder = image["assetSubfolder"].toString(m_imageAssetSubfolder);
    }
}

void PreferencesManager::setKeybinding(const QString& keybinding)
{
    if (m_keybinding != keybinding) {
        m_keybinding = keybinding;
        save();
        emit keybindingChanged(keybinding);
    }
}

void PreferencesManager::setFontSize(int size)
{
    if (m_fontSize != size) {
        m_fontSize = size;
        save();
        emit fontChanged(m_fontFamily, m_fontSize);
    }
}

void PreferencesManager::setFontFamily(const QString& family)
{
    if (m_fontFamily != family) {
        m_fontFamily = family;
        save();
        emit fontChanged(m_fontFamily, m_fontSize);
    }
}

void PreferencesManager::setWordWrap(bool wrap)
{
    if (m_wordWrap != wrap) {
        m_wordWrap = wrap;
        save();
        emit wordWrapChanged(wrap);
    }
}

void PreferencesManager::setLineNumbers(bool show)
{
    if (m_lineNumbers != show) {
        m_lineNumbers = show;
        save();
        emit lineNumbersChanged(show);
    }
}

void PreferencesManager::setAutoSaveEnabled(bool enabled)
{
    if (m_autoSaveEnabled != enabled) {
        m_autoSaveEnabled = enabled;
        save();
        emit autoSaveChanged(m_autoSaveEnabled, m_autoSaveIntervalMs);
    }
}

void PreferencesManager::setAutoSaveIntervalMs(int ms)
{
    if (m_autoSaveIntervalMs != ms) {
        m_autoSaveIntervalMs = ms;
        save();
        emit autoSaveChanged(m_autoSaveEnabled, m_autoSaveIntervalMs);
    }
}

void PreferencesManager::setTheme(const QString& theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        save();
        emit themeChanged(theme);
    }
}

void PreferencesManager::setAutoDetectTheme(bool detect)
{
    if (m_autoDetectTheme != detect) {
        m_autoDetectTheme = detect;
        save();
    }
}

void PreferencesManager::setImageAssetSubfolder(const QString& subfolder)
{
    if (m_imageAssetSubfolder != subfolder) {
        m_imageAssetSubfolder = subfolder;
        save();
    }
}
