#include "graphicsscene.h"

static inline QRectF itemGeometry(int row, int column, int rows, int columns, const QRectF &sceneRect)
{
    if (qMin(rows, columns) <= 0)
        return QRectF();
    QRectF r(0, 0, sceneRect.width() / columns, sceneRect.height() / rows);
    r.moveLeft((r.width() * column) + sceneRect.left() );
    r.moveTop((r.height() * row) + sceneRect.top());
    return r;
}

static inline QRectF raisedGeometry(const QRectF &sceneRect)
{
    static qreal adjust = .1;
    return sceneRect.adjusted(sceneRect.width() * adjust, sceneRect.height() * adjust,
                              -sceneRect.width() * adjust, -sceneRect.height() * adjust);
}

class TextAnimation : public QPropertyAnimation
{
public:
    TextAnimation(QObject *o, const QByteArray &propertyName)
        : QPropertyAnimation(o, propertyName)
    {}
    virtual QVariant interpolated(const QVariant &from, const QVariant &to, qreal progress) const
    {
        if (qFuzzyIsNull(progress)) {
            return from;
        } else if (qFuzzyCompare(progress, 1.0)) {
            return to;
        }

#if 0
        if (progress < .5) {
            QString fromString = from.toString();
            const int letters = fromString.size() * (progress * 2);
            fromString.chop(letters);
            return fromString;
        } else {
            const QString toString = to.toString();
            const int letters = toString.size() * ((progress - 0.5) * 2);
            return toString.mid(letters);
        }
#else
        QString fromString = from.toString();
        const QString toString = to.toString();
        const int letters = fromString.size() + toString.size();
        const int current = letters * progress;
        if (current == fromString.size()) {
            return QString();
        } else if (current < fromString.size()) {
            fromString.chop(current);
            return fromString;
        }
        return toString.mid(toString.size() - (current - fromString.size()));
#endif
    }
private:
    QGraphicsWidget *widget;
};



GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    qRegisterMetaType<StateType>("StateType");
    d.elapsed = 0;
    d.currentState = 0;
    d.cancelTeam = 0;
    d.statusBar = new StatusBar;
    addItem(d.statusBar);
    d.statusBar->setMaximum(100);
    d.statusBar->setValue(45);
    connect(&d.timeoutTimer, SIGNAL(timeout()), this, SLOT(nextStateTimeOut()));

    d.wrongAnswerItem = new Item;
    d.wrongAnswerItem->setOpacity(0.0);
    d.wrongAnswerItem->setBackgroundColor(Qt::red);
    d.wrongAnswerItem->setColor(Qt::black);
    d.wrongAnswerItem->setText("Wrong!");
    d.wrongAnswerItem->setZValue(200);
    connect(d.wrongAnswerItem, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));

    addItem(d.wrongAnswerItem);

    d.rightAnswerItem = new Item;
    d.rightAnswerItem->setBackgroundColor(Qt::green);
    d.rightAnswerItem->setColor(Qt::black);
    d.rightAnswerItem->setText("Right!");
    d.rightAnswerItem->setOpacity(0.0);
    d.rightAnswerItem->setZValue(200);
    connect(d.rightAnswerItem, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));

    addItem(d.rightAnswerItem);

    d.teamProxy = new TeamProxy(this);
    d.answerTime = 0;
    d.sceneRectChangedBlocked = false;

    d.framesLeft = 0;
    d.currentFrame = 0;

    const char *states[] = {
        "Normal", "ShowQuestion", "TimeOut", "PickTeam",
        "TeamTimedOut", "PickRightOrWrong", "WrongAnswer",
        "RightAnswer", "NoAnswers", "Finished", 0
    };
    for (int i=0; states[i]; ++i) {
        State *state = new State(static_cast<StateType>(i), &d.stateMachine);
        connect(state, SIGNAL(entered()), this, SLOT(onStateEntered()));
        connect(state, SIGNAL(exited()), this, SLOT(onStateExited()));
        state->setObjectName(states[i]);
        d.states[i] = state;
    }

    QParallelAnimationGroup *proxyGroup = new QParallelAnimationGroup(this);
    proxyGroup->addAnimation(new QPropertyAnimation(&d.proxy, "yRotation"));
    proxyGroup->addAnimation(new QPropertyAnimation(&d.proxy, "geometry"));
    proxyGroup->addAnimation(new QPropertyAnimation(&d.proxy, "backgroundColor"));
    proxyGroup->addAnimation(new QPropertyAnimation(&d.proxy, "color"));
    TextAnimation *textAnimation = new TextAnimation(&d.proxy, "text");
    textAnimation->setDuration(1000);
    proxyGroup->addAnimation(textAnimation);

    QParallelAnimationGroup *teamProxyGroup = new QParallelAnimationGroup(this);
    teamProxyGroup->addAnimation(new QPropertyAnimation(d.teamProxy, "geometry"));

    QParallelAnimationGroup *rightWrongOpacityGroup = new QParallelAnimationGroup(this);
    rightWrongOpacityGroup->addAnimation(new QPropertyAnimation(d.rightAnswerItem, "opacity"));
    rightWrongOpacityGroup->addAnimation(new QPropertyAnimation(d.wrongAnswerItem, "opacity"));

    addTransition(Normal, ShowQuestion);
