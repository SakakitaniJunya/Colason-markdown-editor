#pragma once

#include <QObject>
#include <QString>

class EditorBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dirty READ isDirty NOTIFY documentDirty)

public:
    explicit EditorBridge(QObject* parent = nullptr);

    bool isDirty() const { return m_dirty; }

    // Called from C++ to request JS actions
    Q_INVOKABLE void requestSetContent(const QString& html);
    Q_INVOKABLE void requestSetMarkdown(const QString& markdown);
    Q_INVOKABLE void requestGetContent();
    Q_INVOKABLE void requestExecuteCommand(const QString& command, const QString& argsJson = {});
    Q_INVOKABLE void requestToggleSourceMode();

    // Called from JS via QWebChannel to receive content
    Q_INVOKABLE void receiveContent(const QString& html);

signals:
    // Signals to JS (C++ -> JS)
    void setContentRequested(const QString& html);
    void setMarkdownRequested(const QString& markdown);
    void executeCommandRequested(const QString& command, const QString& argsJson);
    void toggleSourceModeRequested();
    void getContentRequested();

    // Signals from JS (JS -> C++)
    void contentChanged(const QString& html);
    void wordCountChanged(int words, int chars);
    void documentDirty(bool dirty);
    void cursorPositionChanged(int line, int col);
    void headingsChanged(const QString& json);
    void contentReceived(const QString& html);

public slots:
    void setDirty(bool dirty);

private:
    bool m_dirty = false;
};