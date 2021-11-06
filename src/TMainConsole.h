#ifndef MUDLET_TMAINCONSOLE_H
#define MUDLET_TMAINCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2021 by Stephen Lyons                   *
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

#include "pre_guard.h"
#include <QFile>
#include <QTextStream>
#include <QWidget>
#include "post_guard.h"

#include <hunspell/hunspell.h>

#include <list>

class TMainConsole : public TConsole
{
    Q_OBJECT

public:
    explicit TMainConsole(Host*, QWidget* parent = nullptr);
    ~TMainConsole();

    void resizeEvent(QResizeEvent* event) override;

    void resetMainConsole();

    TConsole* createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height);
    bool raiseWindow(const QString& name);
    bool lowerWindow(const QString& name);
    bool showWindow(const QString& name);
    bool hideWindow(const QString& name);
    bool printWindow(const QString& name, const QString& text);
    void setProfileName(const QString&);
    void selectCurrentLine(std::string&);
    std::list<int> getFgColor(std::string& buf);
    std::list<int> getBgColor(std::string& buf);
    QPair<quint8, TChar> getTextAttributes(const QString&) const;
    void luaWrapLine(std::string& buf, int line);
    QString getCurrentLine(std::string&);
    TConsole* createBuffer(const QString& name);
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet);
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text);
    bool setTextFormat(const QString& name, const QColor& fgColor, const QColor& bgColor, const TChar::AttributeFlags& flags);
    TLabel* createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough = false);
    std::pair<bool, QString> createMapper(const QString &windowname, int, int, int, int);
    std::pair<bool, QString> createCommandLine(const QString &windowname, const QString &name, int, int, int, int);
    QSize getUserWindowSize(const QString& windowname) const;
    std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet);
    void setLabelStyleSheet(std::string& buf, std::string& sh);
    std::pair<bool, QString> deleteLabel(const QString&);
    std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration);
    std::pair<bool, QString> setLabelCursor(const QString& name, int shape);
    std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY);
    bool setBackgroundImage(const QString& name, const QString& path);
    bool setBackgroundColor(const QString& name, int r, int g, int b, int alpha);

    void setSystemSpellDictionary(const QString&);
    void setProfileSpellDictionary();

    void showStatistics();

    const QString& getSystemSpellDictionary() const { return mSpellDic; }
    QTextCodec* getHunspellCodec_system() const { return mpHunspellCodec_system; }
    Hunhandle* getHunspellHandle_system() const { return mpHunspell_system; }
    // Either returns the handle of the per profile or the shared Mudlet one or
    // nullptr depending on the state of the flags mEnableUserDictionary and
    // mUseSharedDictionary:
    Hunhandle* getHunspellHandle_user() const {
        return mEnableUserDictionary
                ? (mUseSharedDictionary
                   ? mpHunspell_shared
                   : mpHunspell_profile)
                : nullptr; }
    QSet<QString> getWordSet() const;
    QPair<bool, QString> addWordToSet(const QString&);
    QPair<bool, QString> removeWordFromSet(const QString&);
    bool isUsingSharedDictionary() const { return mUseSharedDictionary; }
    void toggleLogging(bool);
    void printOnDisplay(std::string&, bool isFromServer = false);
    void runTriggers(int);
    void finalize();
    bool saveMap(const QString&, int saveVersion = 0);
    bool loadMap(const QString&);
    bool importMap(const QString&, QString* errMsg = nullptr);


    QMap<QString, TConsole*> mSubConsoleMap;
    QMap<QString, TDockWidget*> mDockWidgetMap;
    QMap<QString, TCommandLine*> mSubCommandLineMap;
    QMap<QString, TLabel*> mLabelMap;
    TBuffer mClipboard;
    QFile mLogFile;
    QString mLogFileName;
    QTextStream mLogStream;
    bool mLogToLogFile;


public slots:
    // Used by mudlet class as told by "Profile Preferences"
    // =>"Copy Map" in another profile to inform a list of
    // profiles - asynchronously - to load in an updated map
    void slot_reloadMap(QList<QString>);


signals:
    // Raised when new data is incoming to trigger Alert handling in mudlet
    // class, second argument is true for a lower priority indication when
    // locally produced information is painted into main console
    void signal_newDataAlert(const QString&, bool isLowerPriorityChange = false);


private:
    // Was public in Host class but made private there and cloned to here
    // (for main TConsole) to prevent it being changed without going through the
    // process to load in the the changed dictionary:
    QString mSpellDic;

    // Cloned from Host
    bool mEnableUserDictionary;
    bool mUseSharedDictionary;

    // Three handles, one for the dictionary the user choses from the system
    // one created by the mudlet class for all profiles and the third for a per
    // profile one - the last pair are built by the user and/or lua functions:
    Hunhandle* mpHunspell_system;
    Hunhandle* mpHunspell_shared;
    Hunhandle* mpHunspell_profile;
    // The user dictionary will always use the UTF-8 codec, but the one
    // selected from the system's ones may not:
    QByteArray mHunspellCodecName_system;
    QTextCodec* mpHunspellCodec_system;
    // To update the profile dictionary we actually have to track all the words
    // in it so we loaded the contents into this on startup and adjust it as we
    // go. Then, at the end of a session we will put the revised contents
    // back into the user's ".dic" file and regenerate the needed pair of lines
    // for the ".aff" file - this member is for the per profile option only as
    // the shared one is held by the mudlet singleton class:
    QSet<QString> mWordSet_profile;
};

#endif // MUDLET_TMAINCONSOLE_H

