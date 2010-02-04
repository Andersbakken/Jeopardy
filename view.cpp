#include "view.h"
#include "scene.h"

GraphicsView::GraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setBackgroundBrush(Qt::red);
    d.scene = 0;
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QAction *action = new QAction(tr("&Quit"), this);
    action->setShortcut(QKeySequence::Quit);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(close()));
    addAction(action);

    action = new QAction(tr("&New game"), this);
    action->setShortcut(QKeySequence::New);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(newGame()));
    addAction(action);
}

void GraphicsView::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    if (scene())
        scene()->setSceneRect(rect());
}
QSize GraphicsView::sizeHint() const
{
    return QSize(800, 600);
}

void GraphicsView::newGame()
{
    QSettings settings("AndersSoft", "Jeopardy");
    const QString directory = settings.value("lastDirectory", QCoreApplication::applicationDirPath()).toString();
    const QString file = QFileDialog::getOpenFileName(this, "Choose game", directory);
    if (QFile::exists(file)) {
        settings.setValue("lastDirectory", QFileInfo(file).absolutePath());
        GraphicsScene *scene = new GraphicsScene(this);
        if (scene->load(file)) {
            setBackgroundBrush(QBrush());
            delete d.scene;
            d.scene = scene;
            setScene(scene);
            d.scene->setSceneRect(rect());
        }
    }
}

void GraphicsView::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!d.scene) {
        d.scene = new GraphicsScene(this);
        d.scene->load(":/questions.txt", QStringList() << "Team 1" << "Team 2");
        setBackgroundBrush(QBrush());
        d.scene->setSceneRect(rect());
        setScene(d.scene);
    } else {
        QGraphicsView::mouseDoubleClickEvent(e);
    }
}
