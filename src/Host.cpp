/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2017 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "Host.h"


#include "LuaInterface.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStringBuilder>
#include "post_guard.h"


#include <errno.h>
#include <zip.h>


Host::Host(int port, const QString& hostname, const QString& login, const QString& pass, int id)
: mTelnet(this)
, mpConsole(nullptr)
, mLuaInterpreter(this, id)
, mTriggerUnit(this)
, mTimerUnit(this)
, mScriptUnit(this)
, mAliasUnit(this)
, mActionUnit(this)
, mKeyUnit(this)
, commandLineMinimumHeight(30)
, mAlertOnNewData(true)
, mAllowToSendCommand(true)
, mAutoClearCommandLineAfterSend(false)
, mBlockScriptCompile(true)
, mEchoLuaErrors(false)
, mBorderBottomHeight(0)
, mBorderLeftWidth(0)
, mBorderRightWidth(0)
, mBorderTopHeight(0)
, mCodeCompletion(true)
, mCommandLineFont(QFont("Bitstream Vera Sans Mono", 10, QFont::Normal))
, mCommandSeparator(QString(";"))
, mDisableAutoCompletion(false)
, mDisplayFont(QFont("Bitstream Vera Sans Mono", 10, QFont::Normal))
, mEnableGMCP(true)
, mEnableMSDP(false)
, mFORCE_GA_OFF(false)
, mFORCE_NO_COMPRESSION(false)
, mFORCE_SAVE_ON_EXIT(false)
, mHostID(id)
, mHostName(hostname)
, mInsertedMissingLF(false)
, mIsGoingDown(false)
, mLF_ON_GA(true)
, mLogin(login)
, mMainIconSize(3)
, mNoAntiAlias(false)
, mPass(pass)
, mpEditorDialog(nullptr)
, mpMap(new TMap(this))
, mpNotePad(nullptr)
, mPort(port)
, mPrintCommand(true)
, mIsCurrentLogFileInHtmlFormat(false)
, mIsLoggingTimestamps(false)
, mResetProfile(false)
, mRetries(5)
, mSaveProfileOnExit(false)
, mScreenHeight(25)
, mScreenWidth(90)
, mTEFolderIconSize(3)
, mTimeout(60)
, mUSE_FORCE_LF_AFTER_PROMPT(false)
, mUSE_IRE_DRIVER_BUGFIX(true)
, mUSE_UNIX_EOL(false)
, mWrapAt(100)
, mWrapIndentCount(0)
, mBlack(Qt::black)
, mLightBlack(Qt::darkGray)
, mRed(Qt::darkRed)
, mLightRed(Qt::red)
, mLightGreen(Qt::green)
, mGreen(Qt::darkGreen)
, mLightBlue(Qt::blue)
, mBlue(Qt::darkBlue)
, mLightYellow(Qt::yellow)
, mYellow(Qt::darkYellow)
, mLightCyan(Qt::cyan)
, mCyan(Qt::darkCyan)
, mLightMagenta(Qt::magenta)
, mMagenta(Qt::darkMagenta)
, mLightWhite(Qt::white)
, mWhite(Qt::lightGray)
, mFgColor(Qt::lightGray)
, mBgColor(Qt::black)
, mCommandBgColor(Qt::black)
, mCommandFgColor(QColor(113, 113, 0))
, mBlack_2(Qt::black)
, mLightBlack_2(Qt::darkGray)
, mRed_2(Qt::darkRed)
, mLightRed_2(Qt::red)
, mLightGreen_2(Qt::green)
, mGreen_2(Qt::darkGreen)
, mLightBlue_2(Qt::blue)
, mBlue_2(Qt::darkBlue)
, mLightYellow_2(Qt::yellow)
, mYellow_2(Qt::darkYellow)
, mLightCyan_2(Qt::cyan)
, mCyan_2(Qt::darkCyan)
, mLightMagenta_2(Qt::magenta)
, mMagenta_2(Qt::darkMagenta)
, mLightWhite_2(Qt::white)
, mWhite_2(Qt::lightGray)
, mFgColor_2(Qt::lightGray)
, mBgColor_2(Qt::black)
, mSpellDic("en_US")
, mLogStatus(false)
, mEnableSpellCheck(true)
, mModuleSaveBlock(false)
, mLineSize(10.0)
, mRoomSize(0.5)
, mBubbleMode(false)
, mShowRoomID(false)
, mMapperUseAntiAlias(true)
, mServerGUI_Package_version(-1)
, mServerGUI_Package_name("nothing")
, mAcceptServerGUI(true)
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
, mFORCE_MXP_NEGOTIATION_OFF(false)
, mpDockableMapWidget()
, mHaveMapperScript(false)
, mEditorTheme("Mudlet")
, mEditorThemeFile("Mudlet.tmTheme")
, mThemePreviewItemID(-1)
, mThemePreviewType(QString())
{
    // mLogStatus = mudlet::self()->mAutolog;
    mLuaInterface.reset(new LuaInterface(this));
    QString directoryLogFile = mudlet::getMudletPath(mudlet::profileDataItemPath, mHostName, QStringLiteral("log"));
    QString logFileName = QStringLiteral("%1/errors.txt").arg(directoryLogFile);
    QDir dirLogFile;
    if (!dirLogFile.exists(directoryLogFile)) {
        dirLogFile.mkpath(directoryLogFile);
    }
    mErrorLogFile.setFileName(logFileName);
    mErrorLogFile.open(QIODevice::Append);
    // This is NOW used (for map
    // file auditing and other issues)
    mErrorLogStream.setDevice(&mErrorLogFile);

    QTimer::singleShot(0, [this]() {
        if (mpMap->restore(QString(), false)) {
            mpMap->audit();
        }
    });

    mMapStrongHighlight = false;
    mGMCP_merge_table_keys.append("Char.Status");
    mDoubleClickIgnore.insert('"');
    mDoubleClickIgnore.insert('\'');

    // search engine load entries
    mSearchEngineData = QMap<QString, QString>(
    {
                    {"Bing",       "https://www.bing.com/search?q="},
                    {"DuckDuckGo", "https://duckduckgo.com/?q="},
                    {"Google",     "https://www.google.com/search?q="}
    });
}

Host::~Host()
{
    if (mpDockableMapWidget) {
        mpDockableMapWidget->deleteLater();
    }
    mIsGoingDown = true;
    mIsClosingDown = true;
    mTelnet.disconnect();
    mErrorLogStream.flush();
    mErrorLogFile.close();
}

