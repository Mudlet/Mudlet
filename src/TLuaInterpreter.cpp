/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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


#include <QDebug>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QDesktopServices>
#include "TLuaInterpreter.h"
#include "TForkedProcess.h"
#include "TTrigger.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"
#include <list>
#include <string>
#include "TEvent.h"
#include "dlgMapper.h"



//#ifdef Q_OS_WIN32
//    #include "lua_yajl.c"
//#else
    #include "lua_yajl1.c"
//#endif

extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}
#include <phonon>
/*//for map var access
union mVarTypes {
    int * i;
    float * f;
    bool * b;
    string * s;
    QString * qs;
};*/

extern QStringList gSysErrors;

using namespace std;

map<lua_State*, Host*> TLuaInterpreter::luaInterpreterMap;

TLuaInterpreter::TLuaInterpreter( Host * pH, int id )
:mpHost( pH )
,mHostID( id )
,purgeTimer(this)
{
    pGlobalLua = 0;

    connect(this,SIGNAL(signalEchoMessage(int, QString)), this,SLOT(slotEchoMessage(int,QString)));//,Qt::DirectConnection);
    connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)));//,Qt::QueuedConnection);

    connect(this,SIGNAL(signalOpenUserWindow(int,QString)), this,SLOT(slotOpenUserWindow(int,QString)));
    connect(this,SIGNAL(signalEchoUserWindow(int,QString,QString)), this,SLOT(slotEchoUserWindow(int,QString,QString)));
    connect(this,SIGNAL(signalEnableTimer(int,QString)),this,SLOT(slotEnableTimer(int,QString)));
    connect(this,SIGNAL(signalDisableTimer(int,QString)),this,SLOT(slotDisableTimer(int,QString)));
    connect(this,SIGNAL(signalClearUserWindow(int,QString)),this,SLOT(slotClearUserWindow(int,QString)));

    connect(this, SIGNAL(signalSelect(int, QString, int)), this, SLOT(slotSelect(int,QString,int)));
    connect(this, SIGNAL(signalSelectSection(int, int,int)), this, SLOT(slotSelectSection(int,int,int)));
    connect(this, SIGNAL(signalTempTimer(int, double,QString,QString)), this, SLOT(slotTempTimer(int,double,QString,QString)));

    connect(this, SIGNAL(signalReplace(int, QString)), this, SLOT(slotReplace(int,QString)));
    connect(this, SIGNAL(signalSetFgColor(int, int,int,int)), this, SLOT(slotSetFgColor(int,int,int,int)));
    connect(this, SIGNAL(signalSetBgColor(int, int,int,int)), this, SLOT(slotSetBgColor(int,int,int,int)));
    connect(&purgeTimer, SIGNAL(timeout()), this, SLOT(slotPurge()));

    mpFileDownloader = new QNetworkAccessManager( this );
    connect(mpFileDownloader, SIGNAL(finished(QNetworkReply*)),this, SLOT(replyFinished(QNetworkReply*)));

    initLuaGlobals();

    purgeTimer.start(2000);
}

lua_State * TLuaInterpreter::getLuaExecutionUnit( int unit )
{
    switch( unit )
    {
        case 1:
            return pGlobalLua;
        case 2:
            return pGlobalLua;
        case 3:
            return pGlobalLua;
        case 4:
            return pGlobalLua;
        case 5:
            return pGlobalLua;
    };
    return 0;
}

void TLuaInterpreter::replyFinished(QNetworkReply * reply )
{
    if( ! downloadMap.contains(reply) ) return;
    QString name = downloadMap[reply];
    QFile file(name);
    file.open( QFile::WriteOnly );
    file.write( reply->readAll() );
    file.flush();
    file.close();

    TEvent * e = new TEvent;
    if( reply->error() == 0 )
    {
        e->mArgumentList << "sysDownloadDone";
        e->mArgumentTypeList << ARGUMENT_TYPE_STRING;
        e->mArgumentList << name;
        e->mArgumentTypeList << ARGUMENT_TYPE_STRING;
    }
    else
    {
        e->mArgumentList << "sysDownloadError";
        e->mArgumentTypeList << ARGUMENT_TYPE_STRING;
        e->mArgumentList << reply->errorString();
        e->mArgumentTypeList << ARGUMENT_TYPE_STRING;
        e->mArgumentList << name;
        e->mArgumentTypeList << ARGUMENT_TYPE_STRING;
    }

    mpHost->raiseEvent( e );
}

void TLuaInterpreter::slotDeleteSender() {
    objectsToDelete.append(sender());
}

void TLuaInterpreter::slotPurge() {
    while (!objectsToDelete.isEmpty()) {
        delete objectsToDelete.takeFirst();
    }
}


int TLuaInterpreter::Wait( lua_State *L )
{
  int n = lua_gettop( L );
  if(n!=1)
  {
      lua_pushstring( L, "Wait: wrong number of arguments" );
      lua_error( L );
      return 1;
  }

  int luaSleepMsec;
  if( ! lua_isnumber( L, 1 ) )
  {
      lua_pushstring( L, "Wait: wrong argument type" );
      lua_error( L );
      return 1;
  }
  else
  {
      luaSleepMsec = lua_tointeger( L, 1 );
  }
  msleep( luaSleepMsec );//FIXME thread::sleep()
  return 0;
}

int TLuaInterpreter::denyCurrentSend( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mAllowToSendCommand = false;
    return 0;
}

int TLuaInterpreter::raiseEvent( lua_State * L )
{
    TEvent * pE = new TEvent;

    int n = lua_gettop( L );
    for( int i=1; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->raiseEvent( pE );
    delete pE;
    return 0;
}

int TLuaInterpreter::resetProfile( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mResetProfile = true;
    return 0;
}



// cursorPositionInLine = select( text ) if not found -1
int TLuaInterpreter::select( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "select: wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    string luaSendText="";
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "select: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, s );
        s++;
    }
    int luaNumOfMatch;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "select: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, s );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n == 2 )
    {
        int pos = pHost->mpConsole->select( QString( luaSendText.c_str() ), luaNumOfMatch );
        lua_pushnumber( L, pos );
        return 1;
    }
    else
    {
        QString _name(a1.c_str());
        int pos = mudlet::self()->selectString( pHost, _name, QString( luaSendText.c_str() ), luaNumOfMatch );
        lua_pushnumber( L, pos );
        return 1;
    }
}

int TLuaInterpreter::selectCurrentLine( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "selectCurrentLine: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->selectCurrentLine( luaSendText );
    return 0;
}

int TLuaInterpreter::isAnsiFgColor( lua_State * L )
{
    int ansiFg;

    std::string console = "main";

    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "isAnsiFgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        ansiFg = lua_tointeger( L, 1 );
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getFgColor( console );
    typedef std::list<int>::iterator IT;
    IT it=result.begin();
    if( result.size() < 3 ) return 0;
    if( ansiFg < 0 ) return 0;
    if( ansiFg > 16 ) return 0;


    QColor c;
    switch( ansiFg )
    {
        case 0: c = pHost->mFgColor;  break;
        case 1: c = pHost->mLightBlack; break;
        case 2: c = pHost->mBlack; break;
        case 3: c = pHost->mLightRed; break;
        case 4: c = pHost->mRed; break;
        case 5: c = pHost->mLightGreen; break;
        case 6: c = pHost->mGreen; break;
        case 7: c = pHost->mLightYellow; break;
        case 8: c = pHost->mYellow; break;
        case 9: c = pHost->mLightBlue; break;
        case 10: c = pHost->mBlue; break;
        case 11: c = pHost->mLightMagenta; break;
        case 12: c = pHost->mMagenta; break;
        case 13: c = pHost->mLightCyan; break;
        case 14: c = pHost->mCyan; break;
        case 15: c = pHost->mLightWhite; break;
        case 16: c = pHost->mWhite; break;
    }

    int val = *it;
    if( val == c.red() )
    {
        it++;
        val = *it;
        if( val == c.green() )
        {
            it++;
            val = *it;
            if( val == c.blue() )
            {
                lua_pushboolean( L, 1 );
                return 1;
            }
        }
    }

    lua_pushboolean( L, 0 );
    return 1;
}

int TLuaInterpreter::isAnsiBgColor( lua_State * L )
{
    int ansiFg;

    std::string console = "main";

    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "isAnsiBgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        ansiFg = lua_tointeger( L, 1 );
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost ) return 0;
    result = pHost->mpConsole->getBgColor( console );
    typedef std::list<int>::iterator IT;
    IT it=result.begin();
    if( result.size() < 3 ) return 0;
    if( ansiFg < 0 ) return 0;
    if( ansiFg > 16 ) return 0;


    QColor c;
    switch( ansiFg )
    {
        case 0: c = pHost->mBgColor;  break;
        case 1: c = pHost->mLightBlack; break;
        case 2: c = pHost->mBlack; break;
        case 3: c = pHost->mLightRed; break;
        case 4: c = pHost->mRed; break;
        case 5: c = pHost->mLightGreen; break;
        case 6: c = pHost->mGreen; break;
        case 7: c = pHost->mLightYellow; break;
        case 8: c = pHost->mYellow; break;
        case 9: c = pHost->mLightBlue; break;
        case 10: c = pHost->mBlue; break;
        case 11: c = pHost->mLightMagenta; break;
        case 12: c = pHost->mMagenta; break;
        case 13: c = pHost->mLightCyan; break;
        case 14: c = pHost->mCyan; break;
        case 15: c = pHost->mLightWhite; break;
        case 16: c = pHost->mWhite; break;
    }

    int val = *it;
    if( val == c.red() )
    {
        it++;
        val = *it;
        if( val == c.green() )
        {
            it++;
            val = *it;
            if( val == c.blue() )
            {
                lua_pushboolean( L, 1 );
                return 1;
            }
        }
    }

    lua_pushboolean( L, 0 );
    return 1;
}

int TLuaInterpreter::getFgColor( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "getFgColor: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }
    QString _name(luaSendText.c_str());
    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getFgColor( luaSendText );
    typedef std::list<int>::iterator IT;
    for( IT it=result.begin(); it!=result.end(); it++ )
    {
        int pos = *it;
        lua_pushnumber( L, pos );
    }
    return result.size();
}

int TLuaInterpreter::getBgColor( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "getBgColor: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getBgColor( luaSendText );
    typedef std::list<int>::iterator IT;
    for( IT it=result.begin(); it!=result.end(); it++ )
    {
        int pos = *it;
        lua_pushnumber( L, pos );
    }
    return result.size();
}

int TLuaInterpreter::wrapLine( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1 = "main";
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrapLine: wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }

    int luaNumOfMatch;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrapLine: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->luaWrapLine( a1, luaNumOfMatch );
    return 0;
}



int TLuaInterpreter::spawn( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    return TForkedProcess::startProcess(pHost->getLuaInterpreter(), L);
}



// cursorPositionInLine = selectCaptureGroup( groupNumber ) if not found -1
int TLuaInterpreter::selectCaptureGroup( lua_State * L )
{
    int luaNumOfMatch;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "selectCaptureGroup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( luaNumOfMatch < 1 )
    {
        lua_pushnumber( L, -1 );
        return 1;
    }
    luaNumOfMatch--; //we want capture groups to start with 1 instead of 0
    if( luaNumOfMatch < static_cast<int>(pHost->getLuaInterpreter()->mCaptureGroupList.size()) )
    {
        TLuaInterpreter * pL = pHost->getLuaInterpreter();
        std::list<std::string>::iterator its = pL->mCaptureGroupList.begin();
        std::list<int>::iterator iti = pL->mCaptureGroupPosList.begin();

        for( int i=0; iti!=pL->mCaptureGroupPosList.end(); ++iti,++i )
        {
            if( i >= luaNumOfMatch ) break;
        }
        for( int i=0; its!=pL->mCaptureGroupList.end(); ++its,++i)
        {
            if( i >= luaNumOfMatch ) break;
        }

        int begin = *iti;
        std::string & s = *its;
        int length = s.size();
        //cout << "selectSection("<<begin<<", "<<length<<")"<<endl;
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"selectCaptureGroup("<<begin<<", "<<length<<")\n">>0;}
        int pos = pHost->mpConsole->selectSection( begin, length );
        lua_pushnumber( L, pos );
    }
    else
    {
        lua_pushnumber( L, -1 );
    }
    return 1;
}

// luaTable result[line_number, content] = getLines( from_cursorPos, to_cursorPos )
int TLuaInterpreter::getLines( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getLines: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 1 );
    }

    int luaTo;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "getLines: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QStringList strList = pHost->mpConsole->getLines( luaFrom, luaTo );

    lua_newtable(L);
    for( int i=0; i<strList.size(); i++ )
    {
        lua_pushnumber( L, i+1 );
        lua_pushstring( L, strList[i].toLatin1().data() );
        lua_settable(L, -3);
    }
    return 1;
}

// luaTable result[line_number, content] = getLines( from_cursorPos, to_cursorPos )
int TLuaInterpreter::getBufferTable( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getBufferTable: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 1 );
    }

    int luaTo;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "getBufferTable: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, 2 );
    }
    /*Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QStringList strList = pHost->getBufferTable( luaFrom, luaTo );
    if( mudlet::debugMode ) qDebug()<<"TLuaInterpreter::getBufferTable() strList="<<strList;
    lua_newtable(L);
    for( int i=0; i<strList.size(); i++ )
    {
        lua_pushnumber( L, i+1 );
        lua_pushstring( L, strList[i].toLatin1().data() );
        lua_settable(L, -3);
    } */
    return 0;
}

int TLuaInterpreter::loadRawFile( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "loadRawFile: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->loadRawFile( luaSendText );
    return 0;
}

int TLuaInterpreter::getCurrentLine( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "getCurrentLine: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString line = pHost->mpConsole->getCurrentLine( luaSendText );
    lua_pushstring( L, line.toLatin1().data() );
    return 1;
}

int TLuaInterpreter::setMiniConsoleFontSize( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setMiniConsoleFontSize: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int luaNumOfMatch;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setMiniConsoleFontSize: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->setMiniConsoleFontSize( luaSendText, luaNumOfMatch );
    return 0;
}

// returns current y position of the user cursor
int TLuaInterpreter::getLineNumber( lua_State * L )
{

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int lineNumber = pHost->mpConsole->getLineNumber();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::updateMap(lua_State * L){
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpM)
            pHost->mpMap->mpM->update();
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                pHost->mpMap->mpMapper->mp2dMap->mNewMoveAction=true;
                pHost->mpMap->mpMapper->mp2dMap->update();
            }
        }
    }
    return 0;
}

int TLuaInterpreter::addMapMenu(lua_State * L){
//    first arg = unique name, second arg= parent name, third arg = display name (=unique name if not provided)
    QString uniqueName;
    QStringList menuList;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "addMapMenu: wrong first argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        uniqueName = lua_tostring( L, 1 );
    }
    if( ! lua_isstring( L, 2 ) )
    {
        menuList << "";
    }
    else
    {
        menuList << lua_tostring( L, 2 );
    }
    if( ! lua_isstring( L, 3 ) )
    {
        menuList << uniqueName;
    }
    else
    {
        menuList << lua_tostring( L, 3 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                pHost->mpMap->mpMapper->mp2dMap->mUserMenus.insert(uniqueName,menuList);
            }
        }
    }
    return 0;
}

int TLuaInterpreter::removeMapMenu(lua_State * L){
    QString uniqueName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "removeMapMenu: wrong first argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        uniqueName = lua_tostring( L, 1 );
    }
    if (uniqueName == "")
        return 0;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                pHost->mpMap->mpMapper->mp2dMap->mUserMenus.remove(uniqueName);
                //remove all entries with this as parent
                QStringList removeList;
                removeList.append(uniqueName);
                bool newElement = true;
                while (newElement){
                    newElement = false;
                    QMapIterator<QString, QStringList> it(pHost->mpMap->mpMapper->mp2dMap->mUserMenus);
                    while (it.hasNext()){
                        it.next();
                        QStringList menuInfo = it.value();
                        QString parent = menuInfo[0];
                        if (removeList.contains(parent)){
                            pHost->mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                            if (it.key() != "" && !removeList.contains(it.key())){
                                pHost->mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                                removeList.append(it.key());
                                newElement = true;
                            }
                        }
                    }
                }
                qDebug()<<removeList;
                QMapIterator<QString, QStringList> it2(pHost->mpMap->mpMapper->mp2dMap->mUserActions);
                while (it2.hasNext()){
                    it2.next();
                    QString actParent = it2.value()[1];
                    if (removeList.contains(actParent)){
                        pHost->mpMap->mpMapper->mp2dMap->mUserActions.remove(it2.key());
                    }
                }
            }
        }
    }
    return 0;
}

int TLuaInterpreter::getMapMenus(lua_State * L){
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                lua_newtable(L);
                QMapIterator<QString, QStringList> it (pHost->mpMap->mpMapper->mp2dMap->mUserMenus);
                while (it.hasNext()){
                    it.next();
                    QString parent, display;
                    QStringList menuInfo = it.value();
                    parent = menuInfo[0];
                    display = menuInfo[1];
                    lua_pushstring( L, it.key().toLatin1().data() );
                    lua_pushstring( L, parent.toLatin1().data() );
                    lua_pushstring( L, display.toLatin1().data() );
                    lua_settable(L, -3);
                }
            }
            return 1;
        }
    }
    return 0;
}


int TLuaInterpreter::addMapEvent(lua_State * L){
    QString uniqueName, eventName, parent, displayName;
    QStringList actionInfo;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "addMapEvent: wrong first argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        uniqueName = lua_tostring( L, 1 );
    }
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "addMapEvent: wrong second argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        actionInfo << lua_tostring( L, 2 );
    }
    if( ! lua_isstring( L, 3 ) )
    {
        actionInfo << "";
    }
    else
    {
        actionInfo << lua_tostring( L, 3 );
    }
    if( ! lua_isstring( L, 4 ) )
    {
        actionInfo << uniqueName;
    }
    else
    {
        actionInfo << lua_tostring( L, 4 );
    }
    //variable number of arguments
    for (int i=5;i<=lua_gettop(L);i++)
        actionInfo << lua_tostring(L,i);
    qDebug()<<actionInfo;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                pHost->mpMap->mpMapper->mp2dMap->mUserActions.insert(uniqueName, actionInfo);
            }
        }
    }
    return 0;
}

int TLuaInterpreter::removeMapEvent(lua_State * L){
    QString displayName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "removeMapEvent: wrong first argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        displayName = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                pHost->mpMap->mpMapper->mp2dMap->mUserActions.remove(displayName);
            }
        }
    }
    return 0;
}

int TLuaInterpreter::getMapEvents(lua_State * L){
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap)
    {
        if (pHost->mpMap->mpMapper){
            if (pHost->mpMap->mpMapper->mp2dMap){
                lua_newtable(L);
                QMapIterator<QString, QStringList> it(pHost->mpMap->mpMapper->mp2dMap->mUserActions);
                while (it.hasNext()){
                    it.next();
                    lua_newtable(L);
                    QStringList eventInfo = it.value();
                    lua_pushstring( L, eventInfo[0].toLatin1().data() );
                    lua_pushstring( L, eventInfo[1].toLatin1().data() );
                    lua_pushstring( L, eventInfo[2].toLatin1().data() );
                    lua_settable(L, -3);
                    lua_pushstring(L, it.key().toLatin1().data());
                    lua_insert(L,-2);
                    lua_settable(L, -3);
                }
            }
            return 1;
        }
    }
    return 0;
}

int TLuaInterpreter::centerview( lua_State * L )
{
    int roomid;
    if( lua_isnumber( L, 1 ) || lua_isstring( L, 1 ) )
    {
        roomid = lua_tointeger( L, 1 );
    }
    else
    {
        lua_pushstring( L, "centerview: need a valid room ID" );
        lua_error( L );
        return 1;
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap && pHost->mpMap->rooms.contains( roomid ) )
    {
        pHost->mpMap->mRoomId = roomid;
        pHost->mpMap->mNewMove = true;
        if( pHost->mpMap->mpM )
        {
            pHost->mpMap->mpM->update();
        }
        if( pHost->mpMap->mpM )
        {
            pHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = true;
            pHost->mpMap->mpMapper->mp2dMap->update();
            pHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = false;
        }

    }

    return 0;
}

int TLuaInterpreter::copy( lua_State * L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( luaWindowName == "main" )
        pHost->mpConsole->copy();
    else
       mudlet::self()->copy( pHost, windowName );
    return 0;
}
int TLuaInterpreter::cut( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->cut();
    return 0;
}
int TLuaInterpreter::paste( lua_State * L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( luaWindowName == "main" )
        pHost->mpConsole->paste();
    else
       mudlet::self()->pasteWindow( pHost, windowName );
    return 0;
}


