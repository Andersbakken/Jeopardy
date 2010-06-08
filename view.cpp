#include "view.h"
#include "scene.h"

MainWindow::MainWindow()
    : QMainWindow()
{
    d.view = new GraphicsView(this);
    setCentralWidget(d.view);
    QMenu *menu = menuBar()->addMenu(tr("&File"));
    menu->addActions(d.view->actions());
}

void MainWindow::showEvent(QShowEvent *e)
{
    restoreGeometry(QSettings().value("geometry").toByteArray());
    QMainWindow::showEvent(e);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    QSettings().setValue("geometry", saveGeometry());
    QMainWindow::closeEvent(e);
}

void MainWindow::load(const QString &string, const QStringList &players)
{
    d.view->load(string, players);
}

GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView(parent)
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
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
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
    QSettings settings;
    const QString directory = settings.value("lastDirectory", QCoreApplication::applicationDirPath()).toString();

    const QString file = QFileDialog::getOpenFileName(this, "Choose game", directory, "Games (*.jgm *.js)");
    if (QFile::exists(file)) {
        settings.setValue("lastDirectory", QFileInfo(file).absolutePath());
        load(file);
    }
}

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    TextEdit() : empty(true)
    {
        setEnabled(false);
        setTabChangesFocus(true);
        connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    }

    void keyPressEvent(QKeyEvent *e)
    {
        if (e->text().contains("|")) {
            e->accept();
        } else {
            QTextEdit::keyPressEvent(e);
        }
    }

    void paintEvent(QPaintEvent *e)
    {
        if (empty) {
            QFont font;
            font.setPixelSize(20);
            QRect r = QFontMetrics(font).boundingRect(objectName());
            r.moveCenter(rect().center());
            if (e->rect().intersects(r)) {
                QPainter p(viewport());
                p.setPen(palette().text().color());
                p.drawText(r, Qt::AlignCenter, objectName());
            }
        }
        QTextEdit::paintEvent(e);
    }
public slots:
    void onTextChanged()
    {
        const QString pipe = QLatin1String("|");
        QTextCursor cursor(document());
        const bool old = document()->blockSignals(true);
        forever {
            cursor = document()->find(pipe, cursor);
            if (cursor.isNull())
                break;
            cursor.removeSelectedText();
        }
        document()->blockSignals(old);
        if (empty != document()->isEmpty()) {
            update();
            empty = !empty;
        }
    }

    void onCategoryEditTextChanged(const QString &text)
    {
        setEnabled(!text.isEmpty());
    }
private:
    bool empty;
};

