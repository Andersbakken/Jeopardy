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
public:
    FrameItem(int value);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    QString question() const;
    void setQuestion(const QString &question);
private:
    struct Data {
        int value;
        QString question;
    } d;
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(QObject *parent = 0);
    bool load(QIODevice *device);
    bool load(const QString &file) { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f); }
    void reset();
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
