#include "StatusBarManager.h"

#include <QStatusBar>
#include <QLabel>
#include <QLocale>

StatusBarManager::StatusBarManager(QObject* parent)
    : QObject(parent)
{
}

void StatusBarManager::setupStatusBar(QStatusBar* statusBar)
{
    // Styling handled by ThemeManager QSS
    m_wordCountLabel = new QLabel(tr("Words: 0"));
    m_charCountLabel = new QLabel(tr("Chars: 0"));
    m_cursorPosLabel = new QLabel(tr("Ln 1, Col 1"));
    m_encodingLabel = new QLabel(tr("UTF-8"));
    m_docTypeLabel = new QLabel(tr("Markdown"));
    m_zoomLabel = new QLabel(tr("100%"));

    statusBar->addPermanentWidget(m_wordCountLabel);
    statusBar->addPermanentWidget(m_charCountLabel);
    statusBar->addPermanentWidget(m_cursorPosLabel);
    statusBar->addPermanentWidget(m_encodingLabel);
    statusBar->addPermanentWidget(m_docTypeLabel);
    statusBar->addPermanentWidget(m_zoomLabel);
}

void StatusBarManager::updateWordCount(int words, int chars)
{
    if (m_wordCountLabel) {
        m_wordCountLabel->setText(tr("Words: %1").arg(formatNumber(words)));
    }
    if (m_charCountLabel) {
        m_charCountLabel->setText(tr("Chars: %1").arg(formatNumber(chars)));
    }
}

void StatusBarManager::updateCursorPosition(int line, int col)
{
    if (m_cursorPosLabel) {
        m_cursorPosLabel->setText(tr("Ln %1, Col %2").arg(line).arg(col));
    }
}

void StatusBarManager::updateEncoding(const QString& encoding)
{
    if (m_encodingLabel) {
        m_encodingLabel->setText(encoding);
    }
}

void StatusBarManager::updateZoom(int percent)
{
    if (m_zoomLabel) {
        m_zoomLabel->setText(tr("%1%").arg(percent));
    }
}

QString StatusBarManager::formatNumber(int n)
{
    return QLocale(QLocale::English).toString(n);
}
