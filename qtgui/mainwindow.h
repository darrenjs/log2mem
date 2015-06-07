#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QPlainTextEdit;
class QTableView;
class QAbstractItemModel;
QT_END_NAMESPACE

class TableModel;
class AppMan;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppMan*);
    ~MainWindow();

private slots:
//    void newFile();
    void open();
    void slot_refresh();
//    bool save();
//    bool saveAs();
    void about();
//    void documentWasModified();

private:
//    Ui::MainWindow *ui;

    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *helpMenu;
    QAction *openAct;
    QAction *refreshAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QPlainTextEdit* m_textEdit;
    QTableView* m_treeView;
    TableModel* m_model;

    AppMan*  m_appman;
};

#endif // MAINWINDOW_H
