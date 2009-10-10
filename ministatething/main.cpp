#include <QtGui>

class Object : public QObject
{
    Q_OBJECT
public:
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

    Object(QObject *parent = 0)
        : QObject(parent)
    {
        d.right = d.wrong = d.timedout = 0;
        d.framesLeft = 0;
        d.currentFrame = 0;
        d.currentTeam = 0;

        const char *states[] = {
            "Normal", "ShowQuestion", "TimeOut", "PickTeam",
            "TeamTimedOut", "PickRightOrWrong", "WrongAnswer",
            "RightAnswer", "Finished", 0
        };
        for (int i=0; states[i]; ++i) {
            QState *state = new QState(&d.stateMachine);
            state->setProperty("type", i);
            connect(state, SIGNAL(entered()), this, SLOT(onStateEntered()));
            connect(state, SIGNAL(exited()), this, SLOT(onStateExited()));
            state->setObjectName(states[i]);
            d.states[i] = state;
        }

        d.states[Normal]->addTransition(this, SIGNAL(nextState()),
                                        d.states[ShowQuestion]);
        d.states[ShowQuestion]->addTransition(this, SIGNAL(nextStateTimeOut()),
                                              d.states[TimeOut]);
        d.states[ShowQuestion]->addTransition(this, SIGNAL(nextState()),
                                              d.states[PickTeam]);
        d.states[TimeOut]->addTransition(this, SIGNAL(nextState()),
                                         d.states[Normal]);
        d.states[TimeOut]->addTransition(this, SIGNAL(nextStateFinished()),
                                         d.states[Finished]);
        d.states[PickTeam]->addTransition(this, SIGNAL(nextStateTimeOut()),
                                          d.states[TeamTimedOut]);
        d.states[PickTeam]->addTransition(this, SIGNAL(nextState()),
                                          d.states[PickRightOrWrong]);
        d.states[TeamTimedOut]->addTransition(this, SIGNAL(nextState()),
                                              d.states[ShowQuestion]);
        d.states[PickRightOrWrong]->addTransition(this, SIGNAL(nextStateRight()),
                                                  d.states[RightAnswer]);
        d.states[PickRightOrWrong]->addTransition(this, SIGNAL(nextStateWrong()),
                                                  d.states[WrongAnswer]);
        d.states[RightAnswer]->addTransition(this, SIGNAL(nextState()),
                                             d.states[Normal]);
        d.states[RightAnswer]->addTransition(this, SIGNAL(nextStateFinished()),
                                             d.states[Finished]);
        d.states[WrongAnswer]->addTransition(this, SIGNAL(nextState()),
                                             d.states[ShowQuestion]);
    }

public slots:
    void newGame()
    {
        d.right = d.wrong = d.timedout = 0;
        qDeleteAll(d.teams);
        d.teams.clear();
        qDeleteAll(d.frames);
        d.frames.clear();
        d.currentTeam = 0;
        d.currentFrame = 0;
        d.teamsAttempted.clear();
        for (int i=0; i<3; ++i) {
            Team *team = new Team;
            team->name = QString("Team %1").arg(i + 1);
            team->score = 0;
            d.teams.append(team);
        }

        for (int i=0; i<20; ++i) {
            Frame *frame = new Frame;
            frame->value = ((i / 5) + 1) * 100;
            frame->question = QString("Question %1/%2 %3$").arg(i / 5).arg(i % 5).arg(frame->value);
            frame->status = Frame::Hidden;
            d.frames.append(frame);
        }
        d.framesLeft = d.frames.size();
        d.stateMachine.setInitialState(d.states[Normal]);
        d.stateMachine.start();
        QTimer::singleShot(0, this, SLOT(next()));
    }
    void next()
    {
        switch (rand() % 4) {
        case 0: emit nextState(); break;
        case 1: emit nextStateWrong(); break;
        case 2: emit nextStateRight(); break;
        case 3: emit nextStateTimeOut(); break;
        }
        QTimer::singleShot(0, this, SLOT(next()));
    }
    void onStateEntered()
    {
        QState *state = qobject_cast<QState*>(sender());
        const int type = state->property("type").toInt();
        switch (type) {
        case Normal:
            Q_ASSERT(d.teamsAttempted.isEmpty());
            Q_ASSERT(!d.currentTeam);
            Q_ASSERT(!d.currentFrame);
            break;
        case ShowQuestion: {
            if (d.teamsAttempted.size() == d.teams.size()) {
                emit nextStateTimeOut();
            } else {
                Q_ASSERT(d.currentFrame);
//                 qDebug() << "showing question" << d.currentFrame->question
//                          << "worth" << d.currentFrame->value << "$";
            }
            break; }
        case TimeOut:
            Q_ASSERT(d.currentFrame);
            d.currentFrame->status = Frame::Failed;
            finishQuestion();
            break;
        case PickTeam:
            Q_ASSERT(!d.currentTeam);
            Q_ASSERT(d.teamsAttempted.size() < d.teams.size());
            do {
                d.currentTeam = d.teams.at(rand() % d.teams.size());
            } while (d.teamsAttempted.contains(d.currentTeam));
            d.teamsAttempted.insert(d.currentTeam);
            break;
        case TeamTimedOut:
            ++d.timedout;
        case WrongAnswer:
            if (type == WrongAnswer)
                ++d.wrong;
            Q_ASSERT(d.currentTeam);
            Q_ASSERT(d.currentFrame);
            d.currentTeam->score -= d.currentFrame->value / 2;
//             qDebug() << d.currentTeam->name
//                      << (state->property("type").toInt() == TeamTimedOut
//                          ? "Didn't answer in time" : "Answered wrong")
//                      << "They lost" << (d.currentFrame->value / 2) << "$. They now have"
//                      << d.currentTeam->score << "$";
            d.currentTeam = 0;
            break;
        case RightAnswer:
            ++d.right;
            Q_ASSERT(d.currentTeam);
            Q_ASSERT(d.currentFrame);
            d.currentTeam->score += d.currentFrame->value;
//             qDebug() << d.currentTeam->name << "answered correctly and earned" << d.currentFrame->value
//                      << "$. They now have" << d.currentTeam->score << "$";
            finishQuestion();
            break;
        case Finished:
            qSort(d.teams.end(), d.teams.begin(), compareTeamsByScore);
            for (int i=0; i<d.teams.size(); ++i) {
                qDebug() << i << d.teams.at(i)->score << d.teams.at(i)->name;
            }
            qDebug() << "right" << d.right << "wrong" << d.wrong << "timedout" << d.timedout;
            d.stateMachine.stop();
            QTimer::singleShot(500, this, SLOT(newGame()));
            break;
        }
//        qDebug() << sender()->objectName() << "entered";
    }
    void onStateExited()
    {
        QState *state = qobject_cast<QState*>(sender());
        switch (state->property("type").toInt()) {
        case Normal: {
            int idx = rand() % d.frames.size();
            while (d.frames.at(idx)->status != Frame::Hidden) {
                if (++idx == d.frames.size())
                    idx = 0;
            }
            Q_ASSERT(!d.currentFrame);
            d.currentFrame = d.frames.at(idx);
            Q_ASSERT(d.currentFrame->status == Frame::Hidden);
            --d.framesLeft;
            break; }
        default:
            break;
        }
//        qDebug() << sender()->objectName() << "exited";
    }

signals:
    void nextState();
    void nextStateWrong();
    void nextStateRight();
    void nextStateTimeOut();
    void nextStateFinished();
private:
    void finishQuestion()
    {
        d.currentFrame = 0;
        d.teamsAttempted.clear();
        d.currentTeam = 0;
        if (d.framesLeft == 0)
            emit nextStateFinished();
    }
    struct Team {
        QString name;
        int score;
    };
    static inline bool compareTeamsByScore(const Team *left, const Team *right)
    {
        return left->score < right->score;
    }
    struct Frame {
        QString question;
        int value;
        enum Status {
            Hidden,
            Failed,
            Succeeded
        } status;
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
    } d;
};

#include "main.moc"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    Object w;
    w.newGame();
    return a.exec();
}
