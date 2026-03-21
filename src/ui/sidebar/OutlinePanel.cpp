#include "OutlinePanel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QStack>

OutlinePanel::OutlinePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void OutlinePanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(12);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setUniformRowHeights(true);

    m_treeWidget->setFocusPolicy(Qt::NoFocus);
    // Styling handled by ThemeManager QSS
    layout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &OutlinePanel::onItemClicked);
}

void OutlinePanel::updateHeadings(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return;

    buildTree(doc.array());
}

void OutlinePanel::buildTree(const QJsonArray& headings)
{
    m_treeWidget->clear();

    // Stack-based tree building: parent items by heading level
    QStack<QTreeWidgetItem*> stack;
    QStack<int> levelStack;

    for (const auto& val : headings) {
        QJsonObject obj = val.toObject();
        QString id = obj["id"].toString();
        QString text = obj["text"].toString();
        int level = obj["level"].toInt();

        // Pop items from stack until we find a parent with lower level
        while (!levelStack.isEmpty() && levelStack.top() >= level) {
            stack.pop();
            levelStack.pop();
        }

        auto* item = new QTreeWidgetItem();
        item->setText(0, text);
        item->setData(0, Qt::UserRole, id);
        item->setToolTip(0, text);

        if (stack.isEmpty()) {
            m_treeWidget->addTopLevelItem(item);
        } else {
            stack.top()->addChild(item);
        }

        stack.push(item);
        levelStack.push(level);
    }

    m_treeWidget->expandAll();
}

void OutlinePanel::setActiveHeading(const QString& id)
{
    // Find and highlight the active heading
    std::function<void(QTreeWidgetItem*)> findAndSelect =
        [&](QTreeWidgetItem* item) {
            if (item->data(0, Qt::UserRole).toString() == id) {
                m_treeWidget->setCurrentItem(item);
                m_treeWidget->scrollToItem(item);
                return;
            }
            for (int i = 0; i < item->childCount(); ++i) {
                findAndSelect(item->child(i));
            }
        };

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        findAndSelect(m_treeWidget->topLevelItem(i));
    }
}

void OutlinePanel::onItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    QString id = item->data(0, Qt::UserRole).toString();
    if (!id.isEmpty()) {
        emit headingClicked(id);
    }
}
