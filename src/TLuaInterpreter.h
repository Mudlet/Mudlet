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


#ifndef LUA_INTERPRETER_H
#define LUA_INTERPRETER_H

#include <string>
#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>

extern "C" 
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <QMutexLocker>
#include <queue>
#include <string>
#include <iostream>
#include <list>
//#include "TTrigger.h"
//#include "Host.h"

#define SERVEROUTPUT 1
#define USERCOMMAND 2
#define PROMPT 3
#define RAWDATA 4


class TLuaMainThread;
class TLuaThread;
class TGatekeeperThread;
class Host;
class TTrigger;

class TLuaInterpreter : public QThread  {
   
Q_OBJECT

public:
  
    TLuaInterpreter( Host * mpHost, int id );      
    void startLuaExecThread();
    void threadLuaInterpreterExec( std::string code );
    bool call( QString & function, QString & mName);
    bool compile( QString & );
    bool compileScript( QString & );
    bool compileAndExecuteScript( QString & );
    //void execLuaCode( QString code );
    QString get_lua_string( QString & stringName );
    void set_lua_string( const QString & varName, QString & varValue );
    void set_lua_table( QString & tableName, QStringList & variableList );
    void setCaptureGroups( const std::list<std::string> &, const std::list<int> & );
    void startLuaSessionInterpreter();
    void adjustCaptureGroups( int x, int a );
    void clearCaptureGroups();
    bool callEventHandler( QString & function, QStringList & argList, QList<int> & typeList );
    
    int startTempTimer( double, QString );
    int startTempTrigger( QString, QString );
    int startTempLineTrigger( int, int, QString );
    int startTempRegexTrigger( QString, QString );
    
    TGatekeeperThread * mpGatekeeperThread;
 
    static int Wait( lua_State * L );
    static int Send( lua_State * L );
    static int sendRaw( lua_State * L );
    static int Echo( lua_State * L );
    static int select( lua_State * L );
    static int selectSection( lua_State * L );
    static int replace( lua_State * L );
    static int setFgColor( lua_State * L );
    static int setBgColor( lua_State * L );
    static int tempTimer( lua_State * L );
    static int openUserWindow( lua_State * L );
    static int echoUserWindow( lua_State * L );
    static int clearUserWindow( lua_State * L );
    static int enableTimer( lua_State * L );
    static int disableTimer( lua_State * L );
    static int killTimer( lua_State * L );
    static int moveCursor( lua_State * L );
    static int insertHTML( lua_State * L );
    static int insertText( lua_State * L );
    static int getLines( lua_State * L );
    static int enableTrigger( lua_State * L );
    static int disableTrigger( lua_State * L );
    static int tempTrigger( lua_State * L );
    static int tempRegexTrigger( lua_State * L );
    static int killTrigger( lua_State * L );
    static int getLineCount( lua_State * L );
    static int getLineNumber( lua_State * L );
    static int getColumnNumber( lua_State * L );
    static int getBufferTable( lua_State * L );
    static int getBufferLine( lua_State * L );
    static int selectCaptureGroup( lua_State * L );
    static int tempLineTrigger( lua_State * L );
    static int raiseEvent( lua_State * L );
    static int deleteLine( lua_State * L );
    static int copy( lua_State * L );
    static int cut( lua_State * L );
    static int paste( lua_State * L );
    static int pasteWindow( lua_State * L );
    static int userWindowLineWrap( lua_State * L );
    static int enableKey( lua_State * );
    static int disableKey( lua_State * );
    static int debug( lua_State * L );
    static int setWindowWrap( lua_State * );
    static int setWindowWrapIndent( lua_State * );
    static int reset( lua_State * );
    static int moveCursorEnd( lua_State * );
    static int getLastLineNumber( lua_State * );

    std::list<std::string> mCaptureGroupList;
    std::list<int> mCaptureGroupPosList;
    
    
    static std::map<lua_State *, Host *> luaInterpreterMap;

signals:
  
