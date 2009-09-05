#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>

class TopicItem : public QGraphicsWidget
{
public:
    TopicItem(const QString &string);
    void setText(const QString &text);
    QString text() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
private:
    struct Data {
        QString text;
    } d;
};

class FrameItem : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
public:
    FrameItem(int value);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    QString question() const;
    void setQuestion(const QString &question);
    void raise();
    void lower();
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    qreal yRotation() const;
    void setYRotation(qreal yy);
public slots:
    void onLowered();
private:
    struct Data {
        int value;
        qreal yRotation;
        QString question;
        QParallelAnimationGroup *animationGroup;
        QPropertyAnimation *geometryAnimation, *rotationAnimation;
    } d;
};

class SelectorItem : public QGraphicsWidget
{
public:
    SelectorItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(QObject *parent = 0);
    bool load(QIODevice *device);
    bool load(const QString &file) { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f); }
    void reset();
    void keyPressEvent(QKeyEvent *e);
public slots:
    void onSceneRectChanged(const QRectF &rect);
private:
    struct Data {
        QList<TopicItem*> topicItems;
        QList<QList<FrameItem*> > frameItems;
        QGraphicsGridLayout *layout;
    } d;
};

#endif
