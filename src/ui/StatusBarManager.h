#pragma once

#include <QObject>

class QStatusBar;
class QLabel;

class StatusBarManager : public QObject
{
    Q_OBJECT

public:
    explicit StatusBarManager(QObject* parent = nullptr);

    void setupStatusBar(QStatusBar* statusBar);
    void updateWordCount(int words, int chars);
    void updateCursorPosition(int line, int col);
    void updateEncoding(const QString& encoding);
    void updateZoom(int percent);

private:
    static QString formatNumber(int n);

    QLabel* m_wordCountLabel = nullptr;
    QLabel* m_charCountLabel = nullptr;
    QLabel* m_cursorPosLabel = nullptr;
    QLabel* m_encodingLabel = nullptr;
    QLabel* m_docTypeLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
};
