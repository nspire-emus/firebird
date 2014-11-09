#include <QApplication>

#include "mainwindow.h"

extern "C"
{
    char target_folder[256];
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow mw;

    mw.show();

    return app.exec();
}
