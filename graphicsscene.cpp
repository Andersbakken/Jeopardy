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

static inline void initTextLayout(QTextLayout *layout, const QRectF &rect)
{
    layout->setCacheEnabled(true);
    int pixelSize = rect.height() / 3;
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


class TextAnimation : public QVariantAnimation
{
public:
    TextAnimation(QGraphicsWidget *w)
        : widget(w)
    {}
    virtual void updateCurrentValue(const QVariant &)
    {
        widget->update();
    }
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
    ::initTextLayout(&layout, r);
    painter->setPen(Qt::white);
    layout.draw(painter, QPointF());
}

FrameItem::FrameItem(int row, int col, int value)
{
    setCacheMode(ItemCoordinateCache);
    d.animationGroup = 0;
    d.geometryAnimation = 0;
    d.value = value;
    d.yRotation = 0;
    d.textAnimation = 0;
    d.row = row;
    d.column = col;
    d.state = Lowered;
}

void FrameItem::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d.answerTimer.timerId()) {
        if (d.timer.elapsed() >= TimeOut) {

        }
        update();
    } else {
        QGraphicsWidget::timerEvent(e);
    }
}


FrameItem::State FrameItem::state() const
{
    if (d.animationGroup && d.animationGroup->state() == QAbstractAnimation::Stopped) {
        return Raised;
    } else {
        return d.state;
    }
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

void FrameItem::raise()
{
    if (d.animationGroup)
        return;

    d.state = Raising;
    enum { Duration = 1000 };
    d.parallelAnimationGroup = new QParallelAnimationGroup;
    d.animationGroup = new QSequentialAnimationGroup;
    d.geometryAnimation = new QPropertyAnimation;
    d.geometryAnimation->setDuration(Duration);
    d.geometryAnimation->setTargetObject(this);
    d.geometryAnimation->setPropertyName("geometry");

    d.geometryAnimation->setEndValue(::raisedGeometry(scene()->sceneRect()));
    d.rotationAnimation = new QPropertyAnimation;
    d.rotationAnimation->setDuration(Duration);
    d.rotationAnimation->setTargetObject(this);
    d.rotationAnimation->setPropertyName("yRotation");
    d.rotationAnimation->setEndValue(360);

    d.textAnimation = new TextAnimation(this);
    d.textAnimation->setStartValue(QString::number(d.value));
    d.textAnimation->setEndValue(d.question);
    d.textAnimation->setDuration(Duration);

    d.parallelAnimationGroup->addAnimation(d.geometryAnimation);
    d.parallelAnimationGroup->addAnimation(d.textAnimation);
    d.animationGroup->addAnimation(d.parallelAnimationGroup);
    d.animationGroup->addAnimation(d.textAnimation);
    d.animationGroup->start();
    connect(d.animationGroup, SIGNAL(finished()), this, SIGNAL(raised()));
    connect(d.parallelAnimationGroup, SIGNAL(finished()), this, SLOT(onQuestionShown()));
    setZValue(10);
}

void FrameItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (d.animationGroup && d.animationGroup->state() == QAbstractAnimation::Stopped) {
        lower();
    } else {
        raise();
    }
}


void FrameItem::lower()
{
    Q_ASSERT(d.animationGroup);
    d.geometryAnimation->setEndValue(static_cast<GraphicsScene*>(scene())->itemGeometry(this));
    d.rotationAnimation->setEndValue(0);
    delete d.textAnimation;
    d.textAnimation = 0;
    connect(d.animationGroup, SIGNAL(finished()), this, SLOT(onLowered()));
    d.animationGroup->start();
    d.state = Lowering;
}

void FrameItem::onLowered()
{
    setZValue(0);
    d.geometryAnimation = 0;
    d.rotationAnimation = 0;
    d.textAnimation = 0;
    d.state = Lowered;
    d.animationGroup->deleteLater();
    d.animationGroup = 0;
    emit lowered();
}

void FrameItem::onQuestionShown()
{
    d.timer.restart();
    d.answerTimer.start(50, this);
}


void FrameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    const QBrush brush = QColor(0x3366ff);
    qDrawShadePanel(painter, option->rect, palette(), false, 5, &brush);
    painter->setPen(Qt::white);
    QString t;
    enum { Margin = 6 } ;
    QRectF r = option->rect.adjusted(Margin, Margin, -Margin, -Margin);
    switch (state()) {
    case Raising:
        if (d.textAnimation->state() == QAbstractAnimation::Running) {
            t = d.textAnimation->currentValue().toString();
            break;
        }
        // fallthrough
    case Lowered:
        t = QString::number(d.value);
        break;
    case Raised:
        r = r.adjusted(0, 0, 0, -painter->fontMetrics().height() + 2);
        // fallthrough
    case Lowering:
        t = d.question;
        break;
    }
    QTextLayout layout(t);
    ::initTextLayout(&layout, r);
    painter->setPen(Qt::white);
    layout.draw(painter, QPointF());
    if (state() == Raised) {
        int msecsLeft = TimeOut - d.timer.elapsed();
        painter->drawText(option->rect, Qt::AlignBottom|Qt::AlignHCenter, QString::number(msecsLeft / 1000.0));
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
    d.raised = 0;
    d.sceneRectChangedBlocked = false;
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
                FrameItem *frame = new FrameItem(row, col, ((row) * 100));
                frame->setQuestion(split.value(0));
                frame->setAnswer(split.value(1));
                connect(frame, SIGNAL(raised()), this, SLOT(onFrameRaised()));
                connect(frame, SIGNAL(lowered()), this, SLOT(onFrameLowered()));
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
            switch (frame->state()) {
            case FrameItem::Lowered:
            case FrameItem::Lowering:
                r = ::itemGeometry(1 + y, x, rows + 1, cols, rect);
                if (frame->state() == FrameItem::Lowered) {
                    frame->setGeometry(r);
                } else {
                    frame->d.geometryAnimation->setEndValue(r);
                }
                break;
            case FrameItem::Raised:
            case FrameItem::Raising:
                r = ::raisedGeometry(rect);
                if (frame->state() == FrameItem::Raised) {
                    frame->setGeometry(r);
                } else {
                    frame->d.geometryAnimation->setEndValue(r);
                }
                break;
            }
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
