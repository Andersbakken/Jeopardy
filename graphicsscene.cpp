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

#define FOO(transition) transition->setObjectName(#transition); connect(transition, SIGNAL(triggered()), this, SLOT(onTransitionTriggered()));

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    d.teamProxy = new TeamProxy(this);
    d.answerTime = 0;
    d.sceneRectChangedBlocked = false;

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

    d.states[Normal]->addTransition(this, SIGNAL(nextState()), d.states[ShowQuestion]);
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

    d.states[Normal]->assignProperty(&d.proxy, "yRotation", 0.0);
    d.states[Normal]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[Normal]->assignProperty(&d.proxy, "progressBarColor", Qt::yellow);

    d.states[ShowQuestion]->assignProperty(&d.proxy, "yRotation", 360.0);
    d.states[ShowQuestion]->assignProperty(&d.proxy, "answerProgress", 1.0);
    d.states[ShowQuestion]->assignProperty(&d.proxy, "progressBarColor", Qt::green);

//    d.states[PickTeam]->assignProperty(d.teamProxy, "opacity", 1.0);
    d.states[PickTeam]->assignProperty(&d.proxy, "answerProgress", 0.0);

    d.states[PickRightOrWrong]->assignProperty(d.rightAnswerItem, "opacity", 1.0);
    d.states[PickRightOrWrong]->assignProperty(d.wrongAnswerItem, "opacity", 1.0);

    d.states[WrongAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::red);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "color", Qt::black);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "yRotation", 0.0);
    d.states[WrongAnswer]->assignProperty(d.rightAnswerItem, "opacity", 0.0);
    d.states[WrongAnswer]->assignProperty(d.wrongAnswerItem, "opacity", 0.0);

    d.states[RightAnswer]->assignProperty(&d.proxy, "backgroundColor", Qt::green);
    d.states[RightAnswer]->assignProperty(&d.proxy, "color", Qt::black);
    d.states[RightAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[RightAnswer]->assignProperty(&d.proxy, "yRotation", 0.0);

    d.states[TeamTimedOut]->assignProperty(d.teamProxy, "backgroundColor", Qt::red);
    d.states[TeamTimedOut]->assignProperty(&d.proxy, "color", Qt::black);
    d.states[RightAnswer]->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.states[RightAnswer]->assignProperty(&d.proxy, "yRotation", 0.0);


#if 0
    enum { Duration = 1000 };
    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup(&d.stateMachine);
        QParallelAnimationGroup *parallel = new QParallelAnimationGroup;
        QPropertyAnimation *geometryAnimation = new QPropertyAnimation(&d.proxy, "geometry");
        geometryAnimation->setDuration(Duration);
        parallel->addAnimation(geometryAnimation);
        QPropertyAnimation *yRotationAnimation = new QPropertyAnimation(&d.proxy, "yRotation");
        yRotationAnimation->setDuration(Duration);
        parallel->addAnimation(yRotationAnimation);
        sequential->addAnimation(parallel);
        sequential->addPause(500);
        TextAnimation *textAnimation = new TextAnimation(&d.proxy, "text");
        textAnimation->setDuration(Duration);
        sequential->addAnimation(textAnimation);

        QParallelAnimationGroup *answerProgressGroup = new QParallelAnimationGroup;
        QPropertyAnimation *answerProgressAnimation = new QPropertyAnimation(&d.proxy, "answerProgress");
        answerProgressAnimation->setDuration(d.answerTime);
        answerProgressGroup->addAnimation(answerProgressAnimation);

        QPropertyAnimation *progressBarColorAnimation = new QPropertyAnimation(&d.proxy, "progressBarColor");
        answerProgressAnimation->setDuration(d.answerTime);
        answerProgressGroup->addAnimation(progressBarColorAnimation);

        sequential->addAnimation(answerProgressGroup);

        QAbstractTransition *showQuestionTransition = d.states[Normal]->addTransition(this, SIGNAL(showQuestion()), d.states[ShowQuestion]);
        FOO(showQuestionTransition);
        showQuestionTransition->addAnimation(sequential);
        connect(sequential, SIGNAL(finished()), this, SIGNAL(showAnswer()));
    }

    {
        QAbstractTransition *showAnswerTransition = d.states[ShowQuestion]->addTransition(this, SIGNAL(showAnswer()), d.states[ShowAnswer]);
        FOO(showAnswerTransition);
        TextAnimation *textAnimation = new TextAnimation(&d.proxy, "text");
        textAnimation->setDuration(Duration);
        showAnswerTransition->addAnimation(textAnimation);
    }

    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup(&d.stateMachine);

        QParallelAnimationGroup *colorGroup = new QParallelAnimationGroup;
        QPropertyAnimation *backgroundColorAnimation = new QPropertyAnimation(&d.proxy, "backgroundColor");
        backgroundColorAnimation->setDuration(Duration);
        colorGroup->addAnimation(backgroundColorAnimation);
        QPropertyAnimation *colorAnimation = new QPropertyAnimation(&d.proxy, "color");
        colorAnimation->setDuration(Duration);
        colorGroup->addAnimation(colorAnimation);
        sequential->addAnimation(colorGroup);

        TextAnimation *textAnimation = new TextAnimation(&d.proxy, "text");
        textAnimation->setDuration(Duration / 2);
        sequential->addAnimation(textAnimation);

        sequential->addPause(2500);

        QParallelAnimationGroup *parallel = new QParallelAnimationGroup;

        QPropertyAnimation *geometryAnimation = new QPropertyAnimation(&d.proxy, "geometry");
        geometryAnimation->setDuration(Duration);
        parallel->addAnimation(geometryAnimation);

        QPropertyAnimation *yRotationAnimation = new QPropertyAnimation(&d.proxy, "yRotation");
        yRotationAnimation->setDuration(Duration);
        parallel->addAnimation(yRotationAnimation);
        sequential->addAnimation(parallel);

        QAbstractTransition *wrongAnswerTransition = d.states[PickRightOrWrong]->addTransition(this, SIGNAL(wrongAnswer()), d.states[WrongAnswer]);
        wrongAnswerTransition->addAnimation(sequential);

        QAbstractTransition *rightAnswerTransition = d.states[PickRightOrWrong]->addTransition(this, SIGNAL(rightAnswer()), d.states[RightAnswer]);
        FOO(rightAnswerTransition);
        rightAnswerTransition->addAnimation(sequential);

        connect(sequential, SIGNAL(finished()), this, SLOT(clearActiveFrame()));
    }

    {
//         QPropertyAnimation *opacityAnimation = new QPropertyAnimation(d.teamProxy, "opacity");
//         opacityAnimation->setDuration(Duration / 2);
        QPropertyAnimation *rectAnimation = new QPropertyAnimation(d.teamProxy, "rect");
        rectAnimation->setDuration(Duration / 2);
        QAbstractTransition *pickTeamTransition = d.states[ShowQuestion]->addTransition(this,
                                                                                        SIGNAL(mouseButtonPressed(QPointF,Qt::MouseButton)),
                                                                                        d.states[PickTeam]);
        FOO(pickTeamTransition);
//        pickTeamTransition->addAnimation(opacityAnimation);
        pickTeamTransition->addAnimation(rectAnimation);
    }

