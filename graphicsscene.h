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

typedef QHash<StateType, QAbstractTransition*> TransitionHash;
Q_DECLARE_METATYPE(TransitionHash);
class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(QObject *parent = 0);
    bool load(QIODevice *device, const QStringList &teams);
    void reset();
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
    QRectF frameGeometry(Frame *frame) const;
    int answerTime() const;
    void setTeamGeometry(const QRectF &rect);
signals:
    void normalState();
    void spaceBarPressed();
    void rightAnswer();
    void wrongAnswer();
    void showQuestion();
    void showAnswer();
    void teamPicked();
    void mouseButtonPressed(const QPointF &, Qt::MouseButton);

    void nextState();
    void nextStateWrong();
    void nextStateRight();
    void nextStateTimeOut();
    void nextStateFinished();
public slots:
    bool load(const QString &file, const QStringList &teams = QStringList())
    { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f, teams); }
    void onClicked(Item *item);
    void clearActiveFrame();
    void onSceneRectChanged(const QRectF &rect);
    void onTransitionTriggered();
    void onStateEntered();
    void onStateExited();
private:
    void finishQuestion();
    QAbstractTransition *transition(StateType from, StateType to) const;
    QAbstractTransition *addTransition(StateType from, const char *sig, StateType to);

    struct Data {
        QStateMachine stateMachine;
        QState *states[NumStates];
        QState *currentState;
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
        StatusBar *statusBar;
    } d;
};

#endif
