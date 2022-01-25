/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2022 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TConsole.h"


#include "Host.h"
#include "TCommandLine.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TSplitter.h"
#include "TTextEdit.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBoundaryFinder>
#include <QTextCodec>
#include <QTextStream>
#include <QPainter>
#include "post_guard.h"


TMainConsole::TMainConsole(Host* pH, QWidget* parent)
: TConsole(pH, TConsole::MainConsole, parent)
, mClipboard(pH)
{
    // During first use where mIsDebugConsole IS true mudlet::self() is null
    // then - but we rely on that flag to avoid having to also test for a
    // non-null mudlet::self() - the connect(...) will produce a debug
    // message and not make THAT connection should it indeed be null but it
    // is not fatal...
    connect(mudlet::self(), &mudlet::signal_profileMapReloadRequested, this, &TMainConsole::slot_reloadMap, Qt::UniqueConnection);
    connect(this, &TMainConsole::signal_newDataAlert, mudlet::self(), &mudlet::slot_newDataOnHost, Qt::UniqueConnection);

    // Load up the spelling dictionary from the system:
    setSystemSpellDictionary(mpHost->getSpellDic());

    // Load up the spelling dictionary for the profile - needs to handle the
    // absence of files for the first run in a new profile or from an older
    // Mudlet version:
    setProfileSpellDictionary();

    // Ensure the QWidget has the profile name embedded into it
    setProperty("HostName", pH->getName());
}

TMainConsole::~TMainConsole()
{
    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
        mpHunspell_system = nullptr;
    }
    if (mpHunspell_profile) {
        Hunspell_destroy(mpHunspell_profile);
        mpHunspell_profile = nullptr;
        // Need to commit any changes to personal dictionary
        qDebug() << "TCommandLine::~TConsole(...) INFO - Saving profile's own Hunspell dictionary...";
        mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mProfileName, qsl("profile")), mWordSet_profile);
    }
}

void TMainConsole::setLabelStyleSheet(std::string& buf, std::string& sh)
{
    QString key{buf.c_str()};
    QString sheet{sh.c_str()};
    if (mLabelMap.find(key) != mLabelMap.end()) {
        QLabel* pC = mLabelMap[key];
        if (!pC) {
            return;
        }
        pC->setStyleSheet(sheet);
        return;
    }
}

std::optional<QString> TMainConsole::getLabelStyleSheet(const QString& name) const
{
    QMap<QString, TLabel*>::const_iterator it = mLabelMap.constFind(name);
    if (it != mLabelMap.cend() && it.key() == name) {
        return it.value()->styleSheet();
    }

    return {};
}

std::optional<QSize> TMainConsole::getLabelSizeHint(const QString& name) const
{
    QMap<QString, TLabel*>::const_iterator it = mLabelMap.constFind(name);
    if (it != mLabelMap.cend() && it.key() == name) {
        return it.value()->sizeHint();
    }

    return {};
}

