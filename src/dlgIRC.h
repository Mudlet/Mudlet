#ifndef DLGIRC_H
#define DLGIRC_H

#include <QMainWindow>
#include "ui_irc.h"

#ifdef Q_CC_MSVC
    #define IRC_SHARED
    #include <ircsession.h>
#else
    #include "irc/include/ircsession.h"
#endif


class dlgIRC : public QMainWindow, public Ui::irc_dlg
{
  Q_OBJECT

public:
    dlgIRC();
    Irc::Session* session;
    QString mNick;

public slots:
    void irc_gotMsg( QString, QString, QString );
    void irc_gotMsg2( QString a, QStringList c );
    void irc_gotMsg3( QString a, uint code, QStringList c );
    void anchorClicked(const QUrl& link);
    void slot_joined(QString, QString);
    void slot_parted(QString, QString, QString);
    void sendMsg();

};

#endif // DLGIRC_H
