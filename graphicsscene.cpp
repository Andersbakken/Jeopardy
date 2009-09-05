#include "graphicsscene.h"

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
    int pixelSize = option->rect.height() / 3;
    painter->setPen(Qt::white);
    forever {
        QFont f;
        f.setPixelSize(pixelSize--);
        QTextLayout layout(d.text, f);
        layout.setCacheEnabled(true);
        layout.beginLayout();
        const int h = QFontMetrics(f).height();
        QPointF pos(r.topLeft());
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(r.width());
            line.setPosition(pos);
            pos += QPointF(0, h);
        }
        layout.endLayout();
        QRectF textRect = layout.boundingRect();
        if (pixelSize <= 8 || r.size().expandedTo(textRect.size()) == r.size()) {
            layout.draw(painter, QPointF());
            // ### center text?
            break;
        }
    }
}

FrameItem::FrameItem(int value)
{
    setCacheMode(ItemCoordinateCache);
    d.animationGroup = 0;
    d.geometryAnimation = 0;
    d.value = value;
    d.yRotation = 0;
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
    d.animationGroup = new QParallelAnimationGroup;
    d.geometryAnimation = new QPropertyAnimation;
    d.geometryAnimation->setDuration(1000);
    d.geometryAnimation->setTargetObject(this);
    d.geometryAnimation->setPropertyName("geometry");
    QRectF r = scene()->sceneRect();
    static qreal adjust = .1;
    d.geometryAnimation->setEndValue(r.adjusted(r.width() * adjust, r.height() * adjust,
                                                -r.width() * adjust, -r.height() * adjust));
    d.rotationAnimation = new QPropertyAnimation;
    d.rotationAnimation->setDuration(1000);
    d.rotationAnimation->setTargetObject(this);
    d.rotationAnimation->setPropertyName("yRotation");
    d.rotationAnimation->setEndValue(360);

    setProperty("originalGeometry", geometry());
    d.animationGroup->addAnimation(d.geometryAnimation);
    d.animationGroup->addAnimation(d.rotationAnimation);
    d.animationGroup->start();
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
    d.geometryAnimation->setEndValue(property("originalGeometry"));
    d.rotationAnimation->setEndValue(0);
    connect(d.animationGroup, SIGNAL(finished()), this, SLOT(onLowered()));
    setProperty("originalGeometry", QVariant());
    d.animationGroup->start();
}

void FrameItem::onLowered()
{
    setZValue(0);
    delete d.animationGroup;
    d.animationGroup = 0;
    d.geometryAnimation = 0;
}

void FrameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    QFont f;
    f.setPixelSize(option->rect.height() / 3);
    painter->setFont(f);
    const QBrush brush = QColor(0x3366ff);
    qDrawShadePanel(painter, option->rect, palette(), false, 5, &brush);
    painter->setPen(Qt::white);
    painter->drawText(option->rect, Qt::AlignCenter, QString::number(d.value));
    // ### multi line breaking?
}


QString FrameItem::question() const
{
    return d.question;
}

void FrameItem::setQuestion(const QString &question)
{
    d.question = question;
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
    d.layout = 0;
    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
}

bool GraphicsScene::load(QIODevice *device)
{
    reset();
    QTextStream ts(device);
    enum State {
        ExpectingTopic,
        ExpectingQuestion
    } state = ExpectingTopic;

    d.layout = new QGraphicsGridLayout;
    d.layout->setHorizontalSpacing(0);
    d.layout->setVerticalSpacing(0);

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
            qDebug() << "creating topic" << line;
            addItem(topic);
            d.frameItems.append(QList<FrameItem*>());
            d.layout->addItem(topic, 0, d.topicItems.size());
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
                const int c = d.frameItems.last().size();
                FrameItem *frame = new FrameItem(((c + 1) * 100));
                addItem(frame);
                d.layout->addItem(frame, c + 1, d.topicItems.size() - 1);
                d.frameItems.last().append(frame);
                if (c == 4)
                    state = ExpectingTopic;
            }
            break;
        }
    }
    return true;
}

void GraphicsScene::onSceneRectChanged(const QRectF &rect)
{
    d.layout->setGeometry(rect);
    qDebug() << rect;
}
void GraphicsScene::reset()
{
    delete d.layout;
    d.layout = 0;
    clear();
    d.topicItems.clear();
    d.frameItems.clear();
}
void GraphicsScene::keyPressEvent(QKeyEvent *e)
{

}