// NOLINTNEXTLINE(readability-make-member-function-const)
std::pair<bool, QString> TMainConsole::setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a userwindow cannot have an empty string as its name")};
    }

    auto pW = mDockWidgetMap.value(name);
    if (pW) {
        pW->setStyleSheet(userWindowStyleSheet);
        return {true, QString()};
    }
    return {false, qsl("userwindow name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::setCmdLineStyleSheet(const QString& name, const QString& styleSheet)
{
    if (name.isEmpty() || !name.compare(qsl("main"))) {
        mpHost->mpConsole->mpCommandLine->setStyleSheet(styleSheet);
        return {true, QString()};
    }

    auto pN = mSubCommandLineMap.value(name);
    if (pN) {
        pN->setStyleSheet(styleSheet);
        return {true, QString()};
    }
    return {false, qsl("command-line name '%1' not found").arg(name)};
}

void TMainConsole::toggleLogging(bool isMessageEnabled)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), qsl("autolog")));
    QDateTime logDateTime = QDateTime::currentDateTime();
    if (!mLogToLogFile) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        file.close();

        QString directoryLogFile;
        QString logFileName;
        // If no log directory is set, default to Mudlet's replay and log files path
        if (mpHost->mLogDir == nullptr || mpHost->mLogDir.isEmpty()) {
            directoryLogFile = mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, mProfileName);
        } else {
            directoryLogFile = mpHost->mLogDir;
        }
        // The format being empty is a signal value that means use a specified
        // name:
        if (mpHost->mLogFileNameFormat.isEmpty()) {
            if (mpHost->mLogFileName.isEmpty()) {
                // If no log name is set, use the default placeholder
                logFileName = tr("logfile", "Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).");
            } else {
                // Otherwise a specific name as one is given
                logFileName = mpHost->mLogFileName;
            }
        } else {
            logFileName = logDateTime.toString(mpHost->mLogFileNameFormat);
        }

        // The preset file name formats are derived from date/times so that
        // alphabetical filename and date sort order are the same...
        QDir dirLogFile;
        if (!dirLogFile.exists(directoryLogFile)) {
            dirLogFile.mkpath(directoryLogFile);
        }

        mpHost->mIsCurrentLogFileInHtmlFormat = mpHost->mIsNextLogFileInHtmlFormat;
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogFileName = qsl("%1/%2.html").arg(directoryLogFile, logFileName);
        } else {
            mLogFileName = qsl("%1/%2.txt").arg(directoryLogFile, logFileName);
        }
        mLogFile.setFileName(mLogFileName);
        // We do not want to use WriteOnly here:
        // Append = "The device is opened in append mode so that all data is
        // written to the end of the file."
        // WriteOnly = "The device is open for writing. Note that this mode
        // implies Truncate."
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogFile.open(QIODevice::ReadWrite);
        } else {
            mLogFile.open(QIODevice::Append);
        }
        mLogStream.setDevice(&mLogFile);
        // We have to set a codec here to convert the QString based QTextStream
        // encoding (from UTF-16) to UTF-8 - by default a local 8-Bit one would
        // be used, which is problematic on Windows for non-ASCII (or Latin1?)
        // characters:
        QTextCodec* pLogCodec = QTextCodec::codecForName("UTF-8");
        mLogStream.setCodec(pLogCodec);
        if (isMessageEnabled) {
            QString message = tr("Logging has started. Log file is %1\n").arg(mLogFile.fileName());
            printSystemMessage(message);
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done BEFORE logging starts - or actually mLogToLogFile gets set!
        }
        mLogToLogFile = true;
    } else {
        file.remove();
        mLogToLogFile = false;
        if (isMessageEnabled) {
            QString message = tr("Logging has been stopped. Log file is %1\n").arg(mLogFile.fileName());
            printSystemMessage(message);
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done AFTER logging ends - or actually mLogToLogFile gets reset!
        }
    }

    if (mLogToLogFile) {
        // Logging is being turned on
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            QString log;
            QTextStream logStream(&log);
            // No setting a QTextCodec here, they don't work on QString based QTextStreams
            QStringList fontsList;                  // List of fonts to become the font-family entry for
                                                    // the master css in the header
            fontsList << this->fontInfo().family(); // Seems to be the best way to get the
                                                    // font in use, as different TConsole
                                                    // instances within the same profile
                                                    // might have different fonts
            fontsList << qsl("Courier New");
            fontsList << qsl("Monospace");
            fontsList << qsl("Courier");
            fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

            logStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
            logStream << "<html>\n";
            logStream << " <head>\n";
            logStream << "  <meta http-equiv='content-type' content='text/html; charset=utf-8'>";
            // put the charset as early as possible as the parser MUST restart when it
            // switches away from the ASCII default
            logStream << "  <meta name='generator' content='" << tr("Mudlet MUD Client version: %1%2").arg(APP_VERSION, APP_BUILD) << "'>\n";
            // Nice to identify what made the file!
            logStream << "  <title>" << tr("Mudlet, log from %1 profile").arg(mProfileName) << "</title>\n";
            // Web-page title
            logStream << "  <style type='text/css'>\n";
            logStream << "   <!-- body { font-family: '" << fontsList.join("', '") << "'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb("
                      << mpHost->mFgColor.red() << "," << mpHost->mFgColor.green() << "," << mpHost->mFgColor.blue()
                      << "); background-color:rgb("
                      << mpHost->mBgColor.red() << "," << mpHost->mBgColor.green() << "," << mpHost->mBgColor.blue() << ");}\n";
            logStream << "        span { white-space: pre-wrap; } -->\n";
            logStream << "  </style>\n";
            logStream << "  </head>\n";
            bool isAtBody = false;
            bool foundBody = false;
            while (!mLogStream.atEnd()) {
                QString line = mLogStream.readLine();
                if (line.contains("<body><div>")) {
                    // Begin writing old log to the current log when the body is
                    // found.
                    isAtBody = true;
                    foundBody = true;
                } else if (line.contains("</div></body>")) {
                    // Stop writing to current log once the end of the old log's
                    // <body> is reached.
                    isAtBody = false;
                }

                if (isAtBody) {
                    logStream << line << "\n";
                }
            }
            if (!foundBody) {
                logStream << "  <body><div>\n";
            } else {
                // Put a horizontal line between separate log sessions
                logStream << "  </div><hr><div>\n";
            }
            logStream << qsl("<p>%1</p>\n")
                         .arg(logDateTime.toString(tr("'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                                      "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale")));
            // <div></div> tags required around outside of the body <span></spans> for
            // strict HTML 4 as we do not use <p></p>s or anything else

            if (!mLogFile.resize(0)) {
                qWarning() << "TConsole::toggleLogging(...) ERROR - Failed to resize HTML Logfile - it may now be corrupted...!";
            }
            mLogStream << log;
            mLogFile.flush();
        } else {
            // File is NOT an HTML one but pure text:
            // Put a horizontal line between separate log sessions
            // Unfortunately QLatin1String does not have a repeated() method,
            // but it does mean we can use non-ASCII/Latin1 characters:
            // Using 10x U+23AF Horizontal line extension from "Box drawing characters":
            if (mLogFile.size() > 5) {
                // Allow a few junk characters ("BOM"???) at the very start of
                // file to not trigger the insertion of this line:
                mLogStream << qsl("⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯").repeated(8).append(QChar::LineFeed);
            }
            mLogStream << qsl("%1\n")
                         .arg(logDateTime.toString(tr("'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                                      "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale")));

        }
        logButton->setToolTip(utils::richText(tr("Stop logging game output to log file.")));
    } else {
        // Logging is being turned off
        buffer.logRemainingOutput();
        QString endDateTimeLine = logDateTime.toString(tr("'Log session ending at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                             "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale"));
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogStream << qsl("<p>%1</p>\n").arg(endDateTimeLine);
            mLogStream << "  </div></body>\n";
            mLogStream << "</html>\n";
        } else {
            // File is NOT an HTML one but pure text:
            mLogStream << endDateTimeLine << "\n";
        }
        mLogFile.flush();
        mLogFile.close();
        logButton->setToolTip(utils::richText(tr("Start logging game output to log file.")));
    }
}

void TMainConsole::selectCurrentLine(std::string& buf)
{
    QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        TConsole::selectCurrentLine();
        return;
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        pC->selectCurrentLine();
    }
}

std::list<int> TMainConsole::getFgColor(std::string& buf)
{
    QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        return TConsole::getFgColor();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->getFgColor();
    }

    return {};
}

std::list<int> TMainConsole::getBgColor(std::string& buf)
{
    QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        return TConsole::getBgColor();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->getBgColor();
    }

    return {};
}

QPair<quint8, TChar> TMainConsole::getTextAttributes(const QString& name) const
{
    if (name.isEmpty() || name == QLatin1String("main")) {
        return TConsole::getTextAttributes();
    }

    auto pC = mSubConsoleMap.value(name);
    if (pC) {
        return pC->getTextAttributes();
    }

    return qMakePair(1, TChar());
}

void TMainConsole::luaWrapLine(std::string& buf, int line)
{
    QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        TConsole::luaWrapLine(line);
        return;
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        pC->luaWrapLine(line);
    }
}

QString TMainConsole::getCurrentLine(std::string& buf)
{
    QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        return TConsole::getCurrentLine();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->getCurrentLine();
    }
    return qsl("ERROR: mini console does not exist");
}


