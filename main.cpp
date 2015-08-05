#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLocale::setDefault(QLocale::C);
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QCoreApplication::setOrganizationName("Natnux");
    QCoreApplication::setApplicationVersion("0.1");
    QCoreApplication::setApplicationName("TempestAlarm");

    MainWindow main_window;
    main_window.setWindowTitle(QString(QCoreApplication::applicationName()+" (v "+QCoreApplication::applicationVersion()+")"));
    main_window.show();

    return a.exec();
}