//    normalToShowQuestion->addAnimation(proxyGroup);
    addTransition(ShowQuestion, TimeOut);
    addTransition(ShowQuestion, PickTeam);
//    showQuestionToPickTeam->addAnimation(teamProxyGroup);
    addTransition(TimeOut, Normal);
    addTransition(TimeOut, Finished);
    addTransition(PickTeam, NoAnswers);

    addTransition(NoAnswers, Normal);
    addTransition(NoAnswers, Finished);

    addTransition(PickTeam, PickRightOrWrong);
    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup(this);
        sequential->addAnimation(teamProxyGroup);
        sequential->addPause(200);
        sequential->addAnimation(rightWrongOpacityGroup);
//        pickTeamToPickRightOrWrong->addAnimation(sequential);
    }

    addTransition(TeamTimedOut, ShowQuestion);
    addTransition(PickRightOrWrong, RightAnswer);
    addTransition(PickRightOrWrong, WrongAnswer);
    addTransition(RightAnswer, Normal);
    addTransition(RightAnswer, Finished);
    addTransition(WrongAnswer, ShowQuestion);
    addTransition(WrongAnswer, Normal);
    addTransition(WrongAnswer, Finished);
    addTransition(PickTeam, WrongAnswer);

    d.states[Normal]->assignProperty(&d.proxy, "yRotation", 0.0);
//    d.states[Normal]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[Normal]->assignProperty(&d.proxy, "progressBarColor", Qt::yellow);

    d.states[ShowQuestion]->assignProperty(&d.proxy, "yRotation", 360.0);
//     d.states[ShowQuestion]->assignProperty(&d.proxy, "answerProgress", 1.0);
//     d.states[ShowQuestion]->assignProperty(&d.proxy, "progressBarColor", Qt::green);
    d.states[ShowQuestion]->assignProperty(&d.proxy, "backgroundColor", Qt::yellow);
    d.states[ShowQuestion]->assignProperty(&d.proxy, "color", Qt::black);

//    d.states[PickTeam]->assignProperty(d.teamProxy, "opacity", 1.0);
//    d.states[PickTeam]->assignProperty(&d.proxy, "answerProgress", 0.0);

    d.states[PickRightOrWrong]->assignProperty(d.rightAnswerItem, "opacity", 1.0);
    d.states[PickRightOrWrong]->assignProperty(d.wrongAnswerItem, "opacity", 1.0);
    d.states[PickRightOrWrong]->assignProperty(d.teamProxy, "color", Qt::black);

//     d.states[WrongAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::red);
//     d.states[WrongAnswer]->assignProperty(&d.proxy, "color", Qt::black);
//    d.states[WrongAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "yRotation", 0.0);
    d.states[WrongAnswer]->assignProperty(d.rightAnswerItem, "opacity", 0.0);
    d.states[WrongAnswer]->assignProperty(d.wrongAnswerItem, "opacity", 0.0);
    d.states[WrongAnswer]->assignProperty(d.teamProxy, "backgroundColor", Qt::red);
    d.states[WrongAnswer]->assignProperty(d.teamProxy, "color", Qt::white);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "color", Qt::red);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::black);

