#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>
#include "items.h"

enum StateType {
    Normal = 0,
    ShowQuestion,
    TimeOut,
    PickTeam,
    TeamTimedOut,
    PickRightOrWrong,
    WrongAnswer,
    RightAnswer,
    NoAnswers,
    Finished,
    NumStates
};

class Transition;
class State : public QState
{
    Q_OBJECT
    Q_PROPERTY(StateType type READ type WRITE setType)
public:
    State(StateType type, QState *parent) : QState(parent) { d.type = type; }
    StateType type() const { return d.type; }
    void setType(StateType type) { d.type = type; }
    Transition *transition(StateType type) const { return d.transitions.value(type); }
    void addTransition(StateType type, Transition *transition);
private:
    struct Data {
        QHash<StateType, Transition *> transitions;
        StateType type;
    } d;
};

class Transition : public QSignalTransition
{
    Q_OBJECT
public:
    Transition(QObject *sender, State *target)
        : QSignalTransition(sender, SIGNAL(next(int)))
    {
        setTargetState(target);
    }
    bool eventTest(QEvent *event)
    {
        if (QSignalTransition::eventTest(event)) {
            QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(event);
            Q_ASSERT(qobject_cast<State*>(targetState()));
            return (se->arguments().value(0).toInt() == qobject_cast<State*>(targetState())->type());
        }
        return false;
    }
};

typedef QHash<StateType, Transition*> TransitionHash;
Q_DECLARE_METATYPE(TransitionHash);
class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(QObject *parent = 0);
    bool load(QIODevice *device, const QStringList &teams);
    void reset();
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
    QRectF frameGeometry(Frame *frame) const;
    int answerTime() const;
    void setTeamGeometry(const QRectF &rect, Qt::Orientation orientation);
signals:
    void next(int type);
    void mouseButtonPressed(const QPointF &, Qt::MouseButton);
public slots:
    void finishQuestion();
    bool load(const QString &file, const QStringList &teams = QStringList())
    { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f, teams); }
    void onClicked(Item *item);
    void clearActiveFrame();
    void onSceneRectChanged(const QRectF &rect);
    void onTransitionTriggered();
    void onStateEntered();
    void onStateExited();
    void nextStateTimeOut() { emit next(TimeOut); }
private:
    Transition *transition(StateType from, StateType to) const;
    Transition *addTransition(StateType from, StateType to);

    struct Data {
        QStateMachine stateMachine;
        State *states[NumStates];
        State *currentState;
        QList<Team*> teams;
        Team *cancelTeam;
        QSet<Team*> teamsAttempted;

        QList<Frame*> frames;
        int framesLeft;
        int right, wrong, timedout;

        Frame *currentFrame;

        QList<Item*> topics;
        QRectF teamsGeometry, framesGeometry;
        bool sceneRectChangedBlocked;
        Proxy proxy;
        TeamProxy *teamProxy;
        int answerTime;
        Item *rightAnswerItem, *wrongAnswerItem;
        QTimer timeoutTimer;
        QTime timeoutTimerStarted;
        int elapsed;
    } d;
};

#endif
