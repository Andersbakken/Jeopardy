#include "graphicsscene.h"

enum { TimeOut = 7000 };

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

TopicItem::TopicItem(const QString &string)
{
    d.text = string;
}
void TopicItem::setText(const QString &text)
{
    d.text = text;
    update();
}

QString TopicItem::text() const
{
    return d.text;
}

void TopicItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    enum { Margin = 6 } ;
    const QBrush brush = QColor(0x3366ff);
    qDrawShadePanel(painter, option->rect, palette(), false, Margin / 2, &brush);
    const QRectF r = option->rect.adjusted(Margin, Margin, -Margin, -Margin);
    QTextLayout layout(d.text);
    ::initTextLayout(&layout, r, r.height() / 2);
    painter->setPen(Qt::white);
    layout.draw(painter, QPointF());
}

FrameItem::FrameItem(int row, int column)
{
    d.row = row;
    d.column = column;
//    setCacheMode(ItemCoordinateCache);
    d.value = 0;
}

void FrameItem::setValue(int value)
{
    d.value = value;
    d.valueString = d.text = QString("%1$").arg(value);
}

int FrameItem::value() const
{
    return d.value;
}

QString FrameItem::valueString() const
{
    return d.valueString;
}

void FrameItem::setText(const QString &text)
{
    d.text = text;
    update();
}

QString FrameItem::text() const
{
    return d.text;
}

void FrameItem::setYRotation(qreal yRotation)
{
    d.yRotation = yRotation;
    QTransform transform;
    transform.rotate(yRotation, Qt::YAxis);
    setTransform(transform);
}

qreal FrameItem::yRotation() const
{
    return d.yRotation;
}

void FrameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    QBrush brush = QColor(0x3366ff);
//    if (painter->worldTransform().m
    const QTransform &worldTransform = painter->worldTransform();
    bool mirrored = false;
    if (worldTransform.m11() < 0 || worldTransform.m22() < 0) {
        mirrored = true;
        brush = Qt::black;
//     if (
//     if (d.text == "200$" && d.column == 2) {
//         qDebug() << painter->transform() << painter->worldTransform();
    }
    qDrawShadePanel(painter, option->rect, palette(), false, 5, &brush);
    if (!mirrored) {
        painter->setPen(Qt::white);
        enum { Margin = 6 } ;
        const QRectF r = option->rect.adjusted(Margin, Margin, -Margin, -Margin);
        QTextLayout layout(d.text);
        ::initTextLayout(&layout, r, r.height() / 5);
        painter->setPen(Qt::white);
        layout.draw(painter, QPointF());
    }
}


QString FrameItem::question() const
{
    return d.question;
}

void FrameItem::setQuestion(const QString &question)
{
    d.question = question;
}

QString FrameItem::answer() const
{
    return d.answer;
}

