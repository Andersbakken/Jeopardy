#include <QtGui>
#include "view.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    GraphicsView w;
    w.show();
    return a.exec();
}
