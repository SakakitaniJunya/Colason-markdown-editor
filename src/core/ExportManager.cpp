#include "ExportManager.h"

#include <QFile>
#include <QTextStream>
#include <QPageSize>
#include <QMarginsF>

#ifdef HAS_WEBENGINE
#include <QWebEnginePage>
#include <QWebEngineSettings>
#endif

ExportManager::ExportManager(QObject* parent)
    : QObject(parent)
{
    // Default: A4 portrait, 20mm margins
    m_pageLayout = QPageLayout(
        QPageSize(QPageSize::A4),
        QPageLayout::Portrait,
        QMarginsF(20, 20, 20, 20),
        QPageLayout::Millimeter
    );
}

QString ExportManager::wrapHtmlWithTheme(const QString& htmlContent, const QString& themeCss) const
{
    return QString(
        "<!DOCTYPE html><html><head>"
        "<meta charset=\"UTF-8\">"
        "<style>"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif; "
        "font-size: 16px; line-height: 1.6; color: #333; max-width: 860px; margin: 0 auto; padding: 40px 30px; }"
        "h1 { font-size: 2em; font-weight: 700; border-bottom: 1px solid #eee; padding-bottom: 0.3em; }"
        "h2 { font-size: 1.5em; font-weight: 600; border-bottom: 1px solid #eee; padding-bottom: 0.3em; }"
        "h3 { font-size: 1.25em; font-weight: 600; }"
        "code { background: #f6f8fa; border: 1px solid #e1e4e8; border-radius: 3px; padding: 0.15em 0.4em; "
        "font-family: 'SFMono-Regular', Consolas, monospace; font-size: 0.875em; }"
        "pre { background: #f6f8fa; border: 1px solid #e1e4e8; border-radius: 6px; padding: 16px; overflow-x: auto; }"
        "pre code { background: none; border: none; padding: 0; }"
        "blockquote { border-left: 4px solid #dfe2e5; padding: 0.25em 1em; color: #6a737d; }"
        "table { border-collapse: collapse; width: 100%; }"
        "th, td { border: 1px solid #dfe2e5; padding: 8px 13px; }"
        "th { background: #f6f8fa; font-weight: 600; }"
        "img { max-width: 100%%; }"
        "hr { border: none; border-top: 2px solid #e1e4e8; margin: 2em 0; }"
        "%1"
        "</style>"
        "</head><body>%2</body></html>"
    ).arg(themeCss, htmlContent);
}

void ExportManager::exportToPdf(const QString& htmlContent, const QString& outputPath,
                                 const QString& themeCss)
{
#ifdef HAS_WEBENGINE
    auto* page = new QWebEnginePage(this);
    QString fullHtml = wrapHtmlWithTheme(htmlContent, themeCss);

    connect(page, &QWebEnginePage::loadFinished, this, [this, page, outputPath](bool ok) {
        if (!ok) {
            emit exportError(tr("Failed to load HTML for PDF export"));
            page->deleteLater();
            return;
        }

        page->printToPdf([this, page, outputPath](const QByteArray& data) {
            if (data.isEmpty()) {
                emit exportError(tr("PDF generation failed"));
            } else {
                QFile file(outputPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(data);
                    emit exportFinished(true, outputPath);
                } else {
                    emit exportError(tr("Cannot write to file: %1").arg(outputPath));
                }
            }
            page->deleteLater();
        }, m_pageLayout);
    });

    page->setHtml(fullHtml);
#else
    emit exportError(tr("PDF export requires WebEngine support"));
#endif
}

void ExportManager::exportToHtml(const QString& htmlContent, const QString& outputPath,
                                  const QString& themeCss, bool includeStyle)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportError(tr("Cannot write to file: %1").arg(outputPath));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    if (includeStyle) {
        out << wrapHtmlWithTheme(htmlContent, themeCss);
    } else {
        out << htmlContent;
    }

    emit exportFinished(true, outputPath);
}

void ExportManager::exportToPng(const QString& htmlContent, const QString& outputPath,
                                 const QString& themeCss, int width)
{
#ifdef HAS_WEBENGINE
    auto* page = new QWebEnginePage(this);
    QString fullHtml = wrapHtmlWithTheme(htmlContent, themeCss);

    connect(page, &QWebEnginePage::loadFinished, this, [this, page, outputPath, width](bool ok) {
        if (!ok) {
            emit exportError(tr("Failed to load HTML for PNG export"));
            page->deleteLater();
            return;
        }

        // Get page height via JavaScript, then capture
        page->runJavaScript("document.body.scrollHeight", [this, page, outputPath, width](const QVariant& result) {
            int height = result.toInt();
            if (height <= 0) height = 800;

            // Use a reasonable max height
            height = qMin(height, 10000);

            page->setContent(page->url().toEncoded(), "text/html");

            // Capture the page as image
            // Note: Full implementation would use QWebEnginePage::capturePage
            // For now, emit an error as this requires more complex async handling
            emit exportError(tr("PNG export is not fully implemented yet"));
            page->deleteLater();
        });
    });

    page->setHtml(fullHtml);
#else
    emit exportError(tr("PNG export requires WebEngine support"));
#endif
}