#if 0
    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup;
        QPropertyAnimation *opacityAnimation = new QPropertyAnimation(d.teamProxy, "opacity");
        opacityAnimation->setDuration(Duration / 2);
        sequential->addAnimation(opacityAnimation);

        QParallelAnimationGroup *parallel = new QParallelAnimationGroup;
        QPropertyAnimation *backgroundColorAnimation = new QPropertyAnimation(&d.proxy, "backgroundColor");
        backgroundColorAnimation->setDuration(Duration);
        parallel->addAnimation(backgroundColorAnimation);

        QPropertyAnimation *colorAnimation = new QPropertyAnimation(&d.proxy, "color");
        colorAnimation->setDuration(Duration);
        parallel->addAnimation(colorAnimation);
        sequential->addAnimation(parallel);

        QPropertyAnimation *answerProgressAnimation = new QPropertyAnimation(&d.proxy, "answerProgress");
        answerProgressAnimation->setDuration(d.answerTime);
        sequential->addAnimation(answerProgressAnimation);
        connect(answerProgressAnimation, SIGNAL(finished()), this, SIGNAL(wrongAnswer()));

        QAbstractTransition *answerTransition = states[PickTeam]->addTransition(this, SIGNAL(teamPicked()), d.states[PickRightOrWrong]);
        FOO(answerTransition);
        answerTransition->addAnimation(sequential);
    }
#endif
    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup;
        QPropertyAnimation *rectAnimation = new QPropertyAnimation(d.teamProxy, "rect");
        rectAnimation->setDuration(Duration / 2);
        sequential->addAnimation(rectAnimation);

        QParallelAnimationGroup *parallel = new QParallelAnimationGroup;
        QPropertyAnimation *opacityAnimation = new QPropertyAnimation(d.rightAnswerItem, "opacity");
        opacityAnimation->setDuration(Duration / 2);
        parallel->addAnimation(opacityAnimation);
        opacityAnimation = new QPropertyAnimation(d.wrongAnswerItem, "opacity");
        opacityAnimation->setDuration(Duration / 2);
        parallel->addAnimation(opacityAnimation);
        sequential->addAnimation(parallel);

        QAbstractTransition *answerTransition = d.states[PickTeam]->addTransition(this, SIGNAL(teamPicked()), d.states[PickRightOrWrong]);
        FOO(answerTransition);
        answerTransition->addAnimation(sequential);
    }

