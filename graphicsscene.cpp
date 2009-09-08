#include "graphicsscene.h"

static inline QRectF itemGeometry(int row, int column, int rows, int columns, const QRectF &sceneRect)
{
    if (qMin(rows, columns) <= 0)
        return QRectF();
    QRectF r(0, 0, sceneRect.width() / columns, sceneRect.height() / rows);
    r.moveLeft(r.width() * column);
    r.moveTop(r.height() * row);
    return r;
}

static inline QRectF raisedGeometry(const QRectF &sceneRect)
{
    static qreal adjust = .1;
    return sceneRect.adjusted(sceneRect.width() * adjust, sceneRect.height() * adjust,
                              -sceneRect.width() * adjust, -sceneRect.height() * adjust);
}

static inline void initTextLayout(QTextLayout *layout, const QRectF &rect, int pixelSize)
{
    layout->setCacheEnabled(true);
    QTextOption option;
    option.setAlignment(Qt::AlignCenter);
    layout->setTextOption(option);
    forever {
        layout->clearLayout();
        QFont f;
        f.setPixelSize(pixelSize--);
        layout->setFont(f);
        layout->beginLayout();
        const int h = QFontMetrics(f).height();
        QPointF pos(rect.topLeft());
        forever {
            QTextLine line = layout->createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(rect.width());
            line.setPosition(pos);
            pos += QPointF(0, h);
        }
        layout->endLayout();
        const QRectF textRect = layout->boundingRect();
        if (pixelSize <= 8 || rect.size().expandedTo(textRect.size()) == rect.size()) {
            break;
        }
    }
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

Item::Item()
{
    setCacheMode(ItemCoordinateCache);
    d.yRotation = 0;
}

void Item::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(this, event->scenePos());
    }
}

QColor Item::backgroundColor() const
{
    return d.backgroundColor;
}

void Item::setBackgroundColor(const QColor &color)
{
    d.backgroundColor = color;
    update();
}

QColor Item::textColor() const
{
    return d.textColor;
}

void Item::setTextColor(const QColor &color)
{
    d.textColor = color;
    update();
}

void Item::setText(const QString &text)
{
    d.text = text;
    update();
}

QString Item::text() const
{
    return d.text;
}

GraphicsScene *Item::graphicsScene() const
{
    return qobject_cast<GraphicsScene*>(scene());
}

void Item::setYRotation(qreal yRotation)
{
    d.yRotation = yRotation;
    QTransform transform;
    const QRectF r = rect();
    transform.translate(r.width() / 2, r.height() / 2);
    transform.rotate(yRotation, Qt::YAxis);
    transform.translate(-r.width() / 2, -r.height() / 2);
    setTransform(transform);
}

qreal Item::yRotation() const
{
    return d.yRotation;
}

void Item::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
//    const QTransform &worldTransform = painter->worldTransform();
//     bool mirrored = false;
//     if (worldTransform.m11() < 0 || worldTransform.m22() < 0) {
//         mirrored = true;
//         brush = Qt::black;
//     }
    QBrush brush = d.backgroundColor;
    enum { Margin = 5 };
    qDrawShadePanel(painter, option->rect, palette(), false, Margin, &brush);
    draw(painter, option->rect);
    Q_ASSERT(d.textColor.isValid());
    painter->setPen(d.textColor);
    const QRectF r = option->rect.adjusted(Margin, Margin, -Margin, -Margin);
    QTextLayout layout(d.text);
    ::initTextLayout(&layout, r, r.height() / 5);
    painter->setPen(d.textColor);
    const QRectF textRect = layout.boundingRect();
    layout.draw(painter, r.center() - textRect.center());
}

SelectorItem::SelectorItem()
{
    setCacheMode(ItemCoordinateCache);
}

void SelectorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    QLinearGradient gradient(0, 0, 0, option->rect.height());
    gradient.setColorAt(0.0, QColor(0, 20, 200, 70));
    gradient.setColorAt(0.5, QColor(0, 40, 250, 260));
    gradient.setColorAt(1.0, QColor(0, 20, 200, 70));

    enum { PenWidth = 5 };
    QLinearGradient penGradient(0, 0, option->rect.width(), 0);
    gradient.setColorAt(0.0, QColor(0, 200, 20, 70));
    gradient.setColorAt(0.5, QColor(0, 250, 40, 260));
    gradient.setColorAt(1.0, QColor(0, 200, 20, 70));
    painter->setPen(QPen(penGradient, PenWidth));
    painter->setBrush(gradient);

    painter->drawRoundedRect(option->rect.adjusted(PenWidth / 2, PenWidth / 2, -PenWidth / 2, -PenWidth / 2), PenWidth, PenWidth);
}

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    d.answerTime = 600;
    d.sceneRectChangedBlocked = false;
    d.currentTeam = 0;

    d.normalState = new QState(&d.stateMachine);
    d.normalState->setObjectName("normalState");
    d.normalState->assignProperty(&d.proxy, "yRotation", 0.0);
    d.normalState->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.normalState->assignProperty(&d.proxy, "progressBarColor", Qt::yellow);

    d.showQuestionState = new QState(&d.stateMachine);
    d.showQuestionState->setObjectName("showQuestionState");
    d.showQuestionState->assignProperty(&d.proxy, "yRotation", 360.0);
    d.showQuestionState->assignProperty(&d.proxy, "answerProgress", 1.0);
    d.showQuestionState->assignProperty(&d.proxy, "progressBarColor", Qt::green);

    d.showAnswerState = new QState(&d.stateMachine);
    d.showAnswerState->assignProperty(&d.proxy, "backgroundColor", Qt::blue);
    d.showAnswerState->assignProperty(&d.proxy, "progressBarColor", Qt::red);
    d.showAnswerState->assignProperty(&d.teamProxy, "opacity", 0.0);
    d.showAnswerState->setObjectName("showAnswerState");

    d.pickTeamState = new QState(&d.stateMachine);
    d.pickTeamState->assignProperty(&d.teamProxy, "opacity", 1.0);
    d.pickTeamState->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.pickTeamState->setObjectName("pickTeamState");

    d.answerState = new QState(&d.stateMachine);
    d.answerState->assignProperty(&d.teamProxy, "opacity", 0.0);
    d.answerState->assignProperty(&d.proxy, "backgroundColor", Qt::yellow);
    d.answerState->assignProperty(&d.proxy, "textColor", Qt::blue);
    d.answerState->assignProperty(&d.proxy, "progressBarColor", Qt::red);
    d.answerState->assignProperty(&d.proxy, "answerProgress", 1.0);
    d.answerState->setObjectName("answerState");

    d.wrongAnswerState = new QState(&d.stateMachine);
    d.wrongAnswerState->assignProperty(&d.proxy, "backgroundColor", Qt::red);
    d.wrongAnswerState->assignProperty(&d.proxy, "textColor", Qt::black);
    d.wrongAnswerState->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.wrongAnswerState->assignProperty(&d.proxy, "yRotation", 0.0);
    d.wrongAnswerState->setObjectName("wrongAnswerState");

    d.correctAnswerState = new QState(&d.stateMachine);
    d.correctAnswerState->assignProperty(&d.proxy, "backgroundColor", Qt::green);
    d.correctAnswerState->assignProperty(&d.proxy, "textColor", Qt::black);
    d.correctAnswerState->assignProperty(&d.proxy, "answerProgress", 0.0);
    d.correctAnswerState->assignProperty(&d.proxy, "yRotation", 0.0);
    d.correctAnswerState->setObjectName("correctAnswerState");

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

        QAbstractTransition *showQuestionTransition = d.normalState->addTransition(this, SIGNAL(showQuestion()), d.showQuestionState);
        showQuestionTransition->addAnimation(sequential);
        connect(sequential, SIGNAL(finished()), this, SIGNAL(showAnswer()));
    }

    {
        QAbstractTransition *showAnswerTransition = d.showQuestionState->addTransition(this, SIGNAL(showAnswer()), d.showAnswerState);
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
        QPropertyAnimation *textColorAnimation = new QPropertyAnimation(&d.proxy, "textColor");
        textColorAnimation->setDuration(Duration);
        colorGroup->addAnimation(textColorAnimation);
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

        QAbstractTransition *wrongAnswerTransition = d.answerState->addTransition(this, SIGNAL(wrongAnswer()), d.wrongAnswerState);
        wrongAnswerTransition->addAnimation(sequential);

        QAbstractTransition *correctAnswerTransition = d.answerState->addTransition(this, SIGNAL(correctAnswer()), d.correctAnswerState);
        correctAnswerTransition->addAnimation(sequential);

        connect(sequential, SIGNAL(finished()), this, SLOT(clearActiveFrame()));
    }

    {
        QPropertyAnimation *opacityAnimation = new QPropertyAnimation(&d.teamProxy, "opacity");
        opacityAnimation->setDuration(Duration / 2);
        QAbstractTransition *pickTeamTransition = d.showQuestionState->addTransition(this, SIGNAL(spaceBarPressed()), d.pickTeamState);
        pickTeamTransition->addAnimation(opacityAnimation);
    }

    {
        QSequentialAnimationGroup *sequential = new QSequentialAnimationGroup;
        QPropertyAnimation *opacityAnimation = new QPropertyAnimation(&d.teamProxy, "opacity");
        opacityAnimation->setDuration(Duration / 2);
        sequential->addAnimation(opacityAnimation);

        QParallelAnimationGroup *parallel = new QParallelAnimationGroup;
        QPropertyAnimation *backgroundColorAnimation = new QPropertyAnimation(&d.proxy, "backgroundColor");
        backgroundColorAnimation->setDuration(Duration);
        parallel->addAnimation(backgroundColorAnimation);

        QPropertyAnimation *textColorAnimation = new QPropertyAnimation(&d.proxy, "textColor");
        textColorAnimation->setDuration(Duration);
        parallel->addAnimation(textColorAnimation);
        sequential->addAnimation(parallel);

        QPropertyAnimation *answerProgressAnimation = new QPropertyAnimation(&d.proxy, "answerProgress");
        answerProgressAnimation->setDuration(d.answerTime);
        sequential->addAnimation(answerProgressAnimation);
        connect(answerProgressAnimation, SIGNAL(finished()), this, SIGNAL(wrongAnswer()));

        QAbstractTransition *answerTransition = d.pickTeamState->addTransition(this, SIGNAL(teamClicked()), d.answerState);
        answerTransition->addAnimation(sequential);
    }

    d.stateMachine.setInitialState(d.normalState);
    d.stateMachine.start();
}