int TLuaInterpreter::feedTriggers( lua_State * L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
    {
        lua_pushstring( L, "feedTriggers: wrong argument type" );
        lua_error( L );
        return 1;
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->printOnDisplay( luaWindowName );
    return 0;
}


int TLuaInterpreter::isPrompt( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int userCursorY = pHost->mpConsole->getLineNumber();
    if( userCursorY < pHost->mpConsole->buffer.promptBuffer.size() && userCursorY >= 0 )
    {
        lua_pushboolean( L, pHost->mpConsole->buffer.promptBuffer.at( userCursorY ) );
        return 1;
    }
    else
    {
        if( pHost->mpConsole->mTriggerEngineMode && pHost->mpConsole->mIsPromptLine )
            lua_pushboolean( L, true );
        else
            lua_pushboolean( L, false );
        return 1;
    }
}

int TLuaInterpreter::setWindowWrap( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setWindowWrap: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int luaFrom;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setWindowWrap: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = luaSendText.c_str();
    if( name == "main" )
        pHost->mpConsole->setWrapAt( luaFrom );
    else
        mudlet::self()->setWindowWrap( pHost, name, luaFrom );
    return 0;
}

int TLuaInterpreter::setWindowWrapIndent( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setWindowWrapIndent: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int luaFrom;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setWindowWrapIndent: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = luaSendText.c_str();
    mudlet::self()->setWindowWrapIndent( pHost, name, luaFrom );
    return 0;
}

int TLuaInterpreter::getLineCount( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( lua_isstring( L, 1 ) )
    {
        string window = lua_tostring( L, 1 );
        QString _window = window.c_str();
        lua_pushnumber( L, mudlet::self()->getLastLineNumber( pHost, _window ) + 1 );
        return 1;
    }
    else
    {
        int lineNumber = pHost->mpConsole->getLineCount();
        lua_pushnumber( L, lineNumber );
        return 1;
    }
    return 0;
}

int TLuaInterpreter::getColumnNumber( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int lineNumber = pHost->mpConsole->getColumnNumber();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::getStopWatchTime( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getStopWatchTime: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double time = pHost->getStopWatchTime( watchID );
    lua_pushnumber( L, time );
    return 1;
}

int TLuaInterpreter::createStopWatch( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double watchID = pHost->createStopWatch();
    lua_pushnumber( L, watchID );
    return 1;
}

int TLuaInterpreter::stopStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "stopStopWatch: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double time = pHost->stopStopWatch( watchID );
    lua_pushnumber( L, time );
    return 1;
}

int TLuaInterpreter::startStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "startStopWatch: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool b = pHost->startStopWatch( watchID );
    lua_pushboolean( L, b );
    return 1;
}

int TLuaInterpreter::resetStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "resetStopWatch: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool b = pHost->resetStopWatch( watchID );
    lua_pushboolean( L, b );
    return 1;
}

// cusorPositionInLine = selectSection( from_cursorPos, to_cursorPos ) -1 on not found
int TLuaInterpreter::selectSection( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "selectSection: wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "selectSection: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, s );
        s++;
    }

    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "selectSection: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    bool ret;
    if( n > 2 )
    {
        QString _name = a1.c_str();
        ret = mudlet::self()->selectSection( pHost, _name, luaFrom, luaTo );
    }
    else
    {
        ret = pHost->mpConsole->selectSection( luaFrom, luaTo );
    }
    lua_pushboolean( L, ret );
    return 1;
}


int TLuaInterpreter::moveCursor( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "moveCursor: wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "moveCursor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, s );
        s++;
    }

    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "moveCursor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( a1 == "main" || n < 3 )
        lua_pushboolean( L, pHost->mpConsole->moveCursor( luaFrom, luaTo ) );
    else
    {
        QString windowName = a1.c_str();
        lua_pushboolean( L, mudlet::self()->moveCursor( pHost, windowName, luaFrom, luaTo ) );
    }
    return 1;
}

int TLuaInterpreter::setConsoleBufferSize( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setConsoleBufferSize: wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setConsoleBufferSize: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, s );
        s++;
    }

    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setConsoleBufferSize: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( a1 == "main" || n < 3 )
    {
        pHost->mpConsole->buffer.setBufferSize( luaFrom, luaTo );
    }
    else
    {
        QString windowName = a1.c_str();
        mudlet::self()->setConsoleBufferSize( pHost, windowName, luaFrom, luaTo );
    }
    return 0;
}

int TLuaInterpreter::getBufferLine( lua_State * L )
{
    int luaLine;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getBufferLine: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaLine = lua_tointeger( L, 1 );
    }

    /*Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString line = pHost->getBufferLine( luaLine );
    if( mudlet::debugMode ) qDebug()<<"TLuaInterpreter::getBufferLine() line="<<line;
    lua_pushstring( L, line.toLatin1().data() );*/
    return 0;
}

// replace( sessionID, replace_with )
int TLuaInterpreter::replace( lua_State * L )
{
    string a1 = "";
    string a2 = "";
    int n = lua_gettop( L );
    int s = 1;
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "replace: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }

    QString _name( a1.c_str() );
    string luaSendText="";
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "replace: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n == 1 )
        pHost->mpConsole->replace( QString(a1.c_str()) );
    else
        mudlet::self()->replace( pHost, _name, QString(a2.c_str()) );
    return 0;
}

int TLuaInterpreter::deleteLine( lua_State * L )
{
    string name="";
    if( lua_gettop( L ) == 1 )
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "deleteLine: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            name = lua_tostring( L, 1 );
        }
    }

    QString _name( name.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( name == "" )
        pHost->mpConsole->skipLine();
    else
        mudlet::self()->deleteLine( pHost, _name );
    return 0;
}

int TLuaInterpreter::saveMap( lua_State * L )
{
    string location="";
    if( lua_gettop( L ) == 1 )
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "saveMap: where do you want to save to?" );
            lua_error( L );
            return 1;
        }
        else
        {
            location = lua_tostring( L, 1 );
        }
    }

    QString _location( location.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    bool error = pHost->mpConsole->saveMap(_location);
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::setExitStub( lua_State * L  ){
    //args:room id, direction (as given by the #define direction table), status
    int roomId, dirType;
    bool status;
    if (!lua_isnumber(L,1)){
        lua_pushstring( L, "setExitStub: Need a room number as first argument" );
        lua_error( L );
        return 1;
    }
    else{
        roomId = lua_tonumber(L,1);
    }
    if (!lua_isnumber(L,2)){
        lua_pushstring( L, "setExitStub: Need a dir number as 2nd argument" );
        lua_error( L );
        return 1;
    }
    else{
        dirType = lua_tonumber(L,2);
    }
    if (!lua_isboolean(L,3)){
        lua_pushstring( L, "setExitStub: Need a true/false for third argument" );
        lua_error( L );
        return 1;
    }
    else{
        status = lua_toboolean(L,3);
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if (!pHost->mpMap->rooms.contains( roomId )){
        lua_pushstring( L, "setExitStub: RoomId doesn't exist" );
        lua_error( L );
        return 1;
    }
    if (dirType>10 || dirType < 1){
        lua_pushstring( L, "setExitStub: dirType must be between 1 and 10" );
        lua_error( L );
        return 1;
    }
    if (status){
        if (pHost->mpMap->rooms[roomId]->exitStubs.contains(dirType))
            return 1;
        pHost->mpMap->rooms[roomId]->setExitStub(dirType, 1);
        }
    else{
        if (pHost->mpMap->rooms[roomId]->exitStubs.contains(dirType))
            pHost->mpMap->rooms[roomId]->setExitStub(dirType, 0);
        }
    return 1;
}

int TLuaInterpreter::connectExitStub( lua_State * L  ){
    //takes exit stubs from the selected room, finds the room in that direction and if
    //that room has a stub, a two way exit is formed
    //args:room id, direction
    //OR if 3 arguments, takes first argument as from room, 2nd at to room, 3rd as direction
    //from start room to end room
    int roomId;
    int toRoom;
    int dirType;
    int roomsGiven = 0;
    if (!lua_isnumber(L,1)){
        lua_pushstring( L, "connectExitStub: Need a room number as first argument" );
        lua_error( L );
        return 1;
    }
    else
        roomId = lua_tonumber(L,1);
    if (!lua_isnumber(L,2)){
        lua_pushstring( L, "connectExitStub: Need a direction number (or room id) as 2nd argument" );
        lua_error( L );
        return 1;
    }
    else
        dirType = lua_tonumber(L,2);
    if (!lua_isnumber(L,3) && !lua_isstring(L,3)){
        roomsGiven = 0;
    }
    else{
        roomsGiven = 1;
        toRoom = lua_tonumber(L,2);
        dirType = lua_tonumber(L,3);
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if (!pHost->mpMap->rooms.contains( roomId )){
        lua_pushstring( L, "connectExitStubs: RoomId doesn't exist" );
        lua_error( L );
        return 1;
    }
    if (!pHost->mpMap->rooms[roomId]->exitStubs.contains(dirType)){
        lua_pushstring( L, "connectExitStubs: ExitStub doesn't exist" );
        lua_error( L );
        return 1;
    }
    if (roomsGiven){
        if (!pHost->mpMap->rooms.contains( toRoom )){
            lua_pushstring( L, "connectExitStubs: toRoom doesn't exist" );
            lua_error( L );
            return 1;
        }
        Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        lua_pushboolean(L, pHost->mpMap->setExit( roomId, toRoom, dirType ) );
        //pHost->mpMap->rooms[roomId]->setExitStub(dirType, 0);
        //setExit( toRoom, roomId, pHost->mpMap->reverseDirections[dirType]);
        //pHost->mpMap->rooms[toRoom]->setExitStub(pHost->mpMap->reverseDirections[dirType], 0);
    }
    else{
        if (!pHost->mpMap->rooms[roomId]->exitStubs.contains(dirType)){
            lua_pushstring( L, "connectExitStubs: ExitStub doesn't exist" );
            lua_error( L );
            return 1;
        }
        pHost->mpMap->connectExitStub(roomId, dirType);
    }
    pHost->mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

int TLuaInterpreter::getExitStubs( lua_State * L  ){
    //args:room id
    int roomId;
    if (!lua_isnumber(L,1)){
        lua_pushstring( L, "getExitStubs: Need a room number as first argument" );
        lua_error( L );
        return 1;
    }
    else
        roomId = lua_tonumber(L,1);
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if (!pHost->mpMap->rooms.contains( roomId )){
        lua_pushstring( L, "getExitStubs: RoomId doesn't exist" );
        lua_error( L );
        return 1;
    }
    else{
        QList<int> stubs = pHost->mpMap->rooms[roomId]->exitStubs;
        if (stubs.size()){
            lua_newtable(L);
            for(int i=0;i<stubs.size();i++){
                int exitType = stubs[i];
                lua_pushnumber( L, i );
                lua_pushnumber( L, exitType );
                lua_settable(L, -3);
            }
        }
    }
    return 1;
}

int TLuaInterpreter::getModulePriority( lua_State * L  ){
    QString moduleName;
    if (!lua_isstring(L,1)){
        lua_pushstring(L, "getModulePriority: Module be be a string");
        lua_error(L);
        return 1;
    }
    else
        moduleName = lua_tostring(L,1);
    Host * mpHost = TLuaInterpreter::luaInterpreterMap[L];
    if (mpHost->mModulePriorities.contains(moduleName)){
        int priority = mpHost->mModulePriorities[moduleName];
        lua_pushnumber( L, priority );
        return 1;
    }
    else{
        lua_pushstring(L, "getModulePriority: Module doesn't exist");
        lua_error(L);
        return 1;
    }
    return 0;
}

int TLuaInterpreter::setModulePriority( lua_State * L  ){
    QString moduleName;
    int modulePriority;
    if (!lua_isstring(L,1)){
        lua_pushstring(L, "setModulePriority: Module be be a string");
        lua_error(L);
        return 1;
    }
    else
        moduleName = lua_tostring(L,1);
    if (!lua_isnumber(L,2)){
        lua_pushstring(L, "setModulePriority: Module priority must be an integer");
        lua_error(L);
        return 1;
    }
    else
        modulePriority = lua_tonumber(L,2);
    Host * mpHost = TLuaInterpreter::luaInterpreterMap[L];
    if (mpHost->mModulePriorities.contains(moduleName))
        mpHost->mModulePriorities[moduleName] = modulePriority;
    else{
        lua_pushstring(L, "setModulePriority: Module doesn't exist");
        lua_error(L);
        return 1;
    }
    return 0;
}

int TLuaInterpreter::loadMap( lua_State * L )
{
    string location="";
    if( lua_gettop( L ) == 1 )
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "loadMap: where do you want to load from?" );
            lua_error( L );
            return 1;
        }
        else
        {
            location = lua_tostring( L, 1 );
        }
    }
    QString _location( location.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool error = pHost->mpConsole->loadMap(_location);
    lua_pushboolean( L, error );
    return 1;
}



// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "enableTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getTimerUnit()->enableTimer( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "disableTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getTimerUnit()->disableTimer( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::enableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "enableKey: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getKeyUnit()->enableKey( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "disableKey: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getKeyUnit()->disableKey( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::enableAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "enableAlias: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getAliasUnit()->enableAlias( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "disableAlias: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getAliasUnit()->disableAlias( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::killAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "killAlias: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->getAliasUnit()->killAlias( text ) );
    return 1;
}

// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "enableTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getTriggerUnit()->enableTrigger( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "disableTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getTriggerUnit()->disableTrigger( text );
    lua_pushboolean( L, error );
    return 1;
}


int TLuaInterpreter::killTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "killTimer: killTimer requires a string ID" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    //pHost->disableTimer( text );
    lua_pushboolean( L, pHost->killTimer( text ) );
    return 1;
}

int TLuaInterpreter::killTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "killTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->killTrigger( text ) );
    return 1;
}

// openUserWindow( session, string window_name )
int TLuaInterpreter::openUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "openUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->openWindow( pHost, text );
    return 0;
}

int TLuaInterpreter::createMiniConsole( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "createMiniConsole: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "createMiniConsole: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createMiniConsole: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createMiniConsole: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "createMiniConsole: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    lua_pushboolean( L, mudlet::self()->createMiniConsole( pHost, name, x, y, width, height ) );
    return 1;
}

int TLuaInterpreter::createLabel( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    bool fillBackground=false;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "createLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fillBackground = lua_toboolean( L, 6 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    lua_pushboolean( L, mudlet::self()->createLabel( pHost, name, x, y, width, height, fillBackground ) );
    return 1;
}

int TLuaInterpreter::createMapper( lua_State *L )
{
    int x,y,width,height;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "createMapper: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "createMapper: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createMapper: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createMapper: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 4 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->createMapper( x, y, width, height );
    return 0;
}



int TLuaInterpreter::createButton( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    bool fillBackground=false;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "createButton: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fillBackground = lua_toboolean( L, 6 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    //TODO FIXME
    mudlet::self()->createLabel( pHost, name, x, y, width, height, fillBackground );
    return 0;
}


int TLuaInterpreter::createBuffer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "createBuffer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->createBuffer( pHost, text );
    return 0;
}

#include "TTextEdit.h"

int TLuaInterpreter::clearUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        pHost->mpConsole->buffer.clear();
        pHost->mpConsole->console->forceUpdate();
        return 0;
//        lua_pushstring( L, "clearUserWindow: wrong argument type" );
//        lua_error( L );
//        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->clearWindow( pHost, text );

    return 0;
}

int TLuaInterpreter::closeUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "closeUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->hideWindow( pHost, text );

    return 0;
}

int TLuaInterpreter::hideUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "hideUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    //mudlet::self()->hideWindow( pHost, text );
    pHost->mpConsole->hideWindow( text );

    return 0;
}

int TLuaInterpreter::setBorderTop( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBorderTop: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderTopHeight = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderBottom( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBorderBottom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderBottomHeight = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderLeft( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBorderLeft: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderLeftWidth = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderRight( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBorderRight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderRightWidth = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::resizeUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "resizeUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "resizeUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "resizeUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->resizeWindow( pHost, text, static_cast<int>(x1), static_cast<int>(y1) );

    return 0;
}

int TLuaInterpreter::moveWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "moveWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "moveWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "moveWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    QString text(luaSendText.c_str());
    mudlet::self()->moveWindow( pHost, text, static_cast<int>(x1), static_cast<int>(y1) );
    return 0;
}

int TLuaInterpreter::setMainWindowSize( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    int y1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    mudlet::self()->resize( x1, y1 );

    return 0;
}

int TLuaInterpreter::setBackgroundColor( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }
    double x2;
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x2 = lua_tonumber( L, 4 );
    }
    double y2;
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "setBackgroundColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y2 = lua_tonumber( L, 5 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->setBackgroundColor( pHost, text, static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2) );

    return 0;
}

int TLuaInterpreter::calcFontWidth( int size )
{
    QFont font = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
    return QFontMetrics( font ).width( QChar('W') );
}

int TLuaInterpreter::calcFontHeight( int size )
{
    QFont font = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
    int fontDescent = QFontMetrics( font ).descent();
    int fontAscent = QFontMetrics( font ).ascent();
    return fontAscent + fontDescent;
}

int TLuaInterpreter::calcFontSize( lua_State *L )
{
    int x = 0;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "calcFontSize: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 1 );
    }

    lua_pushnumber( L, calcFontWidth( x ) );
    lua_pushnumber( L, calcFontHeight( x ) );
    return 2;
}

int TLuaInterpreter::startLogging( lua_State *L )
{
    bool logOn = true;
    if( ! lua_isboolean( L, 1 ) )
    {
        lua_pushstring( L, "startLogging: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        logOn = lua_toboolean( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->mLogToLogFile = ! logOn;
    pHost->mpConsole->slot_toggleLogging();
    return 0;
}

int TLuaInterpreter::setBackgroundImage( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setBackgroundImage: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setBackgroundImage: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setBackgroundImage( pHost, text, name );

    return 0;
}

int TLuaInterpreter::setLabelClickCallback( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setLabelClickCallback: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setLabelClickCallback: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    TEvent * pE = new TEvent;

    int n = lua_gettop( L );
    for( int i=3; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setLabelClickCallback( pHost, text, name, pE );

    return 0;
}

int TLuaInterpreter::setLabelOnEnter( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setLabelOnEnter: wrong first argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setLabelOnEnter: wrong second argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    TEvent * pE = new TEvent;

    int n = lua_gettop( L );
    for( int i=3; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setLabelOnEnter( pHost, text, name, pE );

    return 0;
}

int TLuaInterpreter::setLabelOnLeave( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setLabelClickCallback: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setLabelClickCallback: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    TEvent * pE = new TEvent;

    int n = lua_gettop( L );
    for( int i=3; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setLabelOnLeave( pHost, text, name, pE );

    return 0;
}

int TLuaInterpreter::setTextFormat( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double r1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r1 = lua_tonumber( L, 2 );
    }
    double g1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        g1 = lua_tonumber( L, 3 );
    }
    double b1;
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b1 = lua_tonumber( L, 4 );
    }
    double r2;
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r2 = lua_tonumber( L, 5 );
    }
    double g2;
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        g2 = lua_tonumber( L, 6 );
    }
    double b2;
    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b2 = lua_tonumber( L, 7 );
    }
    double bold;
    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bold = lua_tonumber( L, 8 );
    }
    double underline;
    if( ! lua_isnumber( L, 9 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        underline = lua_tonumber( L, 9 );
    }
    double italics;
    if( ! lua_isnumber( L, 10 ) )
    {
        lua_pushstring( L, "setTextFormat: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        italics = lua_tonumber( L, 10 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    if( text == "main" )
    {
        TConsole * pC = pHost->mpConsole;
        pC->mFormatCurrent.bgR = r1;
        pC->mFormatCurrent.bgG = g1;
        pC->mFormatCurrent.bgB = b1;
        pC->mFormatCurrent.fgR = r2;
        pC->mFormatCurrent.fgG = g2;
        pC->mFormatCurrent.fgB = b2;
        pC->mFormatCurrent.bold = bold;
        pC->mFormatCurrent.underline = underline;
        pC->mFormatCurrent.italics = italics;
        return true;
    }
    else
    {
        mudlet::self()->setTextFormat( pHost, text, static_cast<int>(r1), static_cast<int>(g1), static_cast<int>(b1), static_cast<int>(r2),static_cast<int>(g2), static_cast<int>(b2), static_cast<bool>(bold), static_cast<bool>(underline), static_cast<bool>(italics) );
    }

    return 0;
}

int TLuaInterpreter::showUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "showUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->mpConsole->showWindow( text ));
    return 1;
}

// xRot, yRot, zRot, zoom
int TLuaInterpreter::setMapperView( lua_State *L )
{
    float x, y, z, zoom;

    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setMapperView: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setMapperView: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setMapperView: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        z = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "setMapperView: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        zoom = lua_tonumber( L, 4 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    pHost->mpMap->setView( x, y, z, zoom  );
    return 0;
}

int TLuaInterpreter::setRoomEnv( lua_State *L )
{
    int id, env;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomEnv: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setRoomEnv: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        env = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->rooms[id]->environment = env;
    }

    return 0;
}

int TLuaInterpreter::setRoomName( lua_State *L )
{
    int id;
    string name;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setRoomName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString _name = name.c_str();
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->rooms[id]->name = _name;
    }

    return 0;
}

int TLuaInterpreter::getRoomName( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushstring(L, pHost->mpMap->rooms[id]->name.toLatin1().data() );
    }

    return 1;
}

int TLuaInterpreter::setRoomWeight( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomWeight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    int w;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setRoomWeight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        w = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->rooms[id]->weight = w;
        pHost->mpMap->mMapGraphNeedsUpdate = true;
    }

    return 0;
}

int TLuaInterpreter::connectToServer( lua_State *L )
{
    int port;
    string url;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "connectToServer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        url = lua_tostring( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "connectToServer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        port = lua_tonumber( L, 2 );
    }
    QString _url = url.c_str();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.connectIt( _url, port );
    return 0;
}