class GameDialog : public QDialog
{
    Q_OBJECT
public:
    GameDialog(QWidget *parent)
        : QDialog(parent)
    {
        d.play = false;
        QGridLayout *layout = new QGridLayout(this);
        QLabel *lbl = new QLabel(tr("&Name"));
        layout->addWidget(lbl, 0, 0);
        d.name = new QLineEdit;
        lbl->setBuddy(d.name);
        connect(d.name, SIGNAL(textChanged(QString)), this, SLOT(updateOk()));
        layout->addWidget(d.name, 0, 1, 1, 5);

        for (int i=0; i<=Rows; ++i) {
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
                    static const QLatin1String questionName("Question");
                    edit->setObjectName(questionName);
                    d.edits[j][i - 1][Question] = edit;
                    connect(edit, SIGNAL(textChanged()), this, SLOT(updateOk()));
                    connect(d.categories[j], SIGNAL(textChanged(QString)), edit, SLOT(onCategoryEditTextChanged(QString)));
                    layout->addWidget(edit, (i + 1) * 2, j + 1);
                    edit = new TextEdit;
                    static const QLatin1String answerName("Answer");
                    edit->setObjectName(answerName);
                    d.edits[j][i - 1][Answer] = edit;
                    connect(edit, SIGNAL(textChanged()), this, SLOT(updateOk()));
                    connect(d.categories[j], SIGNAL(textChanged(QString)), edit, SLOT(onCategoryEditTextChanged(QString)));
                    layout->addWidget(edit, ((i + 1) * 2) + 1, j + 1);
                }
            }
        }
        QWidget *last = d.name;
        for (int i=0; i<Columns; ++i) {
            QWidget::setTabOrder(last, d.categories[i]);
            last = d.categories[i];
            for (int j=0; j<Rows; ++j) {
                QWidget::setTabOrder(last, d.edits[i][j][Question]);
                QWidget::setTabOrder(d.edits[i][j][Question], d.edits[i][j][Answer]);
                last = d.edits[i][j][Answer];
            }
        }

        setStyleSheet("QLineEdit:disabled { background: darkGray; }"
                      "QLineEdit { background: white; }"
                      "TextEdit#Question:disabled { background: darkGray; color: white; }"
                      "TextEdit#Answer { color: white; background: black; }"
                      "TextEdit#Answer:disabled { color: white; background: darkGray; }");

        d.buttonBox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel, Qt::Horizontal, this);
        QWidget::setTabOrder(last, d.buttonBox);
        d.playButton = d.buttonBox->addButton(tr("Play"), QDialogButtonBox::ApplyRole);

        connect(d.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(d.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
        layout->addWidget(d.buttonBox, layout->rowCount(), 0, 1, layout->columnCount());
        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        d.playButton->setEnabled(false);
        connect(d.playButton, SIGNAL(clicked()), this, SLOT(onPlay()));
    }

    void accept()
    {
        QString name = d.name->text() + ".jgm";
        if (QFile::exists(name)) {
            name.insert(name.size() - 4, "_%1");
            int idx = 1;
            while (QFile::exists(name.arg(idx)))
                ++idx;
            name = name.arg(idx);
        }
        QFile file(name);
        file.open(QIODevice::WriteOnly);
        save(&file);
        if (d.play) {
            d.file = name;
        }
        qDebug() << d.file;
        QDialog::accept();
    }

    QString file() const
    {
        return d.file;
    }

    void save(QIODevice *device)
    {
        QTextStream ts(device);
        for (int i=0; i<Columns; ++i) {
            Q_ASSERT(isValid(i));
            ts << d.categories[i]->text() << endl;
            for (int j=0; j<Rows; ++j) {
                ts << d.edits[i][j][Question]->toPlainText().replace(QLatin1Char('\n'), QLatin1Char(' ')).simplified()
                   << QLatin1Char('|')
                   << d.edits[i][j][Answer]->toPlainText().replace(QLatin1Char('\n'), QLatin1Char(' ')).simplified()
                   << endl;
            }
            if (!isValid(i + 1))
                break;
            if (i + 1 < Columns)
                ts << endl;
        }
    }

    bool isValid(int column, bool *isEmpty = 0) const
    {
        if (d.categories[column]->text().isEmpty()) {
            if (isEmpty)
                *isEmpty = true;
            return false;
        }
        if (isEmpty)
            *isEmpty = false;
        for (int i=0; i<Rows; ++i) {
            if (d.edits[column][i][Question]->document()->isEmpty()
                || d.edits[column][i][Answer]->document()->isEmpty())
                return false;
        }
        return true;
    }
public slots:
    void onPlay()
    {
        d.play = true;
        accept();
    }
    void updateOk()
    {
        const bool hasName = !d.name->text().isEmpty();
        bool hasCategory = false;
        bool invalid = false;
        if (hasName) {
            for (int i=0; i<Columns; ++i) {
                bool empty;
                if (isValid(i, &empty)) {
                    hasCategory = true;
                } else {
                    if (!empty)
                        invalid = true;
                    break;
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

        d.playButton->setEnabled(hasCategory && !invalid);
        d.buttonBox->button(QDialogButtonBox::Save)->setEnabled(hasCategory && !invalid);
    }
private:
    enum { Rows = 5, Columns = 5, Question = 0, Answer = 1 };
    struct Data {
        QLineEdit *name;
        QDialogButtonBox *buttonBox;
        QPushButton *playButton;
        QLineEdit *categories[Columns];
        TextEdit *edits[Columns][Rows][2];
        QString file;
        bool play;
    } d;
};

#include "view.moc"

void GraphicsView::createGame()
{
    GameDialog dlg(this);
    if (dlg.exec() && !dlg.file().isEmpty()) {
        load(dlg.file());
    }
}

void GraphicsView::load(const QString &fileName, const QStringList &players)
{
    GraphicsScene *scene = new GraphicsScene(this);
    if (scene->load(fileName, players)) {
        setBackgroundBrush(QBrush());
        delete d.scene;
        d.scene = scene;
        d.scene->setSceneRect(rect());
        setScene(scene);
    } else {
        delete scene;
    }
}

