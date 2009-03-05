/***************************************************************************
                          crunningscript.h  -  running script
                             -------------------
    begin                : Ne dec 8 2002
    copyright            : (C) 2002-2003 by Tomas Mecir
    email                : kmuddy@kmuddy.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CRUNNINGSCRIPT_H
#define CRUNNINGSCRIPT_H

#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include "cvariablelist.h"
#include "ckmprotocol.h"
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
#include "cscript.h"
#include "ctriggerlist.h"
#include "ctrigger.h"

class TLuaMainThread;
class TLuaThread;
class TGatekeeperThread;

#define SERVEROUTPUT 1
#define USERCOMMAND 2
#define PROMPT 3
#define RAWDATA 4

class cRunningScript : public QThread  {
   Q_OBJECT

public:
  
  friend class cScript;
  friend class cRunningList;
  
  cRunningScript (cScript *s);
  ~cRunningScript ();

  lua_State * getGlobalLuaState(){ return pGlobalLua; }
  void sendToScript (const QString &msg);
  void launch (int fcState);
  const QString name ();
  void setLuaMainScript( cRunningScript * s ){ pLuaMainScript = s; }
  bool isRunning ();
  void terminate ();
  void kill ();
  int getId () { return id; };
  void setId (int _id) { id = _id; }; /** set script ID; used by cRunningList */
  void runScript();
  void runLuaScript();
  void startLuaExecThread();
  void threadRunLuaScript();
  void threadLuaInterpreterExec(string code);
  void execLuaCode(QString code);
  void startLuaSessionInterpreter();
  TGatekeeperThread * mpGatekeeperThread;
  void initLuaGlobals();
  void invokeSystemMessage(QString MessageType, int session, QString msg);
  
  static const char * getMudOutput( int scriptID,int sessionID );
  static const char * getMudOutputRAW( int, int );
  static const char * getUserCommand(int, int);
  static const char * getPrompt(int, int);
  static int GetMudOutput(lua_State *L);
  static int GetMudOutputRAW(lua_State *L);
  static int GetUserCommand(lua_State *L);
  static int GetPrompt(lua_State *L);
  static int Wait(lua_State *L);
  static int Send(lua_State *L);
  static int Echo(lua_State *L);
  static int AddTrigger(lua_State *L);
  static int DeleteTrigger(lua_State *L);
  static int AddTimer(lua_State *L);
  static int DeleteTimer(lua_State *L);
  static int OpenWindow(lua_State *L);
  static int ClearWindow(lua_State *L);
  static int EchoWindow(lua_State *L);
  
  void processRequest (const QString &type, const QString &data);

  //FIXME: make private and provide getters  
  cScript *script;
  int id;

 
signals:
  void signalAddTimer(int,int,QString,QString);
  void signalOpenWindow(int,QString);
  void signalEchoWindow(int,QString,QString);
  void signalClearWindow(int,QString);
  void signalDeleteTrigger(int, QString);
  void signalNewTrigger(QString TriggerName, QString regex, int sessionID, QString TriggerCallback);
  void signalNewJob(QString);
  void signalEchoMessage(QString, int, QString);
  void signalNewCommand(int, QString); //signal of the lua thread unit command dispatcher for the main event loop to post events
  void signalNewEcho(int, QString); //signal of the lua thread unit echo dispatcher for the main event loop to post events
  void signalNewLuaCodeToExecute(QString);
  void sendText (const QString &text);
  void displayText (const QString &text);
  void scriptFinished (cRunningScript *me, int returnValue);
  void scriptKilled (cRunningScript *me);
  void scriptFailed (cRunningScript *me);

public slots:
  void slotLuaTimerCallback(int session, QString callbackfunc, QString callbackfunc_param);
  void slotAddTimer(int,int,QString,QString);
  void slotOpenWindow(int,QString);
  void slotEchoWindow(int,QString,QString);
  void slotClearWindow(int,QString);
  void slotDeleteTrigger(int, QString);
  void slotNewTrigger(QString TriggerName, QString regex, int sessionID, QString TriggerCallback);
  void slotEchoMessage(QString, int, QString);
  void slotNewCommand(int,QString);
  void slotNewEcho(int,QString);
  void displayOutputMsg();
  void displayErrorMsg();
  void exited (int exitCode, QProcess::ExitStatus exitStatus);

public:
  QString line;
  QProcess *process;
  QString prefix, suffix;

  const char * getMudOutputBuffer();
  const char * getMudOutputRAWBuffer();
  const char * getUserCommandBuffer();
  const char * getPromptBuffer();
  cVariableList * varlist; //sessionVariableList
  QTcpSocket *commandChannelSocket;
  int commandChannelPort;
  
  lua_State* pGlobalLua;

  TLuaMainThread * mpLuaSessionThread;
  cRunningScript * pLuaMainScript;
  string MudOutputBuffer;
  string MudOutputRAWBuffer;
  string UserCommandBuffer;
  string PromptBuffer;
};


class TLuaThread : public QThread
{
public:

TLuaThread(cRunningScript* s){ rs=s; }
void run()
{
  rs->threadRunLuaScript();
}

private:

  cRunningScript * rs;
};

class TLuaMainThread : public QThread
{

public:

  TLuaMainThread(cRunningScript* rs){ runningscript=rs;  }
  void run()
  {  
     cout << "TLuaMainThread::run() called. Initializing Gatekeeper..."<<endl;
  			runningscript->initLuaGlobals();
     exit=false;
     while(!exit)
     {
         if(!mJobQueue.empty()) 
         {
             runningscript->threadLuaInterpreterExec(getJob());
         }
         else
         {
             msleep(100);
         }
     }
     cout << "TLuaMainThread::run() done exit." << endl;
  }
  
  string getJob()
  {
      mutex.lock();
      string job = mJobQueue.front(); 
      mJobQueue.pop();
      mutex.unlock();
      return job;
  }
  
  void postJob(QString code)
  {
  cout << "posting new job <"<<code.toLatin1().data()<<">"<<endl;
     string job = code.toLatin1().data();
     mutex.lock();
     mJobQueue.push(job);
     mutex.unlock();
  cout << "DONE posting new job"<<endl;
  }

  void callExit(){ exit = true; } 
  
private:

  cRunningScript * runningscript;
  QString code;
  QMutex mutex;
  queue<string> mJobQueue;

  bool exit;
};

class TLuaTimer : public QThread
{
public:

   TLuaTimer(cRunningScript* _rs, int _msec,int _session, QString _func, QString _p)
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
   cRunningScript * rs;
   int msec;
   int session;
   QString function;
   QString parameters;   
};

#endif


