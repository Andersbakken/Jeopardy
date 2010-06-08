#include "scene.h"
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

Item::Item()
{
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

void Item::setAcceptHoverEvents(bool enabled) // override
{
    if (!enabled && d.hovered) {
        d.hovered = false;
        update();
    }
    QGraphicsWidget::setAcceptHoverEvents(enabled);
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
    setAcceptHoverEvents(true);
    d.status = Hidden;
    d.row = row;
    d.column = column;
    d.value = 0;
}

void TeamProxy::setGeometry(const QRectF &geometry)
{
    d.geometry = geometry;
    d.scene->setTeamGeometry(geometry, d.orientation);
}

static inline int _q_interpolate(int f, int t, qreal progress)
{
    return int(qreal(f) + (qreal(t) - qreal(f)) * progress);

}

static inline QColor _q_interpolate(const QColor &f, const QColor &t, qreal progress)
{
    return QColor(_q_interpolate(f.red(), t.red(), progress),
                  _q_interpolate(f.green(), t.green(), progress),
                  _q_interpolate(f.blue(), t.blue(), progress),
                  _q_interpolate(f.alpha(), t.alpha(), progress));
}

QVariant Item::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemVisibleChange && !value.toBool()) {
        d.hovered = false;
    }
    return QGraphicsWidget::itemChange(change, value);
}

#ifdef QT_DEBUG
void Frame::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Item::paint(painter, option, widget);
    static const bool showAll = QCoreApplication::arguments().contains("--show-all");
    if (showAll) {
        painter->save();
        const QRect rect = option->rect.adjusted(10, 10, -10, -10);
        QFont font;
        font.setPixelSize(15);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignLeft|Qt::AlignTop, d.question);
        painter->drawText(rect, Qt::AlignLeft|Qt::AlignBottom, d.answer);
        painter->restore();
    }
}
#endif
