#ifndef MUDLET_HOST_H
#define MUDLET_HOST_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2020, 2022-2023, 2025 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
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


#include "ActionUnit.h"
#include "AliasUnit.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "GifTracker.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TimerUnit.h"
#include "TMainConsole.h"
#include "TriggerUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "enums.h"

#include "pre_guard.h"
#include <QColor>
#include <QFile>
#include <QFont>
#include <QList>
#include <QMargins>
#include <QPointer>
#include <QStack>
#include <QTextStream>
#include "post_guard.h"

#include "TMxpMudlet.h"
#include "TMxpProcessor.h"

class QDialog;
class QDockWidget;
class QPushButton;
class QListWidget;

class TEvent;
class TArea;
class LuaInterface;
class TMedia;
class GMCPAuthenticator;
class TRoom;
class TConsole;
class TMainConsole;
class dlgNotepad;
class TMap;
class dlgIRC;
class dlgPackageManager;
class dlgModuleManager;
class dlgProfilePreferences;
class cTelnet;

class stopWatch {
    friend class XMLimport;

public:
    stopWatch();

    bool start();
    bool stop();
    bool reset();
    bool running() const {return mIsRunning;}
    void adjustMilliSeconds(const qint64);
    qint64 getElapsedMilliSeconds() const;
    QString getElapsedDayTimeString() const;
    void setPersistent(const bool state) {mIsPersistent = state;}
    bool persistent() const {return mIsPersistent;}
    void setName(const QString& name) {mName = name;}
    const QString& name() const {return mName;}

#ifndef QT_NO_DEBUG_STREAM
    // Only used for debugging:
    bool initialised() const {return mIsInitialised;}
    qint64 elapsed() const {return mElapsedTime;}
    QDateTime effectiveStartDateTime() const {return mEffectiveStartDateTime;}
#endif // QT_NO_DEBUG_STREAM

private:
    // true once started the first time - but provides some code short cuts if
    // prior to that:
    bool mIsInitialised;
    // true when RUNNING, false when STOPPED:
    bool mIsRunning;
    // If true this stopwatch is saved with the profile and reloaded - if it is
    // running when saved it will seem to continue to run - so can be used to
    // track real time events outside of the profile, it is intended that the
    // parent Host class that makes use of it will save and restore the id
    // number that that class assigns the stopwatch:
    bool mIsPersistent;
    // When RUNNING this contains the effective point in time when the stop
    // watch was started - so that taking a difference between then and
    // "now" gives the total elapsed time:
    QDateTime mEffectiveStartDateTime;
    // When STOPPED this contains the cumulative elasped time in milliSeconds:
    qint64 mElapsedTime;
    // As the id is generated according to what other ones have been created but
    // persists between saves it is useful for the user or script writer to be
    // able to name a particular stop watch for identification later:
    QString mName;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const stopWatch& stopwatch)
{
    const QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << qsl("stopwatch(mIsRunning: %1 mInitialised: %2 mIsPersistent: %3 mEffectiveStartDateTime: %4 mElapsedTime: %5)")
                       .arg((stopwatch.running() ? QLatin1String("true") : QLatin1String("false")),
                            (stopwatch.initialised() ? QLatin1String("true") : QLatin1String("false")),
                            (stopwatch.persistent() ? QLatin1String("true") : QLatin1String("false")),
                            stopwatch.effectiveStartDateTime().toString(),
                            QString::number(stopwatch.elapsed()));
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

class Host : public QObject
{
    Q_OBJECT

    friend class XMLexport;
    friend class XMLimport;
    friend class dlgProfilePreferences;

public:
    Host(int port, const QString& mHostName, const QString& login, const QString& pass, int host_id);
    ~Host();

    enum DiscordOptionFlag {
        DiscordNoOption = 0x0,
        DiscordSetDetail = 0x01,
        DiscordSetState = 0x02,
        DiscordSetLargeIcon = 0x04,
        DiscordSetLargeIconText = 0x08,
        DiscordSetSmallIcon = 0x10,
        DiscordSetSmallIconText = 0x20,
        DiscordSetPartyInfo = 0x80,
        DiscordSetTimeInfo = 0x100,
        DiscordSetSubMask = 0x3ff,
        DiscordLuaAccessEnabled = 0x800
    };
    Q_DECLARE_FLAGS(DiscordOptionFlags, DiscordOptionFlag)




    QString         getName()                        { return mHostName; }
    QString         getCommandSeparator()            { return mCommandSeparator; }
    void            setName(const QString& name);
    QString         getUrl()                         { return mUrl; }
    void            setUrl(const QString& url)       { mUrl = url; }
    QString         getDiscordGameName()             { return mDiscordGameName; }
    void            setDiscordGameName(const QString& name) { mDiscordGameName = name; }
    int             getPort()                        { return mPort; }
    void            setPort(const int port)          { mPort = port; }
    void            setAutoReconnect(const bool reconnect) { mTelnet.setAutoReconnect(reconnect); }
    QString &       getLogin()                       { return mLogin; }
    void            setLogin(const QString& login)       { mLogin = login; }
    QString &       getPass()                        { return mPass; }
    void            setPass(const QString& password) { mPass = password; }
    bool            hasAutoLoginCredentials() const  { return !mLogin.isEmpty() && !mPass.isEmpty(); }
    int             getRetries()                     { return mRetries;}
    void            setRetries(const int retries)    { mRetries = retries; }
    int             getTimeout()                     { return mTimeout; }
    void            setTimeout(const int seconds)    { mTimeout = seconds; }
    bool            wideAmbiguousEAsianGlyphs() { return mWideAmbigousWidthGlyphs; }
    // Uses PartiallyChecked to set the automatic mode, otherwise Checked/Unchecked means use wide/narrow ambiguous glyphs
    void            setWideAmbiguousEAsianGlyphs(Qt::CheckState state);
    // Is used to set preference dialog control directly:
    Qt::CheckState  getWideAmbiguousEAsianGlyphsControlState() {
                           return mAutoAmbigousWidthGlyphsSetting
                                  ? Qt::PartiallyChecked
                                  : (mWideAmbigousWidthGlyphs ? Qt::Checked : Qt::Unchecked); }
    void            setHaveColorSpaceId(const bool state) { mSGRCodeHasColSpaceId = state; }
    bool            getHaveColorSpaceId() { return mSGRCodeHasColSpaceId; }
    void            setMayRedefineColors(const bool state) { mServerMayRedefineColors = state; }
    bool            getMayRedefineColors() { return mServerMayRedefineColors; }
    void            setDiscordApplicationID(const QString& s);
    const QString&  getDiscordApplicationID();
    void            setDiscordInviteURL(const QString& s);
    const QString&  getDiscordInviteURL() const { return mDiscordInviteURL; }
    void            setSpellDic(const QString&);
    QString         getSpellDic();
    void            setUserDictionaryOptions(const bool useDictionary, const bool useShared);
    void            getUserDictionaryOptions(bool& useDictionary, bool& useShared) {
                        useDictionary = mEnableUserDictionary;
                        useShared = mUseSharedDictionary; }
    void            setControlCharacterMode(const ControlCharacterMode mode);
    ControlCharacterMode  getControlCharacterMode() const { return mControlCharacter; }
    bool            getLargeAreaExitArrows() const { return mLargeAreaExitArrows; }
    void            setLargeAreaExitArrows(const bool);
    bool            getUseModern3DMapper() const { return experimentEnabled(qsl("experiment.3dmap.modernmapper")); }

    // Experiment system methods
    bool            experimentEnabled(const QString& experimentKey) const;
    std::pair<bool, QString> setExperimentEnabled(const QString& experimentKey, bool enabled);
    QString         getActiveExperimentInGroup(const QString& group) const;
    QStringList     getAllExperiments() const;
    QStringList     getValidExperiments() const;

    void            forceClose();
    bool            isClosingDown() const { return mIsClosingDown; }
    bool            isClosingForced() const { return mForcedClose; }
    bool            requestClose();

    unsigned int assemblePath();
    bool checkForMappingScript();
    bool checkForCustomSpeedwalk();

    TriggerUnit* getTriggerUnit() { return &mTriggerUnit; }
    TimerUnit*   getTimerUnit()   { return &mTimerUnit; }
    AliasUnit*   getAliasUnit()   { return &mAliasUnit; }
    ActionUnit*  getActionUnit()  { return &mActionUnit; }
    KeyUnit*     getKeyUnit()     { return &mKeyUnit; }
    ScriptUnit*  getScriptUnit()  { return &mScriptUnit; }
    GifTracker*  getGifTracker()  { return &mGifTracker; }

    void send(QString cmd, bool wantPrint = true, bool dontExpandAliases = false);

    int getHostID()
    {
        return mHostID;
    }

    void setHostID(int id)
    {
        mHostID = id;
    }

    TLuaInterpreter* getLuaInterpreter() { return &mLuaInterpreter; }
    LuaInterface* getLuaInterface() { return mLuaInterface.data(); }

    void incomingStreamProcessor(const QString& paragraph, int line);
    void postIrcMessage(const QString&, const QString&, const QString&);
    void enableTimer(const QString&);
    void disableTimer(const QString&);
    void enableTrigger(const QString&);
    void disableTrigger(const QString&);
    void enableKey(const QString&);
    void disableKey(const QString&);
    bool killTimer(const QString&);
    bool killTrigger(const QString&);

    QPair<int, QString> createStopWatch(const QString&);
    bool destroyStopWatch(const int);
    bool adjustStopWatch(const int, const qint64 milliSeconds);
    QList<int> getStopWatchIds() const;
    QPair<bool, double> getStopWatchTime(const int) const;
    QPair<bool, QString> getBrokenDownStopWatchTime(const int) const;
    bool makeStopWatchPersistent(const int, const bool);
    QPair<bool, QString> resetStopWatch(const int);
    QPair<bool, QString> resetStopWatch(const QString&);
    QPair<bool, QString> startStopWatch(const int);
    QPair<bool, QString> startStopWatch(const QString&);
    QPair<bool, QString> stopStopWatch(const int);
    QPair<bool, QString> stopStopWatch(const QString&);
    stopWatch* getStopWatch(const int id) const { return mStopWatchMap.value(id); }
    int findStopWatchId(const QString&) const;
    QPair<bool, QString> setStopWatchName(const int, const QString&);
    QPair<bool, QString> setStopWatchName(const QString&, const QString&);
    QPair<bool, QString> resetAndRestartStopWatch(const int);

    void startSpeedWalk();
    void startSpeedWalk(int sourceRoom, int targetRoom);
    void reloadModule(const QString& reloadModuleName, const QString& syncingFromHost = QString());
    std::pair<bool, QString> changeModuleSync(const QString& enableModuleName, const QLatin1String &value);
    std::pair<bool, QString> getModuleSync(const QString& moduleName);
    bool blockScripts() { return mBlockScriptCompile; }
    void refreshPackageFonts();

    void registerEventHandler(const QString&, TScript*);
    void registerAnonymousEventHandler(const QString& name, const QString& fun);
    void unregisterEventHandler(const QString&, TScript*);
    void raiseEvent(const TEvent& event);
    // This disables all the triggers/timers/keys in preparation to resetting
    // them - and sets a timer to do resetProfile_phase2() when it is safe to do
    // so. We need to do it this way because a lua script containing the call to
    // produce this action will be purged from the Lua system as part of the
    // reset - which causes nasty existential issues (and crashes) from deleting
    // a script as it is being interpreted!
    void resetProfile_phase1();
    // This actually does the bulk of the reset but must wait until the profile
    // is quiescent:
    void resetProfile_phase2();
    std::tuple<bool, QString, QString> saveProfile(const QString& saveLocation = QString(), const QString& saveName = QString(), bool syncModules = false);
    std::tuple<bool, QString, QString> saveProfileAs(const QString& fileName);
    void stopAllTriggers();
    void reenableAllTriggers();

    // get Search Engine
    QPair<QString, QString> getSearchEngine();

    void set_USE_IRE_DRIVER_BUGFIX(bool b)
    {
        mUSE_IRE_DRIVER_BUGFIX = b;
        mTelnet.set_USE_IRE_DRIVER_BUGFIX(b);
    }

    void updateDisplayDimensions();

    std::pair<bool, QString> installPackage(const QString& fileName, enums::PackageModuleType thing);
    bool uninstallPackage(const QString&, enums::PackageModuleType thing);
    bool removeDir(const QString&, const QString&);
    void readPackageConfig(const QString&, QString&, bool);
    QString getPackageConfig(const QString&, bool isModule = false);
    void postMessage(const QString message) { mTelnet.postMessage(message); }
    QColor getAnsiColor(const int ansiCode, const bool isBackground = false) const;
    QPair<bool, QString> writeProfileData(const QString&, const QString&);
    bool writeProfileIniData(const QString& item, const QString& what);
    QString readProfileData(const QString&);
    QString readProfileIniData(const QString& item);
    void xmlSaved(const QString& xmlName);
    bool currentlySavingProfile();
    void processDiscordGMCP(const QString& packageMessage, const QString& data);
    void waitForProfileSave();
    void clearDiscordData();
    void processDiscordMSDP(const QString& variable, QString value);
    bool discordUserIdMatch(const QString& userName, const QString& userDiscriminator) const;
    void setMmpMapLocation(const QString& data);
    QString getMmpMapLocation() const;
    void setMediaLocationGMCP(const QString& mediaUrl);
    QString mediaLocationGMCP() const;
    void setMediaLocationMSP(const QString& mediaUrl);
    QString mediaLocationMSP() const;
    // Use this rather than accessng the TMainConsole::font() as the latter
    // isn't always around during profile start-up:
    QFont getDisplayFont();
    QFont getAndClearTempDisplayFont();
    std::pair<bool, QString> setDisplayFont(const QFont&);
    void setDisplayFontFromString(const QString&);
    void setDisplayFontSize(int size);
    int getConsoleBufferSize() const { return mConsoleBufferSize; }
    void setConsoleBufferSize(int size) { mConsoleBufferSize = size; }
    bool getUseMaxConsoleBufferSize() const { return mUseMaxConsoleBufferSize; }
    void setUseMaxConsoleBufferSize(bool useMax) { mUseMaxConsoleBufferSize = useMax; }
    void updateProxySettings(QNetworkAccessManager* manager);
    std::unique_ptr<QNetworkProxy>& getConnectionProxy();
    void updateAnsi16ColorsInTable();
    // Store/retrieve all the settings in one call:
    void setPlayerRoomStyleDetails(const quint8 styleCode, const quint8 outerDiameter = 120, const quint8 innerDiameter = 70, const QColor& outerColor = QColor(), const QColor& innerColor = QColor());
    void getPlayerRoomStyleDetails(quint8& styleCode, quint8& outerDiameter, quint8& innerDiameter, QColor& outerColor, QColor& innerColor);
    void setSearchOptions(const dlgTriggerEditor::SearchOptions);
    void setBufferSearchOptions(const TConsole::SearchOptions);
    std::pair<bool, QString> setMapperTitle(const QString&);
    void setDebugShowAllProblemCodepoints(const bool);
    bool debugShowAllProblemCodepoints() const { return mDebugShowAllProblemCodepoints; }
    void setCompactInputLine(const bool state);
    bool getCompactInputLine() const { return mCompactInputLine; }
    QPointer<TConsole> findConsole(QString name);

    QPair<bool, QStringList> getLines(const QString& windowName, const int lineFrom, const int lineTo);
    std::pair<bool, QString> openWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area);
    std::pair<bool, QString> createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height);
    std::pair<bool, QString> createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height) const;
    std::pair<bool, QString> createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBg, bool clickthrough);
    bool setClickthrough(const QString& name, bool clickthrough);
    void hideMudletsVariables();
    bool createBuffer(const QString& name);
    QSize calcFontSize(const QString& windowName);
    bool clearWindow(const QString&);
    bool showWindow(const QString&);
    bool hideWindow(const QString&);
    bool resizeWindow(const QString&, int, int);
    bool moveWindow(const QString& name, int, int);
    std::pair<bool, QString> setWindow(const QString& windowname, const QString& name, int x1, int y1, bool show);
    std::pair<bool, QString> openMapWidget(const QString& area, int x, int y, int width, int height);
    std::pair<bool, QString> closeMapWidget();
    bool closeWindow(const QString&);
    bool echoWindow(const QString&, const QString&);
    bool pasteWindow(const QString& name);
    void loadPackageInfo();
    bool setCmdLineAction(const QString&, const int);
    bool resetCmdLineAction(const QString&);
    bool setLabelClickCallback(const QString&, const int);
    bool setLabelDoubleClickCallback(const QString&, const int);
    bool setLabelReleaseCallback(const QString&, const int);
    bool setLabelMoveCallback(const QString&, const int);
    bool setLabelWheelCallback(const QString&, const int);
    bool setLabelOnEnter(const QString&, const int);
    bool setLabelOnLeave(const QString&, const int);
    std::pair<bool, QString> setMovie(const QString& labelName, const QString& moviePath);
    bool setBackgroundColor(const QString& name, int r, int g, int b, int alpha);
    bool setCommandBackgroundColor(const QString& name, int r, int g, int b, int alpha);
    bool setCommandForegroundColor(const QString& name, int r, int g, int b, int alpha);
    std::optional<QColor> getBackgroundColor(const QString& name) const;
    bool setBackgroundImage(const QString& name, QString& path, int mode);
    bool resetBackgroundImage(const QString& name);
    void showHideOrCreateMapper(const bool loadDefaultMap);
    bool setProfileStyleSheet(const QString& styleSheet);
    void check_for_mappingscript();
    void setupIreDriverBugfix();

    void setDockLayoutUpdated(const QString&);
    void setToolbarLayoutUpdated(TToolBar*);
    bool commitLayoutUpdates(bool flush = false);
    void setScreenDimensions(const int width, const int height) { mScreenWidth = width; mScreenHeight = height; }
    std::optional<QString> windowType(const QString& name) const;
    bool getEditorShowBidi() const { return mEditorShowBidi; }
    void setEditorShowBidi(const bool);
    bool caretEnabled() const;
    void setCaretEnabled(bool enabled);
    void setFocusOnHostActiveCommandLine();
    void recordActiveCommandLine(TCommandLine*);
    void forgetCommandLine(TCommandLine*);
    QPointer<TConsole> parentTConsole(QObject*) const;
    QMargins borders() const { return mBorders; }
    void setBorders(const QMargins);
    void loadMap();
    std::tuple<QString, bool> getCmdLineSettings(const TCommandLine::CommandLineType, const QString&);
    void setCmdLineSettings(const TCommandLine::CommandLineType, const bool, const QString&);
    int getCommandLineHistorySaveSize() const { return mCommandLineHistorySaveSize; }
    void setCommandLineHistorySaveSize(const int lines);
    bool showIdsInEditor() const { return mShowIDsInEditor; }
    void setShowIdsInEditor(const bool isShown) { mShowIDsInEditor = isShown; if (mpEditorDialog) {mpEditorDialog->showIDLabels(isShown);} }
    bool getF3SearchEnabled() const { return mF3SearchEnabled; }
    void setF3SearchEnabled(const bool enabled) {
        mF3SearchEnabled = enabled;
        if (mpConsole) {
            mpConsole->setF3SearchEnabled(enabled);
        }
    }
    bool getForceMXPProcessorOn() const { return mForceMXPProcessorOn; }
    void setForceMXPProcessorOn(bool value) {
        if (mForceMXPProcessorOn != value) {
            mForceMXPProcessorOn = value;
            emit signal_forceMXPProcessorOnChanged(value);
        }
    }
    void sendCmdLine(const QString& cmd);
    bool fontsAntiAlias() const { return !mNoAntiAlias; }

