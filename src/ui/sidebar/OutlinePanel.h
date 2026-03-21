#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QJsonArray>

class OutlinePanel : public QWidget
{
    Q_OBJECT

public:
    explicit OutlinePanel(QWidget* parent = nullptr);

    void updateHeadings(const QString& json);
    void setActiveHeading(const QString& id);

signals:
    void headingClicked(const QString& id);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void buildTree(const QJsonArray& headings);

    QTreeWidget* m_treeWidget = nullptr;
};