#endif

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
        d.states[Normal]->assignProperty(team, "backgroundColor", Qt::darkGray);
        d.teams.append(team);
        addItem(team);
    }
    d.teamProxy->setTeams(d.teams);

    d.wrongAnswerItem = new Item;
    d.wrongAnswerItem->setOpacity(0.0);
    d.wrongAnswerItem->setBackgroundColor(Qt::red);
    d.wrongAnswerItem->setColor(Qt::black);
    d.wrongAnswerItem->setText("Wrong!");
    d.wrongAnswerItem->setZValue(200);
    d.rightAnswerItem = new Item;
    d.rightAnswerItem->setBackgroundColor(Qt::green);
    d.rightAnswerItem->setColor(Qt::black);
    d.rightAnswerItem->setText("Right!");
    d.rightAnswerItem->setOpacity(0.0);
    d.rightAnswerItem->setZValue(200);

    onSceneRectChanged(sceneRect());
    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
    return true;
}

void GraphicsScene::onSceneRectChanged(const QRectF &rr)
{
    if (d.sceneRectChangedBlocked || rr.isEmpty())
        return;

    enum { TeamsHeight = 100 };
    d.teamsGeometry = QRectF(rr.topLeft(), QSize(rr.width(), TeamsHeight));
    d.framesGeometry = rr.adjusted(0, TeamsHeight, 0, 0);

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

    if (!d.stateMachine.isRunning()) {
        setTeamGeometry(d.teamsGeometry);
    }
//     static QState *const states[] = {
//         d.states[Normal], d.states[ShowQuestion], d.states[ShowAnswer],
//         d.states[PickRightOrWrong], d.states[RightAnswer], d.states[WrongAnswer], 0
//     };

    for (int i=0; i<NumStates; ++i) {
        if (i != PickTeam && d.states[i])
            d.states[i]->assignProperty(d.teamProxy, "rect", d.teamsGeometry);
    }
    d.states[PickTeam]->assignProperty(d.teamProxy, "rect", raised);
    d.wrongAnswerItem->setGeometry(QRectF(raised.x(), raised.y(), raised.width() / 2, raised.height()));
    d.rightAnswerItem->setGeometry(QRectF(raised.x() + (raised.width() / 2), raised.y(), raised.width() / 2, raised.height()));
    d.sceneRectChangedBlocked = false;
}

void GraphicsScene::reset()
{
    d.currentTeam = 0;
    d.rightAnswerItem = d.wrongAnswerItem = 0;
    d.sceneRectChangedBlocked = false;
    clear();
    d.frames.clear();
    d.topics.clear();
    d.teams.clear();
}

void GraphicsScene::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Space:
        emit spaceBarPressed();
        break;
    default:
        break;
    }
    e->accept();
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    qDebug("%s %d: void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *e)", __FILE__, __LINE__);
    emit mouseButtonPressed(e->scenePos(), e->button());
    QGraphicsScene::mousePressEvent(e);
}


QRectF GraphicsScene::frameGeometry(Frame *frame) const
{
    return ::itemGeometry(frame->row() + 1, frame->column(), 6, d.topics.size(), d.framesGeometry);
}

void GraphicsScene::click(Frame *frame)
{
    if (!d.proxy.activeFrame()) {
        setupStateMachine(frame);
        emit showQuestion();
    } else {
        emit rightAnswer();
    }
}

void GraphicsScene::onClicked(Item *item)
{
    if (Frame *frame = qgraphicsitem_cast<Frame*>(item)) {
        if (frame->flags() & QGraphicsItem::ItemIsSelectable) {
            click(frame);
        }
        return;
    }

    if (Team *team = qgraphicsitem_cast<Team*>(item)) {
        d.currentTeam = team;
        emit teamPicked();
    }
}

void GraphicsScene::setupStateMachine(Frame *frame)
{
    Q_ASSERT(frame);
    d.proxy.setActiveFrame(frame);
    const QRectF r = frameGeometry(frame);
    d.states[Normal]->assignProperty(&d.proxy, "geometry", r);
    d.states[Normal]->assignProperty(&d.proxy, "text", frame->valueString());
    d.states[ShowQuestion]->assignProperty(&d.proxy, "text", frame->question());
    d.states[RightAnswer]->assignProperty(&d.proxy, "text", QString("%1 is the answer :-)").arg(frame->answer()));
    d.states[RightAnswer]->assignProperty(&d.proxy, "geometry", r);
    d.states[WrongAnswer]->assignProperty(&d.proxy, "text", QString("%1 is the answer :-(").arg(frame->answer()));
    d.states[WrongAnswer]->assignProperty(&d.proxy, "geometry", r);
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
    emit normalState();
}

void GraphicsScene::onTransitionTriggered()
{
    qDebug() << sender()->objectName() << "triggered";
}

void GraphicsScene::setTeamGeometry(const QRectF &rect)
{
    d.teamsGeometry = rect;
    Q_ASSERT(!d.teams.isEmpty());
    enum { Margin = 2 };
    QRectF r = rect;
    r.setWidth((rect.width() / d.teams.size()) - Margin);
    for (int i=0; i<d.teams.size(); ++i) {
        d.teams.at(i)->setGeometry(r);
        r.translate(r.width() + Margin, 0);
    }
}


