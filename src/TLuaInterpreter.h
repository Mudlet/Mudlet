#ifndef MUDLET_LUAINTERPRETER_H
#define MUDLET_LUAINTERPRETER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2018-2023 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
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

#include "TMap.h"
#include "TMediaData.h"
#include "TTextCodec.h"
#include "TTrigger.h"
#include "utils.h"

#include "pre_guard.h"
#include <QEvent>
#include <QFileSystemWatcher>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include <edbee/texteditorwidget.h>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB
#include "post_guard.h"

extern "C" {
    #include <lauxlib.h>
    #include <lua.h>
    #include <lualib.h>
}

#include <list>
#include <string>
#include <memory>

class Host;
class TAction;
class TEvent;
class TLuaThread;
class TMapLabel;
class TTrigger;


#define SERVEROUTPUT 1
#define USERCOMMAND 2
#define PROMPT 3
#define RAWDATA 4

using NamedMatchesRanges = QMap<QString, QPair<int, int>>;


class TLuaInterpreter : public QThread
{
    Q_OBJECT

    friend class TForkedProcess;
    friend class LuaInterface;

public:
    Q_DISABLE_COPY(TLuaInterpreter)
    TLuaInterpreter(Host* pH, const QString& hostName, const int id);
    ~TLuaInterpreter();
    void setMSDPTable(QString& key, const QString& string_data);
    void parseJSON(QString& key, const QString& string_data, const QString& protocol);
    void parseMSSP(const QString& string_data);
    void msdp2Lua(const char*);
    void initLuaGlobals();
    void initIndenterGlobals();
    lua_State* getLuaGlobalState();

    bool call(const QString& function, const QString& mName, const bool muteDebugOutput = false);
    std::pair<bool, bool> callReturnBool(const QString& function, const QString& mName);
    bool callMulti(const QString& function, const QString& mName);
    std::pair<bool, bool> callMultiReturnBool(const QString& function, const QString& mName);
    bool callConditionFunction(std::string& function, const QString& mName);
    bool call_luafunction(void* pT);
    void delete_luafunction(void* pT);
    void delete_luafunction(const QString& name);
    std::pair<bool, bool> callLuaFunctionReturnBool(void* pT);
    double condenseMapLoad();
    bool compile(const QString& code, QString& error, const QString& name);
    void setAtcpTable(const QString&, const QString&);
    void signalMXPEvent(const QString& type, const QMap<QString, QString>& attrs, const QStringList& actions);
    void setGMCPTable(QString&, const QString&);
    void setMSSPTable(const QString&);
    void setChannel102Table(int& var, int& arg);
    bool compileAndExecuteScript(const QString&);
    QString formatLuaCode(const QString&);
    void loadGlobal();
    QString getLuaString(const QString& stringName);
    int check_for_mappingscript();
    int check_for_custom_speedwalk();
    void set_lua_integer(const QString& varName, int varValue);
    void set_lua_string(const QString& varName, const QString& varValue);
    void set_lua_table(const QString& tableName, QStringList& variableList);
    void setCaptureGroups(const std::list<std::string>&, const std::list<int>&);
    void setCaptureNameGroups(const NameGroupMatches&, const NamedMatchesRanges&);
    void setMultiCaptureGroups(const std::list<std::list<std::string>>& captureList, const std::list<std::list<int>>& posList, QVector<NameGroupMatches>& nameMatches);
    void adjustCaptureGroups(int x, int a);
    void clearCaptureGroups();
    bool callEventHandler(const QString& function, const TEvent& pE);
    bool callCmdLineAction(const int func, QString);
    bool callAnonymousFunction(const int func, QString name);
    bool callLabelCallbackEvent(const int func, const QEvent* qE = nullptr);
    static QString dirToString(lua_State*, int);
    static int dirToNumber(lua_State*, int);
    void updateAnsi16ColorsInTable();
    void updateExtendedAnsiColorsInTable();
    int createHttpResponseTable(QNetworkReply*);
    void createHttpHeadersTable(lua_State*, QNetworkReply*);
    void createCookiesTable(lua_State*, QNetworkReply*);


    QPair<int, QString> startTempTimer(double timeout, const QString& function, const bool repeating = false);
    int startTempAlias(const QString&, const QString&);
    int startTempKey(int&, int&, const QString&);
    int startTempTrigger(const QString& regex, const QString& function, int expiryCount = -1);
    int startTempBeginOfLineTrigger(const QString&, const QString&, int expiryCount = -1);
    int startTempExactMatchTrigger(const QString&, const QString&, int expiryCount = -1);
    int startTempLineTrigger(int, int, const QString&, int expiryCount = -1);
    int startTempRegexTrigger(const QString&, const QString&, int expiryCount = -1);
    int startTempColorTrigger(int, int, const QString&, int expiryCount = -1);
    int startTempPromptTrigger(const QString& function, int expiryCount = -1);
    std::pair<int, QString> startPermRegexTrigger(const QString& name, const QString& parent, QStringList& patterns, const QString& function);
    std::pair<int, QString> startPermSubstringTrigger(const QString& name, const QString& parent, const QStringList& patterns, const QString& function);
    std::pair<int, QString> startPermBeginOfLineStringTrigger(const QString& name, const QString& parent, QStringList& patterns, const QString& function);
    std::pair<int, QString> startPermPromptTrigger(const QString& name, const QString& parent, const QString& function);
    std::pair<int, QString> startPermTimer(const QString& name, const QString& parent, double timeout, const QString& function);
    std::pair<int, QString> createPermScript(const QString& name, const QString& parent, const QString& luaCode);
    std::pair<int, QString> setScriptCode(const QString& name, const QString& luaCode, const int pos);
    std::pair<int, QString> startPermAlias(const QString& name, const QString& parent, const QString& regex, const QString& function);
    std::pair<int, QString> startPermKey(QString&, QString&, int&, int&, QString&);

