#ifndef APPMAN_H
#define APPMAN_H

#include <QApplication>

class MainWindow;

class AppMan : public QApplication
{
    Q_OBJECT
  public:
    AppMan(int argc, char *argv[]);
    ~AppMan();

    int run();

  private:

      MainWindow * m_mainwindow;
};

#endif // APPMAN_H
