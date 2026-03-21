#include "FileIconProvider.h"

#include <QFileInfo>

FileIconProvider::FileIconProvider()
    : QAbstractFileIconProvider()
{
    m_fileIcon = QIcon(":/icons/file.svg");
    m_textFileIcon = QIcon(":/icons/file-text.svg");
    m_folderIcon = QIcon(":/icons/folder.svg");
    m_folderOpenIcon = QIcon(":/icons/folder-open.svg");
}

QIcon FileIconProvider::icon(IconType type) const
{
    switch (type) {
    case IconType::Folder:
        return m_folderIcon;
    case IconType::File:
        return m_fileIcon;
    default:
        return m_fileIcon;
    }
}

QIcon FileIconProvider::icon(const QFileInfo& info) const
{
    if (info.isDir()) {
        return m_folderIcon;
    }

    QString suffix = info.suffix().toLower();
    if (suffix == "md" || suffix == "markdown" || suffix == "txt"
        || suffix == "json" || suffix == "yaml" || suffix == "yml"
        || suffix == "toml" || suffix == "xml" || suffix == "html"
        || suffix == "css" || suffix == "js" || suffix == "ts"
        || suffix == "cpp" || suffix == "h" || suffix == "py"
        || suffix == "rs" || suffix == "go" || suffix == "java") {
        return m_textFileIcon;
    }

    return m_fileIcon;
}
