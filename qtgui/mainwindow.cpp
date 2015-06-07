#include "mainwindow.h"
#include "tablemodel.h"

#include <QtWidgets>
#include <QFile>
#include <QTextStream>

#include <iostream>

//----------------------------------------------------------------------
MainWindow::MainWindow(AppMan* appman)
  : QMainWindow(NULL),
    m_model(NULL),
    m_appman(appman)
{
  m_treeView = new QTableView();

  m_model = new TableModel(m_appman);

  m_treeView->setModel(m_model);
  m_treeView->resizeColumnsToContents();
  m_treeView->verticalHeader()->hide();

  QStringList headers;
  headers << tr("Title") << tr("Description");
  m_textEdit = new QPlainTextEdit();

//  setCentralWidget(m_textEdit);
  setCentralWidget(m_treeView);

  createActions();
  createMenus();
  createStatusBar();
}

//----------------------------------------------------------------------
MainWindow::~MainWindow()
{
}

//----------------------------------------------------------------------
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(refreshAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);


    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

//----------------------------------------------------------------------
void MainWindow::createActions()

{
    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));


    refreshAct = new QAction(QIcon(":/images/open.png"), tr("&Refresh"), this);
//    refreshAct->setShortcuts(QKeySequence::Open);
    refreshAct->setStatusTip(tr("Refresh"));
    connect(refreshAct,
            SIGNAL(triggered()),
            this,
            SLOT(slot_refresh()));


    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct,
            SIGNAL( triggered() ),
            this,
            SLOT( close() )
      );


    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));

    // TODO: DJS: put back
    //connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    // TODO: DJS: put back
    // connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));

    // TODO: DJS: put back
    // connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));


    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));


    cutAct->setEnabled(false);
    copyAct->setEnabled(false);

    // TODO: DJS: put back
    // connect(textEdit, SIGNAL(copyAvailable(bool)),
    //         cutAct, SLOT(setEnabled(bool)));
    // connect(textEdit, SIGNAL(copyAvailable(bool)),
    //         copyAct, SLOT(setEnabled(bool)));
}

//----------------------------------------------------------------------
void MainWindow::about()

{
   QMessageBox::about(this, tr("About Application"),
            tr("The <b>Application</b> example demonstrates how to "
               "write modern GUI applications using Qt, with a menu bar, "
               "toolbars, and a status bar."));
}

//----------------------------------------------------------------------
void MainWindow::open()
{
  QString fileName = QFileDialog::getOpenFileName(this);
//  if (!fileName.isEmpty())
//    loadFile(fileName);
}

//----------------------------------------------------------------------
void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

//----------------------------------------------------------------------
void MainWindow::slot_refresh()
{
  m_model->refresh();
}