int TLuaInterpreter::setRoomIDbyHash( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomIDbyHash: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    string hash;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setRoomIDbyHash: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        hash = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpMap->hashTable[QString(hash.c_str())] = id;
    return 0;
}

int TLuaInterpreter::getRoomIDbyHash( lua_State *L )
{
    string hash;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "getRoomIDbyHash() wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        hash = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int retID = -1;
    QString _hash = hash.c_str();
    if( pHost->mpMap->hashTable.contains( _hash ) )
    {
        retID = pHost->mpMap->hashTable[_hash];
        lua_pushnumber( L, retID );
    }
    else
        lua_pushnumber( L, -1 );

    return 1;
}

int TLuaInterpreter::solveRoomCollisions( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "solveRoomCollisions() wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }
    int creationDirection=0;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "solveRoomCollisons() wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        creationDirection = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->solveRoomCollision( id, creationDirection );
    }
    return 0;
}

int TLuaInterpreter::roomLocked( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "roomLocked: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        bool r = pHost->mpMap->rooms[id]->isLocked;
        lua_pushboolean( L, r );
    }
    else
    {
        lua_pushboolean(L, false);
    }
    return 1;
}

int TLuaInterpreter::lockRoom( lua_State *L )
{
    bool b = true;
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    if( ! lua_isboolean( L, 2 ) )
    {
        lua_pushstring( L, "lockRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->rooms[id]->isLocked = b;
        pHost->mpMap->mMapGraphNeedsUpdate = true;
        lua_pushboolean(L, true);
    }
    else
    {
        lua_pushboolean(L, false);
    }
    return 1;
}

int TLuaInterpreter::lockExit( lua_State *L )
{
    bool b = true;
    int id;
    int dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }


    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tonumber( L, 2 );
    }

    if( ! lua_isboolean( L, 3 ) )
    {
        lua_pushstring( L, "lockRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, 3 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        pHost->mpMap->rooms[id]->setExitLock( dir, b );
        pHost->mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

int TLuaInterpreter::lockSpecialExit( lua_State *L )
{
    bool b = true;
    int id, to;
    std::string dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        to = lua_tonumber( L, 2 );
    }

    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "lockSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tostring( L, 3 );
    }

    if( ! lua_isboolean( L, 4 ) )
    {
        lua_pushstring( L, "lockSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, 4 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        QString _dir = dir.c_str();
        pHost->mpMap->rooms[id]->setSpecialExitLock( to, _dir, b );
        pHost->mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

int TLuaInterpreter::hasSpecialExitLock( lua_State *L )
{
    bool b = true;
    int id, to;
    std::string dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }


    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        to = lua_tonumber( L, 2 );
    }
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "hasSpecialExitLock: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tostring( L, 3 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        QString _dir = dir.c_str();
        lua_pushboolean( L, pHost->mpMap->rooms[id]->hasSpecialExitLock( to, _dir ) );
        return 1;
    }
    return 0;
}

int TLuaInterpreter::hasExitLock( lua_State *L )
{
    int id;
    int dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }


    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "lockExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tonumber( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushboolean( L, pHost->mpMap->rooms[id]->hasExitLock(dir) );
        return 1;
    }
    return 0;
}

int TLuaInterpreter::isLockedRoom( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "lockRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushboolean( L, pHost->mpMap->rooms[id]->isLocked );
    }
    else
    {
        lua_pushboolean(L, false);
    }
    return 1;
}


int TLuaInterpreter::getRoomExits( lua_State *L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomExits: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        lua_newtable(L);
        if( pHost->mpMap->rooms[id]->north != -1 )
        {
            lua_pushstring( L, "north" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->north );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->northwest != -1 )
        {
            lua_pushstring( L, "northwest" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->northwest );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->northeast != -1 )
        {
            lua_pushstring( L, "northeast" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->northeast );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->south != -1 )
        {
            lua_pushstring( L, "south" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->south );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->southwest != -1 )
        {
            lua_pushstring( L, "southwest" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->southwest );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->southeast != -1 )
        {
            lua_pushstring( L, "southeast" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->southeast );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->west != -1 )
        {
            lua_pushstring( L, "west" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->west );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->east != -1 )
        {
            lua_pushstring( L, "east" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->east );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->up != -1 )
        {
            lua_pushstring( L, "up" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->up );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->down != -1 )
        {
            lua_pushstring( L, "down" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->down );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->in != -1 )
        {
            lua_pushstring( L, "in" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->in );
            lua_settable(L, -3);
        }
        if( pHost->mpMap->rooms[id]->out != -1 )
        {
            lua_pushstring( L, "out" );
            lua_pushnumber( L, pHost->mpMap->rooms[id]->out );
            lua_settable(L, -3);
        }
        return 1;
    }
    else
        return 0;
}

