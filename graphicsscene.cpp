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
    d.value = value;
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

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(onSceneRectChanged(QRectF)));
    static const char *topics[] = { "Ancient greece", "Formulas", "Things that happen", "What?", "Nah", 0 };
    static const int values[] = { 100, 200, 300, 400, 500, -1 };
    d.layout = new QGraphicsGridLayout;
    d.layout->setHorizontalSpacing(0);
    d.layout->setVerticalSpacing(0);
    for (int i=0; topics[i]; ++i) {
        d.topicItems.append(new TopicItem(topics[i]));
        addItem(d.topicItems.at(i));
        d.layout->addItem(d.topicItems[i], 0, i);
        QList<FrameItem*> frames;
        for (int j=0; values[j] != -1; ++j) {
            frames.append(new FrameItem(values[j]));
            addItem(frames.at(j));
            d.layout->addItem(frames.at(j), j + 1, i);
        }
        d.frameItems.append(frames);

    }
}

bool GraphicsScene::load(QIODevice *device)
{
    clear();
    d.topicItems.clear();
    d.frameItems.clear();
}

void GraphicsScene::onSceneRectChanged(const QRectF &rect)
{
    d.layout->setGeometry(rect);
    qDebug() << rect;
}