//     d.states[RightAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::green);
//     d.states[RightAnswer]->assignProperty(&d.proxy, "color", Qt::black);
//    d.states[RightAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[RightAnswer]->assignProperty(&d.proxy, "color", Qt::green);
    d.states[RightAnswer]->assignProperty(d.teamProxy, "backgroundColor", Qt::darkGray);
    d.states[RightAnswer]->assignProperty(d.teamProxy, "color", Qt::white);
    d.states[RightAnswer]->assignProperty(d.rightAnswerItem, "opacity", 0.0);
    d.states[RightAnswer]->assignProperty(d.wrongAnswerItem, "opacity", 0.0);
    d.states[RightAnswer]->assignProperty(&d.proxy, "color", Qt::green);
    d.states[RightAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::black);

    d.states[TeamTimedOut]->assignProperty(d.teamProxy, "backgroundColor", Qt::red);
    d.states[TeamTimedOut]->assignProperty(&d.proxy, "color", Qt::black);
    d.states[TeamTimedOut]->assignProperty(&d.proxy, "color", Qt::red);
    d.states[TeamTimedOut]->assignProperty(&d.proxy, "backgroundColor", Qt::black);

//    d.states[RightAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[RightAnswer]->assignProperty(&d.proxy, "yRotation", 0.0);

    d.stateMachine.setInitialState(d.states[Normal]);
    d.stateMachine.start();
}

static QStringList pickTeams(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(GraphicsScene::tr("Create teams"));
    QGridLayout *layout = new QGridLayout(&dlg);
    QList<QLineEdit*> edits;
    for (int i=0; i<12; ++i) {
        QLineEdit *edit = new QLineEdit;
        if (i < 2)
            edit->setText(GraphicsScene::tr("Team %1").arg(i + 1));
        edits.append(edit);
        layout->addWidget(edit, i / 2, i % 2);
    }
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,
                                                 Qt::Horizontal, &dlg);
    layout->addWidget(box, layout->rowCount(), 0, 1, 2);
    QObject::connect(box, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(box, SIGNAL(rejected()), &dlg, SLOT(reject()));
    QStringList ret;
    edits.first()->selectAll();
    if (dlg.exec()) {
        foreach(QLineEdit *edit, edits) {
            const QString text = edit->text().simplified();
            if (!text.isEmpty())
                ret.append(text);
        }
    }
    return ret;
}

bool GraphicsScene::load(QIODevice *device, const QStringList &tms)
{
    reset();
    disconnect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
    QTextStream ts(device);
    enum State {
        ExpectingTopic,
        ExpectingQuestion
    } state = ExpectingTopic;

//    TopicItem *topic = 0;
    int lineNumber = 0;
    QRegExp commentRegexp("^ *#");
    Item *topic = 0;
    Frame *frame = 0;
    while (!ts.atEnd()) {
        ++lineNumber;
        QString line = ts.readLine();
        line = line.simplified();
        if (line.indexOf(commentRegexp) == 0)
            continue;
        switch (state) {
        case ExpectingTopic:
            if (line.isEmpty())
                continue;
            topic = new Item;
            topic->setFlag(QGraphicsItem::ItemIsSelectable, false);
            topic->setBackgroundColor(Qt::darkBlue);
            topic->setColor(Qt::yellow);
            topic->setText(line);
            addItem(topic);
            d.topics.append(topic);
            state = ExpectingQuestion;
            break;
        case ExpectingQuestion:
            if (line.isEmpty()) {
                qWarning() << "Didn't expect an empty line here. I was looking question number"
                           << (d.frames.size() % 5) << "for" << d.topics.last()->text() << "on line" << lineNumber;
                reset();
                return false;
            } else {
                const QStringList split = line.split('|');
                if (split.size() > 2) {
                    qWarning("I don't understand this line. There can only be one | per question line (%s) line: %d",
                             qPrintable(line), lineNumber);
                    reset();
                    return false;
                }
                const int row = d.frames.size() % 5;
                const int col = d.topics.size() - 1;
                frame = new Frame(row, col);
                connect(frame, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));
                frame->setFlag(QGraphicsItem::ItemIsSelectable, true);
                frame->setBackgroundColor(Qt::blue);
                frame->setColor(Qt::white);
                frame->setValue((row + 1) * 100);
                frame->setQuestion(split.value(0));
                frame->setAnswer(split.value(1));
                frame->setText(frame->valueString());

                addItem(frame);
                d.frames.append(frame);
                if (row == 4)
                    state = ExpectingTopic;
            }
            break;
        }
    }

    const QStringList teams = (tms.isEmpty() ? pickTeams(views().value(0)) : tms);
    if (teams.isEmpty()) {
        return false;
    }
    for (int i=0; i<teams.size(); ++i) {
        Team *team = new Team(teams.at(i));
        connect(team, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));
//        team->setOpacity(0.0);
        team->setZValue(100.0);
        team->setBackgroundColor(Qt::darkGray);
        team->setColor(Qt::white);
