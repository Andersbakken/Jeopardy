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

    QAction *action = new QAction(tr("&New game"), this);
    action->setShortcut(QKeySequence::New);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(newGame()));
    addAction(action);

    action = new QAction(tr("&Create game"), this);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(createGame()));
    addAction(action);

    action = new QAction(this);
    action->setSeparator(true);
    addAction(action);

    action = new QAction(tr("&Quit"), this);
    action->setShortcut(QKeySequence::Quit);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(close()));
    addAction(action);


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

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    TextEdit()
    {
        setEnabled(false);
        setTabChangesFocus(true);
    }

public slots:
    void onCategoryEditTextChanged(const QString &text)
    {
        setEnabled(!text.isEmpty());
    }
};

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
                layout->addWidget(new QLabel(tr("Categories")), 1, 0, 2, 1);
            } else {
                layout->addWidget(new QLabel(QString("$%1").arg(i * 100)), (i + 1) * 2, 0, 2, 1);
            }
            for (int j=0; j<Columns; ++j) {
                if (i == 0) {
                    QLineEdit *edit = new QLineEdit;
                    edit->setEnabled(false);
                    d.categories[j] = edit;
                    connect(edit, SIGNAL(textChanged(QString)), this, SLOT(updateOk()));
                    layout->addWidget(edit, i + 1, j + 1);
                } else {
                    TextEdit *edit = new TextEdit;
                    static const QLatin1String questionName("question");
                    edit->setObjectName(questionName);
                    d.edits[i - 1][j][Question] = edit;
                    connect(edit, SIGNAL(textChanged()), this, SLOT(updateOk()));
                    connect(d.categories[j], SIGNAL(textChanged(QString)), edit, SLOT(onCategoryEditTextChanged(QString)));
                    layout->addWidget(edit, (i + 1) * 2, j + 1);
                    edit = new TextEdit;
                    static const QLatin1String answerName("answer");
                    edit->setObjectName(answerName);
                    d.edits[i - 1][j][Answer] = edit;
                    connect(edit, SIGNAL(textChanged()), this, SLOT(updateOk()));
                    connect(d.categories[j], SIGNAL(textChanged(QString)), edit, SLOT(onCategoryEditTextChanged(QString)));
                    layout->addWidget(edit, ((i + 1) * 2) + 1, j + 1);
                }
            }
        }
        setStyleSheet("TextEdit:disabled,QLineEdit:disabled { background: darkGray; }"
                      "TextEdit#answer { text: white; background: black; }"
                      "TextEdit#answer:disabled { text: white; background: darkGray; }");

        d.buttonBox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel, Qt::Horizontal, this);
        connect(d.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(d.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
        layout->addWidget(d.buttonBox, layout->rowCount(), 0, 1, layout->columnCount());
        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);

    }
public slots:
    void updateOk()
    {
        bool hasCategory = false;
        const bool hasName = !d.name->text().isEmpty();
        if (hasName) {
            for (int i=0; i<Columns; ++i) {
                bool found = false;
                for (int j=0; j<Rows; ++j) {
#warning gotta do stuff here
//                     const bool empty = d.edits[j][i]->text().isEmpty();
//                     if (found && empty) {
//                         d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
//                         return;
//                     } else if (!empty && !found && j > 0) {
//                         d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
//                         return;
//                     } else if (!empty) {
//                         found = true;
//                     }

                }
            }
        }
        int first = -1;
        for (int i=0; i<Columns; ++i) {
            d.categories[i]->setEnabled(hasName && first == -1);
            if (d.categories[i]->text().isEmpty()) {
                first = i;
            }
        }

        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(hasCategory);
    }
private:
    enum { Rows = 5, Columns = 5, Question = 0, Answer = 1 };
    struct Data {
        QLineEdit *name;
        QDialogButtonBox *buttonBox;
        QLineEdit *categories[Columns];
        TextEdit *edits[Rows][Columns][2];
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