int TLuaInterpreter::searchRoom( lua_State *L )
{
    int room_id = 0;
    bool gotRoomID = false;
    string room;
    if( lua_isnumber( L, 1 ) )
    {
        room_id = lua_tointeger( L, 1 );
        gotRoomID = true;
    }
    else if( lua_isstring( L, 1 ) )
    {
        room = lua_tostring( L, 1 );
    }
    else
    {
        lua_pushstring( L, "searchRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( gotRoomID )
    {
        if( pHost->mpMap->rooms.contains( room_id ) )
        {
            lua_pushstring( L, pHost->mpMap->rooms[room_id]->name.toLatin1().data() );
            return 1;
        }
        else
        {
            lua_pushstring( L, "searchRoom ERROR: no such room" );
            return 1;
        }
    }
    else
    {
        QMapIterator<int, TRoom *> it( pHost->mpMap->rooms );
        lua_newtable(L);
        while( it.hasNext() )
        {
            it.next();
            int i = it.key();
            if( pHost->mpMap->rooms[i]->name.contains( QString(room.c_str()), Qt::CaseInsensitive ) )
            {
                QString name = pHost->mpMap->rooms[i]->name;
                int roomID = pHost->mpMap->rooms[i]->id;
                lua_pushnumber( L, roomID );
                lua_pushstring( L, name.toLatin1().data() );
                lua_settable(L, -3);
            }
        }
        return 1;
    }
}

int TLuaInterpreter::searchRoomUserData( lua_State *L )
{
        //basically stolen from searchRoom
        //if we have, searchUserData(key, value)
        //we look through all room ids in a given field for the string and
        //return a table of roomids matching
    string key, value;
    if( lua_isstring( L, 1 ) )
    {
        key = lua_tostring( L, 1 );
    }
    else
    {
        lua_pushstring( L, "searchRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
        if( lua_isstring( L, 2 ) )
    {
        value = lua_tostring( L, 2 );
    }
    else
    {
        lua_pushstring( L, "searchRoomUserData: wrong 2nd argument type" );
        lua_error( L );
        return 1;
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        QMapIterator<int, TRoom *> it( pHost->mpMap->rooms );
        lua_newtable(L);
        while( it.hasNext() )
        {
                it.next();
                int i = it.key();
                QString _key = key.c_str();
        QString _value = value.c_str();
                if (pHost->mpMap->rooms[i]->userData.contains( _key ))
                {
                        QString _keyValue = pHost->mpMap->rooms[i]->userData[_key];
                        if (_keyValue.contains(_value))
                        {
                                lua_pushnumber( L, i );
                                lua_pushstring( L, _keyValue.toLatin1().data() );
                                lua_settable(L, -3);
                        }
                }
        }
        return 1;
}

int TLuaInterpreter::getAreaTable( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QMapIterator<int, QString> it( pHost->mpMap->areaNamesMap );
    lua_newtable(L);
    while( it.hasNext() )
    {
        it.next();
        int roomID = it.key();
        QString name = it.value();
        lua_pushstring( L, name.toLatin1().data() );
        lua_pushnumber( L, roomID );
        lua_settable(L, -3);
    }
    return 1;
}

int TLuaInterpreter::getAreaTableSwap( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QMapIterator<int, QString> it( pHost->mpMap->areaNamesMap );
    lua_newtable(L);
    while( it.hasNext() )
    {
        it.next();
        int roomID = it.key();
        QString name = it.value();
        lua_pushnumber( L, roomID );
        lua_pushstring( L, name.toLatin1().data() );
        lua_settable(L, -3);
    }
    return 1;
}

int TLuaInterpreter::getAreaRooms( lua_State *L )
{
    int area;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getAreaRooms: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->areas.contains( area ) )
    {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    for( int i=0; i<pHost->mpMap->areas[area]->rooms.size(); i++ )
    {
        int roomID = pHost->mpMap->areas[area]->rooms.at( i );
        lua_pushnumber( L, i );
        lua_pushnumber( L, roomID );
        lua_settable(L, -3);
    }
    return 1;
}

int TLuaInterpreter::getRooms( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_newtable(L);
    QMapIterator<int,TRoom*> it(pHost->mpMap->rooms);
    while( it.hasNext() )
    {
        it.next();
        lua_pushnumber( L, it.key() );
        lua_pushstring( L, it.value()->name.toLatin1().data() );
        lua_settable(L, -3);
    }
    return 1;
}



int TLuaInterpreter::getRoomWeight( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( pHost->mpMap->mRoomId ) )
    {
        lua_pushnumber( L, pHost->mpMap->rooms[pHost->mpMap->mRoomId]->weight );
    }

    return 1;
}

int TLuaInterpreter::gotoRoom( lua_State *L )
{
    int r;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "gotoRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool ret = pHost->mpMap->gotoRoom( r );
    pHost->startSpeedWalk();
    lua_pushboolean( L, ret );
    return 1;
}

int TLuaInterpreter::getPath( lua_State *L )
{
    int r1;
    int r2;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getPath: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r1 = lua_tonumber( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "getPath: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r2 = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool ret = pHost->mpMap->gotoRoom( r1, r2 );
    pHost->assemblePath();
    lua_pushboolean( L, ret );
    return 1;
}

int TLuaInterpreter::deselect( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = luaWindowName.c_str();
    if( luaWindowName.size() < 1 || luaWindowName == "main" )
    {
        pHost->mpConsole->deselect();
    }
    else
    {
        mudlet::self()->deselect( pHost, name );
    }
    return 0;
}

int TLuaInterpreter::reset( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = luaWindowName.c_str();
    if( luaWindowName.size() < 1 || luaWindowName == "main" )
    {
        pHost->mpConsole->reset();
    }
    else
    {
        mudlet::self()->resetFormat( pHost, name );
    }
    return 0;
}

int TLuaInterpreter::hasFocus( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushboolean( L, pHost->mpConsole->hasFocus() );//FIXME
    return 1;
}

int TLuaInterpreter::echoUserWindow( lua_State *L )
{
    string luaWindowName="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "echoUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    string luaSendText="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "echoUserWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString windowName(luaWindowName.c_str());
    mudlet::self()->echoWindow( pHost, windowName, text );
    return 0;
}

//qApp->setStyleSheet("QMainWindow::separator{border: 0px;width: 0px; height: 0px; padding: 0px;} QMainWindow::separator:hover {background: red;}");
int TLuaInterpreter::setAppStyleSheet( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";
    qApp->setStyleSheet( luaWindowName.c_str() );
}

// this is an internal only function used by the package system
int TLuaInterpreter::showUnzipProgress( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "showUnzipProgress: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    QString txt = luaSendText.c_str();
    mudlet::self()->showUnzipProgress( txt );
    return 0;
}

#ifdef Q_OS_LINUX
    #include <phonon>
#else
    #include <Phonon>
#endif

int TLuaInterpreter::playSoundFile( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "playSoundFile: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    QString sound = luaSendText.c_str();
    //QSound::play( QString( luaSendText.c_str() ) );
    if( QDir::homePath().contains('\\') )
    {
        sound.replace('/', "\\");
    }
    else
    {
        sound.replace('\\', "/");
    }
    mudlet::self()->playSound( sound );
    return 0;
}

int TLuaInterpreter::moveCursorEnd( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( luaWindowName == "main" )
        pHost->mpConsole->moveCursorEnd();
    else
       mudlet::self()->moveCursorEnd( pHost, windowName );
    return 0;
}

int TLuaInterpreter::getLastLineNumber( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "getLastLineNumber: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    int number;
    if( luaWindowName == "main" )
        number = pHost->mpConsole->getLastLineNumber();
    else
        number = mudlet::self()->getLastLineNumber( pHost, windowName );
    lua_pushnumber( L, number );
    return 1;
}



int TLuaInterpreter::getMudletHomeDir( lua_State * L )
{
    QString home = QDir::homePath();
    home.append( "/.config/mudlet/profiles/" );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = pHost->getName();
    home.append( name );
    QString erg = QDir::toNativeSeparators( home );
    lua_pushstring( L, erg.toLatin1().data() );
    return 1;
}

int TLuaInterpreter::disconnect( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.disconnect();
    return 0;
}

int TLuaInterpreter::reconnect( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.connectIt( pHost->getUrl(), pHost->getPort() );
    return 0;
}

int TLuaInterpreter::setTriggerStayOpen( lua_State *L )
{
    string luaWindowName;
    double b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setTriggerStayOpen: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setTriggerStayOpen: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_tonumber( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getTriggerUnit()->setTriggerStayOpen( QString( luaWindowName.c_str() ), static_cast<int>(b) );
    return 0;
}

int TLuaInterpreter::setLink( lua_State * L )
{
    string luaWindowName;
    string linkText;
    string linkFunction;
    string linkHint;
    int s = 1;
    if( lua_gettop( L ) > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setLink: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    /*if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "setLink: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        linkText = lua_tostring( L, s );
        s++;
    }*/

    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "setLink: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        linkFunction = lua_tostring( L, s );
        s++;
    }
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "setLink: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        linkHint = lua_tostring( L, s );
        s++;
    }


    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    QString _linkText = "";//QString(linkText.c_str());
    QStringList _linkFunction;
    _linkFunction <<  QString(linkFunction.c_str());
    QStringList _linkHint;
    _linkHint << QString(linkHint.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setLink( pHost, windowName, _linkText, _linkFunction, _linkHint );
    else
        pHost->mpConsole->setLink( _linkText, _linkFunction, _linkHint );
    return 0;
}

int TLuaInterpreter::setPopup( lua_State *L )
{
    string a1 = "";
    string a2;
    QStringList _hintList;
    QStringList _commandList;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop( L );
    // console name is an optional first argument
    if( n > 4 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setPopup: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "setPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a2 = lua_tostring( L, s );
        s++;
    }

    if( ! lua_istable( L, s ) )
    {
        lua_pushstring( L, "setPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString cmd = lua_tostring( L, -1 );
                _commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( ! lua_istable( L, s ) )
    {
        lua_pushstring( L, "setPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString hint = lua_tostring( L, -1 );
                _hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( n >= s )
    {
        customFormat = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString txt = a2.c_str();
    QString name = a1.c_str();
    if( _commandList.size() != _hintList.size() )
    {
        lua_pushstring( L, "Error: command list size and hint list size do not match cannot create popup" );
        lua_error( L );
        return 1;
    }

    if( a1 == "" )
    {
        pHost->mpConsole->setLink( txt, _commandList, _hintList );
    }
    else
    {
        mudlet::self()->setLink( pHost, name, txt, _commandList, _hintList );
    }

    return 0;
}


int TLuaInterpreter::setBold( lua_State * L )
{
    string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setBold: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "setBold: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setBold( pHost, windowName, b );
    else
        pHost->mpConsole->setBold( b );
    return 0;
}

int TLuaInterpreter::setItalics( lua_State *L )
{
        string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setItalics: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "setItalics: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setItalics( pHost, windowName, b );
    else
        pHost->mpConsole->setItalics( b );
    return 0;
}
int TLuaInterpreter::setUnderline( lua_State *L )
{
    string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "setUnderline: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "setUnderline: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setUnderline( pHost, windowName, b );
    else
        pHost->mpConsole->setUnderline( b );
    return 0;
}

int TLuaInterpreter::debug( lua_State *L )
{
    int nbargs = lua_gettop(L);
    QString luaDebugText="";
    for (int i=0; i<nbargs; i++)
    {
        luaDebugText += (nbargs > 1 ? " [" + QString::number(i) + "] " : " ") + lua_tostring( L, i+1 );
    }
    return 0;
}

int TLuaInterpreter::hideToolBar( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "hideToolBar: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getActionUnit()->hideToolBar( windowName );
    return 0;
}

int TLuaInterpreter::showToolBar( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "showToolBar: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getActionUnit()->showToolBar( windowName );
    return 0;
}

int TLuaInterpreter::sendATCP( lua_State *L )
{
    string msg;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "sendATCP: what do you want to send?" );
        lua_error( L );
        return 1;
    }
    else
    {
        msg = lua_tostring( L, 1 );
    }

    string what;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "sendATCP: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        what = lua_tostring( L, 2 );
    }
    string _h;
    _h += TN_IAC;
    _h += TN_SB;
    _h += 200;
    _h += msg;
    if (what != "") {
      _h += " ";
      _h += what;
    }
    _h += TN_IAC;
    _h += TN_SE;

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.socketOutRaw( _h );
    return 0;
}

int TLuaInterpreter::sendGMCP( lua_State *L )
{
    string msg;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "sendGMCP: what do you want to send?" );
        lua_error( L );
        return 1;
    }
    else
    {
        msg = lua_tostring( L, 1 );
    }

    string what;
    if( lua_isstring( L, 2 ) )
    {
        what = lua_tostring( L, 2 );
    }
    string _h;
    _h += TN_IAC;
    _h += TN_SB;
    _h += GMCP;
    _h += msg;
    if( what != "" )
    {
        _h += " ";
        _h += what;
    }
    _h += TN_IAC;
    _h += TN_SE;

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.socketOutRaw( _h );
    return 0;
}

int TLuaInterpreter::sendTelnetChannel102( lua_State *L )
{
    string msg;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "sendTelnetChannel102: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        msg = lua_tostring( L, 1 );
    }
    string _h;
    _h += TN_IAC;
    _h += TN_SB;
    _h += 102;
    _h += msg;
    _h += TN_IAC;
    _h += TN_SE;

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.socketOutRaw( _h );
    return 0;
}

int TLuaInterpreter::getButtonState( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int state;
    state = pHost->mpConsole->getButtonState();
    lua_pushnumber( L, state );
    return 1;
}

int TLuaInterpreter::getNetworkLatency( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double number;
    number = pHost->mTelnet.networkLatency;
    lua_pushnumber( L, number );
    return 1;
}

int TLuaInterpreter::getMainConsoleWidth( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int fw = QFontMetrics(pHost->mDisplayFont).width("W");
    fw *= pHost->mWrapAt + 1;
    lua_pushnumber( L, fw );
    return 1;
}

int TLuaInterpreter::getMainWindowSize( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QSize mainWindowSize = pHost->mpConsole->getMainWindowSize();

    lua_pushnumber( L, mainWindowSize.width() );
    lua_pushnumber( L, mainWindowSize.height() );

    return 2;
}

// tempTimer(int session, float seconds, string function to call, string name) // one shot timer.
int TLuaInterpreter::tempTimer( lua_State *L )
{
    double luaTimeout;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "tempTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTimeout = lua_tonumber( L, 1 );
    }

    string luaFunction;
    if( lua_isfunction( L, 2 ) )
    {
        Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
        QString _fun;
        int timerID = pLuaInterpreter->startTempTimer( luaTimeout, _fun );
        TTimer * pT = pHost->getTimerUnit()->getTimer( timerID );
        pT->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata( L, pT );
        lua_pushvalue( L, 2 );
        lua_settable( L, LUA_REGISTRYINDEX );
        lua_pushnumber( L, timerID );
        return 1;
    }
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempTimer( luaTimeout, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::tempExactMatchTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _reg = luaRegex.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempExactMatchTrigger( _reg, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::tempBeginOfLineTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _reg = luaRegex.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempBeginOfLineTrigger( _reg, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}


// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _reg = luaRegex.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempTrigger( _reg, _fun );
    lua_pushnumber( L, timerID );
    //lua_pushstring( L, _reg.toLatin1().data());
    return 1;
}


// temporary color trigger. args: ansiFGColorCode, ansiBgColorCode, luaCode
int TLuaInterpreter::tempColorTrigger( lua_State *L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "tempColorTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 1 );
    }
    int luaTo;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "tempColorTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo = lua_tointeger( L, 2 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "tempColorTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempColorTrigger( luaFrom, luaTo, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}


// triggerID = tempLineTrigger( from, howmany, func )
int TLuaInterpreter::tempLineTrigger( lua_State *L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "tempLineTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 1 );
    }
    int luaTo;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "tempLineTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo = lua_tointeger( L, 2 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "tempLineTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempLineTrigger( luaFrom, luaTo, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

// tempTrigger( string name, string regex, string function to call, multiline, fg, bg, filter, match all(perlSlashG), highlight,
// play sound, fire length(mStayOpen), lineDelta).
int TLuaInterpreter::tempComplexRegexTrigger( lua_State *L )
{
    bool multiLine, matchAll, highlight, playSound, filter, colorTrigger;
    int fireLength, lineDelta;
    QString fgColor, bgColor;
    QStringList regexList;
    QString script, parent, pattern, soundFile;
    QColor hlFgColor, hlBgColor;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        pattern = lua_tostring( L, 2 );
    }
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        script = lua_tostring( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        multiLine = lua_tonumber( L, 4 );
    }
    if( lua_isnumber( L, 5 ) )
    {
        colorTrigger = false;
    }
    else
    {
        colorTrigger = true;
        fgColor = lua_tostring( L, 5 );
    }
    if( lua_isnumber( L, 6 ) )
    {
        colorTrigger = false;
    }
    else
    {
        bgColor = lua_tostring( L, 6 );
    }

    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        filter = lua_tonumber( L, 7 );
    }
    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        matchAll = lua_tonumber( L, 8 );
    }
    if(lua_isnumber( L, 9 ) )
    {
        highlight = false;
    }
    else
    {
        highlight = true;
        hlFgColor.setNamedColor(lua_tostring( L, 9 ));
    }
    if(lua_isnumber( L, 9 ) )
    {
        highlight = false;
    }
    else
    {
        highlight = true;
        hlBgColor.setNamedColor(lua_tostring( L, 9 ));
    }
    //lineDelta).
    if(lua_isnumber( L, 10 ) )
    {
        playSound = false;
    }
    else
    {
        playSound = true;
        soundFile = lua_tostring( L, 10 );
    }
    if( ! lua_isnumber( L, 11 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fireLength = lua_tonumber( L, 11 );
    }
    if( ! lua_isnumber( L, 12 ) )
    {
        lua_pushstring( L, "tempComplexRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lineDelta = lua_tonumber( L, 12 );
    }

    Host * mpHost = TLuaInterpreter::luaInterpreterMap[L];
    //TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    //mpHost = pLuaInterpreter->mpHost;
    TTrigger * pT;
    QList<int> propertyList;

    TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );

    if( !pP )
    {
        regexList << pattern;
        if (colorTrigger)
            propertyList << REGEX_COLOR_PATTERN;
        else
            propertyList << REGEX_PERL;
    }
    else{
        regexList = pP->getRegexCodeList();
        propertyList = pP->getRegexCodePropertyList();
    }

    pT = new TTrigger( "a", regexList, propertyList, multiLine, mpHost );
    //pT->setRegexCodeList( regexList, propertyList );
    pT->setIsFolder( 0 );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();
    pT->setScript( script );
    pT->setName( pattern );
    //pT->setIsMultiline( multiLine );
    pT->mPerlSlashGOption = matchAll;//match all
    pT->mFilterTrigger = filter;
    pT->setConditionLineDelta( lineDelta );//line delta
    pT->mStayOpen = fireLength;//fire length
    pT->mSoundTrigger = playSound;//sound trigger, need to set sound file if true
    if (playSound){
        pT->setSound(soundFile);
    }
    pT->setIsColorizerTrigger(highlight); //highlight
    if (highlight){
        pT->setFgColor( hlFgColor );
        pT->setBgColor( hlBgColor );
    }
    //lua_pushnumber( L, pT->getID() );
    lua_pushstring( L, pattern.toLatin1().data());
    return 1;
}

int TLuaInterpreter::tempButton( lua_State *L){
    //args: parent, name, orientation
    QString toolbar, name;
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << toolbar;
    int orientation;
    if(!lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong first arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        toolbar = lua_tostring( L, 1 );
    }
    if(!lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong 2nd arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 2 );
    }
    if(!lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong first arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        orientation = lua_tonumber( L, 3 );
    }


    Host * mpHost = TLuaInterpreter::luaInterpreterMap[L];
    TAction * pP = mpHost->getActionUnit()->findAction( toolbar );
    if (!pP) return 0;
    TAction * pT = mpHost->getActionUnit()->findAction( name );
    if (pT) return 0;
    pT = new TAction( pP, mpHost );
    pT->setName( name );
    pT->setCommandButtonUp( cmdButtonUp );
    pT->setCommandButtonDown( cmdButtonDown );
    pT->setIsPushDownButton( false );
    pT->mLocation = pP->mLocation;
    pT->mOrientation = orientation;
    pT->setScript( script );
    pT->setIsFolder( false );
    pT->setIsActive( true );



//    pT->setIsPushDownButton( isChecked );
//    pT->mLocation = location;
//    pT->mOrientation = orientation;
//    pT->setIsActive( pT->shouldBeActive() );
//    pT->setButtonColor( color );
//    pT->setButtonRotation( rotation );
//    pT->setButtonColumns( columns );
////      pT->setButtonFlat( flatButton );
//    pT->mUseCustomLayout = useCustomLayout;
//    pT->mPosX = posX;
//    pT->mPosY = posY;
//    pT->mSizeX = sizeX;
//    pT->mSizeY = sizeY;
//    pT->css = mpActionsMainArea->css->toPlainText();


    pT->registerAction();
    int childID = pT->getID();
    mpHost->getActionUnit()->updateToolbar();
    return 1;
}

int TLuaInterpreter::tempButtonToolbar( lua_State *L  )
{//args: name, location(0-4), orientation(0/1)
    QString name;
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << name;
    int location, orientation;

    if(!lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong first arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }
    if(!lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong first arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        location = lua_tonumber( L, 2 );
    }
    if(!lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "tempButtonToolbar: wrong first arg" );
        lua_error( L );
        return 1;
    }
    else
    {
        orientation = lua_tonumber( L, 3 );
    }

    if( location > 0 ) location++;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TAction * pT = pHost->getActionUnit()->findAction( name );
    if (pT) return 0;

        //insert a new root item
    //ROOT_ACTION:

    pT = new TAction( name, pHost );
    pT->setCommandButtonUp( cmdButtonUp );
    QStringList nl;
    nl << name;

    pT->setName( name );
    pT->setCommandButtonUp( cmdButtonUp );
    pT->setCommandButtonDown( cmdButtonDown );
    pT->setIsPushDownButton( false );
    pT->mLocation = location;
    pT->mOrientation = orientation;
    pT->setScript( script );
    pT->setIsFolder( true );
    pT->setIsActive( true );
    pT->registerAction();
    int childID = pT->getID();
    pHost->getActionUnit()->updateToolbar();




    return 1;
}

// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempRegexTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaRegex.c_str();
    int timerID = pLuaInterpreter->startTempRegexTrigger( _luaRegex, _luaFunction );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::tempAlias( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "tempAlias: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "tempAlias: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaRegex.c_str();
    int timerID = pLuaInterpreter->startTempAlias( _luaRegex, _luaFunction );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::exists( lua_State * L )
{
    string _name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "exists: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _name = lua_tostring( L, 1 );
    }
    string _type;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "exists: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _type = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int cnt = 0;
    QString type = _type.c_str();
    type = type.toLower();
    QString name = _name.c_str();
    if( type == "timer")
    {
        QMap<QString, TTimer *>::const_iterator it1 = pHost->getTimerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTimerUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    else if( type == "trigger")
    {
        QMap<QString, TTrigger *>::const_iterator it1 = pHost->getTriggerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTriggerUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    else if( type == "alias")
    {
        QMap<QString, TAlias *>::const_iterator it1 = pHost->getAliasUnit()->mLookupTable.find( name );
        while( it1 != pHost->getAliasUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    lua_pushnumber( L, cnt );
    return 1;
}

int TLuaInterpreter::isActive( lua_State * L )
{
    string _name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "isActive: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _name = lua_tostring( L, 1 );
    }
    string _type;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "isActive: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _type = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int cnt = 0;
    QString type = _type.c_str();
    type = type.toLower();
    QString name = _name.c_str();
    if( type == "timer")
    {
        QMap<QString, TTimer *>::const_iterator it1 = pHost->getTimerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTimerUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    else if( type == "trigger")
    {
        QMap<QString, TTrigger *>::const_iterator it1 = pHost->getTriggerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTriggerUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    else if( type == "alias")
    {
        QMap<QString, TAlias *>::const_iterator it1 = pHost->getAliasUnit()->mLookupTable.find( name );
        while( it1 != pHost->getAliasUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    lua_pushnumber( L, cnt );
    return 1;
}


int TLuaInterpreter::permAlias( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "permAlias: need a name for this alias" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }

    string luaParent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "permAlias: need a parent alias/group to add this alias to" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaParent = lua_tostring( L, 2 );
    }

    string luaRegex;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "permAlias: need the pattern for the alias" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 3 );
    }


    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "permAlias: need Lua code for this alias" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaName = luaName.c_str();
    QString _luaParent = luaParent.c_str();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaRegex.c_str();
    int aliasID = pLuaInterpreter->startPermAlias( _luaName, _luaParent, _luaRegex, _luaFunction );
    lua_pushnumber( L, aliasID );
    return 1;
}

int TLuaInterpreter::permTimer( lua_State * L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "permTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }
    string luaParent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "permTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaParent = lua_tostring( L, 2 );
    }

    double luaTimeout;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "permTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTimeout = lua_tonumber( L, 3 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "permTimer: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = luaName.c_str();
    QString _parent = luaParent.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startPermTimer( _name, _parent, luaTimeout, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::permSubstringTrigger( lua_State * L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "permSubstringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "permSubstringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "permSubstringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "permSubstringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermSubstringTrigger( _name,
                                                          _parent,
                                                          _regList,
                                                          _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

int TLuaInterpreter::permBeginOfLineStringTrigger( lua_State * L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "permBeginOfLineStringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "permBeginOfLineStringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "permBeginOfLineStringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "permBeginOfLineStringTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermBeginOfLineStringTrigger( _name,
                                                                  _parent,
                                                                  _regList,
                                                                  _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

int TLuaInterpreter::permRegexTrigger( lua_State *L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "permRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "permRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "permRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "permRegexTrigger: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermRegexTrigger( _name,
                                                      _parent,
                                                      _regList,
                                                      _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

#include <QFileDialog>

int TLuaInterpreter::invokeFileDialog( lua_State * L )
{
    bool luaDir = false; //default is to chose a directory
    if( ! lua_isboolean( L, 1 ) )
    {
        lua_pushstring( L, "invokeFileDialog: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaDir = lua_toboolean( L, 1 );
    }
    string luaTitle;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "invokeFileDialog: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTitle = lua_tostring( L, 2 );
    }
    if( ! luaDir )
    {
        QString fileName = QFileDialog::getExistingDirectory(0, QString( luaTitle.c_str()),
                                                        QDir::currentPath() );
        lua_pushstring( L, fileName.toLatin1().data() );
        return 1;
    }
    else
    {
        QString fileName = QFileDialog::getOpenFileName(0, QString( luaTitle.c_str()),
                                                        QDir::currentPath() );
        lua_pushstring( L, fileName.toLatin1().data() );
        return 1;
    }
}

int TLuaInterpreter::getTimestamp( lua_State * L )
{
    int luaLine;
    int args = lua_gettop( L );
    int n = 1;
    string name = "";
    if( args < 1 )
    {

        return 0;
    }
    if( args == 2 )
    {
        if( lua_isstring( L, n ) )
        {
            name = lua_tostring( L, n );
            n++;
        }
    }

    if( ! lua_isnumber( L, n ) )
    {
        lua_pushstring( L, "getTimestamp: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaLine = lua_tointeger( L, n );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( name == "" )
    {
        if( luaLine > 0 && luaLine < pHost->mpConsole->buffer.timeBuffer.size() )
        {
            lua_pushstring( L, pHost->mpConsole->buffer.timeBuffer.at(luaLine).toLatin1().data() );
        }
        else
        {
            lua_pushstring( L, "getTimestamp: invalid line number");
        }
        return 1;
    }
    QString _name = name.c_str();
    QMap<QString, TConsole *> & dockWindowConsoleMap = mudlet::self()->mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( _name ) )
    {
        TConsole * pC = dockWindowConsoleMap[_name];
        if( luaLine > 0 && luaLine < pC->buffer.timeBuffer.size() )
        {
            lua_pushstring( L, pC->buffer.timeBuffer.at(luaLine).toLatin1().data() );
        }
        else
        {
            lua_pushstring( L, "getTimestamp: invalid line number");
        }
        return 1;
    }
    return 0;
}

int TLuaInterpreter::setBorderColor( lua_State *L )
{
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setBorderColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRed = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setBorderColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaGreen=lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setBorderColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaBlue = lua_tointeger( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QPalette framePalette;
    framePalette.setColor( QPalette::Text, QColor(0,0,0) );
    framePalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    framePalette.setColor( QPalette::Window, QColor( luaRed, luaGreen, luaBlue, 255 ) );
    pHost->mpConsole->mpMainFrame->setPalette( framePalette );
    return 0;
}


int TLuaInterpreter::setRoomCoordinates( lua_State *L )
{
    int id;
    int x;
    int y;
    int z;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomCoordinates: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setRoomCoordinates: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setRoomCoordinates: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tointeger( L, 3 );
    }

    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "setRoomCoordinates: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        z = lua_tointeger( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushboolean(L, pHost->mpMap->setRoomCoordinates( id, x, y, z ) );
    return 1;
}

int TLuaInterpreter::setCustomEnvColor( lua_State *L )
{
    int id;
    int r;
    int g;
    int b;
    int alpha;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setCustomEnvColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setCustomEnvColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r = lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setCustomEnvColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        g = lua_tointeger( L, 3 );
    }

    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "setCustomEnvColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_tointeger( L, 4 );
    }

    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "setCustomEnvColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        alpha = lua_tointeger( L, 5 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpMap->customEnvColors[id] = QColor( r,g,b,alpha );
    return 0;
}


int TLuaInterpreter::setAreaName( lua_State *L )
{
    int id;
    string name;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setAreaName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setAreaName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString _name = name.c_str();
    pHost->mpMap->areaNamesMap[id] = _name;
    return 0;
}

int TLuaInterpreter::getRoomAreaName( lua_State *L )
{
    int id;
    string name;
    bool gotString = false;
    if( ! lua_isnumber( L, 1 ) )
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "getRoomAreaName: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            name = lua_tostring( L, 1 );
            gotString = true;
        }
    }
    else
    {
        id = lua_tonumber( L, 1 );
    }

    QString _name = name.c_str();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( gotString )
    {
        QList<int> idList = pHost->mpMap->areaNamesMap.keys( _name );
        if( idList.size() > 0 )
        {
            lua_pushnumber( L, idList[0] );
            return 1;
        }
        else
        {
            lua_pushnumber( L, -1 );
            return 1;
        }
    }
    else
    {
        if( pHost->mpMap->areaNamesMap.contains( id ) )
        {
            lua_pushstring( L, pHost->mpMap->areaNamesMap[id].toLatin1().data() );
        }
        else
            lua_pushnumber( L, -1 );
        return 1;
    }
}

int TLuaInterpreter::addAreaName( lua_State *L )
{
    int id;
    string name;

    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "addAreaName: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString _name = name.c_str();
    id = pHost->mpMap->areaNamesMap.size()+1;


    if( ! pHost->mpMap->areaNamesMap.values().contains( _name ) )
    {
        while( pHost->mpMap->areaNamesMap.contains( id ) )
        {
            id++;
        }
        pHost->mpMap->areaNamesMap[id] = _name;
        pHost->mpMap->buildAreas();
        if( pHost->mpMap->mpMapper )
        {
            pHost->mpMap->mpMapper->showArea->clear();
            QMapIterator<int, QString> it( pHost->mpMap->areaNamesMap );
            //sort them alphabetically (case sensitive)
            QMap <QString, QString> areaNames;
            while( it.hasNext() )
            {
                it.next();
                QString name = it.value();
                areaNames.insert(name.toLower(), name);
            }

            QMapIterator<QString, QString> areaIt( areaNames );
            while( areaIt.hasNext() )
            {
                areaIt.next();
                pHost->mpMap->mpMapper->showArea->addItem( areaIt.value() );
            }
        }
        lua_pushnumber( L, id );
    }
    else
        lua_pushnumber( L, -1 );
    return 1;
}

int TLuaInterpreter::deleteArea( lua_State *L )
{
    int id = -1;
    string name;

    if( lua_isnumber( L, 1 ) )
    {
        id = lua_tonumber( L, 1 );
        if( id == -1 ) return 0;
    }
    else if( lua_isstring( L, 1 ) )
    {
        name = lua_tostring( L, 1 );
    }
    else
    {
        lua_pushstring( L, "deleteArea: wrong argument type" );
        lua_error( L );
        return 1;
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( id == -1 )
    {
        QString _name = name.c_str();
        if( pHost->mpMap->areaNamesMap.values().contains( _name ) )
        {
            pHost->mpMap->deleteArea( pHost->mpMap->areaNamesMap.key(_name) );
            pHost->mpMap->mMapGraphNeedsUpdate = false;
            pHost->mpMap->areaNamesMap.remove( pHost->mpMap->areaNamesMap.key(_name) );
        }
        else
            pHost->mpMap->areaNamesMap.remove( pHost->mpMap->areaNamesMap.key(_name) );
    }
    else
    {
        if( pHost->mpMap->areas.contains( id ) )
        {
            pHost->mpMap->deleteArea( id );
            pHost->mpMap->mMapGraphNeedsUpdate = false;
            pHost->mpMap->areaNamesMap.remove( id );
        }
        else
            pHost->mpMap->areaNamesMap.remove( id );
    }

    return 0;
}

int TLuaInterpreter::deleteRoom( lua_State *L )
{
    int id;

    if( lua_isnumber( L, 1 ) )
    {
        id = lua_tonumber( L, 1 );
        if( id <= 0 ) return 0;
    }
    else
    {
        lua_pushstring( L, "addAreaName: wrong argument type" );
        lua_error( L );
        return 1;
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
       pHost->mpMap->deleteRoom( id );
       pHost->mpMap->mMapGraphNeedsUpdate = false;
    }
    return 0;
}


int TLuaInterpreter::setExit( lua_State *L )
{
    int from, to;
    int dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        from = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        to = lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tointeger( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushboolean(L, pHost->mpMap->setExit( from, to, dir ) );
    pHost->mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

int TLuaInterpreter::getRoomCoordinates( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomCoordinates: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushnil( L );
        lua_pushnil( L );
        lua_pushnil( L );

        return 3;
    }
    lua_pushnumber( L, pHost->mpMap->rooms[id]->x );
    lua_pushnumber( L, pHost->mpMap->rooms[id]->y );
    lua_pushnumber( L, pHost->mpMap->rooms[id]->z );
    return 3;
}

int TLuaInterpreter::getRoomArea( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber( L, pHost->mpMap->rooms[id]->area );
    return 1;
}


int TLuaInterpreter::roomExists( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) || ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "roomExists: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushboolean( L, 1 );
        return 1;
    }
    lua_pushboolean( L, 0 );
    return 1;
}

int TLuaInterpreter::addRoom( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushboolean( L, pHost->mpMap->addRoom( id ) );
    pHost->mpMap->mMapGraphNeedsUpdate = true;

    return 1;
}

int TLuaInterpreter::createRoomID( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushnumber( L, pHost->mpMap->createNewRoomID() );
    return 1;
}

int TLuaInterpreter::unHighlightRoom( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->rooms.contains( id ) ) return 0;
    pHost->mpMap->rooms[id]->highlight = false;
    if( pHost->mpMap )
        if( pHost->mpMap->mpMapper )
            pHost->mpMap->mpMapper->mp2dMap->update();
    return 0;
}

// highlightRoom( roomID, colorRed, colorGreen, colorBlue, col2Red, col2Green, col2Blue, (float)highlightRadius, alphaColor1, alphaColor2 )

int TLuaInterpreter::highlightRoom( lua_State * L )
{
    int id, fgr, fgg, fgb, bgr, bgg, bgb;
    float radius;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgr = lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgg = lua_tointeger( L, 3 );
    }

    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgb = lua_tointeger( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgr = lua_tointeger( L, 5 );
    }

    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgg = lua_tointeger( L, 6 );
    }

    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgb = lua_tointeger( L, 7 );
    }
    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        radius = lua_tonumber( L, 8 );
    }
    int alpha1, alpha2;
    if( ! lua_isnumber( L, 9 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        alpha1 = lua_tointeger( L, 9 );
    }
    if( ! lua_isnumber( L, 10 ) )
    {
        lua_pushstring( L, "highlightRoom: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        alpha2 = lua_tointeger( L, 10 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( !pHost->mpMap ) return 0;
    if( ! pHost->mpMap->rooms.contains( id ) ) return 0;
    QColor fg = QColor(fgr,fgg,fgb,alpha1);
    QColor bg = QColor(bgr,bgg,bgb,alpha2);
    pHost->mpMap->rooms[id]->highlight = true;
    pHost->mpMap->rooms[id]->highlightColor = fg;
    pHost->mpMap->rooms[id]->highlightColor2 = bg;
    pHost->mpMap->rooms[id]->highlightRadius = radius;

    if( pHost->mpMap->mpMapper )
        if( pHost->mpMap->mpMapper->mp2dMap )
            pHost->mpMap->mpMapper->mp2dMap->update();
    return 0;
}


//SYNTAX: int labelID = createMapLabel( int area, string text, float posx, float posy, int fgRed, int fgGreen, int fgBlue, bgRed, int bgGreen, int bgBlue, bool showOnTop=true, bool noScaling=true )
int TLuaInterpreter::createMapLabel( lua_State * L )
{
    int area, fgr, fgg, fgb, bgr, bgg, bgb;
    float posx, posy, posz;
    int fontSize = 50;
    float zoom = 30.0;
    string text;
    bool showOnTop = true;
    bool noScaling = true;

    int args = lua_gettop(L);
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        text = lua_tostring( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posx = lua_tonumber( L, 3 );
    }

    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posy = lua_tonumber( L, 4 );
    }

        if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posz = lua_tonumber( L, 5 );
    }

    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgr = lua_tointeger( L, 6 );
    }

    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgg = lua_tointeger( L, 7 );
    }

    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fgb = lua_tointeger( L, 8 );
    }

    if( ! lua_isnumber( L, 9 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgr = lua_tointeger( L, 9 );
    }

    if( ! lua_isnumber( L, 10 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgg = lua_tointeger( L, 10 );
    }

    if( ! lua_isnumber( L, 11 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bgb = lua_tointeger( L, 11 );
    }

    if( args > 11 )
    {
        if( ! lua_isnumber( L, 12 ) )
        {
            lua_pushstring( L, "createMapLabel: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            zoom = lua_tonumber( L, 12 );
        }
        if( ! lua_isnumber( L, 13 ) )
        {
            lua_pushstring( L, "createMapLabel: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            fontSize = lua_tointeger( L, 13 );
        }
        if( args > 13 )
        {
            if( ! lua_isboolean( L, 14 ) )
            {
                lua_pushstring( L, "createMapLabel: wrong argument type" );
                lua_error( L );
                return 1;
            }
            else
            {
                showOnTop = lua_toboolean( L, 14 );
            }

        }
        if( args > 14 )
        {
            if( ! lua_isboolean( L, 15 ) )
            {
                lua_pushstring( L, "createMapLabel: wrong argument type" );
                lua_error( L );
                return 1;
            }
            else
            {
                noScaling = lua_toboolean( L, 15 );
            }

        }
    }

    QString _text = text.c_str();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QColor fg = QColor(fgr,fgg,fgb);
    QColor bg = QColor(bgr, bgg, bgb);
    lua_pushinteger( L, pHost->mpMap->createMapLabel( area, _text, posx, posy, posz, fg, bg, showOnTop, noScaling, zoom, fontSize ) );
    return 1;
}

int TLuaInterpreter::setMapZoom( lua_State * L )
{
    int zoom = 3;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        zoom = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap )
        if( pHost->mpMap->mpMapper )
            if( pHost->mpMap->mpMapper->mp2dMap )
                pHost->mpMap->mpMapper->mp2dMap->setMapZoom( zoom );
    return 0;
}

//SYNTAX: int labelID = createMapImageLabel( int area, string filePath, float posx, float posy, float posz, float width, float height, float zoom, bool showOnTop=true )
int TLuaInterpreter::createMapImageLabel( lua_State * L )
{
    int area;
    float posx, posy, posz, width, height, zoom;
    string text;
    bool showOnTop = true;

    int args = lua_gettop(L);
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        text = lua_tostring( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posx = lua_tonumber( L, 3 );
    }

    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posy = lua_tonumber( L, 4 );
    }

    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        posz = lua_tonumber( L, 5 );
    }

    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 6 );
    }

    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 7 );
    }

    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        zoom = lua_tonumber( L, 8 );
    }

    if( ! lua_isboolean( L, 9 ) )
    {
        lua_pushstring( L, "createMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        showOnTop = lua_toboolean( L, 9 );
    }

    QString _text = text.c_str();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    lua_pushinteger( L, pHost->mpMap->createMapImageLabel( area, _text, posx, posy, posz, width, height, zoom, showOnTop, false ) );
    return 1;
}

//SYNTAX: setDoor( roomID, exitCommand, doorStatus ) status: 0=no door, 1=open, 2=closed, 3=locked
//        to remove a door set doorStatus = 0
int TLuaInterpreter::setDoor( lua_State * L )
{
    int roomID;
    int doorStatus; //0= no door, 1=open, 2=closed, 3=locked
    string exitCmd;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setDoor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setDoor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        exitCmd = lua_tostring( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "roomID: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        doorStatus = lua_tonumber( L, 3 );
    }

    QString exit = exitCmd.c_str();
    exit = exit.toLower();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap )
    {
        if( pHost->mpMap->rooms.contains(roomID) )
        {
            TRoom * pR = pHost->mpMap->rooms[roomID];
            if( doorStatus == 0 )
                pR->doors.remove( exit );
            else
                pR->doors[exit] = doorStatus;
            if( pHost->mpMap->mpMapper )
                if( pHost->mpMap->mpMapper->mp2dMap )
                    pHost->mpMap->mpMapper->mp2dMap->update();
        }
    }
    return 0;
}