    static int getCustomLines(lua_State*);
    static int getCustomLines1(lua_State*);
    static int addCustomLine(lua_State*);
    static int removeCustomLine(lua_State*);
    static int noop(lua_State*);
    static int sendMSDP(lua_State*);
    static int auditAreas(lua_State*);
    static int getAreaExits(lua_State*);
    static int setMergeTables(lua_State*);
    static int addSupportedTelnetOption(lua_State*);
    static int setDoor(lua_State*);
    static int getDoors(lua_State*);
    static int setExitWeight(lua_State*);
    static int getExitWeights(lua_State*);
    static int uninstallPackage(lua_State*);
    static int setMapZoom(lua_State*);
    static int getMapZoom(lua_State*);
    static int createMapImageLabel(lua_State*);
    static int installPackage(lua_State*);
    static int installModule(lua_State*);
    static int uninstallModule(lua_State*);
    static int getModulePath(lua_State*);
    static int reloadModule(lua_State*);
    static int enableModuleSync(lua_State*);
    static int disableModuleSync(lua_State*);
    static int getModuleSync(lua_State*);
    static int getPackages(lua_State*);
    static int getModules(lua_State*);
    static int getPackageInfo(lua_State*);
    static int getModuleInfo(lua_State*);
    static int setPackageInfo(lua_State*);
    static int setModuleInfo(lua_State*);
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
    static int connectToServer(lua_State*);
    static int sendIrc(lua_State*);
    static int getIrcNick(lua_State*);
    static int getIrcServer(lua_State*);
    static int getIrcChannels(lua_State*);
    static int getIrcConnectedHost(lua_State*);
    static int setIrcNick(lua_State*);
    static int setIrcServer(lua_State*);
    static int setIrcChannels(lua_State*);
    static int restartIrc(lua_State*);
    static int showUnzipProgress(lua_State*);
    static int setAppStyleSheet(lua_State*);
    static int setProfileStyleSheet(lua_State*);
    static int setMainWindowSize(lua_State*);
    static int registerAnonymousEventHandler(lua_State*);
    static int setRoomChar(lua_State*);
    static int getRoomChar(lua_State*);
    static int setRoomCharColor(lua_State*);
    static int unsetRoomCharColor(lua_State*);
    static int getRoomCharColor(lua_State*);
    static int deleteArea(lua_State*);
    static int deleteRoom(lua_State*);
    static int getRoomAreaName(lua_State*);
    static int addAreaName(lua_State*);
    static int getRoomIDbyHash(lua_State*);
    static int getRoomHashByID(lua_State*);
    static int setRoomIDbyHash(lua_State*);
    static int sendSocket(lua_State*);
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
    static int getCmdLine(lua_State*);
    static int selectCmdLineText(lua_State*);
    static int addCmdLineSuggestion(lua_State*);
    static int removeCmdLineSuggestion(lua_State*);
    static int clearCmdLineSuggestions(lua_State*);
    static int addCmdLineBlacklist(lua_State*);
    static int removeCmdLineBlacklist(lua_State*);
    static int clearCmdLineBlacklist(lua_State*);
    static int clearSpecialExits(lua_State*);
    static int setGridMode(lua_State*);
    static int getGridMode(lua_State*);
    static int getCustomEnvColorTable(lua_State*);
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
    static int centerview(lua_State*);
    static int getAreaTable(lua_State*);
    static int getAreaTableSwap(lua_State*);
    static int getPath(lua_State*);
    static int getAreaRooms(lua_State*);
    static int getAreaRooms1(lua_State*);
    static int clearCmdLine(lua_State*);
    static int printCmdLine(lua_State*);
    static int searchRoom(lua_State*);
    static int resetProfile(lua_State*);
    static int createMapper(lua_State*);
    static int createCommandLine(lua_State*);
    static int sendTelnetChannel102(lua_State*);
    static int isPrompt(lua_State*);
    static int feedTriggers(lua_State*);
    static int feedTelnet(lua_State*);
    static int Wait(lua_State*);
    static int expandAlias(lua_State*);
    static int sendRaw(lua_State*);
    static int echo(lua_State*);
    static int selectString(lua_State*); // Was select but I think it clashes with the Lua command with that name
    static int getMainConsoleWidth(lua_State*);
    static int selectSection(lua_State*);
    static int getSelection(lua_State*);
    static int replace(lua_State*);
    static int deselect(lua_State*);
    static int getRoomExits(lua_State*);
    static int lockRoom(lua_State*);
    static int hasFocus(lua_State*);
    static int setFgColor(lua_State*);
    static int setBgColor(lua_State*);
    static int tempTimer(lua_State*);
    static int closeMudlet(lua_State*);
    static int loadWindowLayout(lua_State*);
    static int saveWindowLayout(lua_State*);
    static int saveProfile(lua_State*);
    static int setFont(lua_State*);
    static int getFont(lua_State*);
    static int setFontSize(lua_State*);
    static int getFontSize(lua_State*);
    static int openUserWindow(lua_State*);
    static int setUserWindowTitle(lua_State*);
    static int echoUserWindow(lua_State*);
    static int clearUserWindow(lua_State*);
    static int enableTimer(lua_State*);
    static int disableTimer(lua_State*);
    static int killTimer(lua_State*);
    static int remainingTime(lua_State*);
    static int moveCursor(lua_State*);
    static int insertHTML(lua_State*);
    static int insertText(lua_State*);
    static int getLines(lua_State*);
    static int enableTrigger(lua_State*);
    static int disableTrigger(lua_State*);
    static int tempTrigger(lua_State*);
    static int tempRegexTrigger(lua_State*);
    static int tempButtonToolbar(lua_State*);
    static int setButtonStyleSheet(lua_State*);
    static int tempButton(lua_State*);
    static int tempComplexRegexTrigger(lua_State*);
    static int killTrigger(lua_State*);
    static int getLineCount(lua_State*);
    static int getLineNumber(lua_State*);
    static int getColumnNumber(lua_State*);
    static int selectCaptureGroup(lua_State*);
    static int tempLineTrigger(lua_State*);
    static int raiseEvent(lua_State*);
    static int deleteLine(lua_State*);
    static int copy(lua_State*);
    static int cut(lua_State*);
    static int paste(lua_State*);
    static int pasteWindow(lua_State*);
    static int setRoomWeight(lua_State*);
    static int getRoomWeight(lua_State*);
    static int gotoRoom(lua_State*);
    static int permKey(lua_State*);
    static int tempKey(lua_State*);
    static int enableKey(lua_State*);
    static int disableKey(lua_State*);
    static int killKey(lua_State*);
    static int debug(lua_State*);
    static int errorc(lua_State*);
    static int showHandlerError(lua_State*);
    static int setWindowWrap(lua_State*);
    static int getWindowWrap(lua_State*);
    static int setWindowWrapIndent(lua_State*);
    static int resetFormat(lua_State*);
    static int moveCursorEnd(lua_State*);
    static int getLastLineNumber(lua_State*);
    static int getNetworkLatency(lua_State*);
    static int appendBuffer(lua_State*);
    static int createBuffer(lua_State*);
    static int raiseWindow(lua_State*);
    static int lowerWindow(lua_State*);
    static int showWindow(lua_State*);
    static int hideWindow(lua_State*);
    static int closeUserWindow(lua_State*);
    static int resizeWindow(lua_State*);
    static int createStopWatch(lua_State*);
    static int stopStopWatch(lua_State*);
    static int getStopWatchTime(lua_State*);
    static int startStopWatch(lua_State*);
    static int resetStopWatch(lua_State*);
    static int adjustStopWatch(lua_State*);
    static int deleteStopWatch(lua_State*);
    static int setStopWatchPersistence(lua_State*);
    static int getStopWatches(lua_State*);
    static int setStopWatchName(lua_State*);
    static int getStopWatchBrokenDownTime(lua_State*);
    static int createMiniConsole(lua_State*);
    static int createScrollBox(lua_State*);
    static int createLabel(lua_State*);
    static int createLabelMainWindow(lua_State*, const QString& labelName);
    static int createLabelUserWindow(lua_State*, const QString& windowName, const QString& labelName);
    static int deleteLabel(lua_State*);
    static int setLabelToolTip(lua_State*);
    static int setLabelCursor(lua_State*);
    static int setLabelCustomCursor(lua_State*);
    static int moveWindow(lua_State*);
    static int setWindow(lua_State*);
    static int openMapWidget(lua_State*);
    static int closeMapWidget(lua_State*);
    static int setTextFormat(lua_State*);
    static int setBackgroundImage(lua_State*);
    static int resetBackgroundImage(lua_State*);
    static int setBackgroundColor(lua_State*);
    static int setCommandBackgroundColor(lua_State*);
    static int setCommandForegroundColor(lua_State*);
    static int setLabelClickCallback(lua_State*);
    static int setMovie(lua_State*);
    static int startMovie(lua_State*);
    static int setMovieSpeed(lua_State*);
    static int scaleMovie(lua_State*);
    static int setMovieFrame(lua_State*);
    static int pauseMovie(lua_State*);
    static int setCmdLineAction(lua_State*);
    static int resetCmdLineAction(lua_State*);
    static int setCmdLineStyleSheet(lua_State*);
    static int getImageSize(lua_State*);
    static int setLabelDoubleClickCallback(lua_State*);
    static int setLabelReleaseCallback(lua_State*);
    static int setLabelMoveCallback(lua_State*);
    static int setLabelWheelCallback(lua_State*);
    static int setLabelOnEnter(lua_State*);
    static int setLabelOnLeave(lua_State*);
    static int getMainWindowSize(lua_State*);
    static int getUserWindowSize(lua_State*);
    static int getMousePosition(lua_State*);
    static int setMiniConsoleFontSize(lua_State*);
    static int setProfileIcon(lua_State*);
    static int resetProfileIcon(lua_State*);
    static int getCurrentLine(lua_State*);
    static int selectCurrentLine(lua_State*);
    static int spawn(lua_State*);
    static int getButtonState(lua_State*);
    static int setButtonState(lua_State*);
    static int showToolBar(lua_State*);
    static int hideToolBar(lua_State*);
    static int loadReplay(lua_State*);
    static int setBold(lua_State*);
    static int setItalics(lua_State*);
    static int setReverse(lua_State*);
    static int setOverline(lua_State*);
    static int setStrikeOut(lua_State*);
    static int setUnderline(lua_State*);
    static int disconnect(lua_State*);
    static int reconnect(lua_State*);
    static int getMudletHomeDir(lua_State*);
    static int setTriggerStayOpen(lua_State*);
    static int wrapLine(lua_State*);
    static int getFgColor(lua_State*);
    static int getBgColor(lua_State*);
    static int tempColorTrigger(lua_State*);
    static int isAnsiFgColor(lua_State*);
    static int isAnsiBgColor(lua_State*);
    static int receiveMSP(lua_State*);
    static int loadMusicFile(lua_State*);
    static int loadSoundFile(lua_State*);
    static int playMusicFile(lua_State*);
    static int playSoundFile(lua_State*);
    static int getPlayingMusic(lua_State*);
    static int getPlayingSounds(lua_State*);
    static int stopMusic(lua_State*);
    static int stopSounds(lua_State*);
    static int purgeMediaCache(lua_State*);
    static int setBorderSizes(lua_State*);
    static int setBorderTop(lua_State*);
    static int setBorderBottom(lua_State*);
    static int setBorderLeft(lua_State*);
    static int setBorderRight(lua_State*);
    static int setBorderColor(lua_State*);
    static int getBorderTop(lua_State*);
    static int getBorderBottom(lua_State*);
    static int getBorderLeft(lua_State*);
    static int getBorderRight(lua_State*);
    static int getBorderSizes(lua_State*);
    static int getConsoleBufferSize(lua_State*);
    static int setConsoleBufferSize(lua_State*);
    static int enableScrollBar(lua_State*);
    static int disableScrollBar(lua_State*);
    static int disableHorizontalScrollBar(lua_State*);
    static int enableHorizontalScrollBar(lua_State*);
    static int enableScrolling(lua_State*);
    static int disableScrolling(lua_State*);
    static int scrollingActive(lua_State*);
    static int enableCommandLine(lua_State*);
    static int disableCommandLine(lua_State*);
    static int enableClickthrough(lua_State*);
    static int disableClickthrough(lua_State*);
    static int startLogging(lua_State*);
    static int appendLog(lua_State*);
    static int calcFontWidth(int size);
    static int calcFontHeight(int size);
    static int calcFontSize(lua_State*);
    static int permRegexTrigger(lua_State*);
    static int permSubstringTrigger(lua_State*);
    static int permTimer(lua_State*);
    static int permScript(lua_State*);
    static int getScript(lua_State*);
    static int setScript(lua_State*);
    static int enableScript(lua_State*);
    static int disableScript(lua_State*);
    static int permAlias(lua_State*);
    static int exists(lua_State*);
    static int isActive(lua_State*);
    static int isAncestorsActive(lua_State*);
    static int ancestors(lua_State*);
    static int tempAlias(lua_State*);
    static int enableAlias(lua_State*);
    static int disableAlias(lua_State*);
    static int killAlias(lua_State*);
    static int permBeginOfLineStringTrigger(lua_State*);
    static int setLabelStyleSheet(lua_State*);
    static int setUserWindowStyleSheet(lua_State*);
    static int getTime(lua_State*);
    static int getEpoch(lua_State*);
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
    static int saveMap(lua_State*);
    static int loadMap(lua_State*);
    static int setExitStub(lua_State*);
    static int connectExitStub(lua_State*);
    static int getExitStubs(lua_State*);
    static int getExitStubs1(lua_State*);
    static int getExitStubsNames(lua_State*);
    static int getModulePriority(lua_State*);
    static int setModulePriority(lua_State*);
    static int updateMap(lua_State*);
    static int addMapEvent(lua_State*);
    static int removeMapEvent(lua_State*);
    static int getMapEvents(lua_State*);
    static int addMapMenu(lua_State*);
    static int removeMapMenu(lua_State*);
    static int getMapMenus(lua_State*);
    static int getMudletVersion(lua_State*);
    static int openWebPage(lua_State*);
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
    static int getCommandSeparator(lua_State*);
    static int raiseGlobalEvent(lua_State*);
    static int setServerEncoding(lua_State*);
    static int getServerEncoding(lua_State*);
    static int getServerEncodingsList(lua_State*);
    static int alert(lua_State*);
#ifdef QT_TEXTTOSPEECH_LIB
    static int ttsSpeak(lua_State*);
    static int ttsSkip(lua_State*);
    static int ttsSetRate(lua_State*);
    static int ttsSetPitch(lua_State*);
    static int ttsSetVolume(lua_State*);
    static int ttsGetRate(lua_State*);
    static int ttsGetPitch(lua_State*);
    static int ttsGetVolume(lua_State*);
    static int ttsSetVoiceByName(lua_State*);
    static int ttsSetVoiceByIndex(lua_State*);
    static int ttsGetCurrentVoice(lua_State*);
    static int ttsGetVoices(lua_State*);
    static int ttsQueue(lua_State*);
    static int ttsGetQueue(lua_State*);
    static int ttsPause(lua_State*);
    static int ttsResume(lua_State*);
    static int ttsClearQueue(lua_State*);
    static int ttsGetCurrentLine(lua_State*);
    static int ttsGetState(lua_State*);
    static void ttsBuild();
    static void ttsStateChanged(QTextToSpeech::State state);
#endif // QT_TEXTTOSPEECH_LIB
    static int tempPromptTrigger(lua_State*);
    static int permPromptTrigger(lua_State*);
    static int getColumnCount(lua_State*);
    static int getRowCount(lua_State*);
    static int getOS(lua_State*);
    static int getClipboardText(lua_State*);
    static int setClipboardText(lua_State*);
    static int getAvailableFonts(lua_State*);
    static int tempAnsiColorTrigger(lua_State*);
    static int setDiscordApplicationID(lua_State*);
    static int setDiscordGameUrl(lua_State*);
    static int usingMudletsDiscordID(lua_State*);
    static int setDiscordState(lua_State*);
    static int setDiscordDetail(lua_State*);
    static int setDiscordLargeIcon(lua_State*);
    static int setDiscordLargeIconText(lua_State*);
    static int setDiscordSmallIcon(lua_State*);
    static int setDiscordSmallIconText(lua_State*);
    static int setDiscordElapsedStartTime(lua_State*);
    static int setDiscordRemainingEndTime(lua_State*);
    static int setDiscordParty(lua_State*);
    static int getDiscordState(lua_State*);
    static int getDiscordDetail(lua_State*);
    static int getDiscordLargeIcon(lua_State*);
    static int getDiscordLargeIconText(lua_State*);
    static int getDiscordSmallIcon(lua_State*);
    static int getDiscordSmallIconText(lua_State*);
    static int getDiscordTimeStamps(lua_State*);
    static int getDiscordParty(lua_State*);
    static int setDiscordGame(lua_State*);
    static int resetDiscordData(lua_State*);
    static int getPlayerRoom(lua_State*);
    static int getMapSelection(lua_State*);
    static int addWordToDictionary(lua_State*);
    static int removeWordFromDictionary(lua_State*);
    static int spellCheckWord(lua_State*);
    static int spellSuggestWord(lua_State*);
    static int getDictionaryWordList(lua_State*);
    static int getTextFormat(lua_State*);
    static int getCharacterName(lua_State*);
    static int getProfileInformation(lua_State*);
    static int setProfileInformation(lua_State*);
    static int clearProfileInformation(lua_State*);
    static int getWindowsCodepage(lua_State*);
    static int getHTTP(lua_State*);
    static int customHTTP(lua_State*);
    static int putHTTP(lua_State*);
    static int postHTTP(lua_State*);
    static int deleteHTTP(lua_State*);
    static int getConnectionInfo(lua_State*);
    static int unzipAsync(lua_State*);
    static int setMapWindowTitle(lua_State*);
    static int getMudletInfo(lua_State*);
    static int getMapBackgroundColor(lua_State*);
    static int setMapBackgroundColor(lua_State*);
    static int getMapRoomExitsColor(lua_State*);
    static int setMapRoomExitsColor(lua_State*);
    static int showNotification(lua_State*);
    static int saveJsonMap(lua_State*);
    static int loadJsonMap(lua_State*);
    static int registerMapInfo(lua_State*);
    static int killMapInfo(lua_State*);
    static int enableMapInfo(lua_State*);
    static int disableMapInfo(lua_State*);
    static int getProfileTabNumber(lua_State*);
    static int addFileWatch(lua_State*);
    static int removeFileWatch(lua_State*);
    static int addMouseEvent(lua_State*);
    static int removeMouseEvent(lua_State*);
    static int getMouseEvents(lua_State*);
    static int setConfig(lua_State*);
    static int addCommandLineMenuEvent(lua_State*);
    static int removeCommandLineMenuEvent(lua_State*);
    static int deleteMap(lua_State*);
    static int windowType(lua_State*);
    static int getProfileStats(lua_State*);
    static int getBackgroundColor(lua_State*);
    static int getLabelStyleSheet(lua_State*);
    static int getLabelSizeHint(lua_State*);
    static int announce(lua_State*);
    static int scrollTo(lua_State*);
    static int getScroll(lua_State*);
    static int getConfig(lua_State*);
    static int getSaveCommandHistory(lua_State*);
    static int setSaveCommandHistory(lua_State*);
    static int clearMapSelection(lua_State*);
    static int findItems(lua_State*);
    static int holdingModifiers(lua_State*);
    static int getProfiles(lua_State*);
    static int loadProfile(lua_State*);
    static int closeProfile(lua_State*);
    // PLACEMARKER: End of Lua functions declarations
    // check new functions against https://www.linguistic-antipatterns.com when creating them

