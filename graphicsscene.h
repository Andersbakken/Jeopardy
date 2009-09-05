#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>

class TopicItem : public QGraphicsWidget
{
public:
    TopicItem(const QString &string);
    enum { Type = QGraphicsItem::UserType + 1 };
    virtual int type() const { return Type; }
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
    Q_PROPERTY(QString text READ text WRITE setText)
public:
    FrameItem(int row, int col);
    enum { Type = QGraphicsItem::UserType + 2 };
    virtual int type() const { return Type; }
    void setValue(int value);
    int value() const;
    QString valueString() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    QString question() const;
    void setQuestion(const QString &question);
    QString answer() const;
    void setAnswer(const QString &answer);
//     void raise();
//     void lower();
    qreal yRotation() const;
    void setYRotation(qreal yy);
    void timerEvent(QTimerEvent *e);
    void setText(const QString &text);
    QString text() const;
// public slots:
//     void onLowered();
//     void onQuestionShown();
// signals:
//     void raised();
//     void lowered();
private:
    struct Data {
        QString text, question, answer, valueString;
        int value;
        QBasicTimer answerTimer;
        QTime timer;
        qreal yRotation;
        int row, column;
    } d;
    friend class GraphicsScene;
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
    QRectF itemGeometry(FrameItem *item) const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void click(FrameItem *frame);
public slots:
    void onSceneRectChanged(const QRectF &rect);
    void onFrameRaised();
    void onFrameLowered();
private:
    struct Data {
        QStateMachine stateMachine;
        QState *normalState, *raisedState, *showQuestionState, *showAnswerState, *correctAnswerState, *wrongAnswerState;
        FrameItem *raised;
        QList<TopicItem*> topicItems;
        QList<QList<FrameItem*> > frameItems;
        bool sceneRectChangedBlocked;
    } d;
};

#endif
