#ifndef ITEMS_H
#define ITEMS_H

#include <QtGui>

class GraphicsScene;
class Item : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
public:
    Item();
    enum { Type = QGraphicsItem::UserType + 1 };
    virtual int type() const { return Type; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    qreal yRotation() const;
    void setYRotation(qreal yy);
    void setText(const QString &text);
    QString text() const;
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
    void setColor(const QColor &color);
    QColor color() const;
    GraphicsScene *graphicsScene() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void setAcceptHoverEvents(bool enabled); // override
signals:
    void clicked(Item *item, const QPointF &scenePos);
private:
    struct Data {
        QString text;
        qreal yRotation;
        bool hovered;
        QColor backgroundColor, color;
    } d;
//    friend class GraphicsScene;
};

class Frame : public Item
{
    Q_OBJECT
public:
    enum Status {
        Hidden,
        Failed,
        Succeeded
    };

    Frame(int row, int column);
    enum { Type = QGraphicsItem::UserType + 2 };
    virtual int type() const { return Type; }

    Status status() const { return d.status; }
    void setStatus(Status status) { d.status = status; }

    int row() const { return d.row; }
    int column() const { return d.column; }

    void setValue(int value) { d.value = value; d.valueString = QString("$%1").arg(d.value); }
    int value() const { return d.value; }
    QString valueString() const { return d.valueString; }

    QString question() const { return d.question; }
    void setQuestion(const QString &question) { d.question = question; }
    QString answer() const { return d.answer; }
    void setAnswer(const QString &answer) { d.answer = answer; }

private:
    struct Data {
        QString question, answer, valueString;
        int value;
        int row, column;
        Status status;
    } d;
};

class Team : public Item
{
    Q_OBJECT
    Q_PROPERTY(int points READ points WRITE setPoints)
public:
    enum { Type = QGraphicsItem::UserType + 3 };
    virtual int type() const { return Type; }
    Team(const QString &name) : Item() { d.points = 0; setObjectName(name); updatePoints(); }

    int points() const { return d.points; }
    void setPoints(int points) { d.points = points; updatePoints(); }
    void addPoints(int points) { d.points += points; updatePoints(); }
private:
    void updatePoints() { setText(QString("%1 $%2").arg(objectName()).arg(points())); }
    struct Data {
        int points;
    } d;
};

class Proxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal yRotation READ yRotation WRITE setYRotation)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(bool activeFrame READ hasActiveFrame WRITE setActiveFrame)
public:
    Proxy(QObject *parent = 0) : QObject(parent) { d.activeFrame = 0; }

    qreal yRotation() const { return d.activeFrame ? d.activeFrame->yRotation() : qreal(0.0); }
    void setYRotation(qreal yy) { if (d.activeFrame) d.activeFrame->setYRotation(yy); }

    QString text() const { return d.activeFrame ? d.activeFrame->text() : QString(); }
    void setText(const QString &tt) { if (d.activeFrame) d.activeFrame->setText(tt); }

    QColor backgroundColor() const { return d.activeFrame ? d.activeFrame->backgroundColor() : QColor(); }
    void setBackgroundColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setBackgroundColor(tt); }

    QColor color() const { return d.activeFrame ? d.activeFrame->color() : QColor(); }
    void setColor(const QColor &tt) { if (d.activeFrame) d.activeFrame->setColor(tt); }

    QRectF geometry() const { return d.activeFrame ? d.activeFrame->geometry() : QRectF(); }
    void setGeometry(const QRectF &tt) { if (d.activeFrame) d.activeFrame->setGeometry(tt); }

    Frame *activeFrame() const { return d.activeFrame; }
    void setActiveFrame(Frame *frame)
    {
        if (d.activeFrame) {
            d.activeFrame->setZValue(0);
        }
        d.activeFrame = frame;
        if (d.activeFrame) {
            d.activeFrame->setZValue(10);
        }
    }

    bool hasActiveFrame() const { return d.activeFrame; }
    void setActiveFrame(bool on) { Q_ASSERT(!on); Q_UNUSED(on); setActiveFrame(static_cast<Frame*>(0)); }
private:
    struct Data {
        Frame *activeFrame;
    } d;
};

class TeamProxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    TeamProxy(GraphicsScene *scene, QObject *parent = 0)
        : QObject(parent)
    { d.scene = scene; d.activeTeam = 0; d.orientation = Qt::Horizontal; }
    void setTeams(const QList<Team*> &teams) { d.teams = teams; }
    qreal opacity() const { return d.teams.isEmpty() ? 0.0 : d.teams.at(0)->opacity(); }
    void setOpacity(qreal opacity) { foreach(Item *team, d.teams) team->setOpacity(opacity); }
    QRectF geometry() const { return d.geometry; }
    void setGeometry(const QRectF &geometry);
    QColor backgroundColor() const { return d.activeTeam ? d.activeTeam->backgroundColor() : QColor(); }
    void setBackgroundColor(const QColor &tt) { if (d.activeTeam) d.activeTeam->setBackgroundColor(tt); }
    QColor color() const { return d.activeTeam ? d.activeTeam->color() : QColor(); }
    void setColor(const QColor &tt) { Q_ASSERT(d.activeTeam); if (d.activeTeam) { d.activeTeam->setColor(tt); } else { qDebug() << "tried to set it to" << tt; } }
    void setActiveTeam(Team *team) { d.activeTeam = team; }
    Team *activeTeam() const { return d.activeTeam; }
    Qt::Orientation orientation() const { return d.orientation; }
    void setOrientation(Qt::Orientation orientation) { d.orientation = orientation; }
private:
    struct Data {
        Qt::Orientation orientation;
        GraphicsScene *scene;
        QList<Team*> teams;
        Team *activeTeam;
        QRectF geometry;
    } d;
};

class SelectorItem : public QGraphicsWidget
{
public:
    SelectorItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
};

#endif