//SYNTAX: doors table = getDoors( roomID )
int TLuaInterpreter::getDoors( lua_State * L )
{
    int roomID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushfstring( L, "getDoors: bad argument #1 (room ID as number expected, got %s)", luaL_typename(L, 1) );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap )
    {
        if( pHost->mpMap->rooms.contains(roomID) )
        {
            TRoom * pR = pHost->mpMap->rooms[roomID];
            lua_newtable(L);
            QStringList keys = pR->doors.keys();
            for( int i=0; i<keys.size(); i++ )
            {
                lua_pushstring( L, keys[i].toLatin1().data() );
                lua_pushnumber( L, pR->doors[keys[i]] );
                lua_settable(L, -3);
            }
            return 1;
        }
    }
    return 0;
}


//SYNTAX: setExitWeight( roomID, exitCommand, exitWeight )
int TLuaInterpreter::setExitWeight( lua_State * L )
{
    int roomID;
    int weight;
    string text;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setExitWeight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }

    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setExitWeight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        text = lua_tostring( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "setExitWeight: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        weight = lua_tonumber( L, 3 );
    }

    QString _text = text.c_str();
    _text = _text.toLower();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap )
    {
        if( pHost->mpMap->rooms.contains(roomID) )
        {
            TRoom * pR = pHost->mpMap->rooms[roomID];
            pR->exitWeights[_text] = weight;
            pHost->mpMap->mMapGraphNeedsUpdate = true;
        }
    }
    return 0;
}

//SYNTAX: exit weight table = getExitWeights( roomID )
int TLuaInterpreter::getExitWeights( lua_State * L )
{
    int roomID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getExitWeights: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap )
    {
        if( pHost->mpMap->rooms.contains(roomID) )
        {
            TRoom * pR = pHost->mpMap->rooms[roomID];
            lua_newtable(L);
            QStringList keys = pR->exitWeights.keys();
            for( int i=0; i<keys.size(); i++ )
            {
                lua_pushstring( L, keys[i].toLatin1().data() );
                lua_pushnumber( L, pR->exitWeights[keys[i]] );
                lua_settable(L, -3);
            }
            return 1;
        }
    }
    return 0;
}

int TLuaInterpreter::deleteMapLabel( lua_State * L )
{
    int area, labelID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "deleteMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "deleteMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        labelID = lua_tointeger( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpMap->deleteMapLabel( area, labelID );
    return 0;
}

int TLuaInterpreter::getMapLabels( lua_State * L )
{
    int area;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "deleteMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->mapLabels.contains( area ) )
    {
        lua_newtable(L);
        QMapIterator<int,TMapLabel> it(pHost->mpMap->mapLabels[area]);
        while( it.hasNext() )
        {
            it.next();
            lua_pushnumber( L, it.key() );
            lua_pushstring( L, it.value().text.toLatin1().data() );
            lua_settable(L, -3);
        }
    }
    return 1;
}

int TLuaInterpreter::getMapLabel( lua_State * L )
{
    int area, labelId = -1;
    QString labelText;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }
    if (!lua_isstring(L,2) && !lua_isnumber(L,2)){
        lua_pushstring( L, "getMapLabel: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else{
        if (lua_isnumber(L,2))
            labelId = lua_tointeger(L,2);
        else
            labelText = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->mapLabels.contains( area ) )
    {
        lua_newtable(L);
        if (labelId != -1){
            if (pHost->mpMap->mapLabels[area].contains(labelId)){
                TMapLabel label =  pHost->mpMap->mapLabels[area][labelId];
                qDebug()<<"here";
                int x = label.pos.x();
                int y = label.pos.y();
                int z = label.pos.z();
                qDebug()<<"herea";
                float height = label.size.height();
                float width = label.size.width();
                qDebug()<<"hereb";
                QString text = label.text;
                qDebug()<<"herec";

                lua_pushstring( L, "X" );
                lua_pushnumber( L, x );
                lua_settable(L, -3);
                lua_pushstring( L, "Y" );
                lua_pushnumber( L, y );
                lua_settable(L, -3);
                lua_pushstring( L, "Z" );
                lua_pushnumber( L, z );
                lua_settable(L, -3);
                lua_pushstring( L, "Height" );
                lua_pushnumber( L, height );
                lua_settable(L, -3);
                lua_pushstring( L, "Width" );
                lua_pushnumber( L, width );
                lua_settable(L, -3);
                lua_pushstring( L, "Text" );
                lua_pushstring( L, text.toLatin1().data() );
                lua_settable(L, -3);
            }
            else{
                lua_pushstring( L, "getMapLabel: labelId doesn't exist" );
                lua_error( L );
                return 1;
            }
        }
        else{
            QMapIterator<int,TMapLabel> it(pHost->mpMap->mapLabels[area]);
            while( it.hasNext() )
            {
                it.next();
                if(it.value().text==labelText){
                    TMapLabel label = it.value();
                    lua_newtable(L);
                    int id = it.key();
                    int x = label.pos.x();
                    int y = label.pos.y();
                    int z = label.pos.z();
                    float height = label.size.height();
                    float width = label.size.width();
                    QString text = label.text;
                    qDebug()<<"here";
                    lua_pushstring( L, "X" );
                    lua_pushnumber( L, x );
                    lua_settable(L, -3);
                    lua_pushstring( L, "Y" );
                    lua_pushnumber( L, y );
                    lua_settable(L, -3);
                    lua_pushstring( L, "Z" );
                    lua_pushnumber( L, z );
                    lua_settable(L, -3);
                    lua_pushstring( L, "Height" );
                    lua_pushnumber( L, height );
                    lua_settable(L, -3);
                    lua_pushstring( L, "Width" );
                    lua_pushnumber( L, width );
                    lua_settable(L, -3);
                    lua_pushstring( L, "Text" );
                    lua_pushstring( L, text.toLatin1().data() );
                    lua_settable(L, -3);
                    lua_pushnumber(L, id);
                    lua_insert(L,-2);
                    lua_settable(L, -3);

                }
            }
        }
    }
    return 1;
}


int TLuaInterpreter::addSpecialExit( lua_State * L )
{
    int id_from, id_to;
    string dir;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "addSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_from = lua_tointeger( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "addSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_to = lua_tointeger( L, 2 );
    }
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "addSpecialExit: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        dir = lua_tostring( L, 3 );
    }
    QString _dir = dir.c_str();
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id_from ) )
    {
        if( pHost->mpMap->rooms.contains( id_to ) )
        {
            pHost->mpMap->rooms[id_from]->addSpecialExit( id_to, _dir );
            pHost->mpMap->rooms[id_from]->setSpecialExitLock( id_to, _dir, false );
            pHost->mpMap->mMapGraphNeedsUpdate = true;
        }
    }
    return 0;
}

int TLuaInterpreter::clearRoomUserData( lua_State * L )
{
    int id_from;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "clearRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_from = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id_from ) )
    {
        pHost->mpMap->rooms[id_from]->userData.clear();
    }
    return 0;
}

int TLuaInterpreter::clearSpecialExits( lua_State * L )
{
    int id_from;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "clearSpecialExits: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_from = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id_from ) )
    {
        pHost->mpMap->rooms[id_from]->other.clear();
        pHost->mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

int TLuaInterpreter::getSpecialExits( lua_State * L )
{
    int id_from;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getSpecialExits: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_from = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id_from ) )
    {
        QMapIterator<int, QString> it(pHost->mpMap->rooms[id_from]->other);
        lua_newtable(L);
        while( it.hasNext() )
        {
            it.next();
            lua_newtable(L);
            int id_to = it.key();
            QString dir = it.value();
            QString exitStatus;
            if( dir.size() > 0 && ( dir.startsWith('0') || dir.startsWith('1')) )
                exitStatus = dir.left(1);
            else
                exitStatus = "0";
            QString exit;
            if( dir.size() > 0 && ( dir.startsWith('0') || dir.startsWith('1')) )
                exit = dir.remove(0,1);
            else
                exit = dir;
            lua_pushstring( L, exit.toLatin1().data() );//done to remove the prepended special exit status
            lua_pushstring( L, exitStatus.toLatin1().data() );//done to remove the prepended special exit status
            lua_settable(L, -3);
            lua_pushnumber(L, id_to);
            lua_insert(L,-2);
            lua_settable(L, -3);
        }
        return 1;
    }
    return 0;
}

int TLuaInterpreter::getSpecialExitsSwap( lua_State * L )
{
    int id_from;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getSpecialExitsSwap: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id_from = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( id_from ) )
    {
        QMapIterator<int, QString> it(pHost->mpMap->rooms[id_from]->other);
        lua_newtable(L);
        while( it.hasNext() )
        {
            it.next();
            int id_to = it.key();
            QString dir = it.value();
            //lua_pushstring( L, dir.toLatin1().data() );
            QString exitStatus;
            QString exit;
            if( dir.size() > 0 && ( dir.startsWith('0') || dir.startsWith('1')) )
                exitStatus = dir.left(1);
            else
                exitStatus = "0";

            if( dir.size() > 0 && ( dir.startsWith('0') || dir.startsWith('1')) )
                exit = dir.remove(0,1);
            else
                exit = dir;
            lua_pushstring( L, exit.toLatin1().data() );
            lua_pushnumber( L, id_to );
            lua_settable(L, -3);
        }
        return 1;
    }
    return 0;
}

int TLuaInterpreter::getRoomEnv( lua_State * L )
{
    int roomID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomEnv: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( roomID ) )
    {
        lua_pushnumber( L, pHost->mpMap->rooms[roomID]->environment );
        return 1;
    }
    return 0;
}

int TLuaInterpreter::getRoomUserData( lua_State * L )
{
    int roomID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }
    string key;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "getRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        key = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( roomID ) )
    {
        QString _key = key.c_str();
        if( pHost->mpMap->rooms[roomID]->userData.contains( _key ) )
        {
            lua_pushstring( L, pHost->mpMap->rooms[roomID]->userData[_key].toLatin1().data() );
            return 1;
        }
        else
        {
            lua_pushstring( L, "" );
            return 1;
        }
    }
    else
    {
        lua_pushstring( L, "" );
        return 1;
    }
}

int TLuaInterpreter::setRoomUserData( lua_State * L )
{
    int roomID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        roomID = lua_tointeger( L, 1 );
    }
    string key;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "getRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        key = lua_tostring( L, 2 );
    }
    string value;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "getRoomUserData: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        value = lua_tostring( L, 3 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->rooms.contains( roomID ) )
    {
        QString _key = key.c_str();
        QString _value = value.c_str();
        pHost->mpMap->rooms[roomID]->userData[_key] = _value;
        lua_pushboolean( L, true );
        return 1;
    }
    else
    {
        lua_pushboolean( L, false );
        return 1;
    }
}



int TLuaInterpreter::downloadFile( lua_State * L )
{
    string path, url;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "downloadFile: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        path = lua_tostring( L, 1 );
    }
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "downloadFile: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        url = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString _url = url.c_str();
    QString _path = path.c_str();
    QNetworkRequest request = QNetworkRequest( QUrl( _url ) );
    if ( _path.contains("https") )
    {
        QSslConfiguration config( QSslConfiguration::defaultConfiguration() );
        request.setSslConfiguration( config );
    }
    QNetworkReply * reply = pHost->mLuaInterpreter.mpFileDownloader->get( request );
    pHost->mLuaInterpreter.downloadMap[reply] = _path;
    return 0;

}


int TLuaInterpreter::setRoomArea( lua_State * L )
{
    int id, area;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    pHost->mpMap->setRoomArea( id, area );
    return 0;
}

int TLuaInterpreter::setRoomChar( lua_State * L )
{
    int id;
    string c;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        c = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushstring( L, "setRoomArea: room ID does not exist");
        lua_error( L );
        return 1;
    }
    else
    {
        if( c.size() >= 1 )
        {
            pHost->mpMap->rooms[id]->c = c[0];
        }
    }
    return 0;
}

int TLuaInterpreter::getRoomChar( lua_State * L )
{
    int id;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomChar: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        id = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->rooms.contains( id ) )
    {
        lua_pushstring( L, "getRoomChar: room ID does not exist");
        lua_error( L );
        return 1;
    }
    else
    {
        QString c = (QString)pHost->mpMap->rooms[id]->c;
        lua_pushstring( L, c.toLatin1().data() );
        return 1;
    }
    return 0;
}


int TLuaInterpreter::getRoomsByPosition( lua_State * L )
{
    int area, x, y, z;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "getRoomsByPosition: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "getRoomsByPosition: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tointeger( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "getRoomsByPosition: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tointeger( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "getRoomsByPosition: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        z = lua_tointeger( L, 4 );
    }


    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->areas.contains( area ) )
    {
        lua_pushnil( L );
        return 1;
    }
    QList<int> rL = pHost->mpMap->areas[area]->getRoomsByPosition( x, y, z);
    lua_newtable( L );
    for( int i=0; i<rL.size(); i++ )
    {
        lua_pushnumber( L, i );
        lua_pushnumber( L, rL[i] );
        lua_settable(L, -3);
    }
    return 1;
}