private:
    bool mNoAntiAlias = false;
    // These are used only during profile initiation to provide faked details
    // for things looking to the main console font before it gets instantiated:
    std::optional<TFontAttributes> mTempDisplayFontAttributes;
    std::optional<QFont> mTempDisplayFont;

public:
    // Make this the first public member instantiated so we can use ITS font
    // as the "reference" or "master" font for whole profile - and so we don't
    // have to maintain a separate one here in this class which does not, as
    // something derived from a QOject, have one:
    QPointer<TMainConsole> mpConsole;
    cTelnet mTelnet;
    QPointer<dlgPackageManager> mpPackageManager;
    QPointer<dlgModuleManager> mpModuleManager;
    TLuaInterpreter mLuaInterpreter;

    bool mDisablePasswordMasking;
    int commandLineMinimumHeight = 30;
    bool mAlertOnNewData = true;
    bool mAllowToSendCommand = true;
    bool mAutoClearCommandLineAfterSend = false;
    bool mHighlightHistory = true;

    // Set in constructor and used in (bool) TScript::setScript(const QString&)
    // to prevent compilation of the script that was being set therein, cleared
    // after the main TConsole for a new profile has been created during the
    // period when mIsProfileLoadingSequence has been set:
    bool mBlockScriptCompile = true;
    bool mBlockStopWatchCreation = true;
    bool mEchoLuaErrors = false;
    QString mCommandSeparator = qsl(";;");
    bool mEnableGMCP = true;
    bool mEnableMSSP = true;
    bool mEnableMSDP = false;
    bool mEnableMSP = true;
    bool mEnableMTTS = true;
    bool mEnableMNES = false;
    bool mEnableMXP = true;
    bool mPromptedForMXPProcessorOn = false;
    bool mAskTlsAvailable = true;
    bool mPromptedForVersionInTTYPE = false;

    int mMSSPTlsPort = 0;
    QString mMSSPHostName;

    TMxpMudlet mMxpClient;
    TMxpProcessor mMxpProcessor;
    QString mMediaLocationGMCP;
    QString mMediaLocationMSP;
    QTextStream mErrorLogStream;
    QMap<QString, QList<TScript*>> mEventHandlerMap;
    bool mFORCE_GA_OFF = false;
    bool mFORCE_NO_COMPRESSION = false;
    bool mFORCE_SAVE_ON_EXIT = true;

    bool mSslTsl = false;
    bool mSslIgnoreExpired = false;
    bool mSslIgnoreSelfSigned = false;
    bool mSslIgnoreAll = false;

    bool mUseProxy = false;
    QString mProxyAddress;
    quint16 mProxyPort = 0;
    QString mProxyUsername;
    QString mProxyPassword;

    // Used to force the test compilation of the scripts for TActions ("Buttons")
    // that are pushdown buttons that run when they are "pushed down" during
    // loading even though the buttons start out with themselves NOT being
    // pushed down:
    bool mIsProfileLoadingSequence = false;


    dlgTriggerEditor* mpEditorDialog{nullptr};
    QScopedPointer<TMap> mpMap;
    QScopedPointer<TMedia> mpMedia;
    QScopedPointer<GMCPAuthenticator> mpAuth;
    dlgNotepad* mpNotePad{nullptr};

    // Controls how sent commands are displayed on the main TConsole:
    enum class CommandEchoMode {
        Never = 0,       // Never show sent commands regardless of script preferences
        ScriptControl = 1, // Let scripts control via send() wantPrint parameter (default)
        Always = 2       // Always show sent commands regardless of script preferences
    };
    CommandEchoMode mCommandEchoMode = CommandEchoMode::ScriptControl;

    /*
     * This is set when the Server is remote echoing what WE send to it,
     * it will have negotiated the ECHO suboption by sending a IAC WILL ECHO and
     * we have agreed to it with a IAC DO ECHO - in this state it is normal
     * Telnet behaviour to NOT echo what we send - to prevent doubled lines.
     * The rationale behind this is so that when we type passwords - WE do not
     * echo what we type and rely on the other end to, but they do not, so as to
     * hide them on our screen (and from logging!) - It should negate the effect
     * of the above mPrintCommand being true...
     */
    bool mIsRemoteEchoingActive = false;

    // Command echo mode getters and setters
    CommandEchoMode getCommandEchoMode() const { return mCommandEchoMode; }
    void setCommandEchoMode(CommandEchoMode mode) { mCommandEchoMode = mode; }
    
    // Backward compatibility methods - for existing code that expects boolean behavior
    bool getPrintCommand() const { return mCommandEchoMode != CommandEchoMode::Never; }
    void setPrintCommand(bool print) { 
        mCommandEchoMode = print ? CommandEchoMode::ScriptControl : CommandEchoMode::Never; 
    }