TConsole* TMainConsole::createBuffer(const QString& name)
{
    if (!mSubConsoleMap.contains(name)) {
        auto pC = new TConsole(mpHost, Buffer);
        mSubConsoleMap[name] = pC;
        pC->mConsoleName = name;
        pC->setContentsMargins(0, 0, 0, 0);
        pC->hide();
        pC->layerCommandLine->hide();
        return pC;
    } else {
        return nullptr;
    }
}

void TMainConsole::resetMainConsole()
{
    //resetProfile should reset also UserWindows
    QMutableMapIterator<QString, TDockWidget*> itDockWidget(mDockWidgetMap);
    while (itDockWidget.hasNext()) {
        itDockWidget.next();
        itDockWidget.value()->close();
        itDockWidget.remove();
    }

    QMutableMapIterator<QString, TCommandLine*> itCommandLine(mSubCommandLineMap);
    while (itCommandLine.hasNext()) {
        itCommandLine.next();
        itCommandLine.value()->deleteLater();
        itCommandLine.remove();
    }

    QMutableMapIterator<QString, TConsole*> itSubConsole(mSubConsoleMap);
    while (itSubConsole.hasNext()) {
        itSubConsole.next();
        // CHECK: Do we need to handle the float/dockable widgets here:
        itSubConsole.value()->close();
        itSubConsole.remove();
    }

    QMutableMapIterator<QString, TLabel*> itLabel(mLabelMap);
    while (itLabel.hasNext()) {
        itLabel.next();
        itLabel.value()->close();
        itLabel.remove();
    }

    QMutableMapIterator<QString, TScrollBox*> itScrollBox(mScrollBoxMap);
    while (itScrollBox.hasNext()) {
        itScrollBox.next();
        itScrollBox.value()->close();
        itScrollBox.remove();
    }
}

// This is a sub-console overlaid on to the main console
TConsole* TMainConsole::createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    //if pW then add Console as Overlay to the Userwindow
    auto pW = mDockWidgetMap.value(windowname);
    auto pC = mSubConsoleMap.value(name);
    auto pS = mScrollBoxMap.value(windowname);
    if (!pC) {
        if (pS) {
            pC = new TConsole(mpHost, SubConsole, pS->widget());
        } else if (pW) {
            pC = new TConsole(mpHost, SubConsole, pW->widget());
        } else {
            pC = new TConsole(mpHost, SubConsole, mpMainFrame);
        }
        if (!pC) {
            return nullptr;
        }
        mSubConsoleMap[name] = pC;
        pC->setObjectName(name);
        pC->mConsoleName = name;
        pC->setFocusPolicy(Qt::NoFocus);
        const auto& hostCommandLine = mpHost->mpConsole->mpCommandLine;
        pC->setFocusProxy(hostCommandLine);
        pC->mUpperPane->setFocusProxy(hostCommandLine);
        pC->mLowerPane->setFocusProxy(hostCommandLine);
        pC->resize(width, height);
        pC->mOldX = x;
        pC->mOldY = y;
        pC->setContentsMargins(0, 0, 0, 0);
        pC->move(x, y);

        pC->setFontSize(12);
        pC->show();

        return pC;
    } else {
        return nullptr;
    }
}

// This is a scrollBox overlaid on to the main console
TScrollBox* TMainConsole::createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    //if pW then add ScrollBox as Overlay to the Userwindow
    auto pW = mDockWidgetMap.value(windowname);
    auto pSW = mScrollBoxMap.value(windowname);
    auto pS = mScrollBoxMap.value(name);
    if (!pS) {
        if (pW) {
            pS = new TScrollBox(mpHost, pW->widget());
        } else if (pSW) {
            pS = new TScrollBox(mpHost, pSW->widget());
        } else {
            pS = new TScrollBox(mpHost, mpMainFrame);
        }
        mScrollBoxMap[name] = pS;
        pS->setObjectName(name);
        pS->setFocusPolicy(Qt::NoFocus);
        pS->resize(width, height);
        pS->setContentsMargins(0, 0, 0, 0);
        pS->move(x, y);
        pS->show();

        return pS;
    }
    return nullptr;
}

TLabel* TMainConsole::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough)
{
    //if pW put Label in Userwindow
    auto pL = mLabelMap.value(name);
    auto pW = mDockWidgetMap.value(windowname);
    auto pS = mScrollBoxMap.value(windowname);
    if (!pL) {
        if (pW) {
            pL = new TLabel(mpHost, pW->widget());
        } else if (pS) {
            pL = new TLabel(mpHost, pS->widget());
        } else {
            pL = new TLabel(mpHost, mpMainFrame);
        }
        mLabelMap[name] = pL;
        pL->setObjectName(name);
        pL->setAutoFillBackground(fillBackground);
        pL->setClickThrough(clickThrough);
        pL->resize(width, height);
        pL->setContentsMargins(0, 0, 0, 0);
        pL->move(x, y);
        pL->show();
        mpHost->setBackgroundColor(name, 32, 32, 32, 255);
        return pL;
    } else {
        return nullptr;
    }
}

