#include "DocumentManager.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

DocumentManager::DocumentManager(QObject* parent)
    : QObject(parent)
{
}

void DocumentManager::newDocument()
{
    m_filePath.clear();
    m_content.clear();
    setDirty(false);
    emit documentChanged();
}

bool DocumentManager::openDocument(const QString& filePath)
{
    QString content = readFile(filePath);
    if (content.isNull() && !QFile::exists(filePath)) {
        return false;
    }

    m_filePath = filePath;
    m_content = content;
    setDirty(false);
    emit documentChanged();
    return true;
}

bool DocumentManager::saveDocument(const QString& content)
{
    if (m_filePath.isEmpty()) {
        return false;
    }

    if (!writeFile(m_filePath, content)) {
        return false;
    }

    m_content = content;
    setDirty(false);
    return true;
}

bool DocumentManager::saveDocumentAs(const QString& filePath, const QString& content)
{
    if (!writeFile(filePath, content)) {
        return false;
    }

    m_filePath = filePath;
    m_content = content;
    setDirty(false);
    emit documentChanged();
    return true;
}

void DocumentManager::setDirty(bool dirty)
{
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

void DocumentManager::setContent(const QString& content)
{
    m_content = content;
}

bool DocumentManager::writeFile(const QString& filePath, const QString& content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    out.flush();
    file.close();
    return file.error() == QFileDevice::NoError;
}

QString DocumentManager::readFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}