// See: https://github.com/nih-at/libzip/blob/master/API-CHANGES to get
// details of what we can use for various versions of libzip.
#if (LIBZIP_VERSION_MAJOR >= 1)
void Host::saveModules(int sync)
{
    if (mModuleSaveBlock) {
        postMessage(tr("[ ERROR ] - A previous failure detected when saving modules has disabled them\n"
                                   "from being saved for the rest of this run of the Mudlet\n"
                                   "application.  Sorry, but this is likely to mean that any changes\n"
                                   "to modules will be lost!"));
        return;
    }
    QMapIterator<QString, QStringList> it(modulesToWrite);
    QStringList modulesToSync;
    QString dirName = mudlet::getMudletPath(mudlet::moduleBackupsPath);
    QDir savePath = QDir(dirName);
    if (!savePath.exists()) {
        savePath.mkpath(dirName);
    }
    QString dateTimeStampString = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");

    while (it.hasNext()) {
        it.next();
        QStringList moduleData = it.value();
        QString filename_xml = moduleData.at(0);
        // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#hh-mm-ss" (1 of 6)
        QString moduleName = it.key();
        QString zipName;
        zip* archive = nullptr;
        // Filename extension tests should be case insensitive to work on MacOS Platforms...! - Slysven

        if (filename_xml.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)
          ||filename_xml.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            // This branch will overwrite THE EXISTING MODULE FILE WITH NO BACKUP
            // It can also cause DATA LOSS should MORE THAN ONE PROFILE HAVE
            // modified the MODULE and be set to be synced - the last profile to
            // save the module will overwrite the others (or will the first to
            // save the module overwrite the others before they save?) - it all
            // gets a bit existantial and definitely will not be good news for
            // someone...
            QString packagePathName = mudlet::getMudletPath(mudlet::profilePackagePath, mHostName, moduleName);
            filename_xml = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
            int ze;
            archive = zip_open(moduleData.at(0).toStdString().c_str(), ZIP_CHECKCONS, &ze);
            if (!archive) {
                zip_error_t error;
                zip_error_init_with_code(&error, ze);
                postMessage(tr("[ ERROR ] - Failed to open module (archive) file \"%1\", error is: \"%2\".")
                                     .arg(moduleData.at(0), QString::fromUtf8(zip_error_strerror(&error))));
                zip_error_fini(&error);
                // Try again for next module...
                continue;
            }

            zipName = moduleData.at(0);
            QDir packageDir = QDir(packagePathName);
            if (!packageDir.exists()) {
                packageDir.mkpath(packagePathName);
            }
        } else {
            //move the old file, use the key (module name) as the file
            savePath.rename(filename_xml, QStringLiteral("%1%2%3").arg(dirName, moduleName, dateTimeStampString));
        }

        // Save the current Mudlet items in the module as an XML file...
        QFile file_xml(filename_xml);
        if (file_xml.open(QIODevice::WriteOnly)) {
            XMLexport writer(this);
            // This is the only caller of XMLexport::writeModuleXML(...)
            QPair<bool, QString> results = writer.writeModuleXML(&file_xml, it.key());
            if (!results.first) {
                postMessage(tr("[ ERROR ] - Unable to save the XML for module \"%1\" to\n"
                                           "\"%2\".\n"
                                           "There is an error message: \"%3\".")
                            .arg(filename_xml, moduleName, results.second));

                file_xml.close();
                if (!zipName.isEmpty() && archive) {
                    zip_discard(archive);
                    archive = 0;
                }
                // Try again for next module...
                continue;
            }

            file_xml.close();

            // This entry will be non-zero for a "synced" module
            if (moduleData.at(1).toInt()) {
                modulesToSync << it.key();
            }

        } else {
            postMessage(tr("[ ERROR ] - Unable to open the file \"%1\" to save\n"
                                       "the XML for the Mudlet items in module \"%2\".\n"
                                       "Futher saving of modules has been disabled for the rest of this run\n"
                                       "of the Mudlet application.  Sorry, but this is likely to mean that\n"
                                       "any other changes to other modules will be lost!")
                        .arg(moduleName, filename_xml));
            mModuleSaveBlock = true;
            return;
        }

        if (!zipName.isEmpty()) {
            // Opens filename_xml as something that can be read into archive (?)
            // The first 0 means read from the start of the file, the second
            // means read the WHOLE file at the appropriate time {when
            // zip_close(archive) is called}...
            struct zip_source* zipSource = zip_source_file(archive, filename_xml.toStdString().c_str(), 0, 0);
            if (!zipSource) {
                zip_error_t* pError = zip_get_error(archive);
                postMessage(tr("[ ERROR ] - Failed to open file \"%1\" to place into module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(zip_error_strerror(pError))));
                zip_error_fini(pError);
                zip_discard(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            //  Might want to include ZIP_FL_OVERWRITE flag to later version:
            zip_int64_t index = zip_file_add(archive, QStringLiteral("%1.xml").arg(moduleName).toStdString().c_str(), zipSource, ZIP_FL_OVERWRITE|ZIP_FL_ENC_UTF_8);

            // index is -1 on error:
            if (index == -1) {
                zip_error_t* pError = zip_get_error(archive);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(zip_error_strerror(pError))));
                zip_error_fini(pError);
                zip_source_close(zipSource);
                zip_discard(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            if (zip_close(archive) == -1) {
                zip_error_t* pError = zip_get_error(archive);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(zip_error_strerror(pError))));
                zip_error_fini(pError);
                zip_discard(archive);
            } // End of error on closing archive (at the right time)
        } // End of zipName not being empty
    } // End of while having anothor module to write

    modulesToWrite.clear();

    if (sync) {
        //synchronize modules across sessions
        QMap<Host*, TConsole*> activeSessions = mudlet::self()->mConsoleMap;
        QMapIterator<Host*, TConsole*> it2(activeSessions);
        while (it2.hasNext()) {
            it2.next();
            Host* host = it2.key();
            if (host->mHostName == mHostName) {
                continue;
            }
            QMap<QString, int> modulePri = host->mModulePriorities;
            QMapIterator<QString, int> it3(modulePri);
            QMap<int, QStringList> moduleOrder;
            while (it3.hasNext()) {
                it3.next();
                moduleOrder[it3.value()].append(it3.key()); // = moduleEntry;
            }
            QMapIterator<int, QStringList> it4(moduleOrder);
            while (it4.hasNext()) {
                it4.next();
                QStringListIterator itModule(it4.value());
                while (itModule.hasNext()) {
                    if (modulesToSync.contains(itModule.next())) {
                        host->reloadModule(itModule.peekPrevious());
                    }
                }
            }
        }
    }
}
// End of #if (LIBZIP_VERSION_MAJOR >= 1)
#elif (LIBZIP_VERSION_MAJOR == 0) && (LIBZIP_VERSION_MINOR >= 11)
void Host::saveModules(int sync)
{
    if (mModuleSaveBlock) {
        postMessage(tr("[ ERROR ] - A previous failure detected when saving modules has disabled them\n"
                                   "from being saved for the rest of this run of the Mudlet\n"
                                   "application.  Sorry, but this is likely to mean that any changes\n"
                                   "to modules will be lost!"));
        return;
    }
    QMapIterator<QString, QStringList> it(modulesToWrite);
    QStringList modulesToSync;
    QString dirName = mudlet::getMudletPath(mudlet::moduleBackupsPath);
    QDir savePath = QDir(dirName);
    if (!savePath.exists()) {
        savePath.mkpath(dirName);
    }
    QString dateTimeStampString = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");

    while (it.hasNext()) {
        it.next();
        QStringList moduleData = it.value();
        QString filename_xml = moduleData.at(0);
        // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#hh-mm-ss" (1 of 6)
        QString moduleName = it.key();
        QString zipName;
        zip* archive = nullptr;
        // Filename extension tests should be case insensitive to work on MacOS Platforms...! - Slysven

        if (filename_xml.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)
          ||filename_xml.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            // This branch will overwrite THE EXISTING MODULE FILE WITH NO BACKUP
            // It can also cause DATA LOSS should MORE THAN ONE PROFILE HAVE
            // modified the MODULE and be set to be synced - the last profile to
            // save the module will overwrite the others (or will the first to
            // save the module overwrite the others before they save?) - it all
            // gets a bit existantial and definitely will not be good news for
            // someone...
            QString packagePathName = mudlet::getMudletPath(mudlet::profilePackagePath, mHostName, moduleName);
            filename_xml = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
            int ze;
            archive = zip_open(moduleData.at(0).toStdString().c_str(), ZIP_CHECKCONS, &ze);
            if (!archive) {
                // Uses system errno?
                char errorMessageBuffer[128];
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, errno);
                postMessage(tr("[ ERROR ] - Failed to open module (archive) file \"%1\", error is: \"%2\".")
                                     .arg(moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                // Try again for next module...
                continue;
            }

            zipName = moduleData.at(0);
            QDir packageDir = QDir(packagePathName);
            if (!packageDir.exists()) {
                packageDir.mkpath(packagePathName);
            }
        } else {
            //move the old file, use the key (module name) as the file
            savePath.rename(filename_xml, QStringLiteral("%1%2%3").arg(dirName, moduleName, dateTimeStampString));
        }

        // Save the current Mudlet items in the module as an XML file...
        QFile file_xml(filename_xml);
        if (file_xml.open(QIODevice::WriteOnly)) {
            XMLexport writer(this);
            // This is the only caller of XMLexport::writeModuleXML(...)
            QPair<bool, QString> results = writer.writeModuleXML(&file_xml, it.key());
            if (!results.first) {
                postMessage(tr("[ ERROR ] - Unable to save the XML for module \"%1\" to\n"
                                           "\"%2\".\n"
                                           "There is an error message: \"%3\".")
                            .arg(filename_xml, moduleName, results.second));

                file_xml.close();
                if (!zipName.isEmpty() && archive) {
                    zip_discard(archive);
                    archive = 0;
                }
                // Try again for next module...
                continue;
            }

            file_xml.close();

            // This entry will be non-zero for a "synced" module
            if (moduleData.at(1).toInt()) {
                modulesToSync << it.key();
            }

        } else {
            postMessage(tr("[ ERROR ] - Unable to open the file \"%1\" to save\n"
                                       "the XML for the Mudlet items in module \"%2\".\n"
                                       "Futher saving of modules has been disabled for the rest of this run\n"
                                       "of the Mudlet application.  Sorry, but this is likely to mean that\n"
                                       "any other changes to other modules will be lost!")
                        .arg(moduleName, filename_xml));
            mModuleSaveBlock = true;
            return;
        }

        if (!zipName.isEmpty()) {
            // Opens filename_xml as something that can be read into archive (?)
            // The first 0 means read from the start of the file, the second
            // means read the WHOLE file at the appropriate time {when
            // zip_close(archive) is called}...
            struct zip_source* zipSource = zip_source_file(archive, filename_xml.toStdString().c_str(), 0, 0);
            if (!zipSource) {
                int ze;
                // Is se the system errno?
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to open file \"%1\" to place into module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                zip_close(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            // We were using zip_add(...) but that is obsolete (and does not
            // necessarily support UTF-8 encoded file-names)! It also does not
            // work should the file already be present in the archive - which it
            // would be in this situation...
            // In 0.11 the return value was changed from (the too small) int
            // to zip_int64_t whilst we are not expecting THAT many files it
            // may throw off the interpretation of the -1 error value if we do
            // not detect exactly the right thing...
            zip_int64_t index = zip_name_locate(archive, QStringLiteral("%1.xml").arg(moduleName).toStdString().c_str(), 0);
            if (Q_UNLIKELY(index == -1)) {
                // Weird - it was not found but it cannot happen, can it?
                postMessage(tr("[ ERROR ] - Failed to find the module's Mudlet items \"%1\" file\n"
                                           "inside (archive) file \"%2\", this does not\n"
                                           "seem correct so aborting save for this module.")
                            .arg(filename_xml, moduleData.at(0)));
                zip_discard(archive);
                // If this function is not available in your version of
                // libzip, replace it with:
                // zip_source_free(zipSource)
                // and report to Mudlet developers which version of libzip
                // (less than 1.30) you are linking to!
                zip_source_close(zipSource);
                archive = 0;
                // Try again for next module...
                continue;
            } else {
                // OK - file was found in archive - so now to replace it
                // Repurpose the index value to reflect the error status, 0 is
                // on success, -1 is error - the new libzip code also returns -1
                // on error and 0 or greater (being the index of the file in the
                // archive...
                index = zip_replace(archive, index, zipSource);
            }

            // index is -1 on error:
            if (index == -1) {
                int ze;
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                zip_source_close(zipSource);
                zip_close(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            if (zip_close(archive) == -1) {
                int ze;
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                // 0.11 and beyond has a command to throw away an opened archive
                zip_discard(archive);
            } // End of error on closing archive (at the right time)
        } // End of zipName not being empty
    } // End of while having anothor module to write

    modulesToWrite.clear();

    if (sync) {
        //synchronize modules across sessions
        QMap<Host*, TConsole*> activeSessions = mudlet::self()->mConsoleMap;
        QMapIterator<Host*, TConsole*> it2(activeSessions);
        while (it2.hasNext()) {
            it2.next();
            Host* host = it2.key();
            if (host->mHostName == mHostName) {
                continue;
            }
            QMap<QString, int> modulePri = host->mModulePriorities;
            QMapIterator<QString, int> it3(modulePri);
            QMap<int, QStringList> moduleOrder;
            while (it3.hasNext()) {
                it3.next();
                moduleOrder[it3.value()].append(it3.key()); // = moduleEntry;
            }
            QMapIterator<int, QStringList> it4(moduleOrder);
            while (it4.hasNext()) {
                it4.next();
                QStringListIterator itModule(it4.value());
                while (itModule.hasNext()) {
                    if (modulesToSync.contains(itModule.next())) {
                        host->reloadModule(itModule.peekPrevious());
                    }
                }
            }
        }
    }
}
// End of #elif (LIBZIP_VERSION_MAJOR == 0) && (LIBZIP_VERSION_MINOR >= 11)
#else // LIBZIP_VERSION_MAJOR == 0 and LIBZIP_VERSION_MINOR < 11
void Host::saveModules(int sync)
{
    if (mModuleSaveBlock) {
        postMessage(tr("[ ERROR ] - A previous failure detected when saving modules has disabled them\n"
                                   "from being saved for the rest of this run of the Mudlet\n"
                                   "application.  Sorry, but this is likely to mean that any changes\n"
                                   "to modules will be lost!"));
        return;
    }
    QMapIterator<QString, QStringList> it(modulesToWrite);
    QStringList modulesToSync;
    QString dirName = mudlet::getMudletPath(mudlet::moduleBackupsPath);
    QDir savePath = QDir(dirName);
    if (!savePath.exists()) {
        savePath.mkpath(dirName);
    }
    QString dateTimeStampString = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");

    while (it.hasNext()) {
        it.next();
        QStringList moduleData = it.value();
        QString filename_xml = moduleData.at(0);
        // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#hh-mm-ss" (1 of 6)
        QString moduleName = it.key();
        QString zipName;
        zip* archive = nullptr;
        // Filename extension tests should be case insensitive to work on MacOS Platforms...! - Slysven

        if (filename_xml.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)
          ||filename_xml.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            // This branch will overwrite THE EXISTING MODULE FILE WITH NO BACKUP
            // It can also cause DATA LOSS should MORE THAN ONE PROFILE HAVE
            // modified the MODULE and be set to be synced - the last profile to
            // save the module will overwrite the others (or will the first to
            // save the module overwrite the others before they save?) - it all
            // gets a bit existantial and definitely will not be good news for
            // someone...
            QString packagePathName = mudlet::getMudletPath(mudlet::profilePackagePath, mHostName, moduleName);
            filename_xml = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
            int ze;
            archive = zip_open(moduleData.at(0).toStdString().c_str(), ZIP_CHECKCONS, &ze);
            if (!archive) {
                // Uses system errno?
                char errorMessageBuffer[128];
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, errno);
                postMessage(tr("[ ERROR ] - Failed to open module (archive) file \"%1\", error is: \"%2\".")
                                     .arg(moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                // Try again for next module...
                continue;
            }

            zipName = moduleData.at(0);
            QDir packageDir = QDir(packagePathName);
            if (!packageDir.exists()) {
                packageDir.mkpath(packagePathName);
            }
        } else {
            //move the old file, use the key (module name) as the file
            savePath.rename(filename_xml, QStringLiteral("%1%2%3").arg(dirName, moduleName, dateTimeStampString));
        }

        // Save the current Mudlet items in the module as an XML file...
        QFile file_xml(filename_xml);
        if (file_xml.open(QIODevice::WriteOnly)) {
            XMLexport writer(this);
            // This is the only caller of XMLexport::writeModuleXML(...)
            QPair<bool, QString> results = writer.writeModuleXML(&file_xml, it.key());
            if (!results.first) {
                postMessage(tr("[ ERROR ] - Unable to save the XML for module \"%1\" to\n"
                                           "\"%2\".\n"
                                           "There is an error message: \"%3\".")
                            .arg(filename_xml, moduleName, results.second));

                file_xml.close();
                if (!zipName.isEmpty() && archive) {
                    zip_close(archive);
                    archive = 0;
                }
                // Try again for next module...
                continue;
            }

            file_xml.close();

            // This entry will be non-zero for a "synced" module
            if (moduleData.at(1).toInt()) {
                modulesToSync << it.key();
            }

        } else {
            postMessage(tr("[ ERROR ] - Unable to open the file \"%1\" to save\n"
                                       "the XML for the Mudlet items in module \"%2\".\n"
                                       "Futher saving of modules has been disabled for the rest of this run\n"
                                       "of the Mudlet application.  Sorry, but this is likely to mean that\n"
                                       "any other changes to other modules will be lost!")
                        .arg(moduleName, filename_xml));
            mModuleSaveBlock = true;
            return;
        }

        if (!zipName.isEmpty()) {
            // Opens filename_xml as something that can be read into archive (?)
            // The first 0 means read from the start of the file, the second
            // means read the WHOLE file at the appropriate time {when
            // zip_close(archive) is called}...
            struct zip_source* zipSource = zip_source_file(archive, filename_xml.toStdString().c_str(), 0, 0);
            if (!zipSource) {
                int ze;
                // Is se the system errno?
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to open file \"%1\" to place into module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                zip_close(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            /*
             * Fallback code to replace file in archive for old libzip versions:
             */

            // We were using zip_add(...) but that is obsolete (and does not
            // necessarily support UTF-8 encoded file-names)! It also does not
            // work should the file already be present in the archive - which it
            // would be in this situation...
            // In 0.11 the return value was changed from (the too small) int
            // to zip_int64_t whilst we are not expecting THAT many files it
            // may throw off the interpretation of the -1 error value if we do
            // not detect exactly the right thing...
            int err = 0;
            int index = zip_name_locate(archive, QStringLiteral("%1.xml").arg(moduleName).toStdString().c_str(), 0);
            if (Q_UNLIKELY(index == -1)) {
                // Weird - it was not found but it cannot happen, can it?
                postMessage(tr("[ ERROR ] - Failed to find the module's Mudlet items \"%1\" file\n"
                                           "inside (archive) file \"%2\", this does not\n"
                                           "seem correct so aborting save for this module.")
                            .arg(filename_xml, moduleData.at(0)));
                zip_close(archive);
                // At some point between 0.11 and 1.x zip_source_close()
                // became public but it definetly is not available in 0.10
                // so fall back to older (void) function that has the same
                // effect but must only be used on an *unused* "zip_source"
                zip_source_free(zipSource);
                archive = 0;
                // Try again for next module...
                continue;
            } else {
                // OK - file was found in archive - so now to replace it
                // Repurpose the index value to reflect the error status, 0 is
                // on success, -1 is error - the new libzip code also returns -1
                // on error and 0 or greater (being the index of the file in the
                // archive...
                index = zip_replace(archive, index, zipSource);
            }

            // index is -1 on error:
            if (index == -1) {
                int ze;
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                zip_source_free(zipSource);
                zip_close(archive);
                archive = 0;
                // Try again for next module...
                continue;
            }

            if (zip_close(archive) == -1) {
                int ze;
                int se;
                char errorMessageBuffer[128];
                zip_error_get(archive, &ze, &se);
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, se);
                postMessage(tr("[ ERROR ] - Failed to replace file \"%1\" in module (archive)\n"
                                           "file \"%2\",\n"
                                           "error is: \"%3\".")
                            .arg(filename_xml, moduleData.at(0), QString::fromUtf8(errorMessageBuffer)));
                // There is an unavoidable memory leak here for < 0.11 - we
                // cannot close the archive and there is no command to
                // forcibly clear the data associated with the archive data
                // structure.
            } // End of error on closing archive (at the right time)
        } // End of zipName not being empty
    } // End of while having anothor module to write

    modulesToWrite.clear();

    if (sync) {
        //synchronize modules across sessions
        QMap<Host*, TConsole*> activeSessions = mudlet::self()->mConsoleMap;
        QMapIterator<Host*, TConsole*> it2(activeSessions);
        while (it2.hasNext()) {
            it2.next();
            Host* host = it2.key();
            if (host->mHostName == mHostName) {
                continue;
            }
            QMap<QString, int> modulePri = host->mModulePriorities;
            QMapIterator<QString, int> it3(modulePri);
            QMap<int, QStringList> moduleOrder;
            while (it3.hasNext()) {
                it3.next();
                moduleOrder[it3.value()].append(it3.key()); // = moduleEntry;
            }
            QMapIterator<int, QStringList> it4(moduleOrder);
            while (it4.hasNext()) {
                it4.next();
                QStringListIterator itModule(it4.value());
                while (itModule.hasNext()) {
                    if (modulesToSync.contains(itModule.next())) {
                        host->reloadModule(itModule.peekPrevious());
                    }
                }
            }
        }
    }
}
// End of #else // LIBZIP_VERSION_MAJOR == 0 and LIBZIP_VERSION_MINOR < 11
#endif

bool Host::reloadModule(const QString& moduleName, QString* pErrorMessage)
{
    // A QMapIterator works from a copy of what it is iterating over so it is
    // safe if the original gets modified!
    bool isFound = false;
    bool isError = false;
    if (mInstalledModules.contains(moduleName)) {
        isFound = true;
        QStringList moduleData = mInstalledModules.value(moduleName);
        // Preserve the data to prevent it being modified by the unload and loading:
        moduleData.detach();
        if (moduleData.at(1).compare(QLatin1String("0"))) {
            // If we had the module set to be synced in THIS profile
            // AND and we had edited the data
            // THEN we will now lose the changes made in this profile as it is
            // overwritten by the changes made in a different profile if this
            // has been prompted by a save of the module in the other profile...!
            qWarning().nospace() << "Host::reloadModule(...) WARNING: The module: "
                                 << moduleName
                                 << " was set to be synced on saving in the: "
                                 << getName()
                                 << " profile but any changes are now being overwritten, either because a lua script within the profle has invoked this or because the module was set to be synced in another profile and that has just happened; in the latter case, any changes made in the named profile have been lost!";
        }
        if (!uninstallPackage(moduleName, 2, pErrorMessage)) {
            isError = true;
        }
        if (!installPackage(moduleData.at(0), 2, pErrorMessage)) {
            isError = true;
        }
        if (mInstalledModules.contains(moduleName)) {
            // Check to see that we still have the same module name
            // Perhaps it might be instructive to compare the saved moduleData
            // to what it is after the reload...?

            // But stuff the old data back in place:
            mInstalledModules[moduleName] = moduleData;
        }
    }

    if (!isFound) {
        // No need to name the module - the lua system will already have done that...
        *pErrorMessage = QStringLiteral("module not found");
        return false;
    } else if (isError) {
        return false;
    } else {
        return true;
    }
}

void Host::resetProfile()
{
    getTimerUnit()->stopAllTriggers();
    mudlet::self()->mTimerMap.clear();
    getTimerUnit()->removeAllTempTimers();
    getTriggerUnit()->removeAllTempTriggers();


    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mpConsole->resetMainConsole();
    mEventHandlerMap.clear();
    mEventMap.clear();
    mLuaInterpreter.initLuaGlobals();
    mLuaInterpreter.loadGlobal();
    mLuaInterpreter.initIndenterGlobals();
    mBlockScriptCompile = false;


    getTriggerUnit()->compileAll();
    getAliasUnit()->compileAll();
    getActionUnit()->compileAll();
    getKeyUnit()->compileAll();
    getScriptUnit()->compileAll();
    //getTimerUnit()->compileAll();
    mResetProfile = false;

    mTimerUnit.reenableAllTriggers();

    TEvent event;
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(event);
    qDebug() << "resetProfile() DONE";
}

// Saves profile to disk - does not save items dirty in the editor, however.
// takes a directory to save in or an empty string for the default location
// as well as a boolean whenever to sync the modules or not
// returns true+filepath if successful or false+error message otherwise
std::tuple<bool, QString, QString> Host::saveProfile(const QString& saveLocation, bool syncModules)
{
    QString directory_xml;
    if (saveLocation.isEmpty()) {
        directory_xml = mudlet::getMudletPath(mudlet::profileXmlFilesPath, getName());
    } else {
        directory_xml = saveLocation;
    }

    // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#hh-mm-ss" (2 of 6)
    QString filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, QDateTime::currentDateTime().toString(QStringLiteral("dd-MM-yyyy#hh-mm-ss")));
    QDir dir_xml;
    if (!dir_xml.exists(directory_xml)) {
        dir_xml.mkpath(directory_xml);
    }
    QFile file_xml(filename_xml);
    if (file_xml.open(QIODevice::WriteOnly)) {
        XMLexport writer(this);
        writer.exportHost(&file_xml);
        file_xml.close();
        saveModules(syncModules ? 1 : 0);
        return std::make_tuple(true, filename_xml, QString());
    } else {
        return std::make_tuple(false, filename_xml, file_xml.errorString());
    }
}

// Now returns the total weight of the path
const unsigned int Host::assemblePath()
{
    unsigned int totalWeight = 0;
    QStringList pathList;
    for (int i : mpMap->mPathList) {
        QString n = QString::number(i);
        pathList.append(n);
    }
    QStringList directionList = mpMap->mDirList;
    QStringList weightList;
    for (int stepWeight : mpMap->mWeightList) {
        totalWeight += stepWeight;
        QString n = QString::number(stepWeight);
        weightList.append(n);
    }
    QString tableName = QStringLiteral("speedWalkPath");
    mLuaInterpreter.set_lua_table(tableName, pathList);
    tableName = QStringLiteral("speedWalkDir");
    mLuaInterpreter.set_lua_table(tableName, directionList);
    tableName = QStringLiteral("speedWalkWeight");
    mLuaInterpreter.set_lua_table(tableName, weightList);
    return totalWeight;
}

const bool Host::checkForMappingScript()
{
    // the mapper script reminder is only shown once
    // because it is too difficult and error prone (->proper script sequence)
    // to disable this message
    bool ret = (mLuaInterpreter.check_for_mappingscript() || mHaveMapperScript);
    mHaveMapperScript = true;
    return ret;
}

void Host::startSpeedWalk()
{
    int totalWeight = assemblePath();
    Q_UNUSED(totalWeight);
    QString f = QStringLiteral("doSpeedWalk");
    QString n = QString();
    mLuaInterpreter.call(f, n);
}

void Host::adjustNAWS()
{
    mTelnet.setDisplayDimensions();
}

void Host::stopAllTriggers()
{
    mTriggerUnit.stopAllTriggers();
    mAliasUnit.stopAllTriggers();
    mTimerUnit.stopAllTriggers();
}

void Host::reenableAllTriggers()
{
    mTriggerUnit.reenableAllTriggers();
    mAliasUnit.reenableAllTriggers();
    mTimerUnit.reenableAllTriggers();
}

QPair<QString, QString> Host::getSearchEngine()
{
    if(mSearchEngineData.contains(mSearchEngineName))
        return qMakePair(mSearchEngineName, mSearchEngineData.value(mSearchEngineName));
    else
        return qMakePair(QStringLiteral("Google"), mSearchEngineData.value(QStringLiteral("Google")));
}

void Host::send(QString cmd, bool wantPrint, bool dontExpandAliases)
{
    if (wantPrint && mPrintCommand) {
        mInsertedMissingLF = true;
        if ((cmd == "") && (mUSE_IRE_DRIVER_BUGFIX) && (!mUSE_FORCE_LF_AFTER_PROMPT)) {
            ;
        } else {
            // used to print the terminal <LF> that terminates a telnet command
            // this is important to get the cursor position right
            mpConsole->printCommand(cmd);
        }
        mpConsole->update();
    }
    QStringList commandList;
    if (!mCommandSeparator.isEmpty()) {
        commandList = cmd.split(QString(mCommandSeparator), QString::SkipEmptyParts);
    } else {
        // don't split command if the command separator is blank
        commandList << cmd;
    }

    if (!dontExpandAliases) {
        // allow sending blank commands
        if (commandList.size() == 0) {
            sendRaw("\n");
            return;
        }
    }
    for (int i = 0; i < commandList.size(); i++) {
        if (commandList[i].size() < 1) {
            continue;
        }
        QString command = commandList[i];
        command.remove(QChar::LineFeed);
        if (dontExpandAliases) {
            mTelnet.sendData(command);
            continue;
        }

        if (!mAliasUnit.processDataStream(command)) {
            mTelnet.sendData(command);
        }
    }
}

void Host::sendRaw(QString command)
{
    mTelnet.sendData(command);
}

int Host::createStopWatch()
{
    int newWatchID = mStopWatchMap.size() + 1;
    mStopWatchMap[newWatchID] = QTime(0, 0, 0, 0);
    return newWatchID;
}

double Host::getStopWatchTime(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        return static_cast<double>(mStopWatchMap[watchID].elapsed()) / 1000;
    } else {
        return -1.0;
    }
}

bool Host::startStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        mStopWatchMap[watchID].start();
        return true;
    } else {
        return false;
    }
}