std::pair<bool, QString> TMainConsole::deleteLabel(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.take(name);
    if (pL) {
        // Using deleteLater() rather than delete as it seems a safer option
        // given that this item is likely to be linked to some events and
        // suchlike:
        pL->deleteLater();

        // It remains to be seen if the label has "gone" as a result of the
        // above by the time the Lua subsystem processes the following:
        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysLabelDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, qsl("label name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::setLabelToolTip(const QString& name, const QString& text, double duration)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        duration = duration * 1000;
        pL->setToolTip(text);
        pL->setToolTipDuration(duration);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, qsl("label name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::setLabelCursor(const QString& name, int shape)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        if (shape > -1 && shape < 22) {
            pL->setCursor(static_cast<Qt::CursorShape>(shape));
        } else if (shape == -1) {
            pL->unsetCursor();
        } else {
            return {false, qsl("cursor shape '%1' not found. see https://doc.qt.io/qt-5/qt.html#CursorShape-enum").arg(shape)};
        }
        return {true, QString()};
    }
    return {false, qsl("label name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }

    if (pixMapLocation.isEmpty()) {
        return {false, qsl("custom cursor location cannot be an empty string")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        QPixmap cursor_pixmap = QPixmap(pixMapLocation);
        if (cursor_pixmap.isNull()) {
            return {false, qsl("couldn't find custom cursor, is the location \"%1\" correct?").arg(pixMapLocation)};
        }
        QCursor custom_cursor = QCursor(cursor_pixmap, hotX, hotY);
        pL->setCursor(custom_cursor);
        return {true, QString()};
    }

    return {false, qsl("label name '%1' not found").arg(name)};
}

// Called from TLuaInterpreter::createMapper(...) to create a map in a TConsole,
// Host::showHideOrCreateMapper(...) {formerly also called
// createMapper(...)} is used in other cases to make a map in a QDockWidget:
std::pair<bool, QString> TMainConsole::createMapper(const QString& windowname, int x, int y, int width, int height)
{
    auto pW = mDockWidgetMap.value(windowname);
    auto pM = mpHost->mpDockableMapWidget;
    if (pM) {
        return {false, qsl("cannot create mapper. Do you already use a map window?")};
    }
    if (!mpMapper) {
        // Arrange for TMap member values to be copied from the Host masters so they
        // are in place when the 2D mapper is created:
        mpHost->getPlayerRoomStyleDetails(mpHost->mpMap->mPlayerRoomStyle,
                                          mpHost->mpMap->mPlayerRoomOuterDiameterPercentage,
                                          mpHost->mpMap->mPlayerRoomInnerDiameterPercentage,
                                          mpHost->mpMap->mPlayerRoomOuterColor,
                                          mpHost->mpMap->mPlayerRoomInnerColor);
        if (!pW) {
            mpMapper = new dlgMapper(mpMainFrame, mpHost, mpHost->mpMap.data());
        } else {
            mpMapper = new dlgMapper(pW->widget(), mpHost, mpHost->mpMap.data());
        }
        mpHost->mpMap->mpHost = mpHost;
        mpHost->mpMap->mpMapper = mpMapper;
        qDebug() << "TConsole::createMapper() - restore map case 2.";
        mpHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(2) report"), true);
        QDateTime now(QDateTime::currentDateTime());

        if (mpHost->mpMap->restore(QString())) {
            mpHost->mpMap->audit();
            mpMapper->mp2dMap->init();
            mpMapper->updateAreaComboBox();
            mpMapper->resetAreaComboBoxToPlayerRoomArea();
            mpMapper->show();
        }

        mpHost->mpMap->pushErrorMessagesToFile(tr("Loading map(2) at %1 report").arg(now.toString(Qt::ISODate)), true);

        TEvent mapOpenEvent{};
        mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mapOpenEvent);
    }
    mpMapper->resize(width, height);
    mpMapper->move(x, y);

    // Qt bug workaround: on Windows and during profile load only, if the mapper widget is created
    // it gives a height and width to mpLeftToolBar, mpRightToolBar, and mpTopToolBar for
    // some reason. Those widgets size back down immediately after on their own (?!), however if
    // getMainWindowSize() is called right after map create, the sizes reported will be wrong
#if defined(Q_OS_WIN32)
    mpLeftToolBar->setHidden(true);
    mpRightToolBar->setHidden(true);
    mpTopToolBar->setHidden(true);
    mpMapper->show();
    mpLeftToolBar->setVisible(true);
    mpRightToolBar->setVisible(true);
    mpTopToolBar->setVisible(true);
#else
    mpMapper->show();
#endif
    return {true, QString()};
}

std::pair<bool, QString> TMainConsole::createCommandLine(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a commandLine cannot have an empty string as its name")};
    }

    auto pN = mSubCommandLineMap.value(name);
    auto pW = mDockWidgetMap.value(windowname);
    auto pS = mScrollBoxMap.value(windowname);

    if (!pN) {
        if (pS) {
            pN = new TCommandLine(mpHost, mpCommandLine->SubCommandLine, this, pS->widget());
        } else if (pW) {
            pN = new TCommandLine(mpHost, mpCommandLine->SubCommandLine, this, pW->widget());
        } else {
            pN = new TCommandLine(mpHost, mpCommandLine->SubCommandLine, this, mpMainFrame);
        }
        mSubCommandLineMap[name] = pN;
        pN->mCommandLineName = name;
        pN->setObjectName(name);
        pN->resize(width, height);
        pN->move(x, y);
        pN->show();
        return {true, QString()};
    }
    return {false, QLatin1String("couldn't create commandLine")};
}

bool TMainConsole::setBackgroundImage(const QString& name, const QString& path)
{
    auto pL = mLabelMap.value(name);
    if (pL) {
        QPixmap bgPixmap(path);
        pL->setPixmap(bgPixmap);
        return true;
    } else {
        return false;
    }
}

// Does NOT act on the TMainConsole itself:
bool TMainConsole::setBackgroundColor(const QString& name, int r, int g, int b, int alpha)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pC->setPalette(mainPalette);
        pC->mUpperPane->mBgColor = QColor(r, g, b, alpha);
        pC->mLowerPane->mBgColor = QColor(r, g, b, alpha);
        // update the display properly when color selections change.
        pC->mUpperPane->updateScreenView();
        pC->mUpperPane->forceUpdate();
        if (!pC->mUpperPane->mIsTailMode) {
            // The upper pane having mIsTailMode true means lower pane is hidden
            pC->mLowerPane->updateScreenView();
            pC->mLowerPane->forceUpdate();
        }
        return true;
    } else if (pL) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pL->setPalette(mainPalette);
        return true;
    } else {
        return false;
    }
}

