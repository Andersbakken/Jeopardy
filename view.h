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
    void load(const QString &fileName);
public slots:
    void newGame();
    void createGame();
private:
    struct Data {
        GraphicsScene *scene;
    } d;
};

#endif
