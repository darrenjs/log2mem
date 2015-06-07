#include "appman.h"
#include "mainwindow.h"

AppMan::AppMan(int argc, char *argv[])
  : QApplication(argc, argv),
    m_mainwindow(NULL)
{

  m_mainwindow = new MainWindow(this);
  this->setOrganizationName("QtProject");
  this->setApplicationName("Application Example");
}

//----------------------------------------------------------------------
AppMan::~AppMan()
{
}

//----------------------------------------------------------------------
int AppMan::run()
{
  m_mainwindow->show();
  return this->exec();
}
