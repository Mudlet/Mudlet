#pragma once

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "Telnet.h"

#include "Java.h"

#include "pre_guard.h"
#include <QColor>
#include <QFile>
#include <QMutex>
#include <QFont>
#include <QPointer>
#include <QSettings>
#include <QTextStream>
#include "post_guard.h"

class QDialog;
class QPushButton;
class QListWidget;
class Console;

void setupDefaultSettings();


class Profile  : public QObject
{
public:
    static QString DICTIONARY;
    static QString COMMAND_CHARACTER;

    static QString WINDOW_FONT_FAMILY;
    static QString WINDOW_FONT_SIZE;

    static QString WINDOW_WIDTH;
    static QString WINDOW_HEIGHT;
    static QString WINDOW_WRAP;
    static QString WINDOW_WRAP_INDENT;

    static QString CMD_LINE_FONT_FAMILY;
    static QString CMD_LINE_FONT_SIZE;

    static QString CMD_LINE_CLEAR;
    static QString CMD_LINE_FG_COLOR;
    static QString CMD_LINE_BG_COLOR;

    static QSettings DEFAULT_SETTINGS;


public:

    Profile( int port, const QString& mHostName, const QString& login, const QString& pass );
    ~Profile();

    QString             getDictionary();

    QFont               getWindowFont();
    int                 getWindowWrap();
    int                 getWindowHeight();
    int                 getWindowWidth();
    int                 getWindowWrapIndent();
    QFont               getCmdLineFont();
    QString             getId();
    void                setId( const QString & );
    QString             getString(const QString &);
    int                 getInt(const QString &);
    bool                getBool(const QString &);

    void                setBool(const QString &, bool);
    void                setString(const QString &, const QString &);
    void                setInt(const QString &, int);

    void                connectToServer();
    void                send( QString cmd );
    void                sendRaw( QString s );

    void                incomingStreamProcessor(const QString & paragraph, int line );
    void                enableKey(const QString & );
    void                disableKey(const QString & );

    bool                isClosed();
    void                close();

    void                resetProfile();
    class               Exception_NoLogin{};
    class               Exception_NoConnectionAvailable{};

    bool                removeDir( const QString& dirName, const QString& originalPath);
    void                adjustNAWS();

    void                load();
    void                save();

    Telnet             mTelnet;
    QPointer<Console>  console;

    QColor              mBlack;
    QColor              mLightBlack;
    QColor              mRed;
    QColor              mLightRed;
    QColor              mLightGreen;
    QColor              mGreen;
    QColor              mLightBlue;
    QColor              mBlue;
    QColor              mLightYellow;
    QColor              mYellow;
    QColor              mLightCyan;
    QColor              mCyan;
    QColor              mLightMagenta;
    QColor              mMagenta;
    QColor              mLightWhite;
    QColor              mWhite;
    QColor              mFgColor;
    QColor              mBgColor;
    QColor              mCommandBgColor;
    QColor              mCommandFgColor;

    QColor              mBlack_2;
    QColor              mLightBlack_2;
    QColor              mRed_2;
    QColor              mLightRed_2;
    QColor              mLightGreen_2;
    QColor              mGreen_2;
    QColor              mLightBlue_2;
    QColor              mBlue_2;
    QColor              mLightYellow_2;
    QColor              mYellow_2;
    QColor              mLightCyan_2;
    QColor              mCyan_2;
    QColor              mLightMagenta_2;
    QColor              mMagenta_2;
    QColor              mLightWhite_2;
    QColor              mWhite_2;
    QColor              mFgColor_2;
    QColor              mBgColor_2;

private:
    void                copySettings(QSettings &,QSettings&);

    QFont               getFont(const char *);

    Java *              java;
    bool                closed = false;
    QString             id;
    QMutex              lock;

    QSettings           settings;
    QFont               windowFont;
    QFont               cmdLineFont;

};
