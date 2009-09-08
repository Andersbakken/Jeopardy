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
    QColor progressBarColor() const;
    void setProgressBarColor(const QColor &color);
    void setText(const QString &text);
    QString text() const;
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
    void setTextColor(const QColor &color);
    QColor textColor() const;
    int row() const;
    int column() const;
private:
    GraphicsScene *graphicsScene() const;
    struct Data {
        QString text, question, answer, valueString;
        int value;
        qreal yRotation, answerProgress;
        int row, column;
        QColor backgroundColor, textColor, progressBarColor;
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
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor progressBarColor READ progressBarColor WRITE setProgressBarColor)
public:
    Proxy(QObject *parent = 0) : QObject(parent) { d.activeFrame = 0; }

    qreal yRotation() const { return d.activeFrame ? d.activeFrame->yRotation() : qreal(0.0); }
    void setYRotation(qreal yy) { if (d.activeFrame) d.activeFrame->setYRotation(yy); }

    QString text() const { return d.activeFrame ? d.activeFrame->text() : QString(); }
    void setText(const QString &tt) { if (d.activeFrame) d.activeFrame->setText(tt); }

    QColor progressBarColor() const { return d.activeFrame ? d.activeFrame->progressBarColor() : QColor(); }
    void setProgressBarColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setProgressBarColor(tt); }

    QColor backgroundColor() const { return d.activeFrame ? d.activeFrame->backgroundColor() : QColor(); }
    void setBackgroundColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setBackgroundColor(tt); }

    QColor textColor() const { return d.activeFrame ? d.activeFrame->textColor() : QColor(); }
    void setTextColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setTextColor(tt); }

    qreal answerProgress() const { return d.activeFrame ? d.activeFrame->answerProgress() : qreal(0.0); }
    void setAnswerProgress(qreal yy) { if (d.activeFrame) d.activeFrame->setAnswerProgress(yy); }

    QRectF geometry() const { return d.activeFrame ? d.activeFrame->geometry() : QRectF(); }
    void setGeometry(const QRectF &tt) { if (d.activeFrame) d.activeFrame->setGeometry(tt); }

    FrameItem *activeFrame() const { return d.activeFrame; }
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
    void onCorrectAnswer();
    void onSceneRectChanged(const QRectF &rect);
private:
    struct Data {
        QStateMachine stateMachine;
        QState *normalState, *showQuestionState, *showAnswerState, *correctAnswerState, *wrongAnswerState;
        QList<QList<FrameItem*> > frames;
        bool sceneRectChangedBlocked;
        Proxy proxy;
        int answerTime;
    } d;
};

#endif