double Host::stopStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        return static_cast<double>(mStopWatchMap[watchID].elapsed()) / 1000;
    } else {
        return -1.0;
    }
}

bool Host::resetStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        mStopWatchMap[watchID].setHMS(0, 0, 0, 0);
        return true;
    } else {
        return false;
    }
}

void Host::callEventHandlers()
{
}

void Host::incomingStreamProcessor(const QString& data, int line)
{
    mTriggerUnit.processDataStream(data, line);

    mTimerUnit.doCleanup();
    if (mResetProfile) {
        resetProfile();
    }
}

void Host::registerEventHandler(const QString& name, TScript* pScript)
{
    if (mEventHandlerMap.contains(name)) {
        if (!mEventHandlerMap[name].contains(pScript)) {
            mEventHandlerMap[name].append(pScript);
        }
    } else {
        QList<TScript*> scriptList;
        scriptList.append(pScript);
        mEventHandlerMap.insert(name, scriptList);
    }
}
void Host::registerAnonymousEventHandler(const QString& name, const QString& fun)
{
    if (mAnonymousEventHandlerFunctions.contains(name)) {
        if (!mAnonymousEventHandlerFunctions[name].contains(fun)) {
            mAnonymousEventHandlerFunctions[name].push_back(fun);
        }
    } else {
        QStringList newList;
        newList << fun;
        mAnonymousEventHandlerFunctions[name] = newList;
    }
}

