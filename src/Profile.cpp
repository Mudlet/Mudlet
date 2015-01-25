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


#include <QPainter>

#include "Profile.h"


#include "MainWindow.h"
#include "TConsole.h"
#include "TEvent.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <QDir>
#include <QMessageBox>
#include "post_guard.h"

#include "zip.h"
#include "zipconf.h"

#include <errno.h>

QString Profile::DICTIONARY("dictionary");

QString Profile::WINDOW_WRAP("window.wrap");
QString Profile::WINDOW_WRAP_INDENT("window.wrap.indent");
QString Profile::WINDOW_WIDTH("window.width");
QString Profile::WINDOW_HEIGHT("window.width");

QString Profile::WINDOW_FONT_FAMILY("window.font.family");
QString Profile::WINDOW_FONT_SIZE("window.font.size");

QString Profile::CMD_LINE_FONT_FAMILY("cmdLine.font.family");
QString Profile::CMD_LINE_FONT_SIZE("cmdLine.font.size");

QString Profile::CMD_LINE_CLEAR("cmdLine.autoClear");
QString Profile::CMD_LINE_FG_COLOR("cmdLine.fgColor");
QString Profile::CMD_LINE_BG_COLOR("cmdLine.bgColor");

QSettings Profile::DEFAULT_SETTINGS;

void setupDefaultSettings() {
    Profile::DEFAULT_SETTINGS.setValue(Profile::DICTIONARY,"en_US");

    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_FONT_FAMILY,"Monospace");
    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_FONT_SIZE,"12");

    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_WIDTH,"1000");
    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_HEIGHT,"800");
    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_WRAP,"120");
    Profile::DEFAULT_SETTINGS.setValue(Profile::WINDOW_WRAP_INDENT,"0");

    Profile::DEFAULT_SETTINGS.setValue(Profile::CMD_LINE_FONT_FAMILY,"Monospace");
    Profile::DEFAULT_SETTINGS.setValue(Profile::CMD_LINE_FONT_SIZE,"14");

    Profile::DEFAULT_SETTINGS.setValue(Profile::CMD_LINE_CLEAR,false);
    Profile::DEFAULT_SETTINGS.setValue(Profile::CMD_LINE_FG_COLOR,"#FFF");
    Profile::DEFAULT_SETTINGS.setValue(Profile::CMD_LINE_BG_COLOR,"#000");
}

Profile::Profile( int port, const QString& hostname, const QString& login, const QString& pass )
: mTelnet( this )
, console( 0 )
, mKeyUnit           ( this )
, mBlack             (Qt::black)
, mLightBlack        (Qt::darkGray)
, mRed               (Qt::darkRed)
, mLightRed          (Qt::red)
, mLightGreen        (Qt::green)
, mGreen             (Qt::darkGreen)
, mLightBlue         (Qt::blue)
, mBlue              (Qt::darkBlue)
, mLightYellow       (Qt::yellow)
, mYellow            (Qt::darkYellow)
, mLightCyan         (Qt::cyan)
, mCyan              (Qt::darkCyan)
, mLightMagenta      (Qt::magenta)
, mMagenta           (Qt::darkMagenta)
, mLightWhite        (Qt::white)
, mWhite             (Qt::lightGray)
, mFgColor           (Qt::lightGray)
, mBgColor           (Qt::black)
, mCommandBgColor    (Qt::black)
, mCommandFgColor    (QColor(113, 113, 0))
, mBlack_2             (Qt::black)
, mLightBlack_2        (Qt::darkGray)
, mRed_2               (Qt::darkRed)
, mLightRed_2          (Qt::red)
, mLightGreen_2        (Qt::green)
, mGreen_2             (Qt::darkGreen)
, mLightBlue_2         (Qt::blue)
, mBlue_2              (Qt::darkBlue)
, mLightYellow_2       (Qt::yellow)
, mYellow_2            (Qt::darkYellow)
, mLightCyan_2         (Qt::cyan)
, mCyan_2              (Qt::darkCyan)
, mLightMagenta_2      (Qt::magenta)
, mMagenta_2           (Qt::darkMagenta)
, mLightWhite_2        (Qt::white)
, mWhite_2             (Qt::lightGray)
, mFgColor_2           (Qt::lightGray)
, mBgColor_2           (Qt::black)
{
    auto keys = Profile::DEFAULT_SETTINGS.allKeys();

    // copy default settings
    for(auto key : keys) {
        auto value = Profile::DEFAULT_SETTINGS.value(key).toString();
        settings.setValue(key,value);
    }

    qDebug() << "host made!";
}

Profile::~Profile()
{
    close();
}

QFont Profile::getWindowFont() {
    return getFont("window");
}

QString Profile::getDictionary() {
    return getString(Profile::DICTIONARY);
}

int Profile::getWindowHeight() {
    return getInt(Profile::WINDOW_HEIGHT);
}

int Profile::getWindowWidth() {
    return getInt(Profile::WINDOW_WIDTH);
}