public:
    void setRemoteEchoingActive(bool active);
    bool isRemoteEchoingActive() const { return mIsRemoteEchoingActive; }

    // To cover the corner case of the user changing the mode
    // while a log is being written, this stores the mode of
    // the current log file and is set from
    // mIsNextLogFileInHtmlFormat at the point that a log is started.
    bool mIsCurrentLogFileInHtmlFormat = false;

    // To cover the corner case of the user changing the mode
    // while a log is being written, this stores the mode of
    // future logs file as set in the profile preferences. See
    // also mIsCurrentLogFileInHtmlFormat.
    bool mIsNextLogFileInHtmlFormat = false;

    bool mIsLoggingTimestamps = false;

    // Where to put HTML/text logfile (default is the "Logs" under the profile's
    // one):
    QString mLogDir;
    // A user entered name - if blank a language specific default is used:
    QString mLogFileName;
    // The first argument to QDateTime::toString(...) to generate a date/time
    // dependent filename unless it is empty in which case the above value is
    // used - the previously used value of "yyyy-MM-dd#hh-mm-ss" has been
    // changed to "yyyy-MM-dd#HH-mm-ss" as we always want a 24-hour clock.
    QString mLogFileNameFormat = QLatin1String("yyyy-MM-dd#HH-mm-ss");

    bool mResetProfile = false;
    int mScreenHeight = 25;
    int mScreenWidth = 90;

    // has the profile save data been loaded without issues?
    // if there were issues during loading, we should not save anything on close
    bool mLoadedOk = false;

    int mTimeout = 60;

    QString mUrl;

    QString mBackupHostName;
    int mBackupPort = 23;
    QString mBackupUrl;

    bool mUSE_FORCE_LF_AFTER_PROMPT = false;
    bool mUSE_IRE_DRIVER_BUGFIX = false;
    bool mUSE_UNIX_EOL = false;
    int mWrapAt = 100;
    int mWrapIndentCount = 0;
    int mWrapHangingIndentCount = 0;

    int mConsoleBufferSize = 100000;
    bool mUseMaxConsoleBufferSize = false;

    bool mEditorAutoComplete = true;

    // code editor theme (human-friendly name)
    QString mEditorTheme = QLatin1String("Mudlet");
    // code editor theme file on disk for edbee to load
    QString mEditorThemeFile = QLatin1String("Mudlet.tmTheme");
    void editorThemeChanged();

    // search engine URL prefix to search query
    QMap<QString, QString> mSearchEngineData;
    QMap<QString, QMap<QString, QString>> mPackageInfo;
    QMap<QString, QMap<QString, QString>> mModuleInfo;
    QString mSearchEngineName;

    // trigger/alias/script/etc ID whose Lua code to show when previewing a theme
    // remembering this value to show what the user has selected does have its
    // flaws in case of items getting created/deleted, but this is just a
    // convenience feature and if it gets the item wrong, it's no worse
    // than the feature not being there.
    int mThemePreviewItemID = -1;
    // the type of item (a trigger, an alias, etc) that's previewed
    QString mThemePreviewType;

    QColor mBlack{QColorConstants::Black};
    QColor mLightBlack{QColorConstants::DarkGray};
    QColor mRed{QColorConstants::DarkRed};
    QColor mLightRed{QColorConstants::Red};
    QColor mLightGreen{QColorConstants::Green};
    QColor mGreen{QColorConstants::DarkGreen};
    QColor mLightBlue{QColorConstants::Blue};
    QColor mBlue{QColorConstants::DarkBlue};
    QColor mLightYellow{QColorConstants::Yellow};
    QColor mYellow{QColorConstants::DarkYellow};
    QColor mLightCyan{QColorConstants::Cyan};
    QColor mCyan{QColorConstants::DarkCyan};
    QColor mLightMagenta{QColorConstants::Magenta};
    QColor mMagenta{QColorConstants::DarkMagenta};
    QColor mLightWhite{QColorConstants::White};
    QColor mWhite{QColorConstants::LightGray};
    QColor mFgColor{QColorConstants::LightGray};
    QColor mBgColor{QColorConstants::Black};
    QColor mCommandBgColor{QColorConstants::Black};
    QColor mCommandFgColor{QColor(113, 113, 0)};

    QColor mBlack_2{QColorConstants::Black};
    QColor mLightBlack_2{QColorConstants::DarkGray};
    QColor mRed_2{QColorConstants::DarkRed};
    QColor mLightRed_2{QColorConstants::Red};
    QColor mLightGreen_2{QColorConstants::Green};
    QColor mGreen_2{QColorConstants::DarkGreen};
    QColor mLightBlue_2{QColorConstants::Blue};
    QColor mBlue_2{QColorConstants::DarkBlue};
    QColor mLightYellow_2{QColorConstants::Yellow};
    QColor mYellow_2{QColorConstants::DarkYellow};
    QColor mLightCyan_2{QColorConstants::Cyan};
    QColor mCyan_2{QColorConstants::DarkCyan};
    QColor mLightMagenta_2{QColorConstants::Magenta};
    QColor mMagenta_2{QColorConstants::DarkMagenta};
    QColor mLightWhite_2{QColorConstants::White};
    QColor mWhite_2{QColorConstants::LightGray};
    QColor mFgColor_2{QColorConstants::LightGray};
    QColor mBgColor_2{QColorConstants::Black};
    QColor mLowerLevelColor{QColorConstants::DarkGray};
    QColor mUpperLevelColor{QColorConstants::White};
    QColor mRoomBorderColor{QColorConstants::LightGray};
    QColor mRoomCollisionBorderColor{QColorConstants::Yellow};

    QColor mMapInfoBg = QColor(150, 150, 150, 120);
    bool mMapStrongHighlight = false;
    QStringList mGMCP_merge_table_keys;
    bool mLogStatus = false;
    bool mTimeStampStatus = false;
    bool mEnableSpellCheck = true;
    QStringList mInstalledPackages;
    // module name = location on disk, sync to other profiles?, priority
    QMap<QString, QStringList> mInstalledModules;
    // module name = priority
    QMap<QString, int> mModulePriorities;
    // module name = location on disk, sync to other profiles?, priority
    QMap<QString, QStringList> modulesToWrite;
    // module name = {"helpURL" = custom link}
    QMap<QString, QMap<QString, QString>> moduleHelp;

    // Privacy option to allow the game to set Discord Rich Presence information
    bool mDiscordDisableServerSide = true;

    // Discord privacy options to give the user control over what data a Server
    // can set over OOB protocols (MSDP & GMCP) and the user via Lua API:
    DiscordOptionFlags mDiscordAccessFlags{DiscordLuaAccessEnabled | DiscordSetSubMask};

    double mLineSize = 10.0;
    double mRoomSize = 0.5;
    QSet<QString> mMapInfoContributors{qsl("Short")};
    bool mBubbleMode = false;
    bool mMapViewOnly = true;
    bool mShowRoomID = false;
    bool mShowPanel = true;
    bool mShow3DView = false;
    QString mServerGUI_Package_version = QLatin1String("-1");
    QString mServerGUI_Package_name = QLatin1String("nothing");
    bool mAcceptServerGUI = true;
    bool mAcceptServerMedia = true;
    QColor mCommandLineFgColor{Qt::darkGray};
    QColor mCommandLineBgColor{Qt::black};
    bool mMapperUseAntiAlias = true;
    bool mMapperShowRoomBorders = true;
    bool mFORCE_CHARSET_NEGOTIATION_OFF = false;
    bool mForceNewEnvironNegotiationOff = false;
    bool mVersionInTTYPE = false;
    QSet<QChar> mDoubleClickIgnore;
    QPointer<QDockWidget> mpDockableMapWidget;
    bool mEnableTextAnalyzer = false;
    bool mWritingHostAndModules = false;
    // Set from profile preferences, if the timer interval is less
    // than this then the normal reoccuring debug output of the entire command
    // and script for any timer with a timeout LESS than this is NOT shown
    // - this is so the spammy output from short timeout timers can be
    // suppressed.
    // An invalid/null value is treated as the "show all"/inactive case:
    QTime mTimerDebugOutputSuppressionInterval;
    std::unique_ptr<QNetworkProxy> mpConnectionProxy;
    QString mProfileStyleSheet;
    dlgTriggerEditor::SearchOptions mSearchOptions = dlgTriggerEditor::SearchOption::SearchOptionNone;
    TConsole::SearchOptions mBufferSearchOptions = TConsole::SearchOption::SearchOptionNone;
    QPointer<dlgIRC> mpDlgIRC;
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;
    QList<QString> mDockLayoutChanges;
    QList<QPointer<TToolBar>> mToolbarLayoutChanges;

    // string list: 0 - event name, 1 - display label, 2 - tooltip text
    QMap<QString, QStringList> mConsoleActions;

    QMap<QString, QKeySequence*> profileShortcuts;

    bool mTutorialForCompactLineAlreadyShown = false;

    bool mAnnounceIncomingText = true;
    bool mAdvertiseScreenReader = false;
    bool mEnableClosedCaption = false;

    enum class BlankLineBehaviour {
        Show,
        Hide,
        ReplaceWithSpace
    };
    Q_ENUM(BlankLineBehaviour)
    BlankLineBehaviour mBlankLineBehaviour = BlankLineBehaviour::Show;

    // shortcuts options visually impaired players have to switch between the input line and the main window
    enum class CaretShortcut {
        None,
        Tab,
        CtrlTab,
        F6
    };
    Q_ENUM(CaretShortcut)
    // shortcut to switch between the input line and the main window
    CaretShortcut mCaretShortcut = CaretShortcut::None;

    // stops all triggers/aliases/everything from running
    bool mEmergencyStop = false;

