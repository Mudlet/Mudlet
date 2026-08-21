/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2022 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TConsoleModel.h"

#include "Host.h"
#include "mudlet.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFontInfo>

TConsoleModel::TConsoleModel(Host* pHost)
: buffer(pHost)
, mpHost(pHost)
{
}

// Moved here verbatim from TMainConsole so that a profile with no main console
// widget can start, write and stop a log: moving only the stream would have
// left the open/rotate/close half on the widget, and nothing view-less could
// have opened a file to stream into.
//
// Two gotchas carried over with it:
//
// - the text written *into* the log file is deliberately translated against the
//   "TMainConsole" context it came from. The catalogue is keyed on context plus
//   source text, so re-keying these under TConsoleModel would orphan every
//   translation they already have. The two messages printed on *screen* stayed
//   behind in TMainConsole with their own context intact.
// - QFontInfo(Host::getDisplayFont()) is the same font QWidget::fontInfo() used
//   to report here, because Host::getDisplayFont() hands back the main
//   console's own QFont - and unlike the widget call it still answers when
//   there is no widget.
void TConsoleModel::toggleLogging(bool isMessageEnabled)
{
    if (mpHost.isNull()) {
        return;
    }

    const auto loggingPath = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("autolog"));
    QFile file(loggingPath);
    const QDateTime logDateTime = QDateTime::currentDateTime();
    if (!mLogToLogFile) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "TConsoleModel: failed to open autolog file for writing:" << file.errorString();
            return;
        }
        QTextStream out(&file);
        file.close();

        QString directoryLogFile;
        QString logFileName;
        // If no log directory is set, default to Mudlet's replay and log files path
        if (mpHost->mLogDir == nullptr || mpHost->mLogDir.isEmpty()) {
            directoryLogFile = mudlet::getMudletPath(enums::profileReplayAndLogFilesPath, mpHost->getName());
        } else {
            directoryLogFile = mpHost->mLogDir;
        }
        // The format being empty is a signal value that means use a specified
        // name:
        if (mpHost->mLogFileNameFormat.isEmpty()) {
            if (mpHost->mLogFileName.isEmpty()) {
                // If no log name is set, use the default placeholder
                //: Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {one of two copies}).
                logFileName = QCoreApplication::translate("TMainConsole", "logfile");
            } else {
                // Otherwise a specific name as one is given
                logFileName = mpHost->mLogFileName;
            }
        } else {
            logFileName = logDateTime.toString(mpHost->mLogFileNameFormat);
        }

        // The preset file name formats are derived from date/times so that
        // alphabetical filename and date sort order are the same...
        const QDir dirLogFile;
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
            if (!mLogFile.open(QIODevice::ReadWrite)) {
                qWarning() << "TConsoleModel: failed to open log file for reading/writing:" << mLogFile.errorString();
                return;
            }
        } else {
            if (!mLogFile.open(QIODevice::Append)) {
                qWarning() << "TConsoleModel: failed to open log file for appending:" << mLogFile.errorString();
                return;
            }
        }
        mLogStream.setDevice(&mLogFile);

        if (isMessageEnabled) {
            mpHost->raiseLoggingAnnouncement(true, mLogFile.fileName());
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done BEFORE logging starts - or actually mLogToLogFile gets set!
        }
        mLogToLogFile = true;
    } else {
        QFile::remove(loggingPath);
        mLogToLogFile = false;
        if (isMessageEnabled) {
            mpHost->raiseLoggingAnnouncement(false, mLogFile.fileName());
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
            QStringList fontsList;                                     // List of fonts to become the font-family entry for
                                                                       // the master css in the header
            fontsList << QFontInfo(mpHost->getDisplayFont()).family(); // Seems to be the best way to get the
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
            logStream << "  <meta name='generator' content='" << QCoreApplication::translate("TMainConsole", "Mudlet MUD Client version: %1%2").arg(APP_VERSION, mudlet::self()->mAppBuild) << "'>\n";
            // Nice to identify what made the file!
            logStream << "  <title>" << QCoreApplication::translate("TMainConsole", "Mudlet, log from %1 profile").arg(mpHost->getName()) << "</title>\n";
            // Web-page title
            logStream << "  <style type='text/css'>\n";
            logStream << "   <!-- body { font-family: '" << fontsList.join("', '") << "'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(" << mpHost->mFgColor.red() << ","
                      << mpHost->mFgColor.green() << "," << mpHost->mFgColor.blue() << "); background-color:rgb(" << mpHost->mBgColor.red() << "," << mpHost->mBgColor.green() << ","
                      << mpHost->mBgColor.blue() << ");}\n";
            logStream << "        span { white-space: pre-wrap; }\n";

            if (mpHost->getEnableBlinkText()) {
                logStream << "        @keyframes blink-slow { 0%, 100% { opacity: 0.4; } 50% { opacity: 1; } }\n";
                logStream << "        @keyframes blink-fast { 0%, 100% { opacity: 0.4; } 50% { opacity: 1; } }\n";
                logStream << "        .blink-slow { animation: blink-slow 2s ease-in-out infinite; }\n";
                logStream << "        .blink-fast { animation: blink-fast 1s ease-in-out infinite; }\n";
            }

            logStream << "     -->\n";
            logStream << "  </style>\n";
            logStream << "  </head>\n";
            bool isAtBody = false;
            bool foundBody = false;
            while (!mLogStream.atEnd()) {
                const QString line = mLogStream.readLine();
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
            logStream
                    << qsl("<p>%1</p>\n")
                               .arg(logDateTime.toString(
                                       //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
                                       QCoreApplication::translate("TMainConsole", "'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.")));
            // <div></div> tags required around outside of the body <span></spans> for
            // strict HTML 4 as we do not use <p></p>s or anything else

            if (!mLogFile.resize(0)) {
                qWarning() << "TConsoleModel::toggleLogging(...) ERROR - Failed to resize HTML Logfile - it may now be corrupted...!";
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
            mLogStream << qsl("%1\n").arg(logDateTime.toString(
                    //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
                    QCoreApplication::translate("TMainConsole", "'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.")));
        }
    } else {
        // Logging is being turned off
        buffer.logRemainingOutput();
        //: This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale
        const QString endDateTimeLine = logDateTime.toString(QCoreApplication::translate("TMainConsole", "'Log session ending at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'."));
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
    }

    mpHost->raiseLoggingStateChanged(mLogToLogFile);
}