int Profile::getWindowWrap() {
    return getInt(Profile::WINDOW_WRAP);
}

int Profile::getWindowWrapIndent() {
    return getInt(Profile::WINDOW_WRAP_INDENT);
}


QFont Profile::getFont(const char *ch) {
    QString domain(ch);
    QString family = getString(domain + ".font.family");
    qDebug() <<"font family=="<<family;
    QFont font(getString(domain + ".font.family"));

    qDebug() <<"font family=="<<font.family();

    if(!QFontInfo(font).fixedPitch()) {
        qDebug() << "setting style hint to monospace";
        font.setStyleHint(QFont::Monospace);
    }

    if(!QFontInfo(font).fixedPitch()) {
        qDebug() << "setting style hint to typewriter";
        font.setStyleHint(QFont::TypeWriter);
    }

    int pixelSize = getInt(domain + ".font.size");
    qDebug() << "setting pixel size to "<< pixelSize;
    font.setPixelSize(pixelSize);
    qDebug() << "setting pixel size to "<< font.pixelSize();
    qDebug() << QFontMetrics(font).height();

    /*QFont temp(font);
    QPixmap pixmap = QPixmap( 2000, 600 );
    QPainter p(&pixmap);
    temp.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    p.setFont(temp);
    const QRectF r = QRectF(0,0,2000, 600);

    QRectF r2;
    const QString t = "123";
    p.drawText(r,1,t,&r2);

    int width = QFontMetrics( font ).width( QChar('W') );
    qreal letterSpacing = (qreal)((qreal)width-(qreal)(r2.width()/t.size()));

    font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);*/

    return font;
}

QFont Profile::getCmdLineFont() {

    return getFont("cmdLine");
}

void Profile::resetProfile()
{

    console->resetMainConsole();

    getKeyUnit()->compileAll();

    TEvent event;
    event.mArgumentList.append( "sysLoadEvent" );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    // TODO add event
    qDebug()<<"resetProfile() DONE";
}

void Profile::adjustNAWS()
{
    mTelnet.setDisplayDimensions();
}


void Profile::send( QString cmd )
{
        console->printCommand( cmd ); // used to print the terminal <LF> that terminates a telnet command
                                        // this is important to get the cursor position right
    console->update();
    QStringList commandList = cmd.split( QString( ";" ), QString::SkipEmptyParts );

    for( int i=0; i<commandList.size(); i++ )
    {
        if( commandList[i].size() < 1 ) continue;
        QString command = commandList[i];
        command.replace("\n", "");
        mTelnet.sendData( command );
    }
}

HotKey * Profile::getKeyUnit() {
    return & mKeyUnit;
}

void Profile::setString(const QString &key, const QString &value) {
    QMutexLocker locker(& lock);
    settings.setValue(key,value);
}

void Profile::setInt(const QString &key, int value) {
    QMutexLocker locker(& lock);
    settings.setValue(key,value);
}

void Profile::setBool(const QString &key, bool value) {
    QMutexLocker locker(& lock);
    settings.setValue(key,value);
}

void Profile::sendRaw( QString command )
{
    mTelnet.sendData( command );
}

void Profile::incomingStreamProcessor(const QString & data, int line )
{
    //java.handleLine(data,line);

}

QString Profile::getString(const QString &setting) {
    QMutexLocker locker(& lock);
    return settings.value(setting).toString();
}

bool Profile::getBool(const QString &setting) {
    QMutexLocker locker(& lock);
    return settings.value(setting).toBool();
}


int Profile::getInt(const QString &setting) {
    QMutexLocker locker(& lock);
    return settings.value(setting).toInt();
}

QString Profile::getId() {
    QMutexLocker locker(& lock);
    return id;
}

void Profile::load() {

}

void Profile::save() {
    //QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
}

void Profile::setId(const QString &id) {
    QMutexLocker locker(& lock);
    this->id = id;
}

void Profile::enableKey(const QString & name )
{
    mKeyUnit.enableKey( name );
}

void Profile::disableKey(const QString & name )
{
    mKeyUnit.disableKey( name );
}

void Profile::connectToServer()
{
    auto url = settings.value("url").toString();
    auto port = settings.value("port").toInt();
    mTelnet.connectTo( url, port );
}

bool Profile::isClosed() {
    QMutexLocker locker(& lock);
    return closed;
}

void Profile::close() {
    QMutexLocker locker(& lock);
    closed = true;
    mTelnet.disconnect();
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Profile::removeDir( const QString& dirName, const QString& originalPath )
{
    bool result = true;
    QDir dir(dirName);
    if( dir.exists( dirName ) )
    {
        Q_FOREACH( QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            // prevent recursion outside of the original branch
            if( info.isDir() && info.absoluteFilePath().startsWith( originalPath ) )
            {
                result = removeDir( info.absoluteFilePath(), originalPath );
            }
            else
            {
                result = QFile::remove( info.absoluteFilePath() );
            }

            if( !result )
            {
                return result;
            }
        }
        result = dir.rmdir( dirName );
    }

    return result;
}
