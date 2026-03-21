#pragma once

#include <QObject>
#include <QPageLayout>

#ifdef HAS_WEBENGINE
class QWebEnginePage;
#endif

class ExportManager : public QObject
{
    Q_OBJECT

public:
    explicit ExportManager(QObject* parent = nullptr);

    void exportToPdf(const QString& htmlContent, const QString& outputPath,
                     const QString& themeCss = {});
    void exportToHtml(const QString& htmlContent, const QString& outputPath,
                      const QString& themeCss = {}, bool includeStyle = true);
    void exportToPng(const QString& htmlContent, const QString& outputPath,
                     const QString& themeCss = {}, int width = 800);

    void setPageLayout(const QPageLayout& layout) { m_pageLayout = layout; }
    QPageLayout pageLayout() const { return m_pageLayout; }

signals:
    void exportFinished(bool success, const QString& outputPath);
    void exportError(const QString& error);

private:
    QString wrapHtmlWithTheme(const QString& htmlContent, const QString& themeCss) const;

    QPageLayout m_pageLayout;
};