void Host::unregisterEventHandler(const QString& name, TScript* pScript)
{
    if (mEventHandlerMap.contains(name)) {
        mEventHandlerMap[name].removeAll(pScript);
    }
}

void Host::raiseEvent(const TEvent& pE)
{
    if (pE.mArgumentList.isEmpty()) {
        return;
    }

    if (mEventHandlerMap.contains(pE.mArgumentList.at(0))) {
        QList<TScript*> scriptList = mEventHandlerMap.value(pE.mArgumentList.at(0));
        for (auto& script : scriptList) {
            script->callEventHandler(pE);
        }
    }

    if (mAnonymousEventHandlerFunctions.contains(pE.mArgumentList.at(0))) {
        QStringList functionsList = mAnonymousEventHandlerFunctions.value(pE.mArgumentList.at(0));
        for (int i = 0, total = functionsList.size(); i < total; ++i) {
            mLuaInterpreter.callEventHandler(functionsList.at(i), pE);
        }
    }
}

void Host::postIrcMessage(const QString& a, const QString& b, const QString& c)
{
    TEvent event;
    event.mArgumentList << QLatin1String("sysIrcMessage");
    event.mArgumentList << a << b << c;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    raiseEvent(event);
}

void Host::enableTimer(const QString& name)
{
    mTimerUnit.enableTimer(name);
}