// returns true if area exits, otherwise false
int TLuaInterpreter::setGridMode( lua_State * L )
{
    int area;
    bool gridMode = false;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        area = lua_tointeger( L, 1 );
    }
    if( ! lua_isboolean( L, 2 ) )
    {
        lua_pushstring( L, "setRoomArea: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        gridMode = lua_toboolean( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost->mpMap->areas.contains( area ) )
    {
        lua_pushboolean( L, false);
        return 1;
    }
    else
    {
        pHost->mpMap->areas[area]->gridMode = gridMode;
        pHost->mpMap->areas[area]->calcSpan();
        if( pHost->mpMap->mpMapper )
        {
            if( pHost->mpMap->mpMapper->mp2dMap )
            {
                pHost->mpMap->mpMapper->mp2dMap->init();
                cout << "NEW GRID MAP: init" << endl;
            }
        }
    }
    lua_pushboolean( L, true );
    return 1;
}



int TLuaInterpreter::setFgColor( lua_State *L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( n > 3 )
    {
        if( lua_isstring( L, s ) )
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setFgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRed = lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setFgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaGreen=lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setFgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaBlue = lua_tointeger( L, s );
    }

    QString _name( a1.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n < 4 )
        pHost->mpConsole->setFgColor( luaRed, luaGreen, luaBlue );
    else
        mudlet::self()->setFgColor( pHost, _name, luaRed, luaGreen, luaBlue );
    return 0;
}

int TLuaInterpreter::setBgColor( lua_State *L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( n > 3 )
    {
        if( lua_isstring( L, s ) )
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setBgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRed = lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setBgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaGreen=lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "setBgColor: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaBlue = lua_tointeger( L, s );
    }

    QString _name( a1.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n < 4 )
        pHost->mpConsole->setBgColor( luaRed, luaGreen, luaBlue );
    else
        mudlet::self()->setBgColor( pHost, _name, luaRed, luaGreen, luaBlue );
    return 0;
}

// params: [windowName,] "printWhat", "LuaScript", "hint", [boolean=true -> use current format else use standard link format]
int TLuaInterpreter::insertLink( lua_State *L )
{
    QStringList sL;
    int n = lua_gettop( L );
    int s = 1;
    bool b = false;
    bool gotBool = false;
    for( ; s<=n; s++ )
    {
        if( lua_isstring( L, s ) )
        {
            string _str = lua_tostring( L, s );
            QString qs = _str.c_str();
            sL << qs;
        }
        else if( lua_isboolean( L, s ) )
        {
            gotBool = true;
            b = lua_toboolean( L, s );
        }
    }

    if( sL.size() < 4 ) sL.prepend("main");
    if( sL.size() < 4 )
    {
        lua_pushstring( L, "insertLink: wrong number of params or wrong type of params" );
        lua_error( L );
        return 1;
    }

    QString _name( sL[0] );
    QString printScreen = sL[1];
    QStringList command;
    QStringList hint;
    command << sL[2];
    hint << sL[3];

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( _name == "main" )
    {
        pHost->mpConsole->insertLink( printScreen, command, hint, b );
    }
    else
    {
        mudlet::self()->insertLink( pHost, _name, printScreen, command, hint, b );
    }

    return 0;
}

int TLuaInterpreter::insertPopup( lua_State *L )
{
    string a1 = "";
    string a2;
    QStringList _hintList;
    QStringList _commandList;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop( L );
    // console name is an optional first argument
    if( n > 4 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "insertPopup: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "insertPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a2 = lua_tostring( L, s );
        s++;
    }

    if( ! lua_istable( L, s ) )
    {
        lua_pushstring( L, "insertPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString cmd = lua_tostring( L, -1 );
                _commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( ! lua_istable( L, s ) )
    {
        lua_pushstring( L, "insertPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString hint = lua_tostring( L, -1 );
                _hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( n >= s )
    {
        customFormat = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString txt = a2.c_str();
    QString name = a1.c_str();
    if( _commandList.size() != _hintList.size() )
    {
        lua_pushstring( L, "Error: command list size and hint list size do not match cannot create popup" );
        lua_error( L );
        return 1;
    }

    if( a1 == "" )
    {
        pHost->mpConsole->insertLink( txt, _commandList, _hintList, customFormat );
    }
    else
    {
        mudlet::self()->insertLink( pHost, name, txt, _commandList, _hintList, customFormat );
    }

    return 0;
}

int TLuaInterpreter::insertText( lua_State *L )
{
    string a1;
    string a2;
    int n = lua_gettop( L );
    int s = 1;
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "insertText: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }
    QString _name( a1.c_str() );

    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "insertText: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n == 1 )
        pHost->mpConsole->insertText( QString(a1.c_str()) );
    else
        mudlet::self()->insertText( pHost, _name, QString( a2.c_str() ) );
    return 0;
}

int TLuaInterpreter::insertHTML( lua_State *L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "insertHTML: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->insertHTML( QString(luaSendText.c_str()) );
    return 0;
}

int TLuaInterpreter::addSupportedTelnetOption( lua_State *L )
{
    int option;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "addSupportedTelnetOption: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        option = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.supportedTelnetOptions[option] = true;
    return 0;
}

int TLuaInterpreter::Echo( lua_State *L )
{
    string a1;
    string a2;
    int s = 1;
    int n = lua_gettop( L );

    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "Echo: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "Echo: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString txt;
    QString name;
    if( n == 1 )
    {
        txt = a1.c_str();
        pHost->mpConsole->echo( txt );
    }



    else
    {
        txt = a2.c_str();
        name = a1.c_str();
        mudlet::self()->echoWindow( pHost, name, txt );
    }

    return 0;
}

int TLuaInterpreter::echoPopup( lua_State *L )
{
    string a1 = "";
    string a2;
    QStringList _hintList;
    QStringList _commandList;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop( L );
    // console name is an optional first argument
    if( n > 4 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "echoPopup: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "echoPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a2 = lua_tostring( L, s );
        s++;
    }

    if( ! lua_istable( L, s ) )

    {
        lua_pushstring( L, "echoPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString cmd = lua_tostring( L, -1 );
                _commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( ! lua_istable( L, s ) )
    {
        lua_pushstring( L, "echoPopup: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, s ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString hint = lua_tostring( L, -1 );
                _hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if( n >= s )
    {
        customFormat = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString txt = a2.c_str();
    QString name = a1.c_str();
    if( _commandList.size() != _hintList.size() )
    {
        lua_pushstring( L, "Error: command list size and hint list size do not match cannot create popup" );
        lua_error( L );
        return 1;
    }

    if( a1 == "" )
    {
        pHost->mpConsole->echoLink( txt, _commandList, _hintList, customFormat );
    }
    else
    {
        mudlet::self()->echoLink( pHost, name, txt, _commandList, _hintList, customFormat );
    }

    return 0;
}


int TLuaInterpreter::echoLink( lua_State *L )
{
    string a1;
    string a2;
    string a3;
    string a4;
    bool a5 = false;
    bool gotBool = false;

    int s = 1;
    int n = lua_gettop( L );

    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "echoLink: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "echoLink: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
            s++;
        }
    }
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "echoLink: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a3 = lua_tostring( L, s );
            s++;
        }
    }
    if( n > 3 )
    {
        if( lua_isstring( L, s ) )
        {
            a4 = lua_tostring( L, s );
            s++;
        }
        else if( lua_isboolean( L, s ) )
        {
            gotBool = true;
            a5 = lua_toboolean( L, s );
            s++;
        }
    }
    if( n > 4 )
    {
        if( ! lua_isboolean( L, s ) )
        {
            lua_pushstring( L, "echoLink: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a5 = lua_toboolean( L, s );
            gotBool = true;
            s++;
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString txt;
    QString name;
    QStringList func;
    QStringList hint;
    if( n == 3 || ( n == 4 && gotBool ) )
    {
        txt = a1.c_str();
        func << a2.c_str();
        hint << a3.c_str();
        pHost->mpConsole->echoLink( txt, func, hint, a5 );
    }
    else
    {
        txt = a2.c_str();
        func << a3.c_str();
        hint << a4.c_str();
        name = a1.c_str();
        mudlet::self()->echoLink( pHost, name, txt, func, hint, a5 );
    }

    return 0;
}

int TLuaInterpreter::setMergeTables( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    QStringList modulesList;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)
    {
        if (!lua_isstring(L, i))
        {
            lua_pushfstring( L, "setMergeTables: bad argument #%d (string expected, got %s)", i, luaL_typename(L, 1) );
            lua_error(L);
            return 1;
        }
        modulesList << QString(lua_tostring(L, i));
    }

    pHost->mGMCP_merge_table_keys = pHost->mGMCP_merge_table_keys + modulesList;
    pHost->mGMCP_merge_table_keys.removeDuplicates();

    return 0;
}

int TLuaInterpreter::pasteWindow( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "pasteWindow: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name( luaName.c_str());
    mudlet::self()->pasteWindow( pHost, name );
    return 0;
}

int TLuaInterpreter::exportAreaImage( lua_State *L )
{
    int areaID;
    if( lua_isnumber( L, 1 ) )
    {
        areaID = lua_tointeger( L, 1 );
        Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        pHost->mpMap->mpMapper->mp2dMap->exportAreaImage( areaID );
    }
    return 0;
}

int TLuaInterpreter::openUrl( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "openUrl: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }
    QString url( luaName.c_str());
    QDesktopServices::openUrl(url);
    return 0;
}

int TLuaInterpreter::setLabelStyleSheet( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "setLabelStyleSheet: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string a2;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "setLabelStyleSheet: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a2 = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    //qDebug()<<"CSS: name:"<<luaSendText.c_str()<<"<"<<a2.c_str()<<">";
    pHost->mpConsole->setLabelStyleSheet( luaSendText, a2 );
    return 0;
}

int TLuaInterpreter::getCustomEnvColorTable( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( pHost->mpMap->customEnvColors.size() > 0 )
    {
        lua_newtable( L );
        QList<int> colorList = pHost->mpMap->customEnvColors.keys();
        for( int idx=0; idx<colorList.size(); idx++ )
        {
            lua_pushnumber( L, colorList[idx] );
            lua_newtable( L );
            // red component
            {
                lua_pushnumber( L, 1 );
                lua_pushnumber( L, pHost->mpMap->customEnvColors[colorList[idx]].red() );
                lua_settable( L, -3 );//match in matches
            }
            // green component
            {
                lua_pushnumber( L, 2 );
                lua_pushnumber( L, pHost->mpMap->customEnvColors[colorList[idx]].green() );
                lua_settable( L, -3 );//match in matches
            }
            // blue component
            {
                lua_pushnumber( L, 3 );
                lua_pushnumber( L, pHost->mpMap->customEnvColors[colorList[idx]].blue() );
                lua_settable( L, -3 );//match in matches
            }
            lua_settable( L, -3 );//matches in regex
        }
    }
    else
    {
        lua_newtable( L );
    }
    return 1;
}

//syntax: getTime( bool return_string, string time_format ) with return_string == false -> return table
int TLuaInterpreter::getTime( lua_State * L )
{
    int n = lua_gettop( L );
    bool return_string = false;
    QString fmt = "yyyy.MM.dd hh:mm:ss.zzz";
    QString tm;
    if( n > 0 )
    {
        return_string = lua_toboolean( L, 1 );
        if( n > 1 )
        {
            if( ! lua_isstring( L, 2 ) )
            {
                lua_pushstring( L, "getTime: wrong argument type" );
                lua_error( L );
                return 1;
            }
            else
            {
                fmt = lua_tostring( L, 2 );
            }
        }
    }
    QDateTime time = QDateTime::currentDateTime();
    if( return_string )
    {
        tm = time.toString( fmt );
        lua_pushstring( L, tm.toLatin1().data() );
    }
    else
    {
        QDate dt = time.date();
        QTime tm = time.time();
        lua_createtable( L, 0, 4 );
        lua_pushstring( L, "hour" );
        lua_pushinteger( L, tm.hour() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "min" );
        lua_pushinteger( L, tm.minute() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "sec" );
        lua_pushinteger( L, tm.second() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "msec" );
        lua_pushinteger( L, tm.msec() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "year" );
        lua_pushinteger( L, dt.year() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "month" );
        lua_pushinteger( L, dt.month() );
        lua_rawset( L, n+1 );
        lua_pushstring( L, "day" );
        lua_pushinteger( L, dt.day() );
        lua_rawset( L, n+1 );
    }
    return 1;
}


int TLuaInterpreter::appendBuffer( lua_State *L )
{
    string a1;
    string a2;
    int s = 1;
    int n = lua_gettop( L );
    if( n > 0 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "appendBuffer: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( s == 1 )
    {
        pHost->mpConsole->appendBuffer();
    }
    else
    {
        QString name = a1.c_str();
        mudlet::self()->appendBuffer( pHost, name );
    }

    return 0;
}

int TLuaInterpreter::appendCmdLine( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "appendCmdLine(): wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString curText = pHost->mpConsole->mpCommandLine->toPlainText();
    pHost->mpConsole->mpCommandLine->setPlainText( curText + QString( luaSendText.c_str() ) );
    QTextCursor cur = pHost->mpConsole->mpCommandLine->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pHost->mpConsole->mpCommandLine->setTextCursor(cur);
    return 0;
}

int TLuaInterpreter::installPackage( lua_State * L )
{
    string event;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "installPackage(): wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        event = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString package = event.c_str();
    if( pHost )
        pHost->installPackage( package, 0 );
}

int TLuaInterpreter::uninstallPackage( lua_State * L )
{
    string event;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "installPackage(): wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        event = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString package = event.c_str();
    if( pHost )
        pHost->uninstallPackage( package, 0 );
}

int TLuaInterpreter::registerAnonymousEventHandler( lua_State * L )
{
    string event;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "appendCmdLine(): wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        event = lua_tostring( L, 1 );
    }
    string func;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "appendCmdLine(): wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        func = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString e = event.c_str();
    QString f = func.c_str();
    pHost->registerAnonymousEventHandler( e, f );
    return 0;
}


int TLuaInterpreter::Send( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "Send: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    bool wantPrint = true;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isboolean( L, 2 ) )
        {
            lua_pushstring( L, "Send: wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            wantPrint = lua_toboolean( L, 2 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->send( QString( luaSendText.c_str() ), wantPrint, false );
    return 0;
}

int TLuaInterpreter::printCmdLine( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "printCmdLine: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->mpCommandLine->setPlainText( QString( luaSendText.c_str() ) );
    QTextCursor cur = pHost->mpConsole->mpCommandLine->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pHost->mpConsole->mpCommandLine->setTextCursor(cur);
    return 0;
}

int TLuaInterpreter::clearCmdLine( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->mpCommandLine->clear();
    return 0;
}

int TLuaInterpreter::sendRaw( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushfstring( L, "sendRaw: bad argument #1 (string expected, got %s)", luaL_typename(L, 1) );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    bool wantPrint = true;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isboolean( L, 2 ) )
        {
            lua_pushfstring( L, "sendRaw: bad argument #2 (boolean expected, got %s)", luaL_typename(L, 2) );
            lua_error( L );
            return 1;
        }
        else
        {
            wantPrint = lua_toboolean( L, 2 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->send( QString(luaSendText.c_str()), wantPrint, true );
    return 0;
}

int TLuaInterpreter::sendSocket( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "sendSocket: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.socketOutRaw( luaSendText );
    return 0;
}

#include "dlgIRC.h"
int TLuaInterpreter::sendIrc( lua_State * L )
{
    string who;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "sendSocket: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        who = lua_tostring( L, 1 );
    }
    string text;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "sendSocket: wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        text = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString chan = who.c_str();
    QString txt = text.c_str();
    if( ! mudlet::self()->mpIRC ) return 0;
    mudlet::self()->mpIRC->session->cmdMessage( chan, txt );
    return 0;
}


bool TLuaInterpreter::compileAndExecuteScript( QString & code )
{
    if( code.size() < 1 ) return false;
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ) qDebug()<<"LUA ERROR: code did not compile: ERROR:"<<e.c_str();
        emit signalEchoMessage( mHostID, QString( e.c_str() ) );
    }

    lua_pop( L, lua_gettop( L ) );

    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compileScript( QString & code )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    /*lua_newtable( L );

    // set values
    for( int i=0; i<matches.size(); i++ )
    {
        lua_pushnumber( L, i+1 ); // Lua indexes start with 1
        lua_pushstring( L, matches[i].toLatin1().data() );
        lua_settable( L, -3 );
    }
    lua_setglobal( L, "matches" );*/

    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: code did not compile: ERROR:"<<e.c_str()<<"\n">>0;}
    }
    else
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA: code compiled without errors. OK\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );

    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compile( QString & code )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: code did not compile: ERROR:"<<e.c_str()<<"\n">>0;}
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA: code compiled without errors. OK\n" >> 0;}
    }
    lua_pop( L, lua_gettop( L ) );

    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compile( QString & code, QString & errorMsg )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "Lua syntax error:";
        if( lua_isstring( L, 1 ) )
        {
            e.append( lua_tostring( L, 1 ) );
        }
        errorMsg = "<b><font color='blue'>";
        errorMsg.append( e.c_str() );
        errorMsg.append("</b></font>");
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"\n "<<e.c_str()<<"\n">>0;}
    }
    else
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"\nLUA: code compiled without errors. OK\n" >> 0;}
    }
    lua_pop( L, lua_gettop( L ) );

    if( error == 0 ) return true;
    else return false;
}

void TLuaInterpreter::setMultiCaptureGroups( const std::list< std::list<std::string> > & captureList,
                                             const std::list< std::list<int> > & posList )
{
    mMultiCaptureGroupList = captureList;
    mMultiCaptureGroupPosList = posList;

    /*std::list< std::list<string> >::const_iterator mit = mMultiCaptureGroupList.begin();

    int k=1;
    for( ; mit!=mMultiCaptureGroupList.end(); mit++, k++ )
    {
        cout << "regex#"<<k<<" got:"<<endl;
        std::list<string>::const_iterator it = (*mit).begin();
        for( int i=1; it!=(*mit).end(); it++, i++ )
        {
            cout << i<<"#"<<"<"<<*it<<">"<<endl;
        }
        cout << "-----------------------------"<<endl;
    }*/
}

void TLuaInterpreter::setCaptureGroups( const std::list<std::string> & captureList, const std::list<int> & posList )
{
    mCaptureGroupList = captureList;
    mCaptureGroupPosList = posList;

    /*std::list<string>::iterator it2 = mCaptureGroupList.begin();
    std::list<int>::iterator it1 = mCaptureGroupPosList.begin();
    int i=0;
    for( ; it1!=mCaptureGroupPosList.end(); it1++, it2++, i++ )
    {
        cout << "group#"<<i<<" begin="<<*it1<<" len="<<(*it2).size()<<"word="<<*it2<<endl;
    } */
}

void TLuaInterpreter::clearCaptureGroups()
{
    mCaptureGroupList.clear();
    mCaptureGroupPosList.clear();
    mMultiCaptureGroupList.clear();
    mMultiCaptureGroupPosList.clear();

    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
    }

    lua_newtable( L );
    lua_setglobal( L, "matches" );
    lua_newtable( L );
    lua_setglobal( L, "multimatches" );

    lua_pop( L, lua_gettop( L ) );
}


void TLuaInterpreter::adjustCaptureGroups( int x, int a )
{
    // adjust all capture group positions in line if data has been inserted by the user
    typedef std::list<int>::iterator I;
    for( I it=mCaptureGroupPosList.begin(); it!=mCaptureGroupPosList.end(); it++ )
    {
        if( *it >= x )
        {
            *it += a;
        }
    }
}

void TLuaInterpreter::setAtcpTable( QString & var, QString & arg )
{
    lua_State * L = pGlobalLua;
    lua_getglobal( L, "atcp" ); //defined in LuaGlobal.lua
    lua_pushstring( L, var.toLatin1().data() );
    lua_pushstring( L, arg.toLatin1().data() );
    lua_rawset( L, -3 );
    lua_pop( L, 1 );

    TEvent event;
    event.mArgumentList.append( var );
    event.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    event.mArgumentList.append( arg );
    event.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->raiseEvent( & event );
}


void TLuaInterpreter::setGMCPTable(QString & key, QString & string_data)
{
    lua_State * L = pGlobalLua;
    lua_getglobal(L, "gmcp");   //defined in Lua init
    if( !lua_istable(L, -1) )
    {
        qDebug()<<"ERROR: gmcp not defined -> error in LuaGlobal.lua";
        return;
    }
    // key is in format of Blah.Blah or Blah.Blah.Bleh - we want to push & pre-create the tables as appriate

    QStringList tokenList = key.split(".");
    if( ! lua_checkstack( L, tokenList.size()+5 ) ) return;
    int i = 0;
    for( ; i<tokenList.size()-1; i++ )
    {
        lua_getfield(L, -1, tokenList[i].toLatin1().data());
        if( !lua_istable(L, -1) )
        {
            lua_pop(L, 1);
            lua_pushstring(L, tokenList[i].toLatin1().data());
            lua_newtable(L);
            lua_rawset(L, -3);
            lua_getfield(L, -1, tokenList[i].toLatin1().data());
        }
        lua_remove(L, -2);
    }
    bool __needMerge = false;
    lua_getfield(L, -1, tokenList[i].toLatin1().data());
    if( lua_istable(L, -1) )
    {
        // only merge tables (instead of replacing them) if the key has been registered as a need to merge key by the user default is Char.Status only
        if( mpHost->mGMCP_merge_table_keys.contains( key ) )
        {
            __needMerge = true;
        }
    }
    lua_pop( L, 1 );
    if( ! __needMerge )
        lua_pushstring(L, tokenList[i].toLatin1().data());
    else
        lua_pushstring(L, "__needMerge");

    lua_getglobal(L, "json_to_value");

    if( !lua_isfunction(L, -1) )
    {
        lua_settop(L, 0);
        qDebug()<<"CRITICAL ERROR: json_to_value not defined";
        return;
    }
    lua_pushlstring(L, string_data.toLatin1().data(), string_data.size());
    int error = lua_pcall(L, 1, 1, 0);
    if( error == 0 )
    {
        // Top of stack should now contain the lua representation of json.
        lua_rawset(L, -3);
        if( __needMerge )
        {
            lua_settop(L, 0);
            lua_getglobal(L, "__gmcp_merge_gmcp_sub_tables");
            if( !lua_isfunction(L, -1) )
            {
               lua_settop(L, 0);
               qDebug()<<"CRITICAL ERROR: __gmcp_merge_gmcp_sub_tables is not defined in lua_LuaGlobal.lua";
               return;
            }
            lua_getglobal(L, "gmcp");
            i = 0;
            for( ; i<tokenList.size()-1; i++ )
            {
                lua_getfield(L, -1, tokenList[i].toLatin1().data());
                lua_remove(L, -2);
            }
            lua_pushstring( L, tokenList[i].toLatin1().data());
            lua_pcall(L, 2, 0, 0);
        }
    }
    lua_settop(L, 0);

    // events: for key "foo.bar.top" we raise: gmcp.foo, gmcp.foo.bar and gmcp.foo.bar.top
    // with the actual key given as parameter e.g. event=gmcp.foo, param="gmcp.foo.bar"

    QString token = "gmcp";
    key.prepend("gmcp.");
    for( int k=0; k<tokenList.size(); k++ )
    {
        TEvent event;
        token.append( "." );
        token.append( tokenList[k] );
        event.mArgumentList.append( token );
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append( key );
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
        if( mudlet::debugMode )
        {
            QString msg = "\nGMCP event <";
            msg.append( token );
            msg.append("> display(gmcp) to see the full content\n");
            pHost->mpConsole->printSystemMessage(msg);
        }
        pHost->raiseEvent( &event );
    }
    // auto-detect IRE composer
    if( tokenList.size() == 3 && tokenList.at(0) == "IRE" && tokenList.at(1) == "Composer" && tokenList.at(2) == "Edit")
    {
        QRegExp rx("\\{ \"title\": \"(.*)\", \"text\": \"(.*)\" \\}");
        if( rx.indexIn(string_data) != -1 )
        {
            QString title = rx.cap(1);
            QString initialText = rx.cap(2);
            initialText.replace(QString("\\n"), QString("\n"));
            Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
            if( pHost->mTelnet.mpComposer ) return;
            pHost->mTelnet.mpComposer = new dlgComposer( pHost );
            pHost->mTelnet.mpComposer->init( title, initialText );
            pHost->mTelnet.mpComposer->raise();
            pHost->mTelnet.mpComposer->show();
        }
    }
    lua_pop( L, lua_gettop( L ) );
}

void TLuaInterpreter::setChannel102Table( int & var, int & arg )
{
    lua_State * L = pGlobalLua;
    lua_getglobal( L, "channel102" ); //defined in LuaGlobal.lua
    lua_pushnumber( L, var );
    lua_pushnumber( L, arg );
    lua_rawset( L, -3 );
    lua_pop( L, 1 );

    TEvent event;
    QString e = "channel102Message";
    event.mArgumentList.append( e );
    event.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    event.mArgumentList.append( QString::number(var) );
    event.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
    event.mArgumentList.append( QString::number(arg) );
    event.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->raiseEvent( & event );
}

bool TLuaInterpreter::call_luafunction( void * pT )
{
    lua_State * L = pGlobalLua;
    lua_pushlightuserdata( L, pT );
    lua_gettable( L, LUA_REGISTRYINDEX );
    if( lua_isfunction( L, -1 ) )
    {
        int error = lua_pcall( L, 0, LUA_MULTRET, 0 );
        if( error != 0 )
        {
            int nbpossible_errors = lua_gettop(L);
            for (int i=1; i<=nbpossible_errors; i++)
            {
                string e = "";
                if(lua_isstring( L, i) )
                {
                    e = "Lua error:";
                    e+=lua_tostring( L, i );
                    QString _n = "error in anonymous Lua function";
                    QString _n2 = "no debug data available";
                    logError( e, _n, _n2 );
                    if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running anonymous Lua function ERROR:"<<e.c_str()>>0;}
                }
            }
        }
        else
        {
            if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK anonymous Lua function ran without errors\n">>0;}
        }
        lua_pop( L, lua_gettop( L ) );
        //lua_settop(L, 0);
        if( error == 0 )
            return true;
        else
            return false;
    }
    else
    {
        QString _n = "error in anonymous Lua function";
        QString _n2 = "func reference not found by Lua, func can not be called";
        string e = "Lua error:";
        logError( e, _n, _n2 );
    }

    return false;
}


bool TLuaInterpreter::call( QString & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    if( mCaptureGroupList.size() > 0 )
    {
        lua_newtable( L );

        // set values
        int i=1; // Lua indexes start with 1 as a general convention
        std::list<std::string>::iterator it = mCaptureGroupList.begin();
        for( ; it!=mCaptureGroupList.end(); it++, i++ )
        {
            //if( (*it).length() < 1 ) continue; //have empty capture groups to be undefined keys i.e. machts[emptyCapGroupNumber] = nil otherwise it's = "" i.e. an empty string
            lua_pushnumber( L, i );
            lua_pushstring( L, (*it).c_str() );
            lua_settable( L, -3 );
        }
        lua_setglobal( L, "matches" );
    }

    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    int error = lua_pcall( L, 0, LUA_MULTRET, 0 );
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) )
            {
                e += lua_tostring( L, i );
                logError( e, mName, function );
                if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function <<") ERROR:"<<e.c_str()>>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function <<") ran without errors\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}

void TLuaInterpreter::logError( std::string & e, QString & name, QString & function )
{
    //QDateTime time = QDateTime::currentDateTime();
    // QString entry = QString("[%1]object:<%2> function:<%3> error:<%4>").arg(time.toString("MMM:dd:yyyy hh-mm-ss")).arg(name).arg(function).arg(e.c_str());
    //mpHost->mErrorLogStream << entry << endl;
    QColor blue = QColor(0,0,255);
    QColor green = QColor(0,255,0);
    QColor red = QColor(255,0,0);
    QColor black = QColor(0,0,0);
    QString s1 = QString("[ERROR:]");
    QString s2 = QString(" object:<%1> function:<%2>\n").arg(name).arg(function);
    QString s3 = QString("         <%1>\n").arg(e.c_str());
    if( mpHost->mpEditorDialog )
    {
        mpHost->mpEditorDialog->mpErrorConsole->printDebug(blue, black, s1 );
        mpHost->mpEditorDialog->mpErrorConsole->printDebug(green, black, s2 );
        mpHost->mpEditorDialog->mpErrorConsole->printDebug(red, black, s3 );
    }

}

bool TLuaInterpreter::callConditionFunction( std::string & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    lua_getfield( L, LUA_GLOBALSINDEX, function.c_str() );
    int error = lua_pcall( L, 0, 1, 0 );
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) )
            {
                e+=lua_tostring( L, i );
                QString _f = function.c_str();
                logError( e, mName, _f );
                if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function.c_str() <<") ERROR:"<<e.c_str()<<"\n">>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function.c_str() <<") ran without errors\n">>0;}
    }

    int ret = 0;
    int returnValues = lua_gettop( L );
    if( returnValues > 0 )
    {
        // Lua docs: Like all tests in Lua, lua_toboolean returns 1 for any Lua value different from false and nil; otherwise it returns 0
        // This means trigger patterns don't have to strictly return true or false, as it is accepted in Lua */
        ret = lua_toboolean( L, 1 );
    }
    lua_pop( L, returnValues );
    if( (error == 0) && (ret>0) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TLuaInterpreter::callMulti( QString & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    if( mMultiCaptureGroupList.size() > 0 )
    {
        int k=1; // Lua indexes start with 1 as a general convention
        std::list< std::list<std::string> >::iterator mit = mMultiCaptureGroupList.begin();
        lua_newtable( L );//multimatches
        for( ; mit!=mMultiCaptureGroupList.end(); mit++, k++ )
        {
            // multimatches{ trigger_idx{ table_matches{ ... } } }
            lua_pushnumber( L, k );
            lua_newtable( L );//regex-value => table matches
            int i=1; // Lua indexes start with 1 as a general convention
            std::list<std::string>::iterator it = (*mit).begin();
            for( ; it!=(*mit).end(); it++, i++ )
            {
                lua_pushnumber( L, i );
                lua_pushstring( L, (*it).c_str() );
                lua_settable( L, -3 );//match in matches
            }
            lua_settable( L, -3 );//matches in regex
        }
        lua_setglobal( L, "multimatches" );
    }

    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    int error = lua_pcall( L, 0, LUA_MULTRET, 0 );
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) )
            {
                e += lua_tostring( L, i );
                logError( e, mName, function );
                if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function <<") ERROR:"<<e.c_str()<<"\n">>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function <<") ran without errors\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}


bool TLuaInterpreter::callEventHandler( QString & function, TEvent * pE )
{
    lua_State * L = pGlobalLua;
    int error = luaL_dostring(L, QString("return " + function).toLatin1().data());
    QString n;
    if( error != 0 )
    {
        string e;
        if( lua_isstring( L, 1 ) )
        {
            e = "Lua error:";
            e += lua_tostring( L, 1 );
        }
        QString _n = "event handler function";
        logError( e, _n, function );
        return false;
    }
    for( int i=0; i<pE->mArgumentList.size(); i++ )
    {
        if( pE->mArgumentTypeList[i] == ARGUMENT_TYPE_NUMBER )
        {
            lua_pushnumber( L, pE->mArgumentList[i].toInt() );
        }
        else
        {
            lua_pushstring( L, pE->mArgumentList[i].toLatin1().data() );
        }
    }
    error = lua_pcall( L, pE->mArgumentList.size(), LUA_MULTRET, 0 );
    if( error != 0 )
    {
        string e = "";
        if(lua_isstring( L, -1) )
        {
            e+=lua_tostring( L, -1 );
        }
        QString _n = "event handler function";
        logError( e, _n, function );
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< function << " (" << function <<") ERROR:"<<e.c_str()<<"\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 )
        return true;
    else
        return false;
}


void TLuaInterpreter::set_lua_table( QString & tableName, QStringList & variableList )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }
    lua_newtable(L);
    for( int i=0; i<variableList.size(); i++ )
    {
        lua_pushnumber( L, i+1 ); // Lua indexes start with 1
        lua_pushstring( L, variableList[i].toLatin1().data() );
        lua_settable( L, -3 );
    }
    lua_setglobal( L, tableName.toLatin1().data() );
    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );
}

