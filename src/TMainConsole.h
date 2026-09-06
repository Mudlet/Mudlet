#ifndef MUDLET_TMAINCONSOLE_H
#define MUDLET_TMAINCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2022 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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


#include "TConsole.h"
#include <QFile>
#include <QPointer>
#include <QTextStream>
#include <QWidget>
#include <optional>
#include <utility>

#include <hunspell/hunspell.h>

#include <list>

class TMap;
class TMediaPlayer;
class TScrollBox;
class TTextBox;
class QDialog;
class QDockWidget;
class QProgressDialog;

class TMainConsole : public TConsole
{
    Q_OBJECT

public:
    explicit TMainConsole(Host*, QWidget* parent = nullptr);
    ~TMainConsole();

    void resizeEvent(QResizeEvent* event) override;
    void resetMainConsole();
    void closeEvent(QCloseEvent*) override;
    TConsole* createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height);
    bool createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height);
    bool raiseWindow(const QString& name);
    bool lowerWindow(const QString& name);
    bool showWindow(const QString& name);
    bool hideWindow(const QString& name);
    bool printWindow(const QString& name, const QString& text);
    bool clear(const QString& name);
    void setProfileName(const QString&) override;
    void selectCurrentLine(std::string&);
    std::list<int> getFgColor(QString& buf);
    std::list<int> getBgColor(QString& buf);
    QPair<quint8, TChar> getTextAttributes(const QString&) const;
    void luaWrapLine(QString& buf, int line);
    QString getCurrentLine(const std::string&);
    bool createBuffer(const QString& name);
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet);
    std::optional<QString> getUserWindowStyleSheet(const QString& name) const;
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text);
    std::pair<bool, QString> getUserWindowTitle(const QString& name) const;
    bool setTextFormat(const QString& name, const QColor& fgColor, const QColor& bgColor, const TChar::AttributeFlags& flags);
    bool createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough = false);
    std::pair<bool, QString> createMapper(const QString& windowname, int, int, int, int);
    std::pair<bool, QString> createCommandLine(const QString& windowname, const QString& name, int, int, int, int);
    void registerSubCommandLine(const QString& name, TCommandLine* pCommandLine);
    void deregisterSubCommandLine(TCommandLine* pCommandLine);
    std::pair<bool, QString> createTextBox(const QString& windowname, const QString& name, int, int, int, int);
    QSize getUserWindowSize(const QString& windowname) const;
    std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet);
    std::optional<QString> getCmdLineStyleSheet(const QString& name) const;
    std::pair<bool, QString> setLabelStyleSheet(const QString& name, const QString& stylesheet);
    std::optional<QString> getLabelStyleSheet(const QString& name) const;
    std::optional<QSize> getLabelSizeHint(const QString& name) const;
    std::pair<bool, QString> deleteLabel(const QString&);
    std::pair<bool, QString> deleteMiniConsole(const QString&);
    std::pair<bool, QString> deleteCommandLine(const QString&);
    std::pair<bool, QString> deleteTextBox(const QString&);
    std::pair<bool, QString> deleteScrollBox(const QString&);
    std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration);
    std::optional<QString> getLabelToolTip(const QString& name) const;
    std::pair<bool, QString> setLabelCursor(const QString& name, int shape);
    std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY);
    // The label operations Host forwards to this view by name, never by widget;
    // each resolves the name against the view's own map and reports failure for a
    // name that is not a label's.
    bool setLabelClickThrough(const QString& name, bool clickThrough);
    bool setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, bool underline);
    bool resetLabelLinkStyle(const QString& name);
    bool clearLabelVisitedLinks(const QString& name);
    bool showLabel(const QString& name);
    bool hideLabel(const QString& name);
    bool resizeLabel(const QString& name, int width, int height);
    bool moveLabel(const QString& name, int x, int y);
    bool reparentLabel(const QString& windowname, const QString& name, int x, int y, bool show);
    bool setLabelText(const QString& name, const QString& text);
    std::pair<bool, QString> setLabelMovie(const QString& name, const QString& moviePath);
    bool setLabelBackgroundColor(const QString& name, const QColor& color);
    std::optional<QColor> getLabelBackgroundColor(const QString& name) const;
    bool setLabelBackgroundImage(const QString& name, const QString& path);
    bool resetLabelBackgroundImage(const QString& name);
    std::optional<QRect> getLabelGeometry(const QString& name) const;
    std::optional<bool> getLabelVisible(const QString& name) const;
    // For callers that need the widget itself. An accessor rather than the open
    // map, so that inserting and removing entries stays in this class, which is
    // what keeps the window registry in step with it.
    TLabel* labelWidget(const QString& name) const { return mLabelMap.value(name); }
    // The view's half of the sub-console and user-window-dock bookkeeping. Every
    // insertion into and removal from the two maps goes through these four, so
    // that the Host's window registry cannot fall out of step with them.
    void registerSubConsole(const QString& name, TConsole* pConsole);
    TConsole* deregisterSubConsole(const QString& name);
    void registerDockWidget(const QString& name, TDockWidget* pDockWidget);
    TDockWidget* deregisterDockWidget(const QString& name);
    // For callers that need the widgets themselves.
    TConsole* subConsoleWidget(const QString& name) const { return mSubConsoleMap.value(name); }
    QString subConsoleName(TConsole* pConsole) const { return mSubConsoleMap.key(pConsole); }
    TDockWidget* dockWidget(const QString& name) const { return mDockWidgetMap.value(name); }
    QStringList dockWidgetNames() const { return QStringList(mDockWidgetMap.keys()); }
    // The sub-console operations Host forwards to this view by name, never by
    // widget. Each folds in whatever the name's dock needs, so that the core is
    // left with one branch per operation rather than a console-or-dock pair.
    void closeSubConsole(const QString& name);
    void changeSubConsoleColors(const QString& name);
    bool showSubConsole(const QString& name);
    bool hideSubConsole(const QString& name);
    bool resizeSubConsole(const QString& name, int width, int height);
    bool moveSubConsole(const QString& name, int x, int y);
    bool reparentWindow(const QString& windowname, const QString& name, int x, int y, bool show);
    bool pasteToSubConsole(const QString& name);
    std::optional<QSize> subConsoleFontSize(const QString& name) const;
    bool setSubConsoleBackgroundColor(const QString& name, const QColor& color);
    bool setSubConsoleBackgroundImage(const QString& name, const QString& path, int mode);
    bool resetSubConsoleBackgroundImage(const QString& name);
    bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color);
    bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color);
    std::optional<QRect> getSubConsoleGeometry(const QString& name) const;
    std::optional<bool> getSubConsoleVisible(const QString& name) const;
    void setDockWidgetStyleSheets(const QString& styleSheet);
    void setDockLayoutChanged(const QString& name);
    bool clearDockLayoutChanged(const QString& name);
    TCommandLine* subCommandLineWidget(const QString& name) const { return mSubCommandLineMap.value(name); }
    QList<TCommandLine*> subCommandLineWidgets() const { return mSubCommandLineMap.values(); }
    void setCommandLinePlaceholderText(const QString& text);
    void updateCommandLineSpellCheck(bool enabled);
    void setCommandLineText(const QString& text);
    TCommandLine* raiseCommandLine();
    TTextBox* textBoxWidget(const QString& name) const { return mTextBoxMap.value(name); }
    // One set of operations for scroll boxes, command lines and text boxes
    // together rather than one per kind: each is the same plain QWidget call
    // whichever of the three the name turns out to be.
    bool showPlainWindow(const QString& name);
    bool hidePlainWindow(const QString& name);
    bool resizePlainWindow(const QString& name, int width, int height);
    bool movePlainWindow(const QString& name, int x, int y);
    std::optional<QRect> getPlainWindowGeometry(const QString& name) const;
    std::optional<bool> getPlainWindowVisible(const QString& name) const;
    bool setCommandLineAction(const QString& name, const int func);
    bool resetCommandLineAction(const QString& name);
    void setSystemSpellDictionary(const QString&);
    void setProfileSpellDictionary();
    void showStatistics();
    void showPackageDownloadProgress(const QString& title, const QString& cancelText);
    void updatePackageDownloadProgress(qint64 got, qint64 total);
    void closePackageDownloadProgress();
    void showMapTransferProgress(const QString& title, const QString& label, const QString& cancelButtonText);
    void showMapJsonProgress(const QString& title, const QString& label, const QString& cancelButtonText, int maximum);
    void setMapProgressDialogLabel(const QString& text);
    void setMapProgressDialogRange(int minimum, int maximum);
    void setMapProgressDialogValue(int value);
    void disableMapProgressDialogCancel();
    void closeMapProgressDialog();
    void createMapperDock(const QString& title, const QString& objectName);
    dlgMapper* createDockedMapper(TMap* pMap, const QString& styleSheet);
    dlgMapper* dockedMapper() const;
    void showMapWidget();
    void dockMapWidget(Qt::DockWidgetArea area);
    std::pair<bool, QString> placeMapWidget(const QString& area, int x, int y, int width, int height);
    // The map dock answered as values, so that the core is left holding the
    // state of the map window rather than the widget showing it. Having made a
    // dock is not the same as having one on screen, which is what the four
    // after it answer for.
    bool mapWidgetCreated() const;
    bool setMapWidgetTitle(const QString& title);
    std::optional<QString> mapWidgetTitle() const;
    std::optional<QRect> mapWidgetGeometry() const;
    bool hideMapWidget();
    void showMapperScriptReminder();
    void showUnpackingProgress(const QString& message, const QString& title);
    void closeUnpackingProgress();
    void setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded);
    void hideVideoOutput(TMediaPlayer* player);
    const QByteArray& getHunspellCodecName_system();
    Hunhandle* getHunspellHandle_system();
    // Either returns the handle of the per profile or the shared Mudlet one or
    // nullptr depending on the state of the flags mEnableUserDictionary and
    // mUseSharedDictionary:
    Hunhandle* getHunspellHandle_user() const { return mEnableUserDictionary ? (mUseSharedDictionary ? mpHunspell_shared : mpHunspell_profile) : nullptr; }
    QSet<QString> getWordSet() const;
    QPair<bool, QString> addWordToSet(const QString&);
    QPair<bool, QString> removeWordFromSet(const QString&);
    bool isUsingSharedDictionary() const { return mUseSharedDictionary; }
    void toggleLogging(bool);
    void printOnDisplay(std::string&, bool isFromServer = false);
    void finalize();
    bool saveMap(const QString&, int saveVersion = 0);
    bool loadMap(const QString&);
    bool importMap(const QString&, QString* errMsg = nullptr);
    void refreshSubconsoles();


    mutable QMap<QString, QSize> mCachedWindowSizes;
    TBuffer mClipboard;
    // The log lifecycle lives in the core console model so a profile with no
    // view can run one; these four are references aliasing the model's fields,
    // the way buffer and mFgColor alias theirs. mLogToLogFile and mLogFileName
    // are what keep TLuaInterpreter::startLogging() compiling unchanged; the
    // other two are kept so the set does not have to be reasoned about in
    // halves.
    QFile& mLogFile;
    QString& mLogFileName;
    QTextStream& mLogStream;
    bool& mLogToLogFile;
    QPointer<QProgressDialog> mpPackageDownloadProgressDialog;
    QPointer<QProgressDialog> mpMapProgressDialog;
    // Outlives Host::closeMapWidget(), which only hides it, so this being
    // non-null says the profile has made a map widget at some point, not that it
    // has one on screen - see mapWidget() for the latter.
    QPointer<QDockWidget> mpDockableMapWidget;
    QPointer<QDialog> mpUnpackingDialog;


public slots:
    // Used by mudlet class as told by "Profile Preferences"
    // =>"Copy Map" in another profile to inform a list of
    // profiles - asynchronously - to load in an updated map
    void slot_reloadMap(QList<QString>);


private slots:
    // The two halves of a logging change that need this view: the core model
    // owns everything else about it.
    void slot_loggingAnnouncement(const bool isLogging, const QString& logFileName);
    void slot_loggingStateChanged(const bool isLogging);
    void slot_warmSystemSpellDictionary();


signals:
    // Raised when new data is incoming to trigger Alert handling in mudlet
    // class, second argument is true for a lower priority indication when
    // locally produced information is painted into main console
    void signal_newDataAlert(const QString&, bool isLowerPriorityChange = false);


private:
    void createMapProgressDialog(const QString& title, const QString& label, const QString& cancelButtonText, int minimum, int maximum);
    void loadSystemSpellDictionary();
    // Where reparentLabel() and reparentWindow() parent an element named as a
    // setWindow() destination, shared so the two cannot disagree about what
    // "main" means.
    QWidget* parentWidgetFor(const QString& windowname) const;
    // Where the three by-name-alone kinds are resolved to a widget, in the one
    // order the core resolves a name that is more than one of them in.
    QWidget* plainWindowWidget(const QString& name) const;
    // The single answer to "does this profile have a map widget on screen right
    // now" - null both for a profile that has never opened one and for one that
    // put it away again, which a script cannot tell apart and does not need to.
    //
    // isHidden() rather than a flag of our own, because the dock gets hidden by
    // paths that would never think to update one: its own title bar close
    // button, mudlet::slot_showMapperDialog() handing the map over to a main
    // window dock, and QMainWindow::restoreState() replaying a saved layout. It
    // is also not !isVisible(), which would additionally answer "no map widget"
    // whenever the main window itself is hidden, e.g. minimised to the system
    // tray.
    QDockWidget* mapWidget() const;
    void registerLabelWidget(const QString& name, TLabel* pLabel);
    void deregisterLabelWidget(TLabel* pLabel);

    // The view's half of the scroll box and text box bookkeeping, paired with
    // registerSubCommandLine()/deregisterSubCommandLine(). Every insertion into
    // and removal from the three maps goes through these, so that the Host's
    // window registry cannot fall out of step with them.
    void registerScrollBox(const QString& name, TScrollBox* pScrollBox);
    void deregisterScrollBox(TScrollBox* pScrollBox);
    void registerTextBox(const QString& name, TTextBox* pTextBox);
    void deregisterTextBox(TTextBox* pTextBox);

    // The view's half of the named-window bookkeeping; the core's half is the
    // Host's window registry, which this class registers into and deregisters
    // from wherever it adds to or takes from these maps. Private so that pairing
    // cannot be broken from outside - reach a widget through labelWidget(),
    // subConsoleWidget(), dockWidget(), subCommandLineWidget() or
    // textBoxWidget().
    QMap<QString, TLabel*> mLabelMap;
    // QPointer values because a sub-console can die as a Qt child without this
    // class hearing about it: a miniconsole created into a user window belongs
    // to that window's dock, and deleting the window takes it along. The entry
    // then reads back as null instead of as freed memory, which turns every
    // lookup on the dead name into a miss and lets the name be used again.
    QMap<QString, QPointer<TConsole>> mSubConsoleMap;
    QMap<QString, TDockWidget*> mDockWidgetMap;
    QMap<QString, TCommandLine*> mSubCommandLineMap;
    QMap<QString, TTextBox*> mTextBoxMap;
    QMap<QString, TScrollBox*> mScrollBoxMap;

    // Names the dictionary mpHunspell_system is built for. The build is put off
    // until the load has finished, so the profile load never reads the whole
    // dictionary. Host's mSpellDic is the profile's setting; this is only ever
    // what has been requested from it.
    QString mSystemDictionary;

    // Cloned from Host
    bool mEnableUserDictionary = true;
    bool mUseSharedDictionary = false;

    // Three handles, one for the dictionary the user choses from the system
    // one created by the mudlet class for all profiles and the third for a per
    // profile one - the last pair are built by the user and/or lua functions:
    Hunhandle* mpHunspell_system = nullptr;
    Hunhandle* mpHunspell_shared = nullptr;
    Hunhandle* mpHunspell_profile = nullptr;
    // The user dictionary will always use the UTF-8 codec, but the one
    // selected from the system's ones may not:
    QByteArray mHunspellCodecName_system;
    // To update the profile dictionary we actually have to track all the words
    // in it so we loaded the contents into this on startup and adjust it as we
    // go. Then, at the end of a session we will put the revised contents
    // back into the user's ".dic" file and regenerate the needed pair of lines
    // for the ".aff" file - this member is for the per profile option only as
    // the shared one is held by the mudlet singleton class:
    QSet<QString> mWordSet_profile;
    bool mEnableClose = false;
};

#endif // MUDLET_TMAINCONSOLE_H