void Host::disableTimer(const QString& name)
{
    mTimerUnit.disableTimer(name);
}

bool Host::killTimer(const QString& name)
{
    return mTimerUnit.killTimer(name);
}

void Host::enableKey(const QString& name)
{
    mKeyUnit.enableKey(name);
}

void Host::disableKey(const QString& name)
{
    mKeyUnit.disableKey(name);
}


void Host::enableTrigger(const QString& name)
{
    mTriggerUnit.enableTrigger(name);
}

void Host::disableTrigger(const QString& name)
{
    mTriggerUnit.disableTrigger(name);
}

bool Host::killTrigger(const QString& name)
{
    return mTriggerUnit.killTrigger(name);
}


void Host::connectToServer()
{
    mTelnet.connectIt(mUrl, mPort);
}

void Host::closingDown()
{
    QMutexLocker locker(&mLock);
    mIsClosingDown = true;
}

bool Host::isClosingDown()
{
    QMutexLocker locker(&mLock);
    return mIsClosingDown;
}

bool Host::installPackage(const QString& fileName, const int module, QString* pErrorMessage, const bool isToSync)
{
    // As the pointed to dialog is only used now WITHIN this method and this
    // method can be re-entered, it is best to use a local rather than a class
    // pointer just in case we accidently reenter this method in the future.
    QDialog* pUnzipDialog = Q_NULLPTR;

    /*
     * Module notes:
     *
     * A "module" value of:
     * 0 - is a package
     * 1 - means a module is being installed for the first time via the UI
     * 2 - means the module is being synced (so it is "installed" already NOT
     *     that it has been set to be saved when the profile is saved)
     * 3 - means the module is being installed from a script.
     *
     * This separation is necessary to be able to reuse code while avoiding
     * infinite loops from script installations.
     */

    if (fileName.isEmpty()) {
        // A lua invocation will already have detected an empty string so no
        // need to generate a string to put into pErrorMessage
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        if (module) {
            if (pErrorMessage) {
                // Lua system will fill in details about the file...
                *pErrorMessage = QLatin1String("module file not found or is not readable");
            } else {
                postMessage(tr("[ ERROR ] - Installation of \"%1\",\n"
                               "a module file, failed.  Reason: file not found or is not readable.")
                            .arg(fileName));
            }
        } else {
            if (pErrorMessage) {
                // Lua system will fill in details about the file...
                *pErrorMessage = QLatin1String("package file not found or is not readable");
            } else {
                postMessage(tr("[ ERROR ] - Installation of \"%1\",\n"
                               "a package file, failed.  Reason: file not found or is not readable.")
                            .arg(fileName));
            }
        }
        return false;
    }

    QString packageName = fileName.section(QStringLiteral("/"), -1);
    packageName.remove(QStringLiteral(".trigger"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".xml"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".zip"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".mpackage"), Qt::CaseInsensitive);
    packageName.remove(QLatin1Char('\\'));
    packageName.remove(QLatin1Char('.'));

    switch(module) {
    case 3:
        if (mActiveModules.contains(packageName)) {
            // Lua script driven module install - so report back but bail out early
            if (pErrorMessage) {
                *pErrorMessage = QLatin1String("nothing to do, module already installed");
            }
            return true;
        }
        break;
    case 2:
        if (mActiveModules.contains(packageName)) {
            // CHECK: Module sync, not sure we could get any errors and even
            // if we did whether we SHOULD report them back to the main
            // console (as pErrorMessage will be a nullptr)...
            uninstallPackage(packageName, 2, pErrorMessage);
        }
        break;
    case 0:
        if (mInstalledPackages.contains(packageName)) {
            // This is hit by a call from TLuaInterpreter::installPackage(...)
            // OR from the toolbar button/menu action with an existing package
            // already loaded and it could be helpful to produce a Lua message for the
            // first of these even though we report it was successful...
            if (pErrorMessage) {
                *pErrorMessage = QLatin1String("nothing to do, package already installed");
            }
            return true;
        }
        break;
    default:
        // Includes case 1:
        ; //No-op
    }

    //the extra module check is needed here to prevent infinite loops from script loaded modules
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }

    QFile file2;
    if (fileName.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive) || fileName.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)) {
        QString _home = mudlet::getMudletPath(mudlet::profileHomePath, getName());
        QString _dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
        // home directory for the PROFILE
        QDir _tmpDir(_home);
        // directory to store the expanded archive file contents
        _tmpDir.mkpath(_dest);

        // TODO: report failure to create destination folder for package/module in profile

        QUiLoader loader(this);
        QFile uiFile(QStringLiteral(":/ui/package_manager_unpack.ui"));
        uiFile.open(QFile::ReadOnly);
        pUnzipDialog = dynamic_cast<QDialog*>(loader.load(&uiFile, nullptr));
        uiFile.close();
        if (!pUnzipDialog) {
            if (pErrorMessage) {
                *pErrorMessage = QLatin1String("internal error (null pUnzipDialog), please report this to Mudlet developers");
            }
            return false;
        }

        QLabel* pLabel = pUnzipDialog->findChild<QLabel*>(QStringLiteral("label"));
        if (pLabel) {
            if (module) {
                pLabel->setText(tr("Unpacking module:\n\"%1\"\nplease wait...").arg(packageName));
            } else {
                pLabel->setText(tr("Unpacking package:\n\"%1\"\nplease wait...").arg(packageName));
            }
        }
        pUnzipDialog->hide(); // Must hide to change WindowModality
        pUnzipDialog->setWindowTitle(tr("Unpacking"));
        pUnzipDialog->setWindowModality(Qt::ApplicationModal);
        pUnzipDialog->show();
        qApp->processEvents();
        pUnzipDialog->raise();
        pUnzipDialog->repaint(); // Force a redraw
        qApp->processEvents();   // Try to ensure we are on top of any other dialogs and freshly drawn

        // Uses non-null pErrorMessage as a signal that it is being used for a
        // lua function and thus needs to supply a false last argument to unzip
        // to force a shorter, non-translated error message for that usage in
        // results.second:
        auto results = mudlet::unzip(fileName, _dest, _tmpDir, (!pErrorMessage));
        pUnzipDialog->deleteLater();
        pUnzipDialog = Q_NULLPTR;
        if (!results.first) {
            if (pErrorMessage) {
                *pErrorMessage = results.second;
            } else {
                postMessage(results.second);
            }
            return false;
        }

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir(_dest);
        // before we start importing xmls in, see if the config.lua manifest file exists
        // - if it does, update the packageName from it
        if (_dir.exists(QStringLiteral("config.lua"))) {
            // read in the new packageName from Lua. Should be expanded in future to whatever else config.lua will have
            readPackageConfig(_dir.absoluteFilePath(QStringLiteral("config.lua")), packageName);
            // now that the packageName changed, redo relevant checks to make sure it's still valid
            if (module) {
                if (mActiveModules.contains(packageName)) {
                    // We are uninstalling a module with the (potentially)
                    // different module (though called package) name from the
                    // config.lua name from the file - before we get to install
                    // a new version of the module.
                    // CHECK: this will potentially collect any error message
                    // from the uninstall process and will put them onto the
                    // main console or pass back a lua function - perhaps we
                    // should pass a dummy QString to suppress such things here?
                    uninstallPackage(packageName, 2, pErrorMessage);
                }
            } else {
                if (mInstalledPackages.contains(packageName)) {
                    // cleanup and quit if already installed
                    removeDir(_dir.absolutePath(), _dir.absolutePath());
                    if (pErrorMessage) {
                        *pErrorMessage = QLatin1String("nothing to do, package already installed");
                    }
                    // It has been decided NOT to say anything on the main
                    // console if this happens outside of the lua function call
                    // to load a package/module
                    return false;
                }
            }
            // continuing, so update the folder name on disk
            QString newpath(QStringLiteral("%1/%2/").arg(_home, packageName));
            _dir.rename(_dir.absolutePath(), newpath);
            _dir = QDir(newpath);
        }
        QStringList _filterList;
        _filterList << QStringLiteral("*.xml") << QStringLiteral("*.trigger");
        QFileInfoList entries = _dir.entryInfoList(_filterList, QDir::Files);

        // Preserve these details that might get overwritten by some XML files(?)
        QString profileName = getName();
        QString login = getLogin();
        QString pass = getPass();
        bool isError = false;
        QStringList errorMessagesList;

        // This will read in a collection of .xml or .trigger files in the (root)
        // of the archive ALL under the same module/path name, as modules is a
        // QMap those will all be group under the same entry, but for packages
        // each separate file will be appended to the mInstalledPackages list
        // - which may be surprising but may also get squashed down eventually
        // to a single entry - or not...?!?
        for (auto& entry : entries) {
            file2.setFileName(entry.absoluteFilePath());
            file2.open(QFile::ReadOnly | QFile::Text);
            XMLimport reader(this);
            if (module) {
                QStringList moduleData;
                moduleData << fileName;
                if (module == 3) {
                    moduleData << (isToSync ? QLatin1String("1") : QLatin1String("0"));
                } else {
                    moduleData << QLatin1String("0");
                }
                moduleData << QLatin1String("0"); // Add a default priority
                mInstalledModules[packageName] = moduleData;
                mActiveModules.append(packageName);
            } else {
                mInstalledPackages.append(packageName);
                mudlet::self()->refreshPackageManager(this);
            }

            QString thisErrorMessage;
            if (!reader.importPackage(&file2, packageName, module, &thisErrorMessage)) {
                // Note the error but do not give up on rest
                errorMessagesList << thisErrorMessage;
                isError = true;
            };
            file2.close();
        }

        // Restore any changed entries
        setName(profileName);
        setLogin(login);
        setPass(pass);

        if (isError && pErrorMessage) {
            *pErrorMessage = errorMessagesList.join(QLatin1String(",\n"));
            return false;
        }

    } else {
        file2.setFileName(fileName);
        file2.open(QFile::ReadOnly | QFile::Text);
        QString profileName = getName();
        QString login = getLogin();
        QString pass = getPass();
        XMLimport reader(this);
        if (module) {
            QStringList moduleData;
            moduleData << fileName;
            if (module == 3) {
                moduleData << (isToSync ? QLatin1String("1") : QLatin1String("0"));
            } else {
                moduleData << QLatin1String("0");
            }
            moduleData << QLatin1String("0"); // Add a default priority
            mInstalledModules[packageName] = moduleData;
            mActiveModules.append(packageName);
            mudlet::self()->refreshModuleManager(this);
        } else {
            mInstalledPackages.append(packageName);
            mudlet::self()->refreshPackageManager(this);
        }

        if (!reader.importPackage(&file2, packageName, module, pErrorMessage)) {
            // Oh dear, something went wrong...
            file2.close();
            setName(profileName);
            setLogin(login);
            setPass(pass);
            return false;
        }

        file2.close();
        setName(profileName);
        setLogin(login);
        setPass(pass);
    }

    if (mpEditorDialog) {
        mpEditorDialog->doCleanReset();
    }

    if (!module) {
        saveProfile();
    }
    // reorder permanent and temporary triggers: perm first, temp second
    mTriggerUnit.reorderTriggersAfterPackageImport();

    // raise 2 events - a generic one and a more detailed one to serve both
    // a simple need ("I just want the install event") and a more specific need
    // ("I specifically need to know when the module was synced")
    TEvent genericInstallEvent;
    genericInstallEvent.mArgumentList.append(QLatin1String("sysInstall"));
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericInstallEvent.mArgumentList.append(packageName);
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericInstallEvent);

    TEvent detailedInstallEvent;
    switch (module) {
    case 0:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysInstallPackage"));
        break;
    case 1:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysInstallModule"));
        break;
    case 2:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysSyncInstallModule"));
        break;
    case 3:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysLuaInstallModule"));
        break;
    default:
        Q_UNREACHABLE();
    }
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedInstallEvent.mArgumentList.append(packageName);
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedInstallEvent.mArgumentList.append(fileName);
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(detailedInstallEvent);

    return true;
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Host::removeDir(const QString& dirName, const QString& originalPath)
{
    bool result = true;
    QDir dir(dirName);
    if (dir.exists(dirName)) {
        Q_FOREACH (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            // prevent recursion outside of the original branch
            if (info.isDir() && info.absoluteFilePath().startsWith(originalPath)) {
                result = removeDir(info.absoluteFilePath(), originalPath);
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

// This may be called by installPackage(...) in that case however it will have
// module == 2 and in THAT situation it will NOT RE-invoke installPackage(...)
// again - Slysven
bool Host::uninstallPackage(const QString& packageName, int module, QString* pErrorMessage)
{
    //     As with the installPackage, the module codes are:
    //     0=package, 1=uninstall from dialog, 2=uninstall due to module syncing,
    //     3=uninstall from a script

    if (module) {
        if (!mInstalledModules.contains(packageName)) {
            if (pErrorMessage) {
                *pErrorMessage = QLatin1String("module is not installed");
            }
            return false;
        }
    } else {
        if (!mInstalledPackages.contains(packageName)) {
            if (pErrorMessage) {
                *pErrorMessage = QLatin1String("package is not installed");
            }
            return false;
        }
    }

    // raise 2 events - a generic one and a more detailed one to serve both
    // a simple need ("I just want the uninstall event") and a more specific need
    // ("I specifically need to know when the module was uninstalled via Lua")
    TEvent genericUninstallEvent;
    genericUninstallEvent.mArgumentList.append(QLatin1String("sysUninstall"));
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericUninstallEvent.mArgumentList.append(packageName);
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericUninstallEvent);

    TEvent detailedUninstallEvent;
    switch (module) {
    case 0:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysUninstallPackage"));
        break;
    case 1:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysUninstallModule"));
        break;
    case 2:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysSyncUninstallModule"));
        break;
    case 3:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysLuaUninstallModule"));
        break;
    default:
        Q_UNREACHABLE();
    }
    detailedUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedUninstallEvent.mArgumentList.append(packageName);
    detailedUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(detailedUninstallEvent);

    bool dualInstallations = mInstalledModules.contains(packageName) && mInstalledPackages.contains(packageName);

    //we check for the module=3 because if we reset the editor, we will re-execute the
    //module uninstall, thus creating an infinite loop.
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }

    mTriggerUnit.uninstall(packageName);
    mTimerUnit.uninstall(packageName);
    mAliasUnit.uninstall(packageName);
    mActionUnit.uninstall(packageName);
    mScriptUnit.uninstall(packageName);
    mKeyUnit.uninstall(packageName);
    if (module) {
        //if module == 2, this is a temporary uninstall for reloading so we exit here
        QStringList moduleData = mInstalledModules.value(packageName);
        mInstalledModules.remove(packageName);
        mActiveModules.removeAll(packageName);
        if (module == 2) {
            return true;
        }
        //if module == 1/3, we actually uninstall it.
        //reinstall the package if it shared a module name.  This is a kludge, but it's cleaner than adding extra arguments/etc imo
        if (dualInstallations) {
            // we're a dual install, reinstalling a (new ?) copy as a package as
            // well but first we have to take it out of the installed package
            // list so we don't get denied from installPackage:
            mInstalledPackages.removeAll(packageName);
            // Now we reinstall (overwrite?) the package version:
            // CHECK: Should we use a dummy error message to prevent any noise from this?
            installPackage(moduleData.at(0), 0, pErrorMessage);
        }
        mudlet::self()->refreshModuleManager(this);
    } else {
        // We are uninstalling a package with a given name
        mInstalledPackages.removeAll(packageName);
        if (dualInstallations) {
            // However the same item is ALSO present as a module and that must
            // be preserved, so note it's details in that mode
            QStringList moduleData = mInstalledModules.value(packageName);
            // And so we (re)install it as a module - which should not do
            // anything but seem to succeed
            // CHECK: Should we use a dummy error message to prevent any noise from this?
            installPackage(moduleData.at(0), 1, pErrorMessage);
            // restore the module edit flag (? - is that a reference to the
            // "isSynced" item at moduleData.at(2) ?)
            mInstalledModules[packageName] = moduleData;
        }
        mudlet::self()->refreshPackageManager(this);
    }

    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }

    getActionUnit()->updateToolbar();

    QString dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
    removeDir(dest, dest);
    saveProfile();
    //NOW we reset if we're uninstalling a module
    if (mpEditorDialog && module == 3) {
        mpEditorDialog->doCleanReset();
    }
    return true;
}

void Host::readPackageConfig(const QString& luaConfig, QString& packageName)
{
    QFile configFile(luaConfig);
    QStringList strings;
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        while (!in.atEnd()) {
            strings += in.readLine();
        }
    }

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    int error = luaL_loadstring(L, strings.join("\n").toUtf8().constData());

    if (!error) {
        error = lua_pcall(L, 0, 0, 0);
    }

    if (!error) {
        // for now, only read the mpackage parameter
        // would be nice to read author, save & version too later
        lua_getglobal(L, "mpackage");
        if (lua_isstring(L, -1)) {
            packageName = QString(lua_tostring(L, -1));
        }
        lua_pop(L, -1);
        lua_close(L);
        return;
    } else {
        // error
        std::string e = "no error message available from Lua";
        e = lua_tostring(L, -1);
        std::string reason;
        switch (error) {
        case 4:
            reason = "Out of memory";
            break;
        case 3:
            reason = "Syntax error";
            break;
        case 2:
            reason = "Runtime error";
            break;
        case 1:
            reason = "Yield error";
            break;
        default:
            reason = "Unknown error";
            break;
        }

        if (mudlet::debugMode) {
            qDebug() << reason.c_str() << " in config.lua:" << e.c_str();
        }
        // should print error to main display
        QString msg = QString("%1 in config.lua: %2\n").arg(reason.c_str(), e.c_str());
        mpConsole->printSystemMessage(msg);


        lua_pop(L, -1);
        lua_close(L);
    }
}

// Derived from the one in dlgConnectionProfile class - but it does not need a
// host name argument...
QPair<bool, QString> Host::writeProfileData(const QString& item, const QString& what)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        ofs << what;
        file.close();
    }

    if (file.error() == QFile::NoError) {
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, file.errorString());
    }
}

// Similar to the above, a convenience for reading profile data for this host.
QString Host::readProfileData(const QString& item)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    bool success = file.open(QIODevice::ReadOnly);
    QString ret;
    if (success) {
        QDataStream ifs(&file);
        ifs >> ret;
        file.close();
    }

    return ret;
}