bool TMainConsole::raiseWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    auto pM = mpMapper;
    auto pN = mSubCommandLineMap.value(name);
    auto pS = mScrollBoxMap.value(name);

    if (pC) {
        pC->raise();
        return true;
    }
    if (pL) {
        pL->raise();
        return true;
    }
    if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->raise();
        return true;
    }
    if (pS) {
        pS->raise();
        return true;
    }
    if (pN) {
        pN->raise();
        return true;
    }

    return false;
}

bool TMainConsole::lowerWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    auto pM = mpMapper;
    auto pN = mSubCommandLineMap.value(name);
    auto pS = mScrollBoxMap.value(name);

    if (pC) {
        pC->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pL) {
        pL->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pS) {
        pS->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pN) {
        pN->lower();
        mpMainDisplay->lower();
        return true;
    }
    return false;
}

bool TMainConsole::showWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->mUpperPane->updateScreenView();
        pC->mUpperPane->forceUpdate();
        pC->show();

        pC->mLowerPane->updateScreenView();
        pC->mLowerPane->forceUpdate();
        return true;
    } else if (pL) {
        pL->show();
        return true;
    } else {
        return false;
    }
}

bool TMainConsole::hideWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->hide();
        return true;
    } else if (pL) {
        pL->hide();
        return true;
    } else {
        return false;
    }
}

bool TMainConsole::printWindow(const QString& name, const QString& text)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->print(text);
        return true;
    } else if (pL) {
        pL->setText(text);
        return true;
    } else {
        return false;
    }
}

//getUserWindowSize for resizing in Geyser
QSize TMainConsole::getUserWindowSize(const QString& windowname) const
{
    auto pW = mDockWidgetMap.value(windowname);
    if (pW){
        QSize windowSize = pW->widget()->size();
        QSize userWindowSize(windowSize.width(), windowSize.height());
        return userWindowSize;
    }
    return getMainWindowSize();
}

