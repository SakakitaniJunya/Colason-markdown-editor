#include "ImageManager.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QDateTime>

ImageManager::ImageManager(QObject* parent)
    : QObject(parent)
{
}

QString ImageManager::ensureAssetsDir(const QString& documentDir)
{
    QString assetsPath = documentDir + "/" + m_assetsSubfolder;
    QDir dir(assetsPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return assetsPath;
}

QString ImageManager::resolveUniquePath(const QString& dirPath, const QString& fileName)
{
    QFileInfo fi(dirPath + "/" + fileName);
    if (!fi.exists()) return fi.absoluteFilePath();

    QString base = fi.completeBaseName();
    QString ext = fi.suffix();
    int counter = 1;

    while (true) {
        QString newName = QString("%1-%2.%3").arg(base).arg(counter).arg(ext);
        QString newPath = dirPath + "/" + newName;
        if (!QFile::exists(newPath)) return newPath;
        ++counter;
    }
}

QString ImageManager::handleImageDrop(const QString& originalPath, const QString& documentDir)
{
    if (documentDir.isEmpty()) return originalPath;

    QFileInfo fi(originalPath);
    if (!fi.exists()) return {};

    QString assetsDir = ensureAssetsDir(documentDir);
    QString destPath = resolveUniquePath(assetsDir, fi.fileName());

    if (QFile::copy(originalPath, destPath)) {
        // Return relative path from document directory
        QDir docDir(documentDir);
        return docDir.relativeFilePath(destPath);
    }

    return originalPath;
}

QString ImageManager::handleClipboardImage(const QImage& image, const QString& documentDir)
{
    if (image.isNull() || documentDir.isEmpty()) return {};

    QString assetsDir = ensureAssetsDir(documentDir);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString fileName = QString("image-%1.png").arg(timestamp);
    QString destPath = resolveUniquePath(assetsDir, fileName);

    if (image.save(destPath, "PNG")) {
        QDir docDir(documentDir);
        return docDir.relativeFilePath(destPath);
    }

    return {};
}
