#include "EditorBridge.h"

EditorBridge::EditorBridge(QObject* parent)
    : QObject(parent)
{
}

void EditorBridge::requestSetContent(const QString& html)
{
    emit setContentRequested(html);
}

void EditorBridge::requestSetMarkdown(const QString& markdown)
{
    emit setMarkdownRequested(markdown);
}

void EditorBridge::requestGetContent()
{
    emit getContentRequested();
}

void EditorBridge::requestExecuteCommand(const QString& command, const QString& argsJson)
{
    emit executeCommandRequested(command, argsJson);
}

void EditorBridge::requestToggleSourceMode()
{
    emit toggleSourceModeRequested();
}

void EditorBridge::receiveContent(const QString& html)
{
    emit contentReceived(html);
}

void EditorBridge::setDirty(bool dirty)
{
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit documentDirty(dirty);
    }
}