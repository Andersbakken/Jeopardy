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
    qreal yRotation() const;
    void setYRotation(qreal yy);
    void setText(const QString &text);
    QString text() const;
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

class Proxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
public:
    Proxy(QObject *parent = 0)
        : QObject(parent)
    {
        d.activeFrame = 0;
    }

    qreal yRotation() const
    {
        return d.activeFrame ? d.activeFrame->yRotation() : qreal(0.0);
    }

    void setYRotation(qreal yy)
    {
        if (d.activeFrame)
            d.activeFrame->setYRotation(yy);
    }

    QString text() const
    {
        return d.activeFrame ? d.activeFrame->text() : QString();
    }

    void setText(const QString &tt)
    {
        if (d.activeFrame)
            d.activeFrame->setText(tt);
    }

    QRectF geometry() const
    {
        return d.activeFrame ? d.activeFrame->geometry() : QRectF();
    }

    void setGeometry(const QRectF &tt)
    {
        if (d.activeFrame)
            d.activeFrame->setGeometry(tt);
    }

    void setActiveFrame(FrameItem *frame)
    {
        if (d.activeFrame) {
            d.activeFrame->setZValue(0);
        }
        d.activeFrame = frame;
        if (d.activeFrame) {
            d.activeFrame->setZValue(10);
        }
    }
    FrameItem *activeFrame() const
    {
        return d.activeFrame;
    }
private:
    struct Data {
        FrameItem *activeFrame;
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
    QRectF itemGeometry(FrameItem *item) const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void click(FrameItem *frame);
    void setupStateMachine(FrameItem *frame);
signals:
    void correctAnswer();
    void wrongAnswer();
    void showQuestion();
    void showAnswer();
public slots:
    void onSceneRectChanged(const QRectF &rect);
    void onFrameRaised();
    void onFrameLowered();
    void onStateEntered();
private:
    struct Data {
        QStateMachine stateMachine;
        QState *normalState, *showQuestionState, *showAnswerState, *correctAnswerState, *wrongAnswerState;
        FrameItem *raised;
        QList<TopicItem*> topicItems;
        QList<QList<FrameItem*> > frameItems;
        bool sceneRectChangedBlocked;
        FrameItem *activeFrame;
        Proxy proxy;
    } d;
};

#endif