//        d.states[WrongAnswer]->assignProperty(team, "backgroundColor", Qt::darkGray);
//         d.states[RightAnswer]->assignProperty(team, "backgroundColor", Qt::darkGray);
//         d.states[RightAnswer]->assignProperty(team, "color", Qt::white);
        d.states[Normal]->assignProperty(team, "backgroundColor", Qt::darkGray);
        d.states[Normal]->assignProperty(team, "color", Qt::white);
        d.teams.append(team);
        addItem(team);
    }

    d.cancelTeam = new Team(tr("Cancel"));
    connect(d.cancelTeam, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));
    d.states[Normal]->assignProperty(d.cancelTeam, "visible", false);
    d.states[PickTeam]->assignProperty(d.cancelTeam, "visible", true);
    d.states[PickRightOrWrong]->assignProperty(d.cancelTeam, "visible", false);

    d.cancelTeam->setZValue(100.0);
    d.cancelTeam->setBackgroundColor(Qt::black);
    d.cancelTeam->setColor(Qt::red);
    d.cancelTeam->setVisible(false);

    d.teams.append(d.cancelTeam);
    addItem(d.cancelTeam);
    d.teamProxy->setTeams(d.teams);

    onSceneRectChanged(sceneRect());
    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
    d.framesLeft = d.frames.size();
    return true;
}

void GraphicsScene::onSceneRectChanged(const QRectF &rr)
{
    if (d.sceneRectChangedBlocked || rr.isEmpty())
        return;

    enum { TeamsHeight = 100, StatusBarHeight = 20 };
    d.teamsGeometry = QRectF(rr.topLeft(), QSize(rr.width(), TeamsHeight));
    d.framesGeometry = rr.adjusted(0, TeamsHeight, 0, -StatusBarHeight );

    d.states[ShowQuestion]->assignProperty(&d.proxy, "geometry", ::raisedGeometry(d.framesGeometry));

    d.sceneRectChangedBlocked = true;
    const int cols = d.topics.size();
    static const int rows = 5;
    for (int i=0; i<cols; ++i)
        d.topics.at(i)->setGeometry(::itemGeometry(0, i, rows, cols, d.framesGeometry));

    const QRectF raised = ::raisedGeometry(d.framesGeometry);

    for (int i=0; i<rows * cols; ++i) {
        Frame *frame = d.frames.at(i);
        QRectF r;
        if (frame == d.proxy.activeFrame()) {
            r = raised;
        } else {
            const int y = i % rows;
            const int x = i / rows;
            r = ::itemGeometry(y + 1, x, rows + 1, cols, d.framesGeometry);
        }
        frame->setGeometry(r);
    }

    setTeamGeometry(d.teamsGeometry);
//     static QState *const states[] = {
//         d.states[Normal], d.states[ShowQuestion], d.states[ShowAnswer],
//         d.states[PickRightOrWrong], d.states[RightAnswer], d.states[WrongAnswer], 0
//     };

    for (int i=0; i<NumStates; ++i) {
        if (i != PickTeam && d.states[i])
            d.states[i]->assignProperty(d.teamProxy, "geometry", d.teamsGeometry);
    }
    d.states[PickTeam]->assignProperty(d.teamProxy, "geometry", raised);
    d.wrongAnswerItem->setGeometry(QRectF(raised.x(), raised.y(), raised.width() / 2, raised.height()));
    d.rightAnswerItem->setGeometry(QRectF(raised.x() + (raised.width() / 2), raised.y(), raised.width() / 2, raised.height()));
    d.statusBar->setGeometry(rr.left(), rr.bottom() - StatusBarHeight, rr.width(), StatusBarHeight);

    d.sceneRectChangedBlocked = false;
}

void GraphicsScene::reset()
{
    d.teamProxy->setActiveTeam(0);
    Q_ASSERT(d.rightAnswerItem);
    Q_ASSERT(d.wrongAnswerItem);
    removeItem(d.rightAnswerItem);
    removeItem(d.wrongAnswerItem);
    removeItem(d.statusBar);
    d.sceneRectChangedBlocked = false;
    clear();
    d.frames.clear();
    d.topics.clear();
    d.teams.clear();
    addItem(d.rightAnswerItem);
    addItem(d.wrongAnswerItem);
    addItem(d.statusBar);
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    emit mouseButtonPressed(e->scenePos(), e->button());
    QGraphicsScene::mousePressEvent(e);
}


QRectF GraphicsScene::frameGeometry(Frame *frame) const
{
    return ::itemGeometry(frame->row() + 1, frame->column(), 6, d.topics.size(), d.framesGeometry);
}

