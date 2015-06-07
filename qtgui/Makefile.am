


# NOTE: it is not good practice to add "-Wall" and "-03" etc here.  Those
# choices should be made by the user when they invoke the configure script.
AM_CPPFLAGS = -I$(top_srcdir)/liblog2mem -Wall  -D_REENTRANT  -fPIE -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I/usr/include/qt5 -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui -I/usr/include/qt5/QtCore  -L../liblog2mem -llog2mem

AM_LDFLAGS=-pthread -lQt5Widgets -lQt5Gui -lQt5Core -lGL

LDADD = -L../liblog2mem -llog2mem

bin_PROGRAMS=log2memgui
#noinst_PROGRAMS=server_demo

# link statically for the dump tool, so that it is easier to copy around
#log2memgui_LDFLAGS= -L../liblog2mem -llog2mem
log2memgui_SOURCES=main.cpp qrc_resources.cc appman.cpp mainwindow.cpp tablemodel.cpp moc_mainwindow.cpp moc_tablemodel.cpp moc_appman.cpp

#main.cc

# log2memgui_SOURCES=main.cc \
# 	mainwindow.cc	\
# 	moc_mainwindow.cc 	\
# 	moc_tablemodel.cc \
# 	qrc_resources.cc \
# 	tablemodel.cc


# server_dem
#server_demo_SOURCES=server_demo.cc