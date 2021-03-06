#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QtGui>
class GraphicsView;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    void showEvent(QShowEvent *e);
    void closeEvent(QCloseEvent *e);
public slots:
    void load(const QString &file, const QStringList &players);
private:
    struct Data {
        GraphicsView *view;
    } d;
};
class GraphicsScene;
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QWidget *parent = 0);
    void resizeEvent(QResizeEvent *);
    QSize sizeHint() const;
    void load(const QString &file, const QStringList &players = QStringList());
public slots:
    void newGame();
    void createGame();
private:
    struct Data {
        GraphicsScene *scene;
    } d;
};

#endif
