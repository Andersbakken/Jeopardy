#include <QtGui>
#include "view.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setOrganizationName(QLatin1String("AndersSoft"));
    a.setOrganizationDomain(QLatin1String("www.anderssoft.com"));
    a.setApplicationName(QLatin1String("Jeopardy"));

    MainWindow w;
    if (argc > 1)
        QMetaObject::invokeMethod(&w, "load", Qt::QueuedConnection, Q_ARG(QString, QString::fromLocal8Bit(argv[1])));
    w.show();
    return a.exec();
}