void TLuaInterpreter::set_lua_string( const QString & varName, QString & varValue )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }

    //lua_pushstring( L, varName.toLatin1().data() );
    lua_pushstring( L, varValue.toLatin1().data() );
    lua_setglobal( L, varName.toLatin1().data() );
    //lua_setfield( L, LUA_GLOBALSINDEX, s )
    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );

//lua_settable( L, LUA_GLOBALSINDEX );
}

QString TLuaInterpreter::get_lua_string( QString & stringName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return QString( "LUA CRITICAL ERROR" );
    }

    lua_getglobal( L, stringName.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, stringName.toLatin1().data() );
    return QString( lua_tostring( L, 1 ) );
}

int TLuaInterpreter::check_for_mappingscript()
{
    lua_State * L = pGlobalLua;
    lua_getglobal(L, "mudlet");
    if( !lua_istable(L, -1) ) {
        lua_pop( L, 1 );
        return 0;
    }

    lua_getfield(L, -1, "mapper_script");
    if( !lua_isboolean(L, -1) ) {
        lua_pop( L, 2 );
        return 0;
    }

    int r = lua_toboolean(L, -1);
    lua_pop( L, 2 );
    return r;
}


void TLuaInterpreter::threadLuaInterpreterExec( string code )
{
    /* cout << "TLuaMainThread::threadLuaInterpreterExec(code) executing following code:" << endl;
     cout << "--------------------------------------------snip<" <<endl;
     cout << code << endl;
     cout << ">snip--------------------------------------------" <<endl;*/
     lua_State * L = pGlobalLua;
     int error = luaL_dostring(L,code.c_str());
     QString n;
     if( error != 0 )
     {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) )
        {
            e = "Lua error:";
            e += lua_tostring( L, 1 );
        }
        emit signalEchoMessage( mHostID, QString( e.c_str() ) );
        qDebug()<< "LUA_ERROR:"<<e.c_str();
     }

     cout << "cRunningScript::threadLuaInterpreterExec() done" << endl;
}



void TLuaInterpreter::startLuaSessionInterpreter()
{
    //connect(this,SIGNAL(signalOpenWindow(int,QString)), this,SLOT(slotOpenWindow(int,QString)));
    //connect(this,SIGNAL(signalEchoWindow(int,QString,QString)), this,SLOT(slotEchoWindow(int,QString,QString)));
    //connect(this,SIGNAL(signalClearWindow(int,QString)), this,SLOT(slotClearWindow(int,QString)));
    //connect(this,SIGNAL(signalNewTrigger(QString,QString, int, QString)), this,SLOT(slotNewTrigger(QString,QString, int, QString)));
    //connect(this,SIGNAL(signalAddTimer(int,int,QString,QString)),this,SLOT(slotAddTimer(int,int,QString,QString)));
    //connect(this,SIGNAL(signalDeleteTrigger(int,QString)), this,SLOT(slotDeleteTrigger(int,QString)));


    //connect(this,SIGNAL(signalEchoMessage(int,QString)), this,SLOT(slotEchoMessage(int,QString)));//,Qt::DirectConnection);
    //connect(this,SIGNAL(signalNewEcho(int,QString)), this,SLOT(slotNewEcho(int,QString)));
    //connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)));//,Qt::QueuedConnection);

    mpLuaSessionThread = new TLuaMainThread(this);
    mpLuaSessionThread->start(); //calls initLuaGlobals() to initialize the interpreter for this session
}

#ifdef Q_OS_MAC
    #include "luazip.c"
#endif

