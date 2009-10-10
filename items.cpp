#include "graphicsscene.h"
#include "items.h"

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
    setAcceptHoverEvents(true);
    setCacheMode(ItemCoordinateCache);
    d.yRotation = 0;
    d.hovered = false;
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

QColor Item::color() const
{
    return d.color;
}

void Item::setColor(const QColor &color)
{
    d.color = color;
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

void Item::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    d.hovered = true;
    update();
}

void Item::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    d.hovered = false;
    update();
}



void Item::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
//    const QTransform &worldTransform = painter->worldTransform();
//     bool mirrored = false;
//     if (worldTransform.m11() < 0 || worldTransform.m22() < 0) {
//         mirrored = true;
//         brush = Qt::black;
//     }
    QBrush brush = d.hovered ? d.color : d.backgroundColor;
    enum { Margin = 5 };
    qDrawShadePanel(painter, option->rect, palette(), false, Margin, &brush);
    draw(painter, option->rect);
    Q_ASSERT(d.color.isValid());
    painter->setPen(d.color);
    const QRectF r = option->rect.adjusted(Margin, Margin, -Margin, -Margin);
    QTextLayout layout(d.text);
    ::initTextLayout(&layout, r, r.height() / 5);
    painter->setPen(d.hovered ? d.backgroundColor : d.color);
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

void TeamProxy::setRect(const QRectF &rect)
{
    d.rect = rect;
    d.scene->setTeamGeometry(rect);
}
