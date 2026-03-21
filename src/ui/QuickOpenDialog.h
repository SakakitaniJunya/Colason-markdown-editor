#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QStringList>

class QuickOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuickOpenDialog(const QString& rootDir, QWidget* parent = nullptr);

    QString selectedFile() const { return m_selectedFile; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onFilterChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);

private:
    void scanFiles();
    void filterFiles(const QString& query);
    int fuzzyScore(const QString& query, const QString& target) const;

    QLineEdit* m_filterEdit = nullptr;
    QListWidget* m_listWidget = nullptr;
    QString m_rootDir;
    QStringList m_allFiles;
    QString m_selectedFile;
};