// this function initializes the Lua Session interpreter.
// on initialization of a new session *or* in case of an interpreter reset by the user.
void TLuaInterpreter::initLuaGlobals()
{
    pGlobalLua = luaL_newstate();
    TLuaInterpreter::luaInterpreterMap[pGlobalLua]=mpHost;

    luaL_openlibs( pGlobalLua );

    lua_pushstring( pGlobalLua, "SESSION" );
    lua_pushnumber( pGlobalLua, mHostID );
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );

    lua_pushstring( pGlobalLua, "SCRIPT_NAME" );
    lua_pushstring( pGlobalLua, "Global Lua Session Interpreter" );
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );

    lua_pushstring( pGlobalLua, "SCRIPT_ID" );
    lua_pushnumber( pGlobalLua, -1 ); // ID 1 is used to indicate that this is the global Lua interpreter
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );
    lua_register( pGlobalLua, "showUnzipProgress", TLuaInterpreter::showUnzipProgress );//internal function used by the package system NOT FOR USERS
    lua_register( pGlobalLua, "wait", TLuaInterpreter::Wait );
    lua_register( pGlobalLua, "expandAlias", TLuaInterpreter::Send );
    lua_register( pGlobalLua, "echo", TLuaInterpreter::Echo );
    lua_register( pGlobalLua, "selectString", TLuaInterpreter::select );
    lua_register( pGlobalLua, "selectSection", TLuaInterpreter::selectSection );
    lua_register( pGlobalLua, "replace", TLuaInterpreter::replace );
    lua_register( pGlobalLua, "setBgColor", TLuaInterpreter::setBgColor );
    lua_register( pGlobalLua, "setFgColor", TLuaInterpreter::setFgColor );
    lua_register( pGlobalLua, "tempTimer", TLuaInterpreter::tempTimer );
    lua_register( pGlobalLua, "tempTrigger", TLuaInterpreter::tempTrigger );
    lua_register( pGlobalLua, "tempRegexTrigger", TLuaInterpreter::tempRegexTrigger );
    lua_register( pGlobalLua, "openUserWindow", TLuaInterpreter::openUserWindow );
    lua_register( pGlobalLua, "echoUserWindow", TLuaInterpreter::echoUserWindow );
    lua_register( pGlobalLua, "enableTimer", TLuaInterpreter::enableTimer );
    lua_register( pGlobalLua, "disableTimer", TLuaInterpreter::disableTimer );
    lua_register( pGlobalLua, "enableKey", TLuaInterpreter::enableKey );
    lua_register( pGlobalLua, "disableKey", TLuaInterpreter::disableKey );
    lua_register( pGlobalLua, "clearUserWindow", TLuaInterpreter::clearUserWindow );
    lua_register( pGlobalLua, "clearWindow", TLuaInterpreter::clearUserWindow );
    lua_register( pGlobalLua, "killTimer", TLuaInterpreter::killTimer );
    lua_register( pGlobalLua, "moveCursor", TLuaInterpreter::moveCursor );
    lua_register( pGlobalLua, "getLines", TLuaInterpreter::getLines );
    lua_register( pGlobalLua, "getLineNumber", TLuaInterpreter::getLineNumber );
    lua_register( pGlobalLua, "insertHTML", TLuaInterpreter::insertHTML );
    lua_register( pGlobalLua, "insertText", TLuaInterpreter::insertText );
    lua_register( pGlobalLua, "enableTrigger", TLuaInterpreter::enableTrigger );
    lua_register( pGlobalLua, "disableTrigger", TLuaInterpreter::disableTrigger );
    lua_register( pGlobalLua, "killTrigger", TLuaInterpreter::killTrigger );
    lua_register( pGlobalLua, "getLineCount", TLuaInterpreter::getLineCount );
    lua_register( pGlobalLua, "getColumnNumber", TLuaInterpreter::getColumnNumber );
    //lua_register( pGlobalLua, "getBufferTable", TLuaInterpreter::getBufferTable );
    //lua_register( pGlobalLua, "getBufferLine", TLuaInterpreter::getBufferLine );
    lua_register( pGlobalLua, "send", TLuaInterpreter::sendRaw );
    lua_register( pGlobalLua, "selectCaptureGroup", TLuaInterpreter::selectCaptureGroup );
    lua_register( pGlobalLua, "tempLineTrigger", TLuaInterpreter::tempLineTrigger );
    lua_register( pGlobalLua, "raiseEvent", TLuaInterpreter::raiseEvent );
    lua_register( pGlobalLua, "deleteLine", TLuaInterpreter::deleteLine );
    lua_register( pGlobalLua, "copy", TLuaInterpreter::copy );
    lua_register( pGlobalLua, "cut", TLuaInterpreter::cut );
    lua_register( pGlobalLua, "paste", TLuaInterpreter::paste );
    lua_register( pGlobalLua, "pasteWindow", TLuaInterpreter::pasteWindow );
    //lua_register( pGlobalLua, "userWindowLineWrap", TLuaInterpreter::userWindowLineWrap );
    lua_register( pGlobalLua, "debugc", TLuaInterpreter::debug );
    lua_register( pGlobalLua, "setWindowWrap", TLuaInterpreter::setWindowWrap );
    lua_register( pGlobalLua, "setWindowWrapIndent", TLuaInterpreter::setWindowWrapIndent );
    lua_register( pGlobalLua, "resetFormat", TLuaInterpreter::reset );
    lua_register( pGlobalLua, "moveCursorEnd", TLuaInterpreter::moveCursorEnd );
    lua_register( pGlobalLua, "getLastLineNumber", TLuaInterpreter::getLastLineNumber );
    lua_register( pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency );
    lua_register( pGlobalLua, "createMiniConsole", TLuaInterpreter::createMiniConsole );
    lua_register( pGlobalLua, "createLabel", TLuaInterpreter::createLabel );
    lua_register( pGlobalLua, "hideWindow", TLuaInterpreter::hideUserWindow );
    lua_register( pGlobalLua, "showWindow", TLuaInterpreter::showUserWindow );
    lua_register( pGlobalLua, "createBuffer", TLuaInterpreter::createBuffer );
    lua_register( pGlobalLua, "createStopWatch", TLuaInterpreter::createStopWatch );
    lua_register( pGlobalLua, "getStopWatchTime", TLuaInterpreter::getStopWatchTime );
    lua_register( pGlobalLua, "stopStopWatch", TLuaInterpreter::stopStopWatch );
    lua_register( pGlobalLua, "startStopWatch", TLuaInterpreter::startStopWatch );
    lua_register( pGlobalLua, "resetStopWatch", TLuaInterpreter::resetStopWatch );
    lua_register( pGlobalLua, "closeUserWindow", TLuaInterpreter::closeUserWindow );
    lua_register( pGlobalLua, "resizeWindow", TLuaInterpreter::resizeUserWindow );
    lua_register( pGlobalLua, "appendBuffer", TLuaInterpreter::appendBuffer );
    lua_register( pGlobalLua, "setBackgroundImage", TLuaInterpreter::setBackgroundImage );
    lua_register( pGlobalLua, "setBackgroundColor", TLuaInterpreter::setBackgroundColor );
    lua_register( pGlobalLua, "createButton", TLuaInterpreter::createButton );
    lua_register( pGlobalLua, "setLabelClickCallback", TLuaInterpreter::setLabelClickCallback );
    lua_register( pGlobalLua, "setLabelOnEnter", TLuaInterpreter::setLabelOnEnter );
    lua_register( pGlobalLua, "setLabelOnLeave", TLuaInterpreter::setLabelOnLeave );
    lua_register( pGlobalLua, "moveWindow", TLuaInterpreter::moveWindow );
    lua_register( pGlobalLua, "setTextFormat", TLuaInterpreter::setTextFormat );
    lua_register( pGlobalLua, "getMainWindowSize", TLuaInterpreter::getMainWindowSize );
    lua_register( pGlobalLua, "getCurrentLine", TLuaInterpreter::getCurrentLine );
    lua_register( pGlobalLua, "setMiniConsoleFontSize", TLuaInterpreter::setMiniConsoleFontSize );
    lua_register( pGlobalLua, "selectCurrentLine", TLuaInterpreter::selectCurrentLine );
    lua_register( pGlobalLua, "spawn", TLuaInterpreter::spawn );
    lua_register( pGlobalLua, "getButtonState", TLuaInterpreter::getButtonState );
    lua_register( pGlobalLua, "showToolBar", TLuaInterpreter::showToolBar );
    lua_register( pGlobalLua, "hideToolBar", TLuaInterpreter::hideToolBar );
    lua_register( pGlobalLua, "loadRawFile", TLuaInterpreter::loadRawFile );
    lua_register( pGlobalLua, "setBold", TLuaInterpreter::setBold );
    lua_register( pGlobalLua, "setItalics", TLuaInterpreter::setItalics );
    lua_register( pGlobalLua, "setUnderline", TLuaInterpreter::setUnderline );
    lua_register( pGlobalLua, "disconnect", TLuaInterpreter::disconnect );
    lua_register( pGlobalLua, "tempButtonToolbar", TLuaInterpreter::tempButtonToolbar );
    lua_register( pGlobalLua, "tempButton", TLuaInterpreter::tempButton );
    lua_register( pGlobalLua, "reconnect", TLuaInterpreter::reconnect );
    lua_register( pGlobalLua, "getMudletHomeDir", TLuaInterpreter::getMudletHomeDir );
    lua_register( pGlobalLua, "setTriggerStayOpen", TLuaInterpreter::setTriggerStayOpen );
    lua_register( pGlobalLua, "wrapLine", TLuaInterpreter::wrapLine );
    lua_register( pGlobalLua, "getFgColor", TLuaInterpreter::getFgColor );
    lua_register( pGlobalLua, "getBgColor", TLuaInterpreter::getBgColor );
    lua_register( pGlobalLua, "tempColorTrigger", TLuaInterpreter::tempColorTrigger );
    lua_register( pGlobalLua, "isAnsiFgColor", TLuaInterpreter::isAnsiFgColor );
    lua_register( pGlobalLua, "isAnsiBgColor", TLuaInterpreter::isAnsiBgColor );
    lua_register( pGlobalLua, "playSoundFile", TLuaInterpreter::playSoundFile );
    lua_register( pGlobalLua, "setBorderTop", TLuaInterpreter::setBorderTop );
    lua_register( pGlobalLua, "setBorderBottom", TLuaInterpreter::setBorderBottom );
    lua_register( pGlobalLua, "setBorderLeft", TLuaInterpreter::setBorderLeft );
    lua_register( pGlobalLua, "setBorderRight", TLuaInterpreter::setBorderRight );
    lua_register( pGlobalLua, "setBorderColor", TLuaInterpreter::setBorderColor );
    lua_register( pGlobalLua, "setConsoleBufferSize", TLuaInterpreter::setConsoleBufferSize );
    lua_register( pGlobalLua, "startLogging", TLuaInterpreter::startLogging );
    lua_register( pGlobalLua, "calcFontSize", TLuaInterpreter::calcFontSize );
    lua_register( pGlobalLua, "permRegexTrigger", TLuaInterpreter::permRegexTrigger );
    lua_register( pGlobalLua, "permSubstringTrigger", TLuaInterpreter::permSubstringTrigger );
    lua_register( pGlobalLua, "permBeginOfLineStringTrigger", TLuaInterpreter::permBeginOfLineStringTrigger );
    lua_register( pGlobalLua, "tempComplexRegexTrigger", TLuaInterpreter::tempComplexRegexTrigger );
    lua_register( pGlobalLua, "permTimer", TLuaInterpreter::permTimer );
    lua_register( pGlobalLua, "permAlias", TLuaInterpreter::permAlias );
    lua_register( pGlobalLua, "exists", TLuaInterpreter::exists );
    lua_register( pGlobalLua, "isActive", TLuaInterpreter::isActive );
    lua_register( pGlobalLua, "enableAlias", TLuaInterpreter::enableAlias );
    lua_register( pGlobalLua, "tempAlias", TLuaInterpreter::tempAlias );
    lua_register( pGlobalLua, "disableAlias", TLuaInterpreter::disableAlias );
    lua_register( pGlobalLua, "killAlias", TLuaInterpreter::killAlias );
    lua_register( pGlobalLua, "setLabelStyleSheet", TLuaInterpreter::setLabelStyleSheet );
    lua_register( pGlobalLua, "getTime", TLuaInterpreter::getTime );
    lua_register( pGlobalLua, "invokeFileDialog", TLuaInterpreter::invokeFileDialog );
    lua_register( pGlobalLua, "getTimestamp", TLuaInterpreter::getTimestamp );
    lua_register( pGlobalLua, "setLink", TLuaInterpreter::setLink );
    lua_register( pGlobalLua, "deselect", TLuaInterpreter::deselect );
    lua_register( pGlobalLua, "insertLink", TLuaInterpreter::insertLink );
    lua_register( pGlobalLua, "echoLink", TLuaInterpreter::echoLink );
    lua_register( pGlobalLua, "echoPopup", TLuaInterpreter::echoPopup );
    lua_register( pGlobalLua, "insertPopup", TLuaInterpreter::insertPopup );
    lua_register( pGlobalLua, "setPopup", TLuaInterpreter::setPopup );
    lua_register( pGlobalLua, "sendATCP", TLuaInterpreter::sendATCP );
    lua_register( pGlobalLua, "hasFocus", TLuaInterpreter::hasFocus );
    lua_register( pGlobalLua, "isPrompt", TLuaInterpreter::isPrompt );
    lua_register( pGlobalLua, "feedTriggers", TLuaInterpreter::feedTriggers );
    lua_register( pGlobalLua, "sendTelnetChannel102", TLuaInterpreter::sendTelnetChannel102 );
    lua_register( pGlobalLua, "setRoomWeight", TLuaInterpreter::setRoomWeight );
    lua_register( pGlobalLua, "getRoomWeight", TLuaInterpreter::getRoomWeight );
    lua_register( pGlobalLua, "gotoRoom", TLuaInterpreter::gotoRoom );
    lua_register( pGlobalLua, "setMapperView", TLuaInterpreter::setMapperView );
    lua_register( pGlobalLua, "getRoomExits", TLuaInterpreter::getRoomExits );
    lua_register( pGlobalLua, "lockRoom", TLuaInterpreter::lockRoom );
    lua_register( pGlobalLua, "createMapper", TLuaInterpreter::createMapper );
    lua_register( pGlobalLua, "getMainConsoleWidth", TLuaInterpreter::getMainConsoleWidth );
    lua_register( pGlobalLua, "resetProfile", TLuaInterpreter::resetProfile );
    lua_register( pGlobalLua, "printCmdLine", TLuaInterpreter::printCmdLine );
    lua_register( pGlobalLua, "searchRoom", TLuaInterpreter::searchRoom );
    lua_register( pGlobalLua, "clearCmdLine", TLuaInterpreter::clearCmdLine );
    lua_register( pGlobalLua, "getAreaTable", TLuaInterpreter::getAreaTable );
    lua_register( pGlobalLua, "getAreaTableSwap", TLuaInterpreter::getAreaTableSwap );
    lua_register( pGlobalLua, "getAreaRooms", TLuaInterpreter::getAreaRooms );
    lua_register( pGlobalLua, "getPath", TLuaInterpreter::getPath );
    lua_register( pGlobalLua, "centerview", TLuaInterpreter::centerview );
    lua_register( pGlobalLua, "denyCurrentSend", TLuaInterpreter::denyCurrentSend );
    lua_register( pGlobalLua, "tempBeginOfLineTrigger", TLuaInterpreter::tempBeginOfLineTrigger );
    lua_register( pGlobalLua, "tempExactMatchTrigger", TLuaInterpreter::tempExactMatchTrigger );
    lua_register( pGlobalLua, "sendGMCP", TLuaInterpreter::sendGMCP );
    lua_register( pGlobalLua, "roomExists", TLuaInterpreter::roomExists );
    lua_register( pGlobalLua, "addRoom", TLuaInterpreter::addRoom );
    lua_register( pGlobalLua, "setExit", TLuaInterpreter::setExit );
    lua_register( pGlobalLua, "setRoomCoordinates", TLuaInterpreter::setRoomCoordinates );
    lua_register( pGlobalLua, "getRoomCoordinates", TLuaInterpreter::getRoomCoordinates );
    lua_register( pGlobalLua, "createRoomID", TLuaInterpreter::createRoomID );
    lua_register( pGlobalLua, "getRoomArea", TLuaInterpreter::getRoomArea );
    lua_register( pGlobalLua, "setRoomArea", TLuaInterpreter::setRoomArea );
    lua_register( pGlobalLua, "setAreaName", TLuaInterpreter::setAreaName );
    lua_register( pGlobalLua, "roomLocked", TLuaInterpreter::roomLocked );
    lua_register( pGlobalLua, "setCustomEnvColor", TLuaInterpreter::setCustomEnvColor );
    lua_register( pGlobalLua, "getCustomEnvColorTable", TLuaInterpreter::getCustomEnvColorTable );
    //lua_register( pGlobalLua, "setLevelColor", TLuaInterpreter::setLevelColor );
    //lua_register( pGlobalLua, "getLevelColorTable", TLuaInterpreter::getLevelColorTable );
    lua_register( pGlobalLua, "setRoomEnv", TLuaInterpreter::setRoomEnv );
    lua_register( pGlobalLua, "setRoomName", TLuaInterpreter::setRoomName );
    lua_register( pGlobalLua, "getRoomName", TLuaInterpreter::getRoomName );
    lua_register( pGlobalLua, "setGridMode", TLuaInterpreter::setGridMode );
    lua_register( pGlobalLua, "solveRoomCollisions", TLuaInterpreter::solveRoomCollisions );
    lua_register( pGlobalLua, "addSpecialExit", TLuaInterpreter::addSpecialExit );
    lua_register( pGlobalLua, "getSpecialExits", TLuaInterpreter::getSpecialExits );
    lua_register( pGlobalLua, "getSpecialExitsSwap", TLuaInterpreter::getSpecialExitsSwap );
    lua_register( pGlobalLua, "clearSpecialExits", TLuaInterpreter::clearSpecialExits );
    lua_register( pGlobalLua, "getRoomEnv", TLuaInterpreter::getRoomEnv );
    lua_register( pGlobalLua, "getRoomUserData", TLuaInterpreter::getRoomUserData );
    lua_register( pGlobalLua, "setRoomUserData", TLuaInterpreter::setRoomUserData );
    lua_register( pGlobalLua, "searchRoomUserData", TLuaInterpreter::searchRoomUserData );
    lua_register( pGlobalLua, "getRoomsByPosition", TLuaInterpreter::getRoomsByPosition );
    //lua_register( pGlobalLua, "dumpRoomUserData", TLuaInterpreter::dumpRoomUserData );
    lua_register( pGlobalLua, "clearRoomUserData", TLuaInterpreter::clearRoomUserData );
    lua_register( pGlobalLua, "downloadFile", TLuaInterpreter::downloadFile );
    lua_register( pGlobalLua, "appendCmdLine", TLuaInterpreter::appendCmdLine );
    lua_register( pGlobalLua, "openUrl", TLuaInterpreter::openUrl );
    lua_register( pGlobalLua, "sendSocket", TLuaInterpreter::sendSocket );
    lua_register( pGlobalLua, "setRoomIDbyHash", TLuaInterpreter::setRoomIDbyHash );
    lua_register( pGlobalLua, "getRoomIDbyHash", TLuaInterpreter::getRoomIDbyHash );
    lua_register( pGlobalLua, "addAreaName", TLuaInterpreter::addAreaName );
    lua_register( pGlobalLua, "getRoomAreaName", TLuaInterpreter::getRoomAreaName );
    lua_register( pGlobalLua, "deleteArea", TLuaInterpreter::deleteArea );
    lua_register( pGlobalLua, "deleteRoom", TLuaInterpreter::deleteRoom );
    lua_register( pGlobalLua, "setRoomChar", TLuaInterpreter::setRoomChar );
    lua_register( pGlobalLua, "getRoomChar", TLuaInterpreter::getRoomChar );
    lua_register( pGlobalLua, "registerAnonymousEventHandler", TLuaInterpreter::registerAnonymousEventHandler );
    lua_register( pGlobalLua, "saveMap", TLuaInterpreter::saveMap );
    lua_register( pGlobalLua, "loadMap", TLuaInterpreter::loadMap );
    lua_register( pGlobalLua, "setMainWindowSize", TLuaInterpreter::setMainWindowSize );
    // removed because of various Qt crashes
    //lua_register( pGlobalLua, "setAppStyleSheet", TLuaInterpreter::setAppStyleSheet );
    lua_register( pGlobalLua, "sendIrc", TLuaInterpreter::sendIrc );
    lua_register( pGlobalLua, "connectToServer", TLuaInterpreter::connectToServer );
    lua_register( pGlobalLua, "getRooms", TLuaInterpreter::getRooms );
    lua_register( pGlobalLua, "createMapLabel", TLuaInterpreter::createMapLabel );
    lua_register( pGlobalLua, "deleteMapLabel", TLuaInterpreter::deleteMapLabel );
    lua_register( pGlobalLua, "highlightRoom", TLuaInterpreter::highlightRoom );
    lua_register( pGlobalLua, "unHighlightRoom", TLuaInterpreter::unHighlightRoom );
    lua_register( pGlobalLua, "getMapLabels", TLuaInterpreter::getMapLabels );
    lua_register( pGlobalLua, "getMapLabel", TLuaInterpreter::getMapLabel );
    lua_register( pGlobalLua, "lockExit", TLuaInterpreter::lockExit );
    lua_register( pGlobalLua, "hasExitLock", TLuaInterpreter::hasExitLock );
    lua_register( pGlobalLua, "lockSpecialExit", TLuaInterpreter::lockSpecialExit );
    lua_register( pGlobalLua, "hasSpecialExitLock", TLuaInterpreter::hasSpecialExitLock );
    lua_register( pGlobalLua, "setExitStub", TLuaInterpreter::setExitStub );
    lua_register( pGlobalLua, "connectExitStub", TLuaInterpreter::connectExitStub );
    lua_register( pGlobalLua, "getExitStubs", TLuaInterpreter::getExitStubs );
    lua_register( pGlobalLua, "setModulePriority", TLuaInterpreter::setModulePriority );
    lua_register( pGlobalLua, "getModulePriority", TLuaInterpreter::getModulePriority );
    lua_register( pGlobalLua, "updateMap", TLuaInterpreter::updateMap );
    lua_register( pGlobalLua, "addMapEvent", TLuaInterpreter::addMapEvent );
    lua_register( pGlobalLua, "removeMapEvent", TLuaInterpreter::removeMapEvent );
    lua_register( pGlobalLua, "getMapEvents", TLuaInterpreter::getMapEvents );
    lua_register( pGlobalLua, "addMapMenu", TLuaInterpreter::addMapMenu );
    lua_register( pGlobalLua, "removeMapMenu", TLuaInterpreter::removeMapMenu );
    lua_register( pGlobalLua, "getMapMenus", TLuaInterpreter::getMapMenus );
    lua_register( pGlobalLua, "installPackage", TLuaInterpreter::installPackage );
    lua_register( pGlobalLua, "exportAreaImage", TLuaInterpreter::exportAreaImage );
    lua_register( pGlobalLua, "createMapImageLabel", TLuaInterpreter::createMapImageLabel );
    lua_register( pGlobalLua, "setMapZoom", TLuaInterpreter::setMapZoom );
    lua_register( pGlobalLua, "uninstallPackage", TLuaInterpreter::uninstallPackage );
    lua_register( pGlobalLua, "setExitWeight", TLuaInterpreter::setExitWeight );
    lua_register( pGlobalLua, "setDoor", TLuaInterpreter::setDoor );
    lua_register( pGlobalLua, "getDoors", TLuaInterpreter::getDoors );
    lua_register( pGlobalLua, "getExitWeights", TLuaInterpreter::getExitWeights );
    lua_register( pGlobalLua, "addSupportedTelnetOption", TLuaInterpreter::addSupportedTelnetOption );
    lua_register( pGlobalLua, "setMergeTables", TLuaInterpreter::setMergeTables );


    luaopen_yajl(pGlobalLua);
    lua_setglobal( pGlobalLua, "yajl" );

#ifdef Q_OS_MAC
    luaopen_zip( pGlobalLua );
    lua_setglobal( pGlobalLua, "zip" );
#endif
    QString n;
    int error;

    // if using LuaJIT, adjust the cpath to look in /usr/lib as well - it doesn't by default
    luaL_dostring (pGlobalLua, "if jit then package.cpath = package.cpath .. ';/usr/lib/lua/5.1/?.so;/usr/lib/x86_64-linux-gnu/lua/5.1/?.so' end");

    error = luaL_dostring( pGlobalLua, "require \"rex_pcre\"" );

    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( pGlobalLua, 1 );
        }
        QString msg = "[ ERROR ] cannot find Lua module rex_pcre. Some functions may not be available.";
        msg.append( e.c_str() );
        gSysErrors << msg;
    }
    else
    {
        QString msg = "[  OK  ]  -  Lua module rex_pcre loaded";
        gSysErrors << msg;
    }

    error = luaL_dostring( pGlobalLua, "require \"zip\"" );

    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( pGlobalLua, 1 );
        }
        QString msg = "[ ERROR ] cannot find Lua module zip";
        msg.append( e.c_str() );
        gSysErrors << msg;
    }
    else
    {
        QString msg = "[  OK  ]  -  Lua module zip loaded";
        gSysErrors << msg;
    }
    error = luaL_dostring( pGlobalLua, "require \"lfs\"" );

    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( pGlobalLua, 1 );
        }
        QString msg = "[ ERROR ] cannot find Lua module lfs (Lua File System).";
        msg.append( e.c_str() );
        gSysErrors << msg;
    }
    else
    {
        QString msg = "[  OK  ]  -  Lua module lfs loaded";
        gSysErrors << msg;
    }

    error = luaL_dostring( pGlobalLua, "luasql = require \"luasql.sqlite3\"" );

    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( pGlobalLua, 1 );
        }
        QString msg = "[ ERROR ] cannot find Lua module luasql.sqlite3. Database support will not be available.";
        msg.append( e.c_str() );
        gSysErrors << msg;
    }
    else
    {
        QString msg = "[  OK  ]  -  Lua module sqlite3 loaded";
        gSysErrors << msg;
    }

//    QString path = QDir::homePath()+"/.config/mudlet/mudlet-lua/lua/LuaGlobal.lua";
//    error = luaL_dofile( pGlobalLua, path.toLatin1().data() );
//    if( error != 0 )
//    {
//        string e = "no error message available from Lua";
//        if( lua_isstring( pGlobalLua, 1 ) )
//        {
//            e = "[CRITICAL ERROR] LuaGlobal.lua compile error - please report";
//            e += lua_tostring( pGlobalLua, 1 );
//        }
//        gSysErrors << e.c_str();
//    }
//    else
//    {
//        gSysErrors << "[INFO] LuaGlobal.lua loaded successfully.";
//    }

    /*path = QDir::homePath()+"/.config/mudlet/db.lua";
    error = luaL_dofile( pGlobalLua, path.toLatin1().data() );
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "[CRITICAL ERROR] db.lua compile error - please report";
            e += lua_tostring( pGlobalLua, 1 );
        }
        gSysErrors << e.c_str();
    }
    else
    {
        gSysErrors << "[INFO] db.lua loaded successfully.";
    }*/


    QString tn = "atcp";
    QStringList args;
    set_lua_table( tn, args );

    tn = "channel102";
    set_lua_table( tn, args );

    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );

    //FIXME make function call in destructor lua_close(L);
}

void TLuaInterpreter::loadGlobal()
{
    //QString path = QDir::homePath()+"/.config/mudlet/mudlet-lua/lua/LuaGlobal.lua";

    QString path = "mudlet-lua/lua/LuaGlobal.lua";
    int error = luaL_dofile( pGlobalLua, path.toLatin1().data() );
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "[ ERROR ]  -  LuaGlobal.lua compile error - please report!";
            e += lua_tostring( pGlobalLua, 1 );
        }
        gSysErrors << e.c_str();
    }
    else
    {
        gSysErrors << "[  OK  ]  -  mudlet-lua API & Geyser Layout manager loaded.";
    }
}

void TLuaInterpreter::slotEchoMessage(int hostID, QString msg)
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    mudlet::self()->print( pHost, msg );
}


void TLuaInterpreter::slotNewCommand(int hostID, QString cmd)
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    pHost->send( cmd );
}

void TLuaInterpreter::slotOpenUserWindow(int hostID, QString windowName )
{
}

void TLuaInterpreter::slotClearUserWindow(int hostID, QString windowName )
{
}

void TLuaInterpreter::slotEnableTimer(int hostID, QString windowName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    pHost->enableTimer( windowName );
}

void TLuaInterpreter::slotDisableTimer(int hostID, QString windowName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    pHost->disableTimer( windowName );
}

void TLuaInterpreter::slotReplace(int hostID, QString text)
{
}

void TLuaInterpreter::slotEchoUserWindow(int hostID, QString windowName, QString text )
{
}

void TLuaInterpreter::slotTempTimer( int hostID, double timeout, QString function, QString timerName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    QTime time(0,0,0,0);
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    pT = new TTimer( timerName, time2, pHost );
    pT->setName( timerName );
    pT->setTime( time2 );
    //qDebug()<<"setting time of tempTimer to "<<time2.minute()<<":"<<time2.second()<<":"<<time2.msec()<<" timeout="<<timeout;
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTimer( true );
    pT->registerTimer();
}

int TLuaInterpreter::startPermTimer( QString & name, QString & parent, double timeout, QString & function )
{
    QTime time( 0, 0, 0, 0 );
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    if( parent.isEmpty() )
    {
        pT = new TTimer( "a", time2, mpHost );
    }
    else
    {
        TTimer * pP = mpHost->getTimerUnit()->findTimer( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTimer( pP, mpHost );
    }

    pT->setTime( time2 );
    pT->setIsFolder( false );
    pT->setIsTempTimer( false );
    pT->registerTimer();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( name );//darf erst nach isTempTimer gesetzt werde, damit setName() schneller ist
    pT->setIsActive( false );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

int TLuaInterpreter::startTempTimer( double timeout, QString & function )
{
    QTime time( 0, 0, 0, 0 );
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    pT = new TTimer( "a", time2, mpHost );
    pT->setTime( time2 );
    pT->setIsFolder( false );
    pT->setIsTempTimer( true );
    pT->registerTimer();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );//darf erst nach isTempTimer gesetzt werde, damit setName() schneller ist
    pT->setIsActive( true );
    pT->enableTimer( id );
    return id;
}

int TLuaInterpreter::startPermAlias( QString & name, QString & parent, QString & regex, QString & function )
{
    TAlias * pT;

    if( parent.isEmpty() )
    {
        pT = new TAlias("a", mpHost );
    }
    else
    {
        TAlias * pP = mpHost->getAliasUnit()->findAlias( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TAlias( pP, mpHost );
    }
    pT->setRegexCode( regex );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempAlias( false );
    pT->registerAlias();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( name );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

int TLuaInterpreter::startTempAlias( QString & regex, QString & function )
{
    TAlias * pT;
    pT = new TAlias("a", mpHost );
    pT->setRegexCode( regex );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempAlias( true );
    pT->registerAlias();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempExactMatchTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    QList<int> propertyList;
    propertyList << REGEX_EXACT_MATCH;
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempBeginOfLineTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    QList<int> propertyList;
    propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}


int TLuaInterpreter::startTempTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    QList<int> propertyList;
    propertyList << REGEX_SUBSTRING;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempLineTrigger( int from, int howmany, QString & function )
{
    TTrigger * pT;
//    QStringList sList;
//    QList<int> propertyList;
//    propertyList << REGEX_SUBSTRING;// substring trigger is default
//    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger( 0, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->setIsLineTrigger( true );
    pT->setStartOfLineDelta( from );
    pT->setLineDelta( howmany );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempColorTrigger( int fg, int bg, QString & function )
{
    TTrigger * pT;
//    QStringList sList;
//    QList<int> propertyList;
//    propertyList << REGEX_SUBSTRING;// substring trigger is default
//    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger( 0, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->setupTmpColorTrigger( fg, bg );

    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempRegexTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;

    QList<int> propertyList;
    propertyList << REGEX_PERL;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startPermRegexTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_PERL;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    //pT->setName( name );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;

}

int TLuaInterpreter::startPermBeginOfLineStringTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;

}

int TLuaInterpreter::startPermSubstringTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_SUBSTRING;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;

}







