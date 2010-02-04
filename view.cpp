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

    action = new QAction(tr("&Create game"), this);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(createGame()));

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

class GameDialog : public QDialog
{
    Q_OBJECT
public:
    GameDialog(QWidget *parent)
        : QDialog(parent)
    {
        QGridLayout *layout = new QGridLayout(this);
        QLabel *lbl = new QLabel(tr("&Name"));
        layout->addWidget(lbl, 0, 0);
        d.name = new QLineEdit;
        lbl->setBuddy(d.name);
        connect(d.name, SIGNAL(textChanged(QString)), this, SLOT(updateOk()));
        layout->addWidget(d.name, 0, 1, 1, 5);

        for (int i=0; i<Rows; ++i) {
            if (i == 0) {
                layout->addWidget(new QLabel(tr("Categories")), 1, 0);
            } else {
                layout->addWidget(new QLabel(QString("$%1").arg(i * 100)), i + 1, 0);
            }
            for (int j=0; j<Columns; ++j) {
                QLineEdit *edit = new QLineEdit;
                d.edits[i][j] = edit;
                connect(edit, SIGNAL(textChanged(QString)), this, SLOT(updateOk()));
                layout->addWidget(edit, i + 1, j + 1);
            }
        }
        d.buttonBox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel, Qt::Horizontal, this);
        connect(d.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(d.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
        layout->addWidget(d.buttonBox, layout->rowCount(), 0, 1, layout->columnCount());
        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);

    }
public slots:
    void updateOk()
    {
        if (d.name->text().isEmpty()) {
            d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
            return;
        }
        bool hasCategory = false;
        for (int i=0; i<Columns; ++i) {
            bool found = false;
            for (int j=0; j<Rows; ++j) {
                const bool empty = d.edits[j][i]->text().isEmpty();
                if (found && empty) {
                    d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
                    return;
                } else if (!empty && !found && j > 0) {
                    d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
                    return;
                } else if (!empty) {
                    found = true;
                }
            }
        }
        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(hasCategory);
    }
private:
    enum { Rows = 6, Columns = 6 };
    struct Data {
        QLineEdit *name;
        QDialogButtonBox *buttonBox;
        QLineEdit *edits[Rows][Columns];
    } d;
};

#include "view.moc"

void GraphicsView::createGame()
{
    GameDialog dlg(this);
    dlg.exec();
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