QPair<bool, QString> TMainConsole::addWordToSet(const QString& word)
{
    QString errMsg = qsl("the word \"%1\" already seems to be in the user dictionary");
    QPair<bool, QString> result{};
    if (!mEnableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (!mUseSharedDictionary) {
        // The return value from this function is unclear - it does not seems to
        // indicate anything useful
        Hunspell_add(mpHunspell_profile, word.toUtf8().constData());
        if (!mWordSet_profile.contains(word)) {
            mWordSet_profile.insert(word);
            qDebug().noquote().nospace() << "TConsole::addWordToSet(\"" << word << "\") INFO - word added to profile mWordSet.";
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }

    } else {
        auto pMudlet = mudlet::self();
        QPair<bool, bool> sharedDictionaryResult = pMudlet->addWordToSet(word);
        while (!sharedDictionaryResult.first) {
            qDebug() << "TConsole::addWordToSet(...) ALERT - failed to get a write lock to access mWordSet_shared and loaded shared hunspell dictionary, retrying...";
            sharedDictionaryResult = pMudlet->addWordToSet(word);
        }

        if (sharedDictionaryResult.second) {
            // Successfully added word:
            result.first = true;
        } else {
            // Word already present
            result.second = errMsg.arg(word);
        }
    }

    return result;
}

QPair<bool, QString> TMainConsole::removeWordFromSet(const QString& word)
{
    QString errMsg = qsl("the word \"%1\" does not seem to be in the user dictionary");
    QPair<bool, QString> result{};
    if (!mEnableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (!mUseSharedDictionary) {
        // The return value from this function is unclear - it does not seems to
        // indicate anything useful
        Hunspell_remove(mpHunspell_profile, word.toUtf8().constData());
        if (mWordSet_profile.remove(word)) {
            qDebug().noquote().nospace() << "TConsole::removeWordFromSet(\"" << word << "\") INFO - word removed from profile mWordSet.";
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }

    } else {
        auto pMudlet = mudlet::self();
        QPair<bool, bool> sharedDictionaryResult = pMudlet->removeWordFromSet(word);
        while (!sharedDictionaryResult.first) {
            qDebug() << "TConsole::removeWordFromSet(...) ALERT - failed to get a write lock to access mWordSet_shared and loaded shared hunspell dictionary, retrying...";
            sharedDictionaryResult = pMudlet->removeWordFromSet(word);
        }

        if (sharedDictionaryResult.second) {
            // Successfully added word:
            result.first = true;
        } else {
            // Word already present
            result.second = errMsg.arg(word);
        }
    }

    return result;
}

void TMainConsole::setSystemSpellDictionary(const QString& newDict)
{
    if (newDict.isEmpty() || mSpellDic == newDict) {
        return;
    }

    mSpellDic = newDict;

    QString path = mudlet::getMudletPath(mudlet::hunspellDictionaryPath, mpHost->getSpellDic());
    QString spell_aff = qsl("%1%2.aff").arg(path, newDict);
    QString spell_dic = qsl("%1%2.dic").arg(path, newDict);

    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
    }

#if defined(Q_OS_WIN32)
    // strip non-ASCII characters from the path because hunspell can't handle them
    // when compiled with MinGW 7.3.0
    mudlet::self()->sanitizeUtf8Path(spell_aff, qsl("%1.aff").arg(newDict));
    mudlet::self()->sanitizeUtf8Path(spell_dic, qsl("%1.dic").arg(newDict));
#endif

    mpHunspell_system = Hunspell_create(spell_aff.toUtf8().constData(), spell_dic.toUtf8().constData());
    if (mpHunspell_system) {
        mHunspellCodecName_system = QByteArray(Hunspell_get_dic_encoding(mpHunspell_system));
        qDebug().noquote().nospace() << "TMainConsole::setSystemSpellDictionary(\"" << newDict << "\") INFO - System Hunspell dictionary loaded for profile, it uses a \"" << Hunspell_get_dic_encoding(mpHunspell_system) << "\" encoding...";
        mpHunspellCodec_system = QTextCodec::codecForName(mHunspellCodecName_system);
    }
}

// NOTE: mEnabledUserDictionary has been wedged on (it will never be false)
void TMainConsole::setProfileSpellDictionary()
{
    // Determine and copy the configuration settings from the Host instance:
    mpHost->getUserDictionaryOptions(mEnableUserDictionary, mUseSharedDictionary);
    if (!mEnableUserDictionary) {
        if (mpHunspell_profile) {
            Hunspell_destroy(mpHunspell_profile);
            mpHunspell_profile = nullptr;
            // Need to commit any changes to personal dictionary
            qDebug() << "TMainConsole::setProfileSpellDictionary() INFO - Saving profile's own Hunspell dictionary...";
            mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mProfileName, qsl("profile")), mWordSet_profile);
        }
        // Nothing else to do if not using the shared one

    } else {
        if (!mUseSharedDictionary) {
            // Want to use per profile dictionary, is it loaded?
            if (!mpHunspell_profile) {
                // No - so load it
                qDebug() << "TMainConsole::setProfileSpellDictionary() INFO - Preparing profile's own Hunspell dictionary...";
                mpHunspell_profile = mudlet::self()->prepareProfileDictionary(mpHost->getName(), mWordSet_profile);
            }
            // Else no need to load it

        } else {
            // Want to use the shared dictionary - this will open it if needed:
            mpHunspell_shared = mudlet::self()->prepareSharedDictionary();
        }
    }
}

QSet<QString> TMainConsole::getWordSet() const
{
    if (!mEnableUserDictionary) {
        return QSet<QString>();
    }

    if (!mUseSharedDictionary) {
        return mWordSet_profile;
    } else {
        return mudlet::self()->getWordSet();
    }
}

void TMainConsole::setProfileName(const QString& newName)
{
    TConsole::setProfileName(newName);

    for (auto pC : mSubConsoleMap) {
        pC->setProfileName(newName);
    }
}

std::pair<bool, QString> TMainConsole::setUserWindowTitle(const QString& name, const QString& text)
{
    if (name.isEmpty()) {
        return {false, qsl("a user window cannot have an empty string as its name")};
    }

    auto pC = mSubConsoleMap.value(name);
    if (!pC) {
        return {false, qsl("user window name '%1' not found").arg(name)};
    }

    // If it does not have an mType of UserWindow then it does not in a
    // floatable/dockable widget - so it can't have a titlebar...!
    if (pC->getType() != UserWindow) {
        return {false, qsl("\"%1\" is not a user window").arg(name)};
    }

    auto pD = mDockWidgetMap.value(name);
    if (Q_LIKELY(pD)) {
        if (text.isEmpty()) {
            // Reset to default text:
            pD->setWindowTitle(tr("User window - %1 - %2").arg(mpHost->getName(), name));
            return {true, QString()};
        }

        pD->setWindowTitle(text);
        return {true, QString()};
    }

    // This should be:
    Q_UNREACHABLE();
    // as it means that the TConsole is flagged as being a user window yet
    // it does not have a TDockWidget to hold it...
    return {false, qsl("internal error: TConsole \"%1\" is marked as a user window but does not have a TDockWidget to contain it").arg(name)};
}

bool TMainConsole::setTextFormat(const QString& name, const QColor& fgColor, const QColor& bgColor, const TChar::AttributeFlags& flags)
{
    if (name.isEmpty() || name.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        mFormatCurrent.setTextFormat(fgColor, bgColor, flags);
        return true;
    }

    auto pC = mSubConsoleMap.value(name);
    if (pC) {
        pC->mFormatCurrent.setTextFormat(fgColor, bgColor, flags);
        return true;
    }

    return false;
}

