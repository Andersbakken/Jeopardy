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

FrameItem::FrameItem(int row, int column)
{
    d.answerProgress = 0;
    d.answerProgressBarProxy = 0;
    d.answerProgressBar = 0;
    d.row = row;
    d.column = column;
//    setCacheMode(ItemCoordinateCache); // ### need this to know when it's reversed
    d.value = 0;
}

QColor FrameItem::backgroundColor() const
{
    return d.backgroundColor;
}

void FrameItem::setBackgroundColor(const QColor &color)
{
    d.backgroundColor = color;
    update();
}

void FrameItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);
    updateProgressBarGeometry();
}

void FrameItem::updateProgressBarGeometry()
{
    if (d.answerProgressBarProxy) {
        QRectF r(QPointF(), QSizeF(rect().width(), d.answerProgressBar->sizeHint().height()));
        r.moveBottomLeft(rect().bottomLeft());
        d.answerProgressBarProxy->setGeometry(r);
    }
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

GraphicsScene *FrameItem::graphicsScene() const
{
    return qobject_cast<GraphicsScene*>(scene());
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

qreal FrameItem::answerProgress() const
{
    return d.answerProgress;
}

void FrameItem::setAnswerProgress(qreal answerProgress)
{
    d.answerProgress = answerProgress;
    if (qFuzzyIsNull(d.answerProgress)) {
        delete d.answerProgressBar;
        d.answerProgressBar = 0;
        delete d.answerProgressBarProxy;
        d.answerProgressBarProxy = 0;
    } else {

        if (!d.answerProgressBar) {
            d.answerProgressBar = new QProgressBar;
            d.answerProgressBar->setRange(0, graphicsScene()->answerTime());
            d.answerProgressBar->setTextVisible(true);
            d.answerProgressBarProxy = new QGraphicsProxyWidget(this);
            d.answerProgressBarProxy->setWidget(d.answerProgressBar);
            updateProgressBarGeometry();
        }
        d.answerProgressBar->setValue(answerProgress * d.answerProgressBar->maximum());
        d.answerProgress = answerProgress;
    }

}


void FrameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    QBrush brush = d.backgroundColor;
    const QTransform &worldTransform = painter->worldTransform();
    bool mirrored = false;
    if (worldTransform.m11() < 0 || worldTransform.m22() < 0) {
        mirrored = true;
        brush = Qt::black;
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
    d.answerTime = 5000;
    d.activeFrame = 0;
    d.normalState = d.showQuestionState = d.showAnswerState = d.correctAnswerState = d.wrongAnswerState = 0;
    d.sceneRectChangedBlocked = false;

    d.normalState = new QState(&d.stateMachine);
    d.normalState->setObjectName("normalState");
    d.normalState->assignProperty(&d.proxy, "yRotation", 0.0);
    d.normalState->assignProperty(&d.proxy, "answerProgress", 0.0);

    d.showQuestionState = new QState(&d.stateMachine);
    d.showQuestionState->setObjectName("showQuestionState");
    d.showQuestionState->assignProperty(&d.proxy, "yRotation", 360.0);
    d.showQuestionState->assignProperty(&d.proxy, "answerProgress", 1.0);

    d.showAnswerState = new QState(&d.stateMachine);
    d.showAnswerState->setObjectName("showAnswerState");

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

    group->addAnimation(propertyAnimation = new QPropertyAnimation(&d.proxy, "answerProgress"));
    propertyAnimation->setDuration(d.answerTime);

    QAbstractTransition *transition = d.normalState->addTransition(this, SIGNAL(showQuestion()), d.showQuestionState);
    transition->addAnimation(group);
    transition = d.showQuestionState->addTransition(this, SIGNAL(showAnswer()), d.showAnswerState);
    transition->addAnimation(textAnimation);
//    connect(d.raisedState, SIGNAL(polished()), this, SIGNAL(showQuestion()));
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
    FrameItem *frame = 0;
    while (!ts.atEnd()) {
        ++lineNumber;
        const QString line = ts.readLine();
        if (line.indexOf(commentRegexp) == 0)
            continue;
        switch (state) {
        case ExpectingTopic:
            if (line.isEmpty())
                continue;
            frame = new FrameItem(0, d.frames.size());
            frame->setBackgroundColor(Qt::darkBlue);
            frame->setText(line);
            addItem(frame);
            d.frames.append((QList<FrameItem*>() << frame));
            state = ExpectingQuestion;
            break;
        case ExpectingQuestion:
            if (line.isEmpty()) {
                qWarning() << "Didn't expect an empty line here. I was looking for question number";
//                           << (d.frames.last().size() + 1) << "for" << topic->text() << "on line" << lineNumber;
                // ### fix up
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
                const int count = d.frames.last().size();
                const int row = count + 1;
                const int col = d.frames.size() - 1;
                frame = new FrameItem(row, col);
                frame->setBackgroundColor(Qt::blue);
                frame->setValue((row) * 100);
                frame->setQuestion(split.value(0));
                frame->setAnswer(split.value(1));
                frame->setText(frame->valueString());

                addItem(frame);
                d.frames.last().append(frame);
                if (count == 5)
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
    const int cols = d.frames.size();
    const int rows = d.frames.value(0).size();
    for (int x=0; x<cols; ++x) {
        const QList<FrameItem*> &frames = d.frames.at(x);
        for (int y=0; y<rows; ++y) {
            FrameItem *frame = frames.at(y);
            QRectF r;
            if (frame == d.proxy.activeFrame()) {
                r = ::raisedGeometry(rect);
            } else {
                r = ::itemGeometry(y, x, rows, cols, rect);
            }
            frame->setGeometry(r);
        }
    }
    d.sceneRectChangedBlocked = false;
}

void GraphicsScene::reset()
{
    clear();
    d.frames.clear();
}

void GraphicsScene::keyPressEvent(QKeyEvent *e)
{

}

QRectF GraphicsScene::itemGeometry(FrameItem *item) const
{
    return ::itemGeometry(item->d.row, item->d.column,
                          d.frames.value(0).size(),
                          d.frames.size(),
                          sceneRect());
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


int GraphicsScene::answerTime() const
{
    return d.answerTime;
}

