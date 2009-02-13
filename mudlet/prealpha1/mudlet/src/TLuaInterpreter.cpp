/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
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
#include "TLuaInterpreter.h"
#include <QProcess>
#include "TTrigger.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"

extern "C" 
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}


using namespace std;

map<lua_State*, Host*> TLuaInterpreter::luaInterpreterMap;

TLuaInterpreter::TLuaInterpreter( Host * pH, int id )
:mpHost( pH )
,mHostID( id )
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
    initLuaGlobals();
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
    qDebug()<<"MUDLET ERROR: TLuaInterpreter::getLuaExecutionUnit() execution unit undefined";
    return 0;
}

int TLuaInterpreter::Wait( lua_State *L )
{
  int n = lua_gettop( L );
  if(n!=1)
  {
      lua_pushstring( L, "wrong number of arguments to Wait(msec)" );
      lua_error( L );
      return 1;
  }

  int luaSleepMsec;
  if( ! lua_isnumber( L, 1 ) ) 
  {
      lua_pushstring( L, "argument 1 to Wait(msec) must be milliseconds" );
      lua_error( L );
      return 1;
  }
  else
  {
      luaSleepMsec=lua_tonumber( L, 1 );
  }
  msleep( luaSleepMsec );//FIXME thread::sleep()
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
    return 0;
}