signals:
    // Tells TTextEdit instances for this profile how to draw the ambiguous
    // width characters:
    void signal_changeIsAmbigousWidthGlyphsToBeWide(bool);
    void profileSaveStarted();
    void profileSaveFinished();
    void signal_changeSpellDict(const QString&);
    // To tell all TConsole's upper TTextEdit panes to report all Codepoint
    // problems as they arrive as well as a summary upon destruction:
    void signal_changeDebugShowAllProblemCodepoints(const bool);
    // Tells all consoles associated with this Host (but NOT the Central Debug
    // one) to change the way they show  control characters:
    void signal_controlCharacterHandlingChanged(const ControlCharacterMode);
    // Tells all command lines to save their history:
    void signal_saveCommandLinesHistory();
    void signal_editorThemeChanged();
    void signal_remoteEchoChanged(bool enabled);
    void signal_forceMXPProcessorOnChanged(bool enabled);

private slots:
    void slot_purgeTemps();

private:
    void installPackageFonts(const QString &packageName);
    void processGMCPDiscordStatus(const QJsonObject& discordInfo);
    void processGMCPDiscordInfo(const QJsonObject& discordInfo);
    void loadSecuredPassword();
    void removeAllNonPersistentStopWatches();
    void updateConsolesFont();
    void thankForUsingPTB();
    void toggleMapperVisibility();
    void createMapper(const bool);
    void removePackageInfo(const QString &packageName, const bool);
    static void createModuleBackup(const QString &filename, const QString& saveName);
    void writeModule(const QString &moduleName, const QString &filename);
    void waitForAsyncXmlSave();
    void saveModules(bool backup = true);
    void updateModuleZips(const QString &zipName, const QString &moduleName);
    void reloadModules();
    void startMapAutosave(const int interval);
    void timerEvent(QTimerEvent *event) override;
    void autoSaveMap();
    QString sanitizePackageName(const QString packageName) const;
    TCommandLine* activeCommandLine();
    void closeChildren();
    void setupSandboxedLuaState(lua_State* L);

    QStringList mModulesToSync;
    QScopedPointer<LuaInterface> mLuaInterface;

    // Experiment system storage: key -> enabled state
    QMap<QString, bool> mExperiments;
    
    // Static whitelist of valid experiments
    static const QSet<QString> mValidExperiments;

    TriggerUnit mTriggerUnit;
    TimerUnit mTimerUnit;
    ScriptUnit mScriptUnit;
    AliasUnit mAliasUnit;
    ActionUnit mActionUnit;
    KeyUnit mKeyUnit;
    GifTracker mGifTracker;
    // ensures that only one saveProfile call is active when multiple modules are being uninstalled in one go
    std::optional<bool> mSaveTimer;

    QFile mErrorLogFile;

    QMap<QString, TEvent*> mEventMap;

    int mHostID;
    QString mHostName;
    QString mDiscordGameName; // Discord self-reported game name
    bool mIsClosingDown = false;

    QString mLine;
    QString mLogin;
    QString mPass;

    int mPort;

    int mRetries = 5;
    bool mSaveProfileOnExit = false;

    // To keep things simple for Lua the first stopwatch will be allocated a key
    // of 1 - and anything less that that will be rejected - and we force
    // createStopWatch() to return 0 during script loading so that we do not get
    // superious stopwatches from being created then (when
    // mIsProfileLoadingSequence is true):
    QMap<int, stopWatch*> mStopWatchMap;

    QMap<QString, QStringList> mAnonymousEventHandlerFunctions;

    QStringList mActiveModules;

    bool mHaveMapperScript = false;
    // This option makes the control on the preferences tristated so the value
    // used depends - currently - on what the MUD Server encoding is (only set
    // true for GBK, GB18030, Big5/Big-HKCS, EUC-KR ones) - however this was
    // due for revision once locale/language support is brought in - when it
    // could be made dependent on that instead.
    bool mAutoAmbigousWidthGlyphsSetting = true;
    // If above is true is the value deduced from the MUD server encoding, if
    // the above is false is the user's direct setting - this is so that changes
    // in the TTextEdit classes are only made when necessary:
    bool mWideAmbigousWidthGlyphs = false;

    // keeps track of all of the array writers we're currently operating with
    QHash<QString, std::shared_ptr<XMLexport>> writers;

    QFuture<void> mModuleFuture;

    // Will be null/empty if is to use Mudlet's default/own presence
    QString mDiscordApplicationID;

    // Will be null/empty if they have not set their own invite
    QString mDiscordInviteURL;

    // Will be null/empty if we are not concerned to check the use of Discord
    // Rich Presence against the local user currently logged into Discord -
    // these two will be checked against the values from the Discord instance
    // with which we are linked to by the RPC library - and if they do not match
    // we won't use Discord functions.
    QString mRequiredDiscordUserName;
    QString mRequiredDiscordUserDiscriminator;

    // Handles whether to treat 16M-Colour ANSI SGR codes which only use
    // semi-colons as separator have the initial Colour Space Id parameter
    // (true) or not (false):
    bool mSGRCodeHasColSpaceId = false;

    // Flag whether the Server can use ANSI OSC "P#RRGGBB\" to redefine the
    // 16 basic colors (and OSC "R\" to reset them).
    bool mServerMayRedefineColors = false;

    // Was public but hidden to prevent it being changed without going through
    // the process to signal to users that they need to change dictionaries:
    QString mSpellDic;
    // These are hidden to prevent them being changed directly, they are also
    // mirrored/cached in the main TConsole's instance so they do not need to be
    // looked up directly by that class:
    bool mEnableUserDictionary = true;
    bool mUseSharedDictionary = false;

    // These hold values that are needed in the TMap class which are saved with
    // the profile - but which cannot be kept there as that class is not
    // necessarily instantiated when the profile is read.
    // Base color(s) for the player room in the mappers:
    // Mode selected:
    // 0 is closest to original style with adjustable outer diameter
    // 1 is Fixed red color ring with adjustable outer/inner diameter
    // 2 is fixed blue/yellow colors ring with adjustable outer/inner diameter
    // 3 is adjustable outer(primary)/inner(secondary) colors ring with adjustable outer/inner diameter
    quint8 mPlayerRoomStyle = 0;
    QColor mPlayerRoomOuterColor{Qt::red};
    QColor mPlayerRoomInnerColor{Qt::white};
    // Percentage of the room size (actually width) for the outer diameter of
    // the circular marking, integer percentage clamped in the preferences
    // between 200 and 50:
    quint8 mPlayerRoomOuterDiameterPercentage = 120;
    // Percentage of the outer size for the inner diameter of the circular
    // marking, integer percentage clamped in the preferences between 83 and 0.
    // NOT USED FOR "Original" style marking (the 0'th one):
    quint8 mPlayerRoomInnerDiameterPercentage = 70;
    // Whether the TTextEditor class should immediately report to debug output
    // any dodgy codepoints that it has problems with - if false it only reports
    // each codepoint the first time it encounters itL
    bool mDebugShowAllProblemCodepoints = false;

    // Now a per profile option this one represents the state of this profile:
    bool mCompactInputLine = false;

    QTimer purgeTimer;

    // How to display (most) incoming control characters in TConsoles:
    // ControlCharacterMode::AsIs (0x0) = as is, no replacement
    // ControlCharacterMode::Picture (0x1) = as Unicode "Control
    //   Pictures" - use Unicode codepoints in range U+2400 to U+2421
    // ControlCharacterMode::OEM (0x2) = as "OEM Font"
    //   characters (most often seen as a part of CP437
    //   encoding), see the corresponding Wikipedia page, e.g.
    //   EN: https://en.wikipedia.org/wiki/Code_page_437
    //   DE: https://de.wikipedia.org/wiki/Codepage_437
    //   RU: https://ru.wikipedia.org/wiki/CP437
    ControlCharacterMode mControlCharacter = ControlCharacterMode::AsIs;

    bool mLargeAreaExitArrows = false;
    bool mEditorShowBidi = true;
    // should focus should be on the main window with the caret enabled?
    bool mCaretEnabled = false;

    // Tracks which command line was last used for this profile so that we can
    // return to it when switching between profiles:
    QStack<QPointer<TCommandLine>> mpLastCommandLineUsed;

    // ensures that only one "zero-time" timer is created by the lambda in
    // setFocusOnHostActiveCommandLine(), even when it is called multiple
    // times:
    bool mFocusTimerRunning = false;

    QMargins mBorders;

    // The range - applied to ALL command lines - is 0 to 10000, with the knob
    // on the profile preferences having a log-step action with multiples
    // of 10 to integer powers and steps of (0,) 10, 20, 50, 100. Prior to the
    // introduction of this feature the control would effectively have been
    // zero - and whilst the knob shows the special value of "None" then
    // to reproduce that behavior there is little reason to not enable it
    // by default:
    int mCommandLineHistorySaveSize = 500;

    // Whether to display each item's ID number in the editor:
    bool mShowIDsInEditor = false;

    // Whether F3 search functionality is enabled
    bool mF3SearchEnabled = false;

    // Whether to force the MXP processor to be on, even if not negotiated with the
    // MUD Server
    bool mForceMXPProcessorOn = false;

    // Set when the mudlet singleton demands that we close - used to force an
    // attempt to save the profile and map - without asking:
    bool mForcedClose = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Host::DiscordOptionFlags)

#endif // MUDLET_HOST_H
