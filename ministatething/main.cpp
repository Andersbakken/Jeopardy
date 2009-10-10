#include <QtGui>

class Object : public QObject
{
    Q_OBJECT
public:
    enum StateType {
        NoState = 0x000,
        Normal = 0x001,
        ShowQuestion = 0x002,
        ShowAnswer = 0x004, // is this right?
        TimeOut = 0x008,
        PickTeam = 0x010,
        TeamTimedOut = 0x020,
        PickRightOrWrong = 0x040,
        WrongAnswer = 0x080,
        RightAnswer = 0x100,
        Finished = 0x200
    };

    Object(QObject *parent = 0)
        : QObject(parent)
    {
        d.framesLeft = 0;
        d.currentFrame = 0;
        d.currentTeam = 0;
        struct {
            const StateType type;
            const char *name;
        } const states[] = {
            { Normal, "Normal" },
            { ShowQuestion, "ShowQuestion" },
            { ShowAnswer, "ShowAnswer" },
            { TimeOut, "TimeOut" },
            { PickTeam, "PickTeam" },
            { TeamTimedOut, "TeamTimedOut" },
            { PickRightOrWrong, "PickRightOrWrong" },
            { WrongAnswer, "WrongAnswer" },
            { RightAnswer, "RightAnswer" },
            { Finished, "Finished" },
            { NoState, 0 }
        };
        for (int i=0; states[i].type != NoState; ++i) {
            QState *state = new QState(&d.stateMachine);
            state->setProperty("type", states[i].type);
            connect(state, SIGNAL(entered()), this, SLOT(onStateEntered()));
            connect(state, SIGNAL(exited()), this, SLOT(onStateExited()));
            state->setObjectName(states[i].name);
            d.states[states[i].type] = state;
        }

        d.states.value(Normal)->addTransition(this, SIGNAL(nextState()),
                                              d.states.value(ShowQuestion));
        d.states.value(ShowQuestion)->addTransition(this, SIGNAL(nextStateTimeOut()),
                                                    d.states[TimeOut]);
        d.states.value(ShowQuestion)->addTransition(this, SIGNAL(nextState()),
                                                    d.states[PickTeam]);
        d.states.value(TimeOut)->addTransition(this, SIGNAL(nextState()),
                                               d.states.value(Normal));
        d.states.value(TimeOut)->addTransition(this, SIGNAL(nextStateFinished()),
                                               d.states.value(Finished));
        d.states.value(PickTeam)->addTransition(this, SIGNAL(nextStateTimeOut()),
                                                d.states.value(TeamTimedOut));
        d.states.value(PickTeam)->addTransition(this, SIGNAL(nextState()),
                                                d.states.value(PickRightOrWrong));
        d.states.value(TeamTimedOut)->addTransition(this, SIGNAL(nextState()),
                                                    d.states.value(ShowQuestion));
        d.states.value(PickRightOrWrong)->addTransition(this, SIGNAL(nextStateRight()),
                                                        d.states.value(RightAnswer));
        d.states.value(PickRightOrWrong)->addTransition(this, SIGNAL(nextStateWrong()),
                                                        d.states.value(WrongAnswer));
        d.states.value(RightAnswer)->addTransition(this, SIGNAL(nextState()),
                                                   d.states.value(Normal));
        d.states.value(RightAnswer)->addTransition(this, SIGNAL(nextStateFinished()),
                                                   d.states.value(Finished));
        d.states.value(WrongAnswer)->addTransition(this, SIGNAL(nextState()),
                                                   d.states.value(ShowQuestion));
    }

public slots:
    void newGame()
    {
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
        d.stateMachine.setInitialState(d.states.value(Normal));
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
        switch (state->property("type").toInt()) {
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
                qDebug() << "showing question" << d.currentFrame->question
                         << "worth" << d.currentFrame->value << "$";
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
        case WrongAnswer:
            Q_ASSERT(d.currentTeam);
            Q_ASSERT(d.currentFrame);
            d.currentTeam->score -= d.currentFrame->value / 2;
            qDebug() << d.currentTeam->name
                     << (state->property("type").toInt() == TeamTimedOut
                         ? "Didn't answer in time" : "Answered wrong")
                     << "They lost" << (d.currentFrame->value / 2) << "$. They now have"
                     << d.currentTeam->score << "$";
            d.currentTeam = 0;
            break;
        case RightAnswer:
            Q_ASSERT(d.currentTeam);
            Q_ASSERT(d.currentFrame);
            d.currentTeam->score += d.currentFrame->value;
            qDebug() << d.currentTeam->name << "answered correctly and earned" << d.currentFrame->value
                     << "$. They now have" << d.currentTeam->score << "$";
            finishQuestion();
            break;
        case Finished:
            qSort(d.teams.begin(), d.teams.end(), compareTeamsByScore);
            for (int i=0; i<d.teams.size(); ++i) {
                qDebug() << i << d.teams.at(i)->score << d.teams.at(i)->name;
            }
            d.stateMachine.stop();
            QTimer::singleShot(5000, this, SLOT(newGame()));
            break;
        }
        qDebug() << sender()->objectName() << "entered";
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
        QHash<StateType, QState*> states;
        QList<Team*> teams;
        QSet<Team*> teamsAttempted;
        Team *currentTeam;

        QList<Frame*> frames;
        int framesLeft;

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
