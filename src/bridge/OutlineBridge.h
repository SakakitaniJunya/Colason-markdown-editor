#pragma once

#include <QObject>
#include <QString>

class OutlineBridge : public QObject
{
    Q_OBJECT

public:
    explicit OutlineBridge(QObject* parent = nullptr);

    Q_INVOKABLE void requestScrollToHeading(const QString& headingId);

signals:
    // C++ -> JS
    void scrollToHeadingRequested(const QString& headingId);

    // JS -> C++
    void headingsChanged(const QString& json);
    void activeHeadingChanged(const QString& headingId);
};