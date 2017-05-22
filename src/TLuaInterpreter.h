#ifndef MUDLET_LUAINTERPRETER_H
#define MUDLET_LUAINTERPRETER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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

#include "pre_guard.h"
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include "post_guard.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <queue>
#include <string>


class Host;
class TEvent;
class TLuaThread;
class TTrigger;


#define SERVEROUTPUT 1
#define USERCOMMAND 2
#define PROMPT 3
#define RAWDATA 4


class TLuaInterpreter : public QThread
{
    Q_OBJECT

    Q_DISABLE_COPY(TLuaInterpreter)

    friend class TForkedProcess;
    friend class LuaInterface;

public:
    TLuaInterpreter(Host* mpHost, int id);
    ~TLuaInterpreter();
    void setMSDPTable(QString& key, const QString& string_data);
    void parseJSON(QString& key, const QString& string_data, const QString& protocol);
    void msdp2Lua(char* src, int srclen);
    void initLuaGlobals();
    bool call(const QString& function, const QString& mName);
    bool callMulti(const QString& function, const QString& mName);
    bool callConditionFunction(std::string& function, const QString& mName);
    bool call_luafunction(void*);
    bool compile(const QString& code, QString& error, const QString& name);
    bool compileScript(const QString&);
    void setAtcpTable(const QString&, const QString&);
    void setGMCPTable(QString&, const QString&);
    void setChannel102Table(int& var, int& arg);
    bool compileAndExecuteScript(const QString&);
    void loadGlobal();
    QString get_lua_string(const QString& stringName);
    int check_for_mappingscript();
    void set_lua_string(const QString& varName, const QString& varValue);
    void set_lua_table(const QString& tableName, QStringList& variableList);
    void setCaptureGroups(const std::list<std::string>&, const std::list<int>&);
    void setMultiCaptureGroups(const std::list<std::list<std::string>>& captureList, const std::list<std::list<int>>& posList);

    void adjustCaptureGroups(int x, int a);
    void clearCaptureGroups();
    bool callEventHandler(const QString& function, const TEvent& pE);
    static QString dirToString(lua_State*, int);
    static int dirToNumber(lua_State*, int);


    int startTempTimer(double, const QString&);
    int startTempAlias(const QString&, const QString&);
    int startTempTrigger(const QString&, const QString&);
    int startTempBeginOfLineTrigger(const QString&, const QString&);
    int startTempExactMatchTrigger(const QString&, const QString&);
    int startTempLineTrigger(int, int, const QString&);
    int startTempRegexTrigger(const QString&, const QString&);
    int startTempColorTrigger(int, int, const QString&);
    int startPermRegexTrigger(const QString& name, const QString& parent, QStringList& regex, const QString& function);
    int startPermSubstringTrigger(const QString& name, const QString& parent, const QStringList& regex, const QString& function);
    int startPermBeginOfLineStringTrigger(const QString& name, const QString& parent, QStringList& regex, const QString& function);
    int startPermTimer(const QString& name, const QString& parent, double timeout, const QString& function);
    int startPermAlias(const QString& name, const QString& parent, const QString& regex, const QString& function);