bool GraphicsScene::load(QIODevice *device)
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
        const QString line = ts.readLine();
        if (line.indexOf(commentRegexp) == 0)
            continue;
        switch (state) {
        case ExpectingTopic:
            if (line.isEmpty())
                continue;
            topic = new Item;
            topic->setFlag(QGraphicsItem::ItemIsSelectable, false);
            topic->setBackgroundColor(Qt::darkBlue);
            topic->setTextColor(Qt::yellow);
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
                frame->setTextColor(Qt::white);
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
    const char *teams[] = { "Team 1", "Team 2" , "Team 3", 0 };
    for (int i=0; teams[i]; ++i) {
        Team *team = new Team(teams[i]);
        connect(team, SIGNAL(clicked(Item*, QPointF)), this, SLOT(onClicked(Item*)));
        team->setOpacity(0.0);
        team->setZValue(100.0);
        team->setBackgroundColor(Qt::darkGray);
        team->setTextColor(Qt::white);
        d.teams.append(team);
        addItem(team);
    }
    d.teamProxy.setTeams(d.teams);

    onSceneRectChanged(sceneRect());
    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
    return true;
}

void GraphicsScene::onSceneRectChanged(const QRectF &rect)
{
    if (d.sceneRectChangedBlocked || rect.isEmpty())
        return;

    d.showQuestionState->assignProperty(&d.proxy, "geometry", ::raisedGeometry(sceneRect()));

    d.sceneRectChangedBlocked = true;
    const int cols = d.topics.size();
    static const int rows = 5;
    for (int i=0; i<cols; ++i)
        d.topics.at(i)->setGeometry(::itemGeometry(0, i, rows, cols, rect));

    const QRectF raised = ::raisedGeometry(rect);

    for (int i=0; i<rows * cols; ++i) {
        Item *frame = d.frames.at(i);
        QRectF r;
        if (frame == d.proxy.activeFrame()) {
            r = raised;
        } else {
            const int y = i % rows;
            const int x = i / rows;
            r = ::itemGeometry(y + 1, x, rows, cols, rect);
        }
        frame->setGeometry(r);
    }

    Q_ASSERT(!d.teams.isEmpty());
    enum { Margin = 2 };
    QRectF r(0, 0, (raised.width() / d.teams.size()) - (d.teams.size() - 1 * Margin),
             raised.height() / 2);
    r.moveCenter(raised.center());
    r.moveLeft(raised.left());
    for (int i=0; i<d.teams.size(); ++i) {
        d.teams.at(i)->setGeometry(r);
        r.translate(r.width() + Margin, 0);
    }

    d.sceneRectChangedBlocked = false;
}