void TMainConsole::printOnDisplay(std::string& incomingSocketData, const bool isFromServer)
{
    mProcessingTimer.restart();
    mTriggerEngineMode = true;
    buffer.translateToPlainText(incomingSocketData, isFromServer);
    mTriggerEngineMode = false;

    // dequeues MXP events and raise them through the LuaInterpreter
    // TODO: move this somewhere else more appropriate
    auto& mxpEventQueue = mpHost->mMxpClient.mMxpEvents;
    while (!mxpEventQueue.isEmpty()) {
        const auto& event = mxpEventQueue.dequeue();
        mpHost->mLuaInterpreter.signalMXPEvent(event.name, event.attrs, event.actions);
    }

    double processT = mProcessingTimer.elapsed() / 1000.0;
    if (mpHost->mTelnet.mGA_Driver) {
        mpLineEdit_networkLatency->setText(tr("N:%1 S:%2",
                                              // intentional comment to separate arguments
                                              "The first argument 'N' represents the 'N'etwork latency; the second 'S' the "
                                              "'S'ystem (processing) time")
                                                   .arg(mpHost->mTelnet.networkLatencyTime, 0, 'f', 3)
                                                   .arg(processT, 0, 'f', 3));
    } else {
        mpLineEdit_networkLatency->setText(tr("<no GA> S:%1",
                                              // intentional comment to separate arguments
                                              "The argument 'S' represents the 'S'ystem (processing) time, in this situation "
                                              "the Game Server is not sending \"GoAhead\" signals so we cannot deduce the "
                                              "network latency...")
                                                   .arg(processT, 0, 'f', 3));
    }
    // Modify the tab text if this is not the currently active host - this
    // method is only used on the "main" console so no need to filter depending
    // on TConsole types:

    emit signal_newDataAlert(mProfileName);
}

void TMainConsole::runTriggers(int line)
{
    mUserCursor.setY(line);
    mIsPromptLine = buffer.promptBuffer.at(line);
    mEngineCursor = line;
    mUserCursor.setX(0);
    mCurrentLine = buffer.line(line);
    mpHost->getLuaInterpreter()->set_lua_string(cmLuaLineVariable, mCurrentLine);
    mCurrentLine.append('\n');

    if (mudlet::debugMode) {
        TDebug(Qt::darkGreen, Qt::black) << "new line arrived:" >> mpHost;
        TDebug(Qt::lightGray, Qt::black) << TDebug::csmContinue << mCurrentLine << "\n" >> mpHost;
    }
    mpHost->incomingStreamProcessor(mCurrentLine, line);
    mIsPromptLine = false;

    //FIXME: rewrite: if lines above the current line get deleted -> redraw clean slice
    //       otherwise just delete
}

void TMainConsole::finalize()
{
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

// TODO: It may be worth considering moving the (now) three following methods
// to the TMap class...?
bool TMainConsole::saveMap(const QString& location, int saveVersion)
{
    QDir dir_map;
    QString filename_map;
    QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);

    if (location.isEmpty()) {
        filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
    } else {
        filename_map = location;
    }

    if (!dir_map.exists(directory_map)) {
        dir_map.mkpath(directory_map);
    }
    QFile file_map(filename_map);
    if (file_map.open(QIODevice::WriteOnly)) {
        QDataStream out(&file_map);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        mpHost->mpMap->serialize(out, saveVersion);
        file_map.close();
        return true;
    }

    return false;
}

bool TMainConsole::loadMap(const QString& location)
{
    Host* pHost = mpHost;
    if (!pHost) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        return false;
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // No map or map currently loaded - so try and created mapper
        // but don't load a map here by default, we do that below and it may not
        // be the default map anyhow
        pHost->showHideOrCreateMapper(false);
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // And that failed so give up
        return false;
    }

    pHost->mpMap->mapClear();

    qDebug() << "TMainConsole::loadMap() - restore map case 1.";
    pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(1) report"), true);
    QDateTime now(QDateTime::currentDateTime());

    bool result = false;
    if (pHost->mpMap->restore(location)) {
        pHost->mpMap->audit();
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
        pHost->mpMap->mpMapper->show();
        result = true;
    } else {
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->show();
    }

    if (location.isEmpty()) {
        pHost->mpMap->pushErrorMessagesToFile(tr("Loading map(1) at %1 report").arg(now.toString(Qt::ISODate)), true);
    } else {
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Loading map(1) "%1" at %2 report)").arg(location, now.toString(Qt::ISODate)), true);
    }

    pHost->mpMap->update();

    return result;
}

// Used by TLuaInterpreter::loadMap() and dlgProfilePreferences for import/load
// of files ending in ".xml"
// The TLuaInterpreter::loadMap() supplies a pointer to an error Message which
// it requires in the event of an error (it should be written in a structure
// to match "loadMap: XXXXX." format) - the presence of a non-null pointer here
// should be used to suppress the writing of error messages direct to the
// console - if possible!
bool TMainConsole::importMap(const QString& location, QString* errMsg)
{
    Host* pHost = mpHost;
    if (!pHost) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        if (errMsg) {
            *errMsg = qsl("loadMap: NULL Host pointer {in TConsole::importMap(...)} - something is wrong!");
        }
        return false;
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // No map or mapper currently loaded/present - so try and create mapper
        pHost->showHideOrCreateMapper(false);
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // And that failed so give up
        if (errMsg) {
            *errMsg = qsl("loadMap: unable to initialise mapper {in TConsole::importMap(...)} - something is wrong!");
        }
        return false;
    }

    // Dump any outstanding map errors from past activities that had not yet
    // been logged...
    qDebug() << "TMainConsole::importingMap() - importing map case 1.";
    pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map importing(1) report"), true);
    QDateTime now(QDateTime::currentDateTime());

    bool result = false;

    QFileInfo fileInfo(location);
    QString filePathNameString;
    if (!fileInfo.filePath().isEmpty()) {
        if (fileInfo.isRelative()) {
            // Resolve the name relative to the profile home directory:
            filePathNameString = QDir::cleanPath(mudlet::getMudletPath(mudlet::profileDataItemPath, mProfileName, fileInfo.filePath()));
        } else {
            if (fileInfo.exists()) {
                filePathNameString = fileInfo.canonicalFilePath(); // Cannot use canonical path if file doesn't exist!
            } else {
                filePathNameString = fileInfo.absoluteFilePath();
            }
        }
    }

    QFile file(filePathNameString);
    if (!file.exists()) {
        if (!errMsg) {
            QString infoMsg = tr("[ ERROR ]  - Map file not found, path and name used was:\n"
                                 "%1.")
                                      .arg(filePathNameString);
            pHost->postMessage(infoMsg);
        } else {
            // error message for lua loadMap()
            *errMsg = tr("loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" was not found).")
                              .arg(filePathNameString);
        }
        return false;
    }

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        if (!errMsg) {
            QString infoMsg = tr("[ INFO ]  - Map file located and opened, now parsing it...");
            pHost->postMessage(infoMsg);
        }

        result = pHost->mpMap->importMap(file, errMsg);

        file.close();
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Importing map(1) "%1" at %2 report)").arg(location, now.toString(Qt::ISODate)));
    } else {
        if (!errMsg) {
            QString infoMsg = tr(R"([ INFO ]  - Map file located but it could not opened, please check permissions on:"%1".)").arg(filePathNameString);
            pHost->postMessage(infoMsg);
        } else {
            *errMsg = tr("loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" could not be opened for reading).")
                              .arg(filePathNameString);
        }
        return false;
    }

    pHost->mpMap->update();

    return result;
}

void TMainConsole::slot_reloadMap(QList<QString> profilesList)
{
    Host* pHost = getHost();
    if (!pHost) {
        return;
    }

    if (!profilesList.contains(mProfileName)) {
        qDebug() << "TMainConsole::slot_reloadMap(" << profilesList << ") request received but we:" << mProfileName << "are not mentioned - so we are ignoring it...!";
        return;
    }

    QString infoMsg = tr("[ INFO ]  - Map reload request received from system...");
    pHost->postMessage(infoMsg);

    QString outcomeMsg;
    if (loadMap(QString())) {
        outcomeMsg = tr("[  OK  ]  - ... System Map reload request completed.");
    } else {
        outcomeMsg = tr("[ WARN ]  - ... System Map reload request failed.");
    }

    pHost->postMessage(outcomeMsg);
}

void TMainConsole::resizeEvent(QResizeEvent* event)
{
    auto pHost = getHost();
    if (!pHost) {
        return;
    }

    // Process the event like other TConsoles
    TConsole::resizeEvent(event);

    // Update the record of the text area size for NAWS purposes:
    pHost->updateDisplayDimensions();
}

void TMainConsole::showStatistics()
{
    auto pHost = getHost();
    if (!pHost) {
        return;
    }

    QString header = tr("+--------------------------------------------------------------+\n"
                        "|                      system statistics                       |\n"
                        "+--------------------------------------------------------------+\n",
                        "Header for the system's statistics information displayed in the console, it is 64 'narrow' characters wide");
    print(header, QColor(150, 120, 0), Qt::black);

    QStringList subjects;
    QStringList tables;
    if (pHost->mTelnet.isGMCPEnabled()) {
        subjects << tr("GMCP events:", "Heading for the system's statistics information displayed in the console");
        tables << QLatin1String("gmcp");
    }
    if (pHost->mTelnet.isATCPEnabled()) {
        subjects << tr("ATCP events:", "Heading for the system's statistics information displayed in the console");
        tables << QLatin1String("atcp");
    }
    if (pHost->mTelnet.isChannel102Enabled()) {
        subjects << tr("Channel102 events:", "Heading for the system's statistics information displayed in the console");
        tables << QLatin1String("channel102");
    }
    if (pHost->mTelnet.isMSSPEnabled()) {
        subjects << tr("MSSP events:", "Heading for the system's statistics information displayed in the console");
        tables << QLatin1String("mssp");
    }
    if (pHost->mTelnet.isMSDPEnabled()) {
        // This might be a nil rather than an empty table if not present:
        subjects << tr("MSDP events:", "Heading for the system's statistics information displayed in the console");
        tables << QLatin1String("msdp");
    }

    Q_ASSERT_X(subjects.count() == tables.count(), "TMainConsole::showStatistics()", "mismatch in titles and built-in tables to show");
    for (int i = 0, total = subjects.count(); i < total; ++i) {
        mpHost->mLuaInterpreter.compileAndExecuteScript(QStringLiteral("setFgColor(190,150,0); setUnderline(true); echo([[\n\n%1\n]]);setUnderline(false);setFgColor(150,120,0);display( %2 );")
                                                        .arg(subjects.at(i), tables.at(i)));
    }

    const QString itemScript = "setFgColor(190,150,0); setUnderline(true); echo([[\n\n%1\n]]); setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Trigger Report:", "Heading for the system's statistics information displayed in the console")));
    QString itemMsg = std::get<0>(mpHost->getTriggerUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Timer Report:", "Heading for the system's statistics information displayed in the console")));
    itemMsg = std::get<0>(mpHost->getTimerUnit()->assembleReport());;
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Alias Report:", "Heading for the system's statistics information displayed in the console")));
    itemMsg = std::get<0>(mpHost->getAliasUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Keybinding Report:", "Heading for the system's statistics information displayed in the console")));
    itemMsg = std::get<0>(mpHost->getKeyUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Script Report:", "Heading for the system's statistics information displayed in the console")));
    itemMsg = std::get<0>(mpHost->getScriptUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    // Footer for the system's statistics information displayed in the console, it should be 64 'narrow' characters wide
    QString footer = QStringLiteral("\n+--------------------------------------------------------------+\n");
    mpHost->mpConsole->print(footer, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(QLatin1String("resetFormat();"));

    mpHost->mpConsole->raise();
}