void GraphicsScene::onClicked(Item *item)
{
    if (d.currentState && item) {
        const StateType type = d.currentState->type();
        switch (type) {
        case NoAnswers:
            break;
        case Normal:
            if (Frame *frame = qgraphicsitem_cast<Frame*>(item)) {
                if (frame->status() != Frame::Hidden)
                    break;
                d.currentFrame = frame;
                d.proxy.setActiveFrame(frame);
                const QRectF r = frameGeometry(frame);
                d.states[Normal]->assignProperty(&d.proxy, "geometry", r);
                d.states[Normal]->assignProperty(&d.proxy, "text", frame->valueString());
                d.states[ShowQuestion]->assignProperty(&d.proxy, "text", frame->question());
                d.states[RightAnswer]->assignProperty(&d.proxy, "text", QString("%1 is the answer :-)").arg(frame->answer()));
                d.states[RightAnswer]->assignProperty(&d.proxy, "geometry", r);
                d.states[WrongAnswer]->assignProperty(&d.proxy, "text", QString("%1 is the answer :-(").arg(frame->answer()));
                d.states[WrongAnswer]->assignProperty(&d.proxy, "geometry", r);
                emit next(ShowQuestion);
            }
            break;
        case ShowQuestion:
            if (item == d.currentFrame) {
                d.elapsed += d.timeoutTimerStarted.msecsTo(QTime::currentTime());
                d.timeoutTimer.stop();
                emit next(PickTeam);
            }
            break;
        case TimeOut:
            break;
        case PickTeam:
            if (Team *team = qgraphicsitem_cast<Team*>(item)) {
                if (team == d.cancelTeam) {
                    emit next(NoAnswers);
                } else if (team->acceptsHoverEvents()) {
                    item->d.hovered = false; // hack
                    team->update();
                    d.teamProxy->setActiveTeam(team);
                    emit next(PickRightOrWrong);
                }
            }
            break;
        case TeamTimedOut:
            break;
        case PickRightOrWrong:
            if (item == d.rightAnswerItem) {
                emit next(RightAnswer);
            } else if (item == d.wrongAnswerItem) {
                emit next(WrongAnswer);
            }
            break;
        case WrongAnswer:
            break;
        case RightAnswer:
            break;
        case Finished:
            break;
        case NumStates:
            break;
        }
    }
}

int GraphicsScene::answerTime() const
{
    return d.answerTime;
}

void GraphicsScene::clearActiveFrame()
{
    Q_ASSERT(d.proxy.activeFrame());
    d.proxy.activeFrame()->setFlag(QGraphicsItem::ItemIsSelectable, false);
    // ### add/remove score here
    d.proxy.setActiveFrame(static_cast<Frame*>(0));
    emit next(Normal);
}

void GraphicsScene::onTransitionTriggered()
{
    qDebug() << sender()->objectName() << "triggered";
}

void GraphicsScene::setTeamGeometry(const QRectF &rect)
{
    d.teamsGeometry = rect;
    Q_ASSERT(!d.teams.isEmpty());
    int count = d.teams.size();
    for (int i=count - 1; i>=0; --i) {
        if (!d.teams.at(i)->isVisible())
            --count;
    }

    enum { Margin = 2 };
    QRectF r = rect;
    r.setWidth((rect.width() / count) - Margin);
    for (int i=0; i<d.teams.size(); ++i) {
        if (d.teams.at(i)->isVisible()) {
            d.teams.at(i)->setGeometry(r);
            r.translate(r.width() + Margin, 0);
        }
    }
}
static inline bool compareTeamsByScore(const Team *left, const Team *right)
{
    return left->points() < right->points();
}

