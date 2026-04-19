#include <QApplication>
#include "NoddleMainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Noddle");

    NoddleMainWindow window;
    window.show();

    return app.exec();
}
