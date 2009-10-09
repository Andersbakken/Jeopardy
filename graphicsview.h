#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWidget>
class GraphicsScene;
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QGraphicsScene *scene = 0, QWidget *parent = 0);
    void resizeEvent(QResizeEvent *);
    QSize sizeHint() const;
public slots:
    void newGame();
private:
    struct Data {
        GraphicsScene *scene;
    } d;
};

#endif