// cursorPositionInLine = select( text ) if not found -1
int TLuaInterpreter::select( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to select must be a string containing the text to select from the current line." );
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
        lua_pushstring( L, "argument 2 to select must be the number of match in case there is more than 1 match." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaNumOfMatch = lua_tonumber( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int pos = pHost->mpConsole->select( QString( luaSendText.c_str() ), luaNumOfMatch );
    lua_pushnumber( L, pos );
    return 1;
}

// cursorPositionInLine = selectCaptureGroup( groupNumber ) if not found -1
int TLuaInterpreter::selectCaptureGroup( lua_State * L )
{
    int luaNumOfMatch;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to select must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaNumOfMatch = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( luaNumOfMatch < 1 )
    {
        lua_pushnumber( L, -1 );
        return 1;
    }
    luaNumOfMatch--; //we want capture groups to start with #1 instead of #0
    if( luaNumOfMatch < pHost->getLuaInterpreter()->mCaptureGroupList.size() )
    {
        if( mudlet::debugMode ) qDebug()<<"selectCaptureGroup("<<pHost->getLuaInterpreter()->mCaptureGroupPosList[luaNumOfMatch]<<", "<< pHost->getLuaInterpreter()->mCaptureGroupList[luaNumOfMatch].size()<<")";
        int pos = pHost->mpConsole->selectSection( pHost->getLuaInterpreter()->mCaptureGroupPosList[luaNumOfMatch], pHost->getLuaInterpreter()->mCaptureGroupList[luaNumOfMatch].size()  );
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
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tonumber( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tonumber( L, 2 );
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
        lua_pushstring( L, "wrong argument" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tonumber( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tonumber( L, 2 );
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

// returns current y position of the user cursor
int TLuaInterpreter::getLineNumber( lua_State * L )
{
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getLineNumber();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::copy( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->copy();
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
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->paste();
    return 0;
}

int TLuaInterpreter::getLineCount( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getLineCount();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::getColumnNumber( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getColumnNumber();
    lua_pushnumber( L, lineNumber );
    return 1; 
}

int TLuaInterpreter::userWindowLineWrap( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    
    bool luaBool;
    if( ! lua_isboolean( L, 2 ) ) 
    {    
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaBool = lua_toboolean( L, 2 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString name( luaSendText.c_str() );
    mudlet::self()->userWindowLineWrap( pHost, name, luaBool );
    return 0; 
}

// cusorPositionInLine = selectSection( from_cursorPos, to_cursorPos ) -1 on not found
int TLuaInterpreter::selectSection( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tonumber( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tonumber( L, 2 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    bool ret = false;
    if( (ret = pHost->mpConsole->selectSection( luaFrom, luaTo ) ) )
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    lua_pushboolean( L, ret );
    return 1;
}

// error = moveCursor( x, y ) x and y can be negative. if move is not possible error is returned
int TLuaInterpreter::moveCursor( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tonumber( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tonumber( L, 2 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    bool ret = false;
    if( ( ret = pHost->mpConsole->moveCursor( luaFrom, luaTo ) ) )
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    lua_pushboolean( L, ret );
    return 1;
}

int TLuaInterpreter::getBufferLine( lua_State * L )
{
    int luaLine;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "bad argument" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaLine = lua_tonumber( L, 1 );
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
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 1 to replace must be a string containing the text that is to replace the selected text." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->replace( QString(luaSendText.c_str()) );
    return 0;
}

int TLuaInterpreter::deleteLine( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->skipLine();
    return 0;
}

// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->enableTimer( text );    
    return 0;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->disableTimer( text );    
    return 0;
}

int TLuaInterpreter::enableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->enableKey( text );    
    return 0;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->disableKey( text );    
    return 0;
}



// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->enableTrigger( text );    
    return 0;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->disableTrigger( text );    
    return 0;
}


int TLuaInterpreter::killTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->disableTimer( text );    
    pHost->killTimer( text );    
    return 0;
}

int TLuaInterpreter::killTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    pHost->disableTrigger( text );    
    pHost->killTrigger( text );    
    return 0;
}

// openUserWindow( session, string window_name )
int TLuaInterpreter::openUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    mudlet::self()->openUserWindow( pHost, text );
    return 0;
}

// openUserWindow( session, string window_name )
int TLuaInterpreter::clearUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    mudlet::self()->clearUserWindow( pHost, text );    
    
    return 0;
}

int TLuaInterpreter::echoUserWindow( lua_State *L )
{
    string luaWindowName="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
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
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
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
    mudlet::self()->echoUserWindow( pHost, windowName, text ); 
    return 0;
}



// tempTimer(int session, float seconds, string function to call, string name) // one shot timer.
int TLuaInterpreter::tempTimer( lua_State *L )
{
    double luaTimeout;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTimeout = lua_tonumber( L, 1 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int timerID = pLuaInterpreter->startTempTimer( luaTimeout, QString(luaFunction.c_str()));
    lua_pushnumber( L, timerID );
    return 1;
}

// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
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
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int timerID = pLuaInterpreter->startTempTrigger( QString(luaRegex.c_str()), QString(luaFunction.c_str()));
    lua_pushnumber( L, timerID );
    return 1;
}

// triggerID = tempLineTrigger( from, howmany, func )
int TLuaInterpreter::tempLineTrigger( lua_State *L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "error" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tonumber( L, 1 );
    }      
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "error" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo = lua_tonumber( L, 2 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 3 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int timerID = pLuaInterpreter->startTempLineTrigger( luaFrom, luaTo, QString(luaFunction.c_str()));
    lua_pushnumber( L, timerID );
    return 1;
}


// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempRegexTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
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
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int timerID = pLuaInterpreter->startTempRegexTrigger( QString(luaRegex.c_str()), QString(luaFunction.c_str()));
    lua_pushnumber( L, timerID );
    return 1;
}


int TLuaInterpreter::setFgColor( lua_State *L )
{
    int luaRed;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaRed=lua_tonumber( L, 1 );
    }      
    
    int luaGreen;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaGreen=lua_tonumber( L, 2 );
    }      
    
    int luaBlue;
    if( ! lua_isnumber( L, 3 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaBlue = lua_tonumber( L, 3 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->setFgColor( luaRed, luaGreen, luaBlue ); 
    return 0;
}

int TLuaInterpreter::setBgColor( lua_State *L )
{
    int luaRed;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaRed=lua_tonumber( L, 1 );
    }      
    
    int luaGreen;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaGreen=lua_tonumber( L, 2 );
    }      
    
    int luaBlue;
    if( ! lua_isnumber( L, 3 ) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaBlue = lua_tonumber( L, 3 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->setBgColor( luaRed, luaGreen, luaBlue );    
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
    TDebug() << "Debug:" << luaDebugText >>0;
    return 0;
}


int TLuaInterpreter::insertText( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->insertText( QString(luaSendText.c_str()) );    
    return 0;
}

int TLuaInterpreter::insertHTML( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
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

int TLuaInterpreter::Echo( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString txt(luaSendText.c_str());
    qDebug()<<"TLua::Echo() calling console::echo("<<txt<<")";
    pHost->mpConsole->echo( txt );    
    return 0;
}

int TLuaInterpreter::pasteWindow( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud." );
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


int TLuaInterpreter::Send( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud.");
		      lua_error( L );
        return 1;
		  }
		  else
    { 
  		  		luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->send( QString(luaSendText.c_str()) );    
    return 0;
}

int TLuaInterpreter::sendRaw( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud.");
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->sendRaw( QString(luaSendText.c_str()) );    
    return 0;
}


bool TLuaInterpreter::compileAndExecuteScript( QString & code )
{
    if( mudlet::debugMode )
    {
        qDebug("TLuaInterpreter: compiling following code:");
        qDebug("--------------------------------------------snip<");
        qDebug() << code;
        qDebug(">snip--------------------------------------------");
    }
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
    if( mudlet::debugMode )
    {
        qDebug("TLuaInterpreter: compiling following code:");
        qDebug("--------------------------------------------snip<");
        qDebug() << code;
        qDebug(">snip--------------------------------------------");
    }
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
        if( mudlet::debugMode ) TDebug()<<"\nLUA: code did not compile: ERROR:"<<e.c_str()>>0;
    }
    else 
    {
        if( mudlet::debugMode ) TDebug()<<"\nLUA: code compiled without errors. OK">>0;
    }
    lua_pop( L, lua_gettop( L ) );
    
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compile( QString & code )
{
    if( mudlet::debugMode )
    {
        qDebug("TLuaInterpreter: compiling following code:");
        qDebug("--------------------------------------------snip<");
        qDebug() << code;
        qDebug(">snip--------------------------------------------");
    }
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
        if( mudlet::debugMode ) TDebug()<<"\nLUA: code did not compile: ERROR:"<<e.c_str()<<"\n">>0;
    }
    else
    {
        if( mudlet::debugMode ) TDebug()<<"\nLUA: code compiled without errors. OK\n" >> 0;
    }
    lua_pop( L, lua_gettop( L ) );
    
    if( error == 0 ) return true;
    else return false;
}

void TLuaInterpreter::setCaptureGroups( QStringList captureList, QList<int> posList )
{
    mCaptureGroupList = captureList;
    for( int i=0; i<mCaptureGroupList.size(); i++ )
    {
        mCaptureGroupPosList.append( posList[i] );
    }
}


void TLuaInterpreter::clearCaptureGroups()
{
    mCaptureGroupList.clear();
    mCaptureGroupPosList.clear();
}

void TLuaInterpreter::adjustCaptureGroups( int x, int a )
{
    // adjust all capture group positions in line if data has been inserted by the user
    for( int i=0; i<mCaptureGroupList.size(); i++ )
    {
        if( mCaptureGroupPosList[i] >= x )
        {
            mCaptureGroupPosList[i] += a;
        }
    }
}

bool TLuaInterpreter::call( QString & function, int numMatches, QStringList & matches, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }
        
    lua_newtable( L );      
       
    // set values
    for( int i=0; i<matches.size(); i++ )
    {
        // only pass matches - the first element of matches is the entire text -> skip
        lua_pushnumber( L, i+1 ); // Lua indexes start with 1
        lua_pushstring( L, matches[i].toLatin1().data() );
        lua_settable( L, -3 );
    }
    lua_setglobal( L, "matches" );
    
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
                e = "Lua error:";
                e+=lua_tostring( L, i );

                if( mudlet::debugMode ) TDebug()<<"LUA: ERROR running script "<< mName << " (" << function <<") ERROR:"<<e.c_str()>>0;
            }
        }
    }
    else
    {
        if( mudlet::debugMode ) TDebug()<<"LUA OK script "<<mName << " (" << function <<") ran without errors">>0;
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::callEventHandler( QString & function, QStringList & argList, QList<int> & typeList )
{
    lua_State * L = pGlobalLua;
    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    for( int i=0; i<argList.size(); i++ )
    {
        if( typeList[i] == ARGUMENT_TYPE_NUMBER )
        {
            lua_pushnumber( L, i ); 
        }
        else
        {
            lua_pushstring( L, argList[i].toLatin1().data() );
        }
    }
    int error = lua_pcall( L, argList.size(), LUA_MULTRET, 0 );     
    if( error != 0 )
    {
        string e = "";
        if(lua_isstring( L, 1) ) 
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ) qDebug()<<"LUA: ERROR running script "<< function <<" ERROR:"<<e.c_str();
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
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

// this function initializes the Lua Session interpreter. 
// on initialization of a new session *or* in case of an interpreter reset by the user.
void TLuaInterpreter::initLuaGlobals()
{
    pGlobalLua = lua_open();
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
    lua_register( pGlobalLua, "userWindowLineWrap", TLuaInterpreter::userWindowLineWrap );
    lua_register( pGlobalLua, "debug", TLuaInterpreter::debug );

 
    
    QString n;
    QString path = QDir::homePath()+"/.config/mudlet/LuaGlobal.lua";
    int error = luaL_dofile( pGlobalLua, path.toLatin1().data() );
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) ) 
        {
            e = "Lua error:";
            e += lua_tostring( pGlobalLua, 1 );
        }
        //emit signalNewEcho(script->session, QString(e.c_str()));
        qDebug()<<"LUA_ERROR: "<<e.c_str();
    }
    else
    {
        qDebug()<<"LUA_MESSAGE: LuaGlobal.lua loaded successfully.";
    }
    
    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );
    
    //FIXME make function call in destructor lua_close(L);
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
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    mudlet::self()->openUserWindow( pHost, windowName );
}

void TLuaInterpreter::slotClearUserWindow(int hostID, QString windowName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    mudlet::self()->clearUserWindow( pHost, windowName );
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
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    mudlet::self()->echoUserWindow( pHost, windowName, text );
}

void TLuaInterpreter::slotTempTimer( int hostID, double timeout, QString function, QString timerName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    QTime time(0,0,0,0);
    int msec = timeout * 1000;
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

int TLuaInterpreter::startTempTimer( double timeout, QString function )
{
    QTime time( 0, 0, 0, 0 );
    int msec = timeout * 1000;
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    pT = new TTimer( "a", time2, mpHost );
    pT->setTime( time2 );
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTimer( true );
    pT->registerTimer();    
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

int TLuaInterpreter::startTempTrigger( QString regex, QString function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    QList<int> propertyList;
    propertyList << REGEX_SUBSTRING;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();    
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

int TLuaInterpreter::startTempLineTrigger( int from, int howmany, QString function )
{
    TTrigger * pT;
    QStringList sList;
    QList<int> propertyList;
    propertyList << REGEX_SUBSTRING;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->setIsLineTrigger( true );
    pT->setStartOfLineDelta( from );
    pT->setLineDelta( howmany );
    pT->registerTrigger();    
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}


int TLuaInterpreter::startTempRegexTrigger( QString regex, QString function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    
    QList<int> propertyList;
    propertyList << REGEX_PERL;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();    
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

void TLuaInterpreter::slotSelect( int hostID, QString text, int numOfMatch )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}
void TLuaInterpreter::slotSelectSection(int hostID, int from, int to )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}

void TLuaInterpreter::slotSetFgColor(int hostID, int r, int g, int b )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}

void TLuaInterpreter::slotSetBgColor(int hostID, int r, int g, int b )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    pHost->mpConsole->setBgColor( r, g, b );
}







/*void TLuaInterpreter::execLuaCode( QString code )
{
    mpLuaSessionThread->postJob( code );
} */  

/*
void TLuaInterpreter::runLuaScript()
{
    //OLD EX:connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)));
    //OLD EX:connect(this,SIGNAL(signalNewEcho(int,QString)), this,SLOT(slotNewEcho(int,QString)));


    //connect(this,SIGNAL(signalOpenWindow(int,QString)), this,SLOT(slotOpenWindow(int,QString)));
    //connect(this,SIGNAL(signalEchoWindow(int,QString,QString)), this,SLOT(slotEchoWindow(int,QString,QString)));
    //connect(this,SIGNAL(signalClearWindow(int,QString)), this,SLOT(slotClearWindow(int,QString))); 
    //connect(this,SIGNAL(signalNewTrigger(QString,QString, int, QString)), this,SLOT(slotNewTrigger(QString,QString, int, QString)));
    //connect(this,SIGNAL(signalAddTimer(int,int,QString,QString)),this,SLOT(slotAddTimer(int,int,QString,QString)));
    //connect(this,SIGNAL(signalDeleteTrigger(int,QString)), this,SLOT(slotDeleteTrigger(int,QString)));

    connect(this,SIGNAL(signalEchoMessage(QString,int,QString)), this,SLOT(slotEchoMessage(QString,int,QString)),Qt::DirectConnection);
    connect(this,SIGNAL(signalNewEcho(int,QString)), this,SLOT(slotNewEcho(int,QString)));
    connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)),Qt::QueuedConnection);

    TLuaThread * pThread = new TLuaThread(this);
    pThread->start();
}   */


/*void TLuaInterpreter::slotEchoWindow( Host * pH, QString window, QString txt)
{
    string key = window.toLatin1().data();
    if(cKMProtocol::consoleMap.find(key) == cKMProtocol::consoleMap.end())
    {
        // a window with this name doesn't exist
        return;
    }
    cConsole * pTe = cKMProtocol::consoleMap[key];
    //FIXME
    pTe->addLine(txt);
}

void TLuaInterpreter::slotClearWindow( Host * pH, QString window )
{
    string key = window.toLatin1().data();
    if( cKMProtocol::dockWindowMap.find( key ) == cKMProtocol::dockWindowMap.end())
    {
        return;
    }
    TConsole * pC = cKMProtocol::consoleMap[key];
    pC->clear();
}

void TLuaInterpreter::slotAddTimer( Host * pH, int timeout, QString callbackfunction, QString parameters )
{
    qDebug()<<"DEBUG: cRunningScript::slotAddTimer() timeout="<<timeout<<" callback="<<callbackfunction<<"("<<parameters<<") called"; 
    TLuaTimer * t = new TLuaTimer(this,timeout, pH, callbackfunction, parameters);
    //connect(t,SIGNAL(signalLuaTimerTimeout(int,QString,QString)),this,SLOT(slotLuaTimerCallback(int,QString,QString)));
    t->start();
}

void TLuaInterpreter::slotLuaTimerCallback( Host * pH, QString callbackfunction, QString parameters)
{
    qDebug()<<"DEBUG: cRunningScript::slotLuaTimerCallback() called Lua function="<<callbackfunction<<"("<<parameters<<")";  
    QString code = callbackfunction+"(\""+parameters+"\")";
    mpLuaSessionThread->postJob(code);
} 


void TLuaInterpreter::slotDeleteTrigger( Host* pH, QString name)
{
    qDebug()<<"TLuaInterpreter::slotDeleteTrigger() called";
    // FIXME
}

void TLuaInterpreter::slotNewTrigger(QString name, QString regex, int sessionID, QString TriggerCallback)
{
    qDebug()<<"cRunningScript::slotNewTrigger() regex="<<regex<<" sessionID="<<sessionID<<" triggercallback="<<TriggerCallback;
}



int TLuaInterpreter::OpenWindow(lua_State *L)
{
    int n = lua_gettop(L);
    if(n!=2)
    {
        lua_pushstring(L,"wrong number of arguments to Send(SESSION, TextToSend)");
        lua_error(L);
    }
    int luaSessionID;
    if(!lua_isnumber(L, 1)) 
    {
        lua_pushstring(L, "argument 1 to Send must be the session ID stored in SESSION");
        lua_error(L);
    }
    else
    { 
        luaSessionID=lua_tonumber(L,1);
    }  
    string luaWindow="";
    if(!lua_isstring(L,2))
    {
        lua_pushstring(L, "argument 2 to Send must be a string containing the text to send to the mud.");
        lua_error(L);
    }
    else
    { 
        luaWindow=lua_tostring(L,2);
    }

    emit signalOpenWindow( mpHost, QString(luaWindow.c_str()));

    return 0;
}


void TLuaInterpreter::slotOpenWindow( Host * pHost, QString window )
{
    string key = window.toLatin1().data();
    if(cKMProtocol::dockWindowMap.find(key) != cKMProtocol::dockWindowMap.end())
    {
        return;
    }
    QDockWidget * dock = new QDockWidget( window, mudlet::self() );
    cKMProtocol::dockWindowMap[key] = dock;
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    mudlet::self()->setDockOptions( QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks|QMainWindow::AllowNestedDocks);
    TConsole * pC = new TConsole;
    cKMProtocol::consoleMap[key]=te;
    dock->setWidget( pC );
    mudlet::self()->addDockWidget( Qt::RightDockWidgetArea, dock );
}

int TLuaInterpreter::ClearWindow( lua_State * L )
{
    int n = lua_gettop( L );
    if( n != 2 )
    {
        lua_pushstring( L, "wrong number of arguments to Send(SESSION, TextToSend)");
        lua_error( L );
    }
    int luaSessionID;
    if( ! lua_isnumber( L, 1) ) 
    {
        lua_pushstring( L, "argument 1 to Send must be the session ID stored in SESSION");
        lua_error( L );
    }
    else
    { 
        luaSessionID=lua_tonumber( L, 1 );
    }  
    string luaWindow="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the text to send to the mud.");
        lua_error( L );
  }
  else
  { 
      luaWindow=lua_tostring(L,2);
  }

  emit signalClearWindow( mpHost, QString( luaWindow.c_str() ) );
  return 0;
}



int TLuaInterpreter::EchoWindow( lua_State * L )
{
    int n = lua_gettop( L );
    if( n != 3 )
    {
        lua_pushstring( L, "wrong number of arguments to EchoWindow(session_id,windowname,txt) ");
        lua_error( L );
    }
    int luaSessionID;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "syntax error: EchoWindow(session_id, windowname, txt) ");
        lua_error( L );
    }
    else
    { 
        luaSessionID = lua_tonumber( L, 1 );
    }

    string luaWindow="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "syntax error: EchoWindow(session_id,windowname,txt) ");
        lua_error( L );
    }
    else
    { 
        luaWindow=lua_tostring( L, 2 );
    }

    string luaSendText = "";
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "syntax error: EchoWindow(session_id,windowname,txt) ");
        lua_error( L );
    }
    else
    { 
        luaSendText = lua_tostring( L, 3 );
    }

    emit signalEchoWindow( mpHost, QString(luaWindow.c_str()), QString(luaSendText.c_str()));

    return 0;
}
  */

/*
int TLuaInterpreter::AddTrigger( lua_State * L )
{
    int n = lua_gettop( L );
    if( n != 4 )
    {
        lua_pushstring( L, "wrong number of arguments to AddTrigger(SESSION, trigger_name, trigger_type, callback_function)");
        lua_error( L );
    }
    int luaSessionID;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "syntax error: argumen 1 of AddTrigger() must be a number");
        lua_error( L );
    }
    else
    { 
        luaSessionID = lua_tonumber( L, 1 );
    }  
    string luaTriggerName = "";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the trigger name.");
        lua_error( L );
    }
    else
    { 
        luaTriggerName = lua_tostring( L, 2 );
    }
    string luaRegex = "";
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring(L, "argument 3 to AddTrigger() must be a string containing the trigger type.");
        lua_error(L);
    }
    else
    { 
      luaRegex=lua_tostring( L, 3 );
    }
    string luaTriggerCallback = "";
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "argument 2 to Send must be a string containing the the name of the Lua function to call when the trigger matches.");
        lua_error( L );
    }
    else
    { 
        luaTriggerCallback = lua_tostring( L, 4 );
    }

    emit signalNewTrigger( QString( luaTriggerName.c_str()), QString(luaRegex.c_str()), luaSessionID, QString(luaTriggerCallback.c_str()));
    return 0;
}

int TLuaInterpreter::AddTimer( lua_State * L )
{
  int n = lua_gettop( L );
  if( n != 4 )
  {
      lua_pushstring( L, "wrong number of arguments to AddTimer(SESSION, timeout in milliseconds, \"callback_function\", \"callback_function_parameter_string\")");
      lua_error( L );
  }

  int luaSessionID;
  if( ! lua_isnumber( L, 1 ) ) 
  {
      lua_pushstring( L, "wrong argument 1 to AddTimer(). This must be the session ID stored in the Lua global SESSION");
      lua_error( L );
  }
  else
  { 
      luaSessionID = lua_tonumber( L, 1 );
  }  

  int luaTimeout = 0;
  if( ! lua_isnumber( L, 2 ) )
  {
      lua_pushstring( L, "Argument 2 to AddTimer() must be a number - i.e. the timeout specified in milliseconds");
      lua_error( L );
  }
  else
  { 
      luaTimeout = lua_tonumber( L, 2 );
  }  

  string luaCallbackfunction = "";
  if( ! lua_isstring( L, 3 ) )
  {
      lua_pushstring( L, "Argument 3 to AddTimer() must be the Lua function name (in quotes) to be called when the timer fires");
      lua_error( L );
  }
  else
  { 
      luaCallbackfunction = lua_tostring( L, 3 );
  }  

  string luaParameter = "";
  if( ! lua_isstring( L, 4 ) )
  {
      lua_pushstring( L, "Argument 4 to AddTimer() must be a string (in quotes) representing the parameter passed to the Lua callback function when the timer fires.");
      lua_error( L );
  }
  else
  { 
      luaParameter = lua_tostring( L, 4 );
  }  

  emit signalAddTimer( mpHost, luaTimeout, QString(luaCallbackfunction.c_str()), QString(luaParameter.c_str()));

  return 0;
}



int TLuaInterpreter::DeleteTrigger(lua_State *L)
{
  int n = lua_gettop(L);
  if(n!=2)
  {
      lua_pushstring(L,"wrong number of arguments to Send(SESSION, \"TextToSend\")");
      lua_error(L);
  }
  int luaSessionID;
  if(!lua_isnumber(L, 1)) 
  {
      lua_pushstring(L, "argument 1 to Send must be the session ID stored in SESSION");
      lua_error(L);
  }
  else
  { 
      luaSessionID=lua_tonumber(L,1);
  }  
  string luaTriggerName="";
  if(!lua_isstring(L,2))
  {
      lua_pushstring(L, "argument 2 to Send must be a stringn (in quotes) containing the text to send to the mud.");
      lua_error(L);
  }
  else
  { 
      luaTriggerName=lua_tostring(L,2);
  }  
  emit signalDeleteTrigger(mpHost, QString(luaTriggerName.c_str()));
  return 0;
}

int TLuaInterpreter::DeleteTimer(lua_State *L)
{
  int n = lua_gettop(L);
  if(n!=2)
  {
      lua_pushstring(L,"wrong number of arguments to Send(SESSION, TextToSend)");
      lua_error(L);
  }
  int luaSessionID;
  if(!lua_isnumber(L, 1)) 
  {
      lua_pushstring(L, "argument 1 to Send must be the session ID stored in SESSION");
      lua_error(L);
  }
  else
  { 
      luaSessionID=lua_tonumber(L,1);
  }  
  string luaSendText="";
  if(!lua_isstring(L,2))
  {
      lua_pushstring(L, "argument 2 to Send must be a string containing the text to send to the mud.");
      lua_error(L);
  }
  else
  { 
      luaSendText=lua_tostring(L,2);
  }  
  emit signalNewCommand(mpHost, QString(luaSendText.c_str()));
  return 0;
}
  */
/*
void TLuaInterpreter::threadRunLuaScript()
{
  lua_State * L = lua_open();
  luaL_openlibs(L);

  lua_pushstring(L, "SESSION");
  lua_pushnumber(L, script->session);
  lua_settable(L, LUA_GLOBALSINDEX);

  lua_pushstring(L, "SCRIPT_NAME");
  lua_pushstring(L, script->name.toLatin1().data());
  lua_settable(L, LUA_GLOBALSINDEX);

  lua_pushstring(L, "SCRIPT_ID");
  lua_pushnumber(L, id);
  lua_settable(L, LUA_GLOBALSINDEX);

  lua_register(L, "Wait", cRunningScript::Wait);
  lua_register(L, "Send", cRunningScript::Send);
  lua_register(L, "Echo", cRunningScript::Echo);
    //  lua_register(L, "EchoWindow", cRunningScript::EchoWindow);
    //lua_register(L, "AddTrigger", cRunningScript::AddTrigger);
    //lua_register(L, "DeleteTrigger", cRunningScript::DeleteTrigger);
    //lua_register(L, "AddTimer", cRunningScript::AddTimer);
    //lua_register(L, "OpenWindow", cRunningScript::OpenWindow);
    //lua_register(L, "ClearWindow", cRunningScript::ClearWindow);

  int error = luaL_dofile(L, script->getCommand().toLatin1().data());
  if( error != 0 )
  {
      string e = "no error message available from Lua";
      if(lua_isstring(L,1)) 
      {
          e = "Lua error:";
          e+=lua_tostring(L,1);
      }
      //FIXME
      //emit signalNewEcho(script->session, QString(e.c_str()));
      qDebug()<<"LUA_ERROR:"<<e.c_str();
  }
  lua_close(L);    
}
 */