    void signalOpenUserWindow( int, QString );
    void signalEchoUserWindow( int, QString, QString );
    void signalClearUserWindow( int, QString );
    void signalEnableTimer( int, QString );
    void signalDisableTimer( int, QString );
    void signalNewJob( QString );
    void signalEchoMessage( int, QString );
    void signalSelect( int, QString, int );
    void signalSelectSection( int, int, int );
    void signalReplace( int, QString );
    void signalSetFgColor( int, int, int, int );
    void signalSetBgColor( int, int, int, int );
    void signalTempTimer( int, double, QString, QString );
    void signalNewCommand( int, QString ); //signal of the lua thread unit command dispatcher for the main event loop to post events
    void signalNewLuaCodeToExecute( QString );

public slots:
    
    void slotOpenUserWindow( int, QString );
    void slotEchoUserWindow( int, QString, QString );
    void slotClearUserWindow( int, QString );
    void slotEnableTimer( int, QString );
    void slotDisableTimer( int, QString );
    void slotSelect( int, QString, int );
    void slotSelectSection( int, int, int );
    void slotReplace( int, QString );
    void slotSetFgColor( int, int, int, int );
    void slotSetBgColor( int, int, int, int );
    void slotEchoMessage( int, QString );
    void slotNewCommand( int, QString );
    void slotTempTimer( int hostID, double timeout, QString function, QString timerName );
        
        //void slotNewEcho(int,QString);

    //public:
private:
    void initLuaGlobals();
    lua_State * getLuaExecutionUnit( int unit );
    lua_State* pGlobalLua;
    TLuaMainThread * mpLuaSessionThread;
    
    Host * mpHost;
    int mHostID;
    //std::list<std::string> mCaptureList;
    
    
    
    lua_State * pGlobalLuaAliasExecutionUnit;
    lua_State * pGlobalLuaTriggerExecutionUnit;
    lua_State * pGlobalLuaGuiExecutionUnit;
    lua_State * pGlobalLuaScriptExecutionUnit;
    lua_State * pGlobalLuaTimerExecutionUnit;
};

/*
class TLuaThread : public QThread
{
public:

    TLuaThread( TLuaInterpreter * s){ rs=s; }
    void run(){ rs->threadRunLuaScript();}

private:

    TLuaInterpreter * rs;
};
*/

class TLuaMainThread : public QThread
{

public:

  TLuaMainThread( TLuaInterpreter * pL ){ pLuaInterpreter = pL;  }
  void run()
  {  
     std::cout << "TLuaMainThread::run() called. Initializing Gatekeeper..."<<std::endl;
      //pLuaInterpreter->initLuaGlobals();
     exit=false;
     while( ! exit )
     {
         if( ! mJobQueue.empty() ) 
         {
             pLuaInterpreter->threadLuaInterpreterExec( getJob() );
         }
         else
         {
             msleep(100);
         }
     }
     std::cout << "TLuaMainThread::run() done exit." << std::endl;
  }
  
  std::string getJob()
  {
      mutex.lock();
      std::string job = mJobQueue.front(); 
      mJobQueue.pop();
      mutex.unlock();
      return job;
  }
  
  void postJob(QString code)
  {
     std::cout << "posting new job <"<<code.toLatin1().data()<<">"<<std::endl;
     std::string job = code.toLatin1().data();
     mutex.lock();
     mJobQueue.push(job);
     mutex.unlock();
     std::cout << "DONE posting new job"<<std::endl;
  }

  void callExit(){ exit = true; } 
  
private:

  TLuaInterpreter * pLuaInterpreter;
  QString code;
  QMutex mutex;
  std::queue<std::string> mJobQueue;
  
  bool exit;
};

/*
class TLuaTimer : public QThread
{
public:

   TLuaTimer( TLuaInterpreter * _rs, int _msec,int _session, QString _func, QString _p)
   {
       rs=_rs;
       msec = _msec;
       session = _session;
       function = _func;
       parameters = _p;
   }
   
   void run()
   {
       qDebug()<<"TLuaTimer::run() goingin to sleep for "<<msec<<" milliseconds...";
       msleep(msec);
       qDebug()<<"TLuaTimer::run() waking up! emitting signalLuaTimerTimeout("<<session<<","<<function<<","<<parameters;
       QString code = function+"(\""+parameters+"\")";
       rs->mpLuaSessionThread->postJob(code);
       qDebug()<<"TLuaTimer::run() exiting thread";
   }
 
private:
    
   TLuaInterpreter * rs;
   int msec;
   int session;
   QString function;
   QString parameters;   
};
  */
#endif



