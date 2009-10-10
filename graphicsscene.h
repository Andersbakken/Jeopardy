#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QtGui>
#include "items.h"

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
    void click(Frame *frame);
    void setupStateMachine(Frame *frame);
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
public slots:
    bool load(const QString &file, const QStringList &teams = QStringList())
    { QFile f(file); return f.open(QIODevice::ReadOnly) && load(&f, teams); }
    void onClicked(Item *item);
    void clearActiveFrame();
    void onSceneRectChanged(const QRectF &rect);
    void onTransitionTriggered();
private:
    enum StateType {
        Normal = 0,
        ShowQuestion,
        TimeOut,
        PickTeam,
        TeamTimedOut,
        PickRightOrWrong,
        WrongAnswer,
        RightAnswer,
        Finished,
        NumStates
    };

    struct Data {
        QStateMachine stateMachine;
        QState *states[NumStates];
        QList<Team*> teams;
        QSet<Team*> teamsAttempted;
        Team *currentTeam;

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
    } d;
};

#endif
