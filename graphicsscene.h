#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>

class GraphicsScene;
class Item : public QGraphicsWidget
{
    Q_OBJECT
public:
    Item();
    enum { Type = QGraphicsItem::UserType + 1 };
    virtual int type() const { return Type; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    qreal yRotation() const;
    void setYRotation(qreal yy);
    void setText(const QString &text);
    QString text() const;
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
    void setColor(const QColor &color);
    QColor color() const;
    GraphicsScene *graphicsScene() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void draw(QPainter *, const QRect &) {}
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

signals:
    void clicked(Item *item, const QPointF &scenePos);
private:
    struct Data {
        QString text;
        qreal yRotation;
        bool hovered;
        QColor backgroundColor, color;
    } d;
};

class Frame : public Item
{
    Q_OBJECT
public:
    Frame(int row, int column);
    enum { Type = QGraphicsItem::UserType + 2 };
    virtual int type() const { return Type; }

    int row() const { return d.row; }
    int column() const { return d.column; }

    void setValue(int value) { d.value = value; d.valueString = QString("%1$").arg(d.value); }
    int value() const { return d.value; }
    QString valueString() const { return d.valueString; }

    qreal answerProgress() const { return d.answerProgress; }
    void setAnswerProgress(qreal answerProgress) { d.answerProgress = answerProgress; update(); }
    QColor progressBarColor() const { return d.progressBarColor; }
    void setProgressBarColor(const QColor &color) { d.progressBarColor = color; update(); }

    QString question() const { return d.question; }
    void setQuestion(const QString &question) { d.question = question; }
    QString answer() const { return d.answer; }
    void setAnswer(const QString &answer) { d.answer = answer; }

    virtual void draw(QPainter *, const QRect &);
private:
    struct Data {
        QString question, answer, valueString;
        int value;
        qreal answerProgress;
        int row, column;
        QColor progressBarColor;
    } d;
};

class Team : public Item
{
    Q_OBJECT
    Q_PROPERTY(int points READ points WRITE setPoints)
public:
    enum { Type = QGraphicsItem::UserType + 3 };
    virtual int type() const { return Type; }
    Team(const QString &name) : Item() { d.points = 0; setObjectName(name); updatePoints(); }

    int points() const { return d.points; }
    void setPoints(int points) { d.points = points; updatePoints(); }
    void addPoints(int points) { d.points += points; updatePoints(); }
private:
    void updatePoints() { setText(QString("%1 - %2$").arg(objectName()).arg(points())); }
    struct Data {
        int points;
    } d;
};

class Proxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(qreal answerProgress READ answerProgress WRITE setAnswerProgress)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QColor progressBarColor READ progressBarColor WRITE setProgressBarColor)
    Q_PROPERTY(bool activeFrame READ hasActiveFrame WRITE setActiveFrame)
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

    QColor color() const { return d.activeFrame ? d.activeFrame->color() : QColor(); }
    void setColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setColor(tt); }

    qreal answerProgress() const { return d.activeFrame ? d.activeFrame->answerProgress() : qreal(0.0); }
    void setAnswerProgress(qreal yy) { if (d.activeFrame) d.activeFrame->setAnswerProgress(yy); }

    QRectF geometry() const { return d.activeFrame ? d.activeFrame->geometry() : QRectF(); }
    void setGeometry(const QRectF &tt) { if (d.activeFrame) d.activeFrame->setGeometry(tt); }

    Frame *activeFrame() const { return d.activeFrame; }
    void setActiveFrame(Frame *frame)
    {
        if (d.activeFrame) {
            d.activeFrame->setZValue(0);
        }
        d.activeFrame = frame;
        if (d.activeFrame) {
            d.activeFrame->setZValue(10);
        }
    }

    bool hasActiveFrame() const { return d.activeFrame; }
    void setActiveFrame(bool on) { Q_ASSERT(!on); Q_UNUSED(on); setActiveFrame(static_cast<Frame*>(0)); }
private:
    struct Data {
        Frame *activeFrame;
    } d;
};

class TeamProxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    TeamProxy(QObject *parent = 0) : QObject(parent) {}
    void setTeams(const QList<Team*> &teams) { d.teams = teams; }
    qreal opacity() const { return d.teams.isEmpty() ? 0.0 : d.teams.at(0)->opacity(); }
    void setOpacity(qreal opacity) { foreach(Item *team, d.teams) team->setOpacity(opacity); }
private:
    struct Data {
        QList<Team*> teams;
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
    void reset();
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
    QRectF frameGeometry(Frame *frame) const;
    void click(Frame *frame);
    void setupStateMachine(Frame *frame);
    int answerTime() const;
signals:
    void normalState();
    void spaceBarPressed();
    void correctAnswer();
    void wrongAnswer();
    void showQuestion();
    void showAnswer();
    void teamPicked();
    void mouseButtonPressed(const QPointF &, Qt::MouseButton);
public slots:
    bool load(const QString &file) { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f); }
    void onClicked(Item *item);
    void clearActiveFrame();
    void onSceneRectChanged(const QRectF &rect);
    void onTransitionTriggered();
private:
    struct Data {
        QStateMachine stateMachine;
        QState *normalState, *showQuestionState, *showAnswerState, *pickTeamState,
            *answerState, *correctAnswerState, *wrongAnswerState;

        QList<Item*> topics;
        QList<Frame*> frames;
        QList<Team*> teams;
        QRectF teamsGeometry, framesGeometry;
        bool sceneRectChangedBlocked;
        Proxy proxy;
        TeamProxy teamProxy;
        int answerTime;
        Team *currentTeam;
    } d;
};

#endif