void GraphicsScene::reset()
{
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

QRectF GraphicsScene::frameGeometry(Frame *frame) const
{
    return ::itemGeometry(frame->row() + 1, frame->column(), 5, d.topics.size(), sceneRect());
}

void GraphicsScene::click(Frame *frame)
{
    qDebug() << "clicking" << (frame != 0);
    if (!d.proxy.activeFrame()) {
        setupStateMachine(frame);
        emit showQuestion();
    } else {
        emit correctAnswer();
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
        emit teamClicked();
    }
}

void GraphicsScene::setupStateMachine(Frame *frame)
{
    Q_ASSERT(frame);
    d.proxy.setActiveFrame(frame);
    const QRectF r = frameGeometry(frame);
    d.normalState->assignProperty(&d.proxy, "geometry", r);
    d.normalState->assignProperty(&d.proxy, "text", frame->valueString());
    d.showQuestionState->assignProperty(&d.proxy, "text", frame->question());
    d.showAnswerState->assignProperty(&d.proxy, "text", frame->answer());
    d.correctAnswerState->assignProperty(&d.proxy, "text", QString("%1 is the answer :-)").arg(frame->answer()));
    d.correctAnswerState->assignProperty(&d.proxy, "geometry", r);
    d.wrongAnswerState->assignProperty(&d.proxy, "text", QString("%1 is the answer :-(").arg(frame->answer()));
    d.wrongAnswerState->assignProperty(&d.proxy, "geometry", r);
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

Frame::Frame(int row, int column)
    : Item()
{
    d.row = row;
    d.column = column;
    d.value = 0;
    d.answerProgress = 0;;
}

void Frame::draw(QPainter *painter, const QRect &rect)
{
    if (!qFuzzyIsNull(d.answerProgress) && !qFuzzyCompare(d.answerProgress, 1.0)) {
        enum { Margin = 5, PenWidth = 3 };
        painter->setPen(QPen(Qt::black, PenWidth));
        const qreal adjust = Margin + (PenWidth / 2);
        QRectF r = rect.adjusted(adjust, 0, -adjust, -adjust);
        r.setWidth(r.width() * d.answerProgress);
        r.setTop(rect.bottom() - qBound(10, rect.height() / 4, 30));
        painter->setBrush(d.progressBarColor);
        painter->drawRect(r);
    }
}