    void freeLuaRegistryIndex(int index);
    void freeAllInLuaRegistry(TEvent);

    inline static const QMap<Qt::MouseButton, QString> csmMouseButtons = {
        {Qt::NoButton, qsl("NoButton")},
        {Qt::LeftButton, qsl("LeftButton")},
        {Qt::RightButton, qsl("RightButton")},
        {Qt::MiddleButton, qsl("MidButton")},
        {Qt::BackButton, qsl("BackButton")},
        {Qt::ForwardButton, qsl("ForwardButton")},
        {Qt::TaskButton, qsl("TaskButton")},
        {Qt::ExtraButton4, qsl("ExtraButton4")},
        {Qt::ExtraButton5, qsl("ExtraButton5")},
        {Qt::ExtraButton6, qsl("ExtraButton6")},
        {Qt::ExtraButton7, qsl("ExtraButton7")},
        {Qt::ExtraButton8, qsl("ExtraButton8")},
        {Qt::ExtraButton9, qsl("ExtraButton9")},
        {Qt::ExtraButton10, qsl("ExtraButton10")},
        {Qt::ExtraButton11, qsl("ExtraButton11")},
        {Qt::ExtraButton12, qsl("ExtraButton12")},
        {Qt::ExtraButton13, qsl("ExtraButton13")},
        {Qt::ExtraButton14, qsl("ExtraButton14")},
        {Qt::ExtraButton15, qsl("ExtraButton15")},
        {Qt::ExtraButton16, qsl("ExtraButton16")},
        {Qt::ExtraButton17, qsl("ExtraButton17")},
        {Qt::ExtraButton18, qsl("ExtraButton18")},
        {Qt::ExtraButton19, qsl("ExtraButton19")},
        {Qt::ExtraButton20, qsl("ExtraButton20")},
        {Qt::ExtraButton21, qsl("ExtraButton21")},
        {Qt::ExtraButton22, qsl("ExtraButton22")},
        {Qt::ExtraButton23, qsl("ExtraButton23")},
        {Qt::ExtraButton24, qsl("ExtraButton24")}};

