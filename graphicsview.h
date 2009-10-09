#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QtGui>
class GraphicsScene;
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QGraphicsScene *scene = 0, QWidget *parent = 0);
    void resizeEvent(QResizeEvent *);
    QSize sizeHint() const;
    void mouseDoubleClickEvent(QMouseEvent *e);
public slots:
    void newGame();
private:
    struct Data {
        GraphicsScene *scene;
    } d;
};

#endif
