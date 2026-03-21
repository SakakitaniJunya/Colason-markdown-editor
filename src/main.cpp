#include <QApplication>
#include <QIcon>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{

    QApplication app(argc, argv);
    app.setApplicationName("Colason");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Colason");
    app.setWindowIcon(QIcon(":/icons/colason.svg"));

    MainWindow window;
    window.show();

    return app.exec();
}