void GraphicsScene::onStateEntered()
{
    d.currentState = qobject_cast<State*>(sender());
    qDebug() << d.currentState->objectName() << "entered";
    const StateType type = d.currentState->type();
    switch (type) {
    case Normal:
        Q_ASSERT(d.teamsAttempted.isEmpty());
        d.teamProxy->setActiveTeam(0);
        Q_ASSERT(!d.teamProxy->activeTeam());
        Q_ASSERT(!d.currentFrame);
        d.elapsed = 0;
        break;
    case ShowQuestion: {
//        d.timeoutTimer.start(5000 - d.elapsed);
        if (d.teamsAttempted.size() == d.teams.size()) {
            emit next(TimeOut);
        } else {
            Q_ASSERT(d.currentFrame);
//                 qDebug() << "showing question" << d.currentFrame->question
//                          << "worth" << d.currentFrame->value << "$";
        }
        break; }
    case TimeOut:
        Q_ASSERT(d.currentFrame);
        d.currentFrame->setStatus(Frame::Failed);
        finishQuestion();
        break;
    case PickTeam:
        d.teamProxy->setActiveTeam(0);
        foreach(Team *team, d.teams) {
            if (!d.teamsAttempted.contains(team))
                team->setAcceptHoverEvents(true);
        }
        Q_ASSERT(!d.teamProxy->activeTeam());
        break;
    case TeamTimedOut:
        ++d.timedout;
    case WrongAnswer:
        if (type == WrongAnswer)
            ++d.wrong;
        Q_ASSERT(d.teamProxy->activeTeam());
        Q_ASSERT(d.currentFrame);
        d.teamProxy->activeTeam()->addPoints(-d.currentFrame->value() / 2);
        d.currentFrame->setStatus(Frame::Failed);
        if (d.teamsAttempted.size() + 2 == d.teams.size()) {
            finishQuestion();
        } else {
            d.teamsAttempted.insert(d.teamProxy->activeTeam());
            emit next(ShowQuestion);
        }
        break;
    case NoAnswers:
        ++d.wrong;
        Q_ASSERT(d.currentFrame);
        Q_ASSERT(!d.teamProxy->activeTeam());
        d.currentFrame->setStatus(Frame::Failed);
        finishQuestion();
        break;
    case RightAnswer:
        ++d.right;
        Q_ASSERT(d.teamProxy->activeTeam());
        Q_ASSERT(d.currentFrame);
        d.teamProxy->activeTeam()->addPoints(d.currentFrame->value());
//             qDebug() << d.teamProxy->activeTeam()->name << "answered correctly and earned" << d.currentFrame->value
//                      << "$. They now have" << d.teamProxy->activeTeam()->score << "$";
        d.currentFrame->setStatus(Frame::Succeeded);
        finishQuestion();
        break;
    case Finished:
        qSort(d.teams.end(), d.teams.begin(), compareTeamsByScore);
        for (int i=0; i<d.teams.size(); ++i) {
            qDebug() << i << d.teams.at(i)->points() << d.teams.at(i)->objectName();
        }
        qDebug() << "right" << d.right << "wrong" << d.wrong << "timedout" << d.timedout;
        d.stateMachine.stop();
        break;
    case PickRightOrWrong:
        break;
    case NumStates:
        break;
    }
//        qDebug() << sender()->objectName() << "entered";
}

void GraphicsScene::onStateExited()
{
    State *state = qobject_cast<State*>(sender());
    const StateType type = state->type();
    switch (type) {
    case PickTeam:
        foreach(Team *team, d.teams)
            team->setAcceptHoverEvents(false);
        break;

    case Normal: {
        d.teamProxy->setActiveTeam(0);
        Q_ASSERT(!d.teamProxy->activeTeam());
//         int idx = rand() % d.frames.size();
//         while (d.frames.at(idx)->status() != Frame::Hidden) {
//             if (++idx == d.frames.size())
//                 idx = 0;
//         }
//         Q_ASSERT(!d.currentFrame);
//         d.currentFrame = d.frames.at(idx);
//         Q_ASSERT(d.currentFrame->status() == Frame::Hidden);
//         --d.framesLeft;
        break; }
    default:
        break;
    }
    qDebug() << sender()->objectName() << "exited";
}


void GraphicsScene::finishQuestion()
{
    d.currentFrame->setAcceptHoverEvents(false);
    static_cast<Item*>(d.currentFrame)->d.hovered = false; // ### hack
    d.currentFrame = 0;
    d.teamsAttempted.clear();
    if (d.framesLeft == 0) {
        emit next(Finished);
    } else {
        emit next(Normal);
    }
}

Transition *GraphicsScene::transition(StateType from, StateType to) const
{
    Q_ASSERT(from != to);
    State *state = d.states[from];
    Q_ASSERT(state);
    return state->transition(to);
}

Transition *GraphicsScene::addTransition(StateType from, StateType to)
{
    Q_ASSERT(!transition(from, to));
    State *fromState = d.states[from];
    State *toState = d.states[to];
    Q_ASSERT(fromState && toState && fromState != toState);
    Transition *transition = new Transition(this, toState);
    fromState->addTransition(to, transition);
    return transition;
}

void State::addTransition(StateType type, Transition *transition)
{
    d.transitions[type] = transition;
    QState::addTransition(transition);
}