    static int getCustomLines(lua_State*);
    static int addCustomLine(lua_State*);
    static int noop(lua_State*);
    static int sendMSDP(lua_State*);
    static int auditAreas(lua_State*);
    static int getAreaExits(lua_State*);
    static int setMergeTables(lua_State* L);
    static int addSupportedTelnetOption(lua_State*);
    static int setDoor(lua_State*);
    static int getDoors(lua_State*);
    static int setExitWeight(lua_State*);
    static int getExitWeights(lua_State*);
    static int uninstallPackage(lua_State*);
    static int setMapZoom(lua_State* L);
    static int createMapImageLabel(lua_State*);
    static int exportAreaImage(lua_State*);
    static int installPackage(lua_State*);
    static int installModule(lua_State* L);
    static int uninstallModule(lua_State* L);
    static int getModulePath(lua_State* L);
    static int reloadModule(lua_State* L);
    static int lockExit(lua_State*);
    static int lockSpecialExit(lua_State*);
    static int hasExitLock(lua_State*);
    static int hasSpecialExitLock(lua_State*);
    static int getMapLabels(lua_State*);
    static int getMapLabel(lua_State*);
    static int highlightRoom(lua_State*);
    static int unHighlightRoom(lua_State*);
    static int createMapLabel(lua_State*);
    static int deleteMapLabel(lua_State*);
    static int getRooms(lua_State*);
    static int connectToServer(lua_State* L);
    static int sendIrc(lua_State*);
    static int showUnzipProgress(lua_State*);
    static int setAppStyleSheet(lua_State*);
    static int setMainWindowSize(lua_State* L);
    static int registerAnonymousEventHandler(lua_State* L);
    static int setRoomChar(lua_State*);
    static int getRoomChar(lua_State*);
    static int deleteArea(lua_State*);
    static int deleteRoom(lua_State*);
    static int getRoomAreaName(lua_State*);
    static int addAreaName(lua_State* L);
    static int getRoomIDbyHash(lua_State* L);
    static int setRoomIDbyHash(lua_State* L);
    static int sendSocket(lua_State* L);
    static int openUrl(lua_State*);
    static int getRoomsByPosition(lua_State*);
    static int getRoomEnv(lua_State*);
    static int downloadFile(lua_State*);
    static int setRoomUserData(lua_State*);
    static int getRoomUserData(lua_State*);
    static int searchRoomUserData(lua_State*);
    static int clearRoomUserData(lua_State*);
    static int clearRoomUserDataItem(lua_State*);
    static int addSpecialExit(lua_State*);
    static int removeSpecialExit(lua_State*);
    static int getSpecialExits(lua_State*);
    static int getSpecialExitsSwap(lua_State*);
    static int appendCmdLine(lua_State*);
    static int getCmdLine(lua_State* L);
    static int clearSpecialExits(lua_State*);
    static int solveRoomCollisions(lua_State*);
    static int setGridMode(lua_State* L);
    static int getCustomEnvColorTable(lua_State* L);
    static int setRoomName(lua_State*);
    static int getRoomName(lua_State*);
    static int setRoomEnv(lua_State*);
    static int setCustomEnvColor(lua_State*);
    static int roomLocked(lua_State*);
    static int setAreaName(lua_State*);
    static int getRoomCoordinates(lua_State*);
    static int setRoomCoordinates(lua_State*);
    static int roomExists(lua_State*);
    static int addRoom(lua_State*);
    static int setExit(lua_State*);
    static int createRoomID(lua_State*);
    static int setRoomArea(lua_State*);
    static int resetRoomArea(lua_State*);
    static int getRoomArea(lua_State*);
    static int denyCurrentSend(lua_State*);
    static int tempBeginOfLineTrigger(lua_State*);
    static int tempExactMatchTrigger(lua_State*);
    static int centerview(lua_State* L);
    static int getAreaTable(lua_State* L);
    static int getAreaTableSwap(lua_State* L);
    static int getPath(lua_State*);
    static int getAreaRooms(lua_State*);
    static int clearCmdLine(lua_State*);
    static int printCmdLine(lua_State*);
    static int searchRoom(lua_State*);
    static int resetProfile(lua_State*);
    static int createMapper(lua_State*);
    static int sendTelnetChannel102(lua_State* L);
    static int isPrompt(lua_State* L);
    static int feedTriggers(lua_State*);
    static int Wait(lua_State* L);
    static int Send(lua_State* L);
    static int sendRaw(lua_State* L);
    static int Echo(lua_State* L);
    static int selectString(lua_State* L); // Was select but I think it clashes with the Lua command with that name
    static int getMainConsoleWidth(lua_State* L);
    static int selectSection(lua_State* L);
    static int replace(lua_State* L);
    static int deselect(lua_State* L);
    static int getRoomExits(lua_State* L);
    static int lockRoom(lua_State* L);
    static int hasFocus(lua_State* L);
    static int setFgColor(lua_State* L);
    static int setBgColor(lua_State* L);
    static int tempTimer(lua_State* L);
    static int closeMudlet(lua_State* L);
    static int loadWindowLayout(lua_State* L);
    static int saveWindowLayout(lua_State* L);
    static int saveProfile(lua_State* L);
    static int openUserWindow(lua_State* L);
    static int echoUserWindow(lua_State* L);
    static int clearUserWindow(lua_State* L);
    static int enableTimer(lua_State* L);
    static int disableTimer(lua_State* L);
    static int killTimer(lua_State* L);
    static int moveCursor(lua_State* L);
    static int insertHTML(lua_State* L);
    static int insertText(lua_State* L);
    static int getLines(lua_State* L);
    static int enableTrigger(lua_State* L);
    static int disableTrigger(lua_State* L);
    static int tempTrigger(lua_State* L);
    static int tempRegexTrigger(lua_State* L);
    static int tempButtonToolbar(lua_State* L);
    static int tempButton(lua_State* L);
    static int tempComplexRegexTrigger(lua_State* L);
    static int killTrigger(lua_State* L);
    static int getLineCount(lua_State* L);
    static int getLineNumber(lua_State* L);
    static int getColumnNumber(lua_State* L);
    static int selectCaptureGroup(lua_State* L);
    static int tempLineTrigger(lua_State* L);
    static int raiseEvent(lua_State* L);
    static int deleteLine(lua_State* L);
    static int copy(lua_State* L);
    static int cut(lua_State* L);
    static int paste(lua_State* L);
    static int pasteWindow(lua_State* L);
    static int setRoomWeight(lua_State* L);
    static int getRoomWeight(lua_State* L);
    static int gotoRoom(lua_State* L);
    static int setMapperView(lua_State* L);
    static int enableKey(lua_State*);
    static int disableKey(lua_State*);
    static int debug(lua_State* L);
    static int setWindowWrap(lua_State*);
    static int setWindowWrapIndent(lua_State*);
    static int resetFormat(lua_State*);
    static int moveCursorEnd(lua_State*);
    static int getLastLineNumber(lua_State*);
    static int getNetworkLatency(lua_State*);
    static int appendBuffer(lua_State*);
    static int createBuffer(lua_State*);
    static int raiseWindow(lua_State*);
    static int lowerWindow(lua_State*);
    static int showUserWindow(lua_State*);
    static int hideUserWindow(lua_State*);
    static int closeUserWindow(lua_State*);
    static int resizeUserWindow(lua_State*);
    static int createStopWatch(lua_State*);
    static int stopStopWatch(lua_State*);
    static int getStopWatchTime(lua_State*);
    static int startStopWatch(lua_State*);
    static int resetStopWatch(lua_State*);
    static int createMiniConsole(lua_State*);
    static int createLabel(lua_State*);
    static int moveWindow(lua_State*);
    static int setTextFormat(lua_State*);
    static int setBackgroundImage(lua_State*);
    static int setBackgroundColor(lua_State*);
    static int createButton(lua_State*);
    static int setLabelClickCallback(lua_State*);
    static int setLabelReleaseCallback(lua_State*);
    static int setLabelOnEnter(lua_State*);
    static int setLabelOnLeave(lua_State*);
    static int getMainWindowSize(lua_State*);
    static int getMousePosition(lua_State*);
    static int setMiniConsoleFontSize(lua_State*);
    static int getCurrentLine(lua_State*);
    static int selectCurrentLine(lua_State*);
    static int spawn(lua_State*);
    static int getButtonState(lua_State*);
    static int showToolBar(lua_State*);
    static int hideToolBar(lua_State*);
    static int loadRawFile(lua_State*);
    static int setBold(lua_State*);
    static int setItalics(lua_State*);
    static int setUnderline(lua_State*);
    static int setStrikeOut(lua_State*);
    static int disconnect(lua_State*);
    static int reconnect(lua_State*);
    static int getMudletHomeDir(lua_State*);
    static int getMudletLuaDefaultPaths(lua_State*);
    static int setTriggerStayOpen(lua_State*);
    static int wrapLine(lua_State*);
    static int getFgColor(lua_State*);
    static int getBgColor(lua_State*);
    static int tempColorTrigger(lua_State*);
    static int isAnsiFgColor(lua_State*);
    static int isAnsiBgColor(lua_State*);
    static int stopSounds(lua_State*);
    static int playSoundFile(lua_State*);
    static int setBorderTop(lua_State*);
    static int setBorderBottom(lua_State*);
    static int setBorderLeft(lua_State*);
    static int setBorderRight(lua_State*);
    static int setBorderColor(lua_State*);
    static int setConsoleBufferSize(lua_State*);
    static int startLogging(lua_State* L);
    static int calcFontWidth(int size);
    static int calcFontHeight(int size);
    static int calcFontSize(lua_State*);
    static int permRegexTrigger(lua_State*);
    static int permSubstringTrigger(lua_State*);
    static int permTimer(lua_State*);
    static int permAlias(lua_State*);
    static int exists(lua_State*);
    static int isActive(lua_State*);
    static int tempAlias(lua_State*);
    static int enableAlias(lua_State*);
    static int disableAlias(lua_State*);
    static int killAlias(lua_State*);
    static int permBeginOfLineStringTrigger(lua_State*);
    static int setLabelStyleSheet(lua_State*);
    static int getTime(lua_State*);
    static int invokeFileDialog(lua_State*);
    static int getTimestamp(lua_State*);
    static int setLink(lua_State*);
    static int echoLink(lua_State*);
    static int insertLink(lua_State*);
    static int echoPopup(lua_State*);
    static int insertPopup(lua_State*);
    static int setPopup(lua_State*);
    static int sendATCP(lua_State*);
    static int sendGMCP(lua_State*);
    static int saveMap(lua_State* L);
    static int loadMap(lua_State* L);
    static int setExitStub(lua_State* L);
    static int connectExitStub(lua_State* L);
    static int getExitStubs(lua_State* L);
    static int getExitStubs1(lua_State* L);
    static int getModulePriority(lua_State* L);
    static int setModulePriority(lua_State* L);
    static int updateMap(lua_State* L);
    static int addMapEvent(lua_State* L);
    static int removeMapEvent(lua_State* L);
    static int getMapEvents(lua_State* L);
    static int addMapMenu(lua_State* L);
    static int removeMapMenu(lua_State* L);
    static int getMapMenus(lua_State* L);
    static int getMudletVersion(lua_State* L);
    static int openWebPage(lua_State* L);
    static int getAllRoomEntrances(lua_State*);
    static int getRoomUserDataKeys(lua_State*);
    static int getAllRoomUserData(lua_State*);
    static int searchAreaUserData(lua_State*);
    static int getMapUserData(lua_State*);
    static int getAreaUserData(lua_State*);
    static int setMapUserData(lua_State*);
    static int setAreaUserData(lua_State*);
    static int getAllAreaUserData(lua_State*);
    static int getAllMapUserData(lua_State*);
    static int clearAreaUserData(lua_State*);
    static int clearAreaUserDataItem(lua_State*);
    static int clearMapUserData(lua_State*);
    static int clearMapUserDataItem(lua_State*);
    static int setDefaultAreaVisible(lua_State*);
    static int getProfileName(lua_State*);
    static int raiseGlobalEvent(lua_State*);
    static int setServerEncoding(lua_State *);
    static int getServerEncoding(lua_State *);
    static int getServerEncodingsList(lua_State *);
    static int alert(lua_State* L);
// PLACEMARKER: End of Lua functions declarations

public slots:
    void slot_replyFinished(QNetworkReply*);
    void slotPurge();
    void slotDeleteSender();

private:
    QNetworkAccessManager* mpFileDownloader;

    std::list<std::string> mCaptureGroupList;
    std::list<int> mCaptureGroupPosList;
    std::list<std::list<std::string>> mMultiCaptureGroupList;
    std::list<std::list<int>> mMultiCaptureGroupPosList;
    void logError(std::string& e, const QString&, const QString& function);

    static std::map<lua_State*, Host*> luaInterpreterMap;
    QMap<QNetworkReply*, QString> downloadMap;

    lua_State* pGlobalLua;

    QPointer<Host> mpHost;
    int mHostID;
    QList<QObject*> objectsToDelete;
    QTimer purgeTimer;
};

#endif // MUDLET_LUAINTERPRETER_H
