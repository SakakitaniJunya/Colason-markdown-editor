#pragma once

#include <QObject>
#include <QString>
#include <QFileInfo>

class DocumentManager : public QObject
{
    Q_OBJECT

public:
    explicit DocumentManager(QObject* parent = nullptr);

    void newDocument();
    bool openDocument(const QString& filePath);
    bool saveDocument(const QString& content);
    bool saveDocumentAs(const QString& filePath, const QString& content);

    QString currentFilePath() const { return m_filePath; }
    QString currentContent() const { return m_content; }
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty);
    void setContent(const QString& content);

signals:
    void documentChanged();
    void dirtyChanged(bool dirty);

private:
    bool writeFile(const QString& filePath, const QString& content);
    QString readFile(const QString& filePath);

    QString m_filePath;
    QString m_content;
    bool m_dirty = false;
};
