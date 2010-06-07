#include <QtGui>
#include "view.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setOrganizationName(QLatin1String("AndersSoft"));
    a.setOrganizationDomain(QLatin1String("www.anderssoft.com"));
    a.setApplicationName(QLatin1String("Jeopardy"));

    MainWindow w;
    w.show();
    if (argc > 1)
        w.load(QString::fromLatin1(argv[1]));
    return a.exec();
}
