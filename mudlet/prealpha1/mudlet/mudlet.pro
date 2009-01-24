QT += webkit
unix:LIBS += -lqscintilla2 -L/usr/lib
win32:LIBS += c:\mylibs\math.lib
SUBDIRS += src
TEMPLATE = subdirs 
CONFIG += uic-qt4 \
          qt \
          thread 
