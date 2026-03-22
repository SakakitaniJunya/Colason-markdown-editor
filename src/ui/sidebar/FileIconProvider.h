#pragma once

#include <QAbstractFileIconProvider>
#include <QIcon>

class FileIconProvider : public QAbstractFileIconProvider
{
public:
    FileIconProvider();

    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo& info) const override;

private:
    QIcon m_fileIcon;
    QIcon m_textFileIcon;
    QIcon m_markdownIcon;
    QIcon m_folderIcon;
    QIcon m_folderOpenIcon;
};