    static const QString csmInvalidRoomID;
    static const QString csmInvalidStopWatchID;
    static const QString csmInvalidRedValue;
    static const QString csmInvalidGreenValue;
    static const QString csmInvalidBlueValue;
    static const QString csmInvalidAlphaValue;
    static const QString csmInvalidExitRoomID;
    static const QString csmInvalidItemID;
    static const QString csmInvalidAreaID;
    static const QString csmInvalidAreaName;

public slots:
    void slot_httpRequestFinished(QNetworkReply*);
    void slot_pathChanged(const QString& path);
    void slot_purge();
    void slot_deleteSender(int, QProcess::ExitStatus);

private:
    static bool getVerifiedBool(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static QString getVerifiedString(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static int getVerifiedInt(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static float getVerifiedFloat(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static double getVerifiedDouble(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static std::pair<bool, QString> getVerifiedStringOrInteger(lua_State*, const char* functionName, const int pos, const char* publicName, const bool isOptional = false);
    static void errorArgumentType(lua_State*, const char* functionName, const int pos, const char* publicName, const char* publicType, const bool isOptional = false);
    static int warnArgumentValue(lua_State*, const char* functionName, const QString& message, const bool useFalseInsteadofNil = false);
    static int warnArgumentValue(lua_State*, const char* functionName, const char* message, const bool useFalseInsteadofNil = false);
    static int setLabelCallback(lua_State*, const QString& funcName);
    static int movieFunc(lua_State*, const QString& funcName);
    static std::pair<bool, QString> discordApiEnabled(lua_State*, bool writeAccess = false);
    static void setRequestDefaults(const QUrl& url, QNetworkRequest& request);
    static int performHttpRequest(lua_State*, const char* functionName, const int pos, QNetworkAccessManager::Operation operation, const QString& verb);
    static void raiseDownloadProgressEvent(lua_State*, QString, qint64, qint64);
    // The last argument is only needed if the third one is true:
    static void generateElapsedTimeTable(lua_State*, const QStringList&, const bool, const qint64 elapsedTimeMilliSeconds = 0);
    static std::tuple<bool, int> getWatchId(lua_State*, Host&);
    static void pushMapLabelPropertiesToLua(lua_State*, const TMapLabel& label);
    static std::pair<int, TAction*> getTActionFromIdOrName(lua_State*, const int, const char*);
    static int loadMediaFileAsOrderedArguments(lua_State*);
    static int loadMediaFileAsTableArgument(lua_State*);
    static int playMusicFileAsOrderedArguments(lua_State*);
    static int playMusicFileAsTableArgument(lua_State*);
    static int playSoundFileAsOrderedArguments(lua_State*);
    static int playSoundFileAsTableArgument(lua_State*);
    static void processPlayingMediaTable(lua_State*, TMediaData&);
    static int getPlayingMusicAsOrderedArguments(lua_State*);
    static int getPlayingMusicAsTableArgument(lua_State*);
    static int getPlayingSoundsAsOrderedArguments(lua_State*);
    static int getPlayingSoundsAsTableArgument(lua_State*);
    static int stopMusicAsOrderedArguments(lua_State*);
    static int stopMusicAsTableArgument(lua_State*);
    static int stopSoundsAsOrderedArguments(lua_State*);
    static int stopSoundsAsTableArgument(lua_State*);
    static void parseCommandOrFunction(lua_State*, const char*, int&, QString&, int&);
    static void parseCommandsOrFunctionsTable(lua_State*, const char*, int&, QStringList&, QVector<int>&);
    static void parseHintsTable(lua_State*, const char*, int&, QStringList&);
    static QByteArray parseTelnetCodes(const QByteArray&);

    bool callReference(lua_State*, QString name, int parameters);
    void logError(std::string& e, const QString&, const QString& function);
    void logEventError(const QString& event, const QString& error);
    std::pair<bool, QString> validLuaCode(const QString &code);
    std::pair<bool, QString> validateLuaCodeParam(int index);
    QByteArray encodeBytes(const char*);
    void setMatches(lua_State*);
    void setupLanguageData();
    QString readScriptFile(const QString& path) const;
    void handleHttpOK(QNetworkReply*);
#if defined(Q_OS_WINDOWS)
    void loadUtf8Filenames();
#endif

    void insertColorTableEntry(lua_State*, const QColor&, const QString&);
    struct lua_state_deleter {
        void operator()(lua_State* ptr) const noexcept {
            lua_close(ptr);
        }
    };
    void updateEditor();


    bool loadLuaModule(QQueue<QString>& resultMsgQueue, const QString& requirement, const QString& failureConsequence = QString(), const QString& description = QString(), const QString& luaModuleId = QString());
    void insertNativeSeparatorsFunction(lua_State*);


    const int LUA_FUNCTION_MAX_ARGS = 50;
    std::list<std::string> mCaptureGroupList;
    std::list<int> mCaptureGroupPosList;
    std::list<std::list<std::string>> mMultiCaptureGroupList;
    std::list<std::list<int>> mMultiCaptureGroupPosList;
    QVector<QPair<QString, QString>> mCapturedNameGroups;
    QMap<QString, QPair<int, int>> mCapturedNameGroupsPosList;
    QVector<QVector<QPair<QString, QString>>> mMultiCaptureNameGroups;
    QMap<QNetworkReply*, QString> downloadMap;
    lua_State* pGlobalLua = nullptr;
    std::unique_ptr<lua_State, lua_state_deleter> pIndenterState;
    QPointer<Host> mpHost;
    QString hostName;
    int mHostID = 0;
    QList<QObject*> objectsToDelete;
    QTimer purgeTimer;
    QNetworkAccessManager* mpFileDownloader = nullptr;
    QFileSystemWatcher* mpFileSystemWatcher = nullptr;

    // Holds the list of places to look for the LuaGlobal.lua file:
    QStringList mPossiblePaths;
};

Host& getHostFromLua(lua_State*);

#endif // MUDLET_LUAINTERPRETER_H
