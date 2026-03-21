#pragma once

#include <QObject>
#include <QKeySequence>

class QMenuBar;
class QAction;
class QWidget;
class QPushButton;
class QHBoxLayout;
class MainWindow;

class MenuBarManager : public QObject
{
    Q_OBJECT

public:
    explicit MenuBarManager(MainWindow* mainWindow);

    void setupMenuBar(QMenuBar* menuBar);

    // Menu bar visibility control
    QWidget* createMenuWidget(QMenuBar* menuBar);
    void showMenuBar();
    void hideMenuBar();
    bool isPinned() const { return m_pinned; }

    // Window control button state
    void updateMaximizeIcon(bool isMaximized);

private:
    void setupFileMenu(QMenuBar* menuBar);
    void setupEditMenu(QMenuBar* menuBar);
    void setupParagraphMenu(QMenuBar* menuBar);
    void setupFormatMenu(QMenuBar* menuBar);
    void setupViewMenu(QMenuBar* menuBar);
    void setupHelpMenu(QMenuBar* menuBar);

    QAction* createAction(const QString& text, const QKeySequence& shortcut = {});

    void togglePin();
    void updateVisibility();
    void savePinState();
    void restorePinState();
    void connectMenuSignals(QMenuBar* menuBar);

    MainWindow* m_mainWindow;
    QMenuBar* m_menuBar = nullptr;
    QPushButton* m_hamburgerButton = nullptr;
    QPushButton* m_pinButton = nullptr;
    QPushButton* m_minimizeButton = nullptr;
    QPushButton* m_maximizeButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_pinned = false;
};