void FrameItem::setAnswer(const QString &answer)
{
    d.answer = answer;
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
    d.activeFrame = 0;
    d.normalState = d.showQuestionState = d.showAnswerState = d.correctAnswerState = d.wrongAnswerState = 0;
    d.raised = 0;
    d.sceneRectChangedBlocked = false;

    d.normalState = new QState(&d.stateMachine);
    d.normalState->setObjectName("normalState");
    d.normalState->assignProperty(&d.proxy, "yRotation", 0.0);
    d.normalState->assignProperty(&d.proxy, "text", "zot");
//    connect(d.normalState, SIGNAL(entered()), this, SLOT(onStateEntered()));

    d.showQuestionState = new QState(&d.stateMachine);
    d.showQuestionState->setObjectName("showQuestionState");
    d.showQuestionState->assignProperty(&d.proxy, "yRotation", 360.0);
    d.showQuestionState->assignProperty(&d.proxy, "text", "bar");
//    connect(d.showQuestionState, SIGNAL(entered()), this, SLOT(onStateEntered()));

    d.showAnswerState = new QState(&d.stateMachine);
    d.showAnswerState->setObjectName("showAnswerState");
    d.showAnswerState->assignProperty(&d.proxy, "text", "foo");

    qDebug() << d.normalState << d.showQuestionState << d.showAnswerState;
//    connect(d.showAnswerState, SIGNAL(entered()), this, SLOT(onStateEntered()));

    enum { Duration = 1000 };
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(&d.stateMachine);
    QParallelAnimationGroup *parallel = new QParallelAnimationGroup;
    QPropertyAnimation *propertyAnimation;
    parallel->addAnimation(propertyAnimation = new QPropertyAnimation(&d.proxy, "geometry"));
    propertyAnimation->setDuration(Duration);
    parallel->addAnimation(propertyAnimation = new QPropertyAnimation(&d.proxy, "yRotation"));
    propertyAnimation->setDuration(Duration);
    group->addAnimation(parallel);
    group->addPause(500);
    TextAnimation *textAnimation = new TextAnimation(&d.proxy, "text");
    textAnimation->setDuration(Duration);
    group->addAnimation(textAnimation);

    QAbstractTransition *transition = d.normalState->addTransition(this, SIGNAL(showQuestion()), d.showQuestionState);
    transition->addAnimation(group);
    transition = d.showQuestionState->addTransition(this, SIGNAL(showAnswer()), d.showAnswerState);
    transition->addAnimation(textAnimation);
//    connect(d.raisedState, SIGNAL(polished()), this, SIGNAL(showQuestion()));
    d.stateMachine.setInitialState(d.normalState);
    qDebug() << group->animationCount();
    d.stateMachine.start();
//        QApplication::sendPostedEvents(&d.stateMachine, 0);
    // ### hack needed to work around QueuedConnection initialization in QStateMachine
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


    TopicItem *topic = 0;
    int lineNumber = 0;
    QRegExp commentRegexp("^ *#");
    while (!ts.atEnd()) {
        ++lineNumber;
        const QString line = ts.readLine();
        if (line.indexOf(commentRegexp) == 0)
            continue;
        switch (state) {
        case ExpectingTopic:
            if (line.isEmpty())
                continue;
            topic = new TopicItem(line);
            addItem(topic);
            d.frameItems.append(QList<FrameItem*>());
            d.topicItems.append(topic);
            state = ExpectingQuestion;
            break;
        case ExpectingQuestion:
            if (line.isEmpty()) {
                qWarning() << "Didn't expect an empty line here. I was looking for question number"
                           << (d.frameItems.last().size() + 1) << "for" << topic->text() << "on line" << lineNumber;
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
                const int count = d.frameItems.last().size();
                const int row = count + 1;
                const int col = d.topicItems.size() - 1;
                FrameItem *frame = new FrameItem(row, col);
                frame->setValue((row) * 100);
                frame->setQuestion(split.value(0));
                frame->setAnswer(split.value(1));
                frame->setText(frame->valueString());

                addItem(frame);
                d.frameItems.last().append(frame);
                if (count == 4)
                    state = ExpectingTopic;
            }
            break;
        }
    }
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
    const int cols = d.topicItems.size();
    const int rows = d.frameItems.value(0).size();
    for (int x=0; x<cols; ++x) {
        const QRectF r = ::itemGeometry(0, x, rows + 1, cols, rect);
        d.topicItems.at(x)->setGeometry(r);
        const QList<FrameItem*> &frames = d.frameItems.at(x);
        for (int y=0; y<rows; ++y) {
            FrameItem *frame = frames.at(y);
            QRectF r;
            if (frame == d.proxy.activeFrame()) {
                r = ::raisedGeometry(rect);
            } else {
                r = ::itemGeometry(1 + y, x, rows + 1, cols, rect);
            }
            frame->setGeometry(r);
        }
    }
    d.sceneRectChangedBlocked = false;
}

void GraphicsScene::reset()
{
    clear();
    d.topicItems.clear();
    d.frameItems.clear();
}

void GraphicsScene::keyPressEvent(QKeyEvent *e)
{

}

QRectF GraphicsScene::itemGeometry(FrameItem *item) const
{
    return ::itemGeometry(item->d.row, item->d.column,
                          d.frameItems.value(0).size() + 1,
                          d.topicItems.size(),
                          sceneRect());
}

void GraphicsScene::onFrameRaised()
{
    d.raised = qobject_cast<FrameItem*>(sender());
}

void GraphicsScene::onFrameLowered()
{
    if (d.raised == qobject_cast<FrameItem*>(sender()))
        d.raised = 0;
}

void GraphicsScene::click(FrameItem *frame)
{
    if (!d.activeFrame) {
        setupStateMachine(frame);
        emit showQuestion();
    }
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    FrameItem *frame = qgraphicsitem_cast<FrameItem*>(itemAt(event->scenePos()));
    if (frame) {
        click(frame);
    }
}

void GraphicsScene::setupStateMachine(FrameItem *frame)
{
    Q_ASSERT(frame);
    d.proxy.setActiveFrame(frame);
    d.normalState->assignProperty(&d.proxy, "geometry", itemGeometry(frame));
    d.normalState->assignProperty(&d.proxy, "text", frame->valueString());
    d.showQuestionState->assignProperty(&d.proxy, "text", frame->question());
    d.showAnswerState->assignProperty(&d.proxy, "text", frame->answer());
}
void GraphicsScene::onStateEntered()
{
    qDebug() << sender()->objectName();
}
