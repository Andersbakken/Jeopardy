#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>

class GraphicsScene;
class FrameItem : public QGraphicsWidget
{
    Q_OBJECT
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
    qreal answerProgress() const;
    void setAnswerProgress(qreal answerProgress);
    void setText(const QString &text);
    QString text() const;
    virtual void resizeEvent(QGraphicsSceneResizeEvent *event);
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
private:
    void updateProgressBarGeometry();
    GraphicsScene *graphicsScene() const;
    struct Data {
        QString text, question, answer, valueString;
        int value;
        qreal yRotation, answerProgress;
        int row, column;
        QGraphicsProxyWidget *answerProgressBarProxy;
        QProgressBar *answerProgressBar;
        QColor backgroundColor;
    } d;
    friend class GraphicsScene;
};

class Proxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(qreal answerProgress READ answerProgress WRITE setAnswerProgress)
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

    qreal answerProgress() const
    {
        return d.activeFrame ? d.activeFrame->answerProgress() : qreal(0.0);
    }

    void setAnswerProgress(qreal yy)
    {
        if (d.activeFrame)
            d.activeFrame->setAnswerProgress(yy);
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
    int answerTime() const;
signals:
    void correctAnswer();
    void wrongAnswer();
    void showQuestion();
    void showAnswer();
public slots:
    void onSceneRectChanged(const QRectF &rect);
private:
    struct Data {
        QStateMachine stateMachine;
        QState *normalState, *showQuestionState, *showAnswerState, *correctAnswerState, *wrongAnswerState;
        QList<QList<FrameItem*> > frames;
        bool sceneRectChangedBlocked;
        FrameItem *activeFrame;
        Proxy proxy;
        int answerTime;
    } d;
};

#endif
