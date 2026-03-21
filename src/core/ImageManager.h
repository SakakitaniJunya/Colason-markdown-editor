#pragma once

#include <QObject>
#include <QString>

class ImageManager : public QObject
{
    Q_OBJECT

public:
    explicit ImageManager(QObject* parent = nullptr);

    QString handleImageDrop(const QString& originalPath, const QString& documentDir);
    QString handleClipboardImage(const QImage& image, const QString& documentDir);
    void setAssetsSubfolder(const QString& subfolder) { m_assetsSubfolder = subfolder; }
    QString assetsSubfolder() const { return m_assetsSubfolder; }

private:
    QString ensureAssetsDir(const QString& documentDir);
    QString resolveUniquePath(const QString& dirPath, const QString& fileName);

    QString m_assetsSubfolder = "assets";
};
