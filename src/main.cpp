#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{

    QApplication app(argc, argv);
    app.setApplicationName("Colason");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Colason");
    app.setWindowIcon(QIcon(":/icons/colason-app.svg"));

    MainWindow window;

    // Open a file given on the command line or via the OS ("open with"):
    //   colason path/to/file.md
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        if (QFile::exists(path)) {
            window.setInitialFile(QFileInfo(path).absoluteFilePath());
        }
    }

    window.show();

    return app.exec();
}
