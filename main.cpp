#include <QtGui>
#include "view.h"

int main(int argc, char **argv)
{
    srand(QDateTime::currentDateTime().toMSecsSinceEpoch());

    QApplication a(argc, argv);
    a.setOrganizationName(QLatin1String("AndersSoft"));
    a.setOrganizationDomain(QLatin1String("www.anderssoft.com"));
    a.setApplicationName(QLatin1String("Jeopardy"));

    MainWindow w;
    QString file;
    QStringList players;
    for (int i=1; i<argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg.startsWith("--players=")) {
            players = arg.mid(10).split(',');
        } else if (QFile::exists(arg)) {
            file = arg;
        }
    }

    if (!file.isEmpty())
        QMetaObject::invokeMethod(&w, "load", Qt::QueuedConnection,
                                  Q_ARG(QString, file), Q_ARG(QStringList, players));
    w.show();
    return a.exec();
}
