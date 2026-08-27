/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2023 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "THyperlinkVisibilityManager.h"
#include "TLabel.h"
#include "TMap.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TTextBox.h"
#include "TTextEdit.h"
#include "dlgMapper.h"
#include "mudlet.h"
#include "GifTracker.h"

#include <QDialog>
#include <QDockWidget>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QUiLoader>
#include <QScrollBar>
#include <QShortcut>
#include <QSizePolicy>
#include <QTextBoundaryFinder>
#include <QTextCodec>
#include <QPainter>
#include <QVideoWidget>


TMainConsole::TMainConsole(Host* pH, QWidget* parent)
: TConsole(pH, qsl("main"), TConsole::MainConsole, parent)
, mClipboard(pH)
, mLogFile(model().mLogFile)
, mLogFileName(model().mLogFileName)
, mLogStream(model().mLogStream)
, mLogToLogFile(model().mLogToLogFile)
{
    setFont(pH->getAndClearTempDisplayFont());

    // The log lifecycle runs core-side; announcing the change on this console
    // and re-labelling the log button are the only parts of it that need a view.
    connect(pH, &Host::signal_loggingAnnouncement, this, &TMainConsole::slot_loggingAnnouncement, Qt::UniqueConnection);
    connect(pH, &Host::signal_loggingStateChanged, this, &TMainConsole::slot_loggingStateChanged, Qt::UniqueConnection);

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
    // There is one window in which a command line's destroyed() handler is unsafe:
    // after this console's members - mSubCommandLineMap among them - have been
    // destroyed, but before ~QObject severs incoming connections. The only command
    // lines that can be destroyed inside it are the ones QWidget::~QWidget deletes,
    // i.e. this console's own children, so sweeping those is enough. Command lines
    // created into a user window belong to a TDockWidget reparented onto the main
    // window instead, and can only die after ~QObject has already dropped the
    // connection. Children rather than map entries, because deleteCommandLine() and
    // resetMainConsole() drop the entry while the widget lives on until its
    // deferred delete is delivered.
    for (auto commandLine : findChildren<TCommandLine*>()) {
        disconnect(commandLine, &QObject::destroyed, this, nullptr);
    }
    mSubCommandLineMap.clear();

    // Neither is a child of this console: the map dock is reparented onto the main
    // window by addDockWidget(), and the unpacking dialog is parentless. So neither
    // dies with the console automatically.
    if (mpDockableMapWidget) {
        mpDockableMapWidget->deleteLater();
    }
    if (mpUnpackingDialog) {
        mpUnpackingDialog->deleteLater();
    }
    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
        mpHunspell_system = nullptr;
    }
    if (mpHunspell_profile) {
        Hunspell_destroy(mpHunspell_profile);
        mpHunspell_profile = nullptr;
        if (mudlet::self()) {
            // Need to commit any changes to personal dictionary
            qDebug() << "TCommandLine::~TConsole(...) INFO - Saving profile's own Hunspell dictionary...";
            mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(enums::profileDataItemPath, mProfileName, qsl("profile")), mWordSet_profile);
        }
    }
}

std::pair<bool, QString> TMainConsole::setLabelStyleSheet(const QString& name, const QString& stylesheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        pL->setStyleSheet(stylesheet);
        return {true, QString()};
    }
    return {false, qsl("label name '%1' not found").arg(name)};
}

std::optional<QString> TMainConsole::getLabelStyleSheet(const QString& name) const
{
    QMap<QString, TLabel*>::const_iterator const it = mLabelMap.constFind(name);
    if (it != mLabelMap.cend() && it.key() == name) {
        return it.value()->styleSheet();
    }

    return {};
}

std::optional<QSize> TMainConsole::getLabelSizeHint(const QString& name) const
{
    QMap<QString, TLabel*>::const_iterator const it = mLabelMap.constFind(name);
    if (it != mLabelMap.cend() && it.key() == name) {
        return it.value()->sizeHint();
    }

    return {};
}

std::optional<QString> TMainConsole::getLabelToolTip(const QString& name) const
{
    auto pL = mLabelMap.value(name);
    if (!pL) {
        return {};
    }

    return {pL->toolTip()};
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

std::optional<QString> TMainConsole::getUserWindowStyleSheet(const QString& name) const
{
    auto pW = mDockWidgetMap.value(name);
    if (!pW) {
        return {};
    }

    return {pW->styleSheet()};
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

std::optional<QString> TMainConsole::getCmdLineStyleSheet(const QString& name) const
{
    if (name.isEmpty() || !name.compare(qsl("main"))) {
        if (auto pMain = mpHost->mpConsole->mpCommandLine) {
            return {pMain->styleSheet()};
        }
        return {};
    }

    auto pN = mSubCommandLineMap.value(name);
    if (!pN) {
        return {};
    }

    return {pN->styleSheet()};
}

// The lifecycle itself is core (TConsoleModel::toggleLogging): this keeps the
// entry point the toolbar button and the Lua subsystem already call.
void TMainConsole::toggleLogging(bool isMessageEnabled)
{
    model().toggleLogging(isMessageEnabled);
}

void TMainConsole::slot_loggingAnnouncement(const bool isLogging, const QString& logFileName)
{
    const QString message = isLogging ? tr("Logging has started. Log file is %1").arg(logFileName) : tr("Logging has been stopped. Log file is %1").arg(logFileName);
    printSystemMessage(qsl("%1\n").arg(message));
}

void TMainConsole::slot_loggingStateChanged(const bool isLogging)
{
    logButton->setToolTip(utils::richText(isLogging ? tr("Stop logging game output to log file.") : tr("Start logging game output to log file.")));
}

void TMainConsole::selectCurrentLine(std::string& buf)
{
    const QString key = buf.c_str();
    if (key.isEmpty() || key == QLatin1String("main")) {
        TConsole::selectCurrentLine();
        return;
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        pC->selectCurrentLine();
    }
}

std::list<int> TMainConsole::getFgColor(QString& buf)
{
    if (buf.isEmpty() || buf == QLatin1String("main")) {
        return TConsole::getFgColor();
    }
    auto pC = mSubConsoleMap.value(buf);
    if (pC) {
        return pC->getFgColor();
    }

    return {};
}

std::list<int> TMainConsole::getBgColor(QString& buf)
{
    if (buf.isEmpty() || buf == QLatin1String("main")) {
        return TConsole::getBgColor();
    }
    auto pC = mSubConsoleMap.value(buf);
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

void TMainConsole::luaWrapLine(QString& buf, int line)
{
    if (buf.isEmpty() || buf == QLatin1String("main")) {
        TConsole::luaWrapLine(line);
        return;
    }
    auto pC = mSubConsoleMap.value(buf);
    if (pC) {
        pC->luaWrapLine(line);
    }
}

QString TMainConsole::getCurrentLine(const std::string& buf)
{
    const QString key = buf.c_str();
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
        auto pC = new TConsole(mpHost, name, Buffer);
        mSubConsoleMap[name] = pC;
        pC->setContentsMargins(0, 0, 0, 0);
        pC->hide();
        pC->layerCommandLine->hide();
        return pC;
    }

    return nullptr;
}

void TMainConsole::resetMainConsole()
{
    // Delete DockWidgets first — their child UserWindow TConsoles will be
    // cascade-deleted by Qt's parent-child ownership. Remove the corresponding
    // TConsole entries from mSubConsoleMap to avoid dangling pointers.
    QMutableMapIterator<QString, TDockWidget*> itDockWidget(mDockWidgetMap);
    while (itDockWidget.hasNext()) {
        itDockWidget.next();
        mSubConsoleMap.remove(itDockWidget.key());
        itDockWidget.value()->deleteLater();
        itDockWidget.remove();
    }

    const QList<TCommandLine*> commandLines = mSubCommandLineMap.values();
    for (auto commandLine : commandLines) {
        deregisterSubCommandLine(commandLine);
        commandLine->deleteLater();
    }

    // Remaining SubConsole/Buffer entries (UserWindow ones were already removed above)
    QMutableMapIterator<QString, TConsole*> itSubConsole(mSubConsoleMap);
    while (itSubConsole.hasNext()) {
        itSubConsole.next();
        itSubConsole.value()->deleteLater();
        itSubConsole.remove();
    }

    QMutableMapIterator<QString, TLabel*> itLabel(mLabelMap);
    while (itLabel.hasNext()) {
        itLabel.next();
        if (itLabel.value()->mpMovie) {
            mpHost->getGifTracker()->unregisterGif(itLabel.value()->mpMovie);
        }
        itLabel.value()->deleteLater();
        itLabel.remove();
    }

    QMutableMapIterator<QString, TScrollBox*> itScrollBox(mScrollBoxMap);
    while (itScrollBox.hasNext()) {
        itScrollBox.next();
        itScrollBox.value()->deleteLater();
        itScrollBox.remove();
    }

    QMutableMapIterator<QString, TTextBox*> itTextBox(mTextBoxMap);
    while (itTextBox.hasNext()) {
        itTextBox.next();
        itTextBox.value()->deleteLater();
        itTextBox.remove();
    }
}

// This is a sub-console overlaid on to the main or other console
TConsole* TMainConsole::createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    //if pW then add Console as Overlay to the Userwindow
    auto pW = mDockWidgetMap.value(windowname);
    auto pC = mSubConsoleMap.value(name);
    auto pS = mScrollBoxMap.value(windowname);
    if (!pC) {
        if (pS) {
            pC = new TConsole(mpHost, name, SubConsole, pS->widget());
        } else if (pW) {
            pC = new TConsole(mpHost, name, SubConsole, pW->widget());
        } else {
            pC = new TConsole(mpHost, name, SubConsole, mpMainFrame);
        }
        if (!pC) {
            return nullptr;
        }
        mSubConsoleMap[name] = pC;
        pC->setObjectName(name);
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
    }
    return nullptr;
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
            pL = new TLabel(mpHost, name, pW->widget());
        } else if (pS) {
            pL = new TLabel(mpHost, name, pS->widget());
        } else {
            pL = new TLabel(mpHost, name, mpMainFrame);
        }
        mLabelMap[name] = pL;
        pL->setAutoFillBackground(fillBackground);
        pL->setClickThrough(clickThrough);
        pL->resize(width, height);
        pL->setContentsMargins(0, 0, 0, 0);
        pL->move(x, y);
        pL->show();
        // fillBackground = 0 gets this grey too, which is not what the argument reads
        // like: honouring it would turn every such label in an installed script
        // transparent. What the argument does decide is what survives a later
        // stylesheet - setAutoFillBackground(false) above means the colour is dropped
        // rather than kept. A script wanting a transparent label has to say so with
        // setBackgroundColor(name, 0, 0, 0, 0).
        mpHost->setBackgroundColor(name, 32, 32, 32, 255);
        return pL;
    }

    return nullptr;
}

std::pair<bool, QString> TMainConsole::deleteLabel(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.take(name);
    if (pL) {
        if (pL->mpMovie) {
            mpHost->getGifTracker()->unregisterGif(pL->mpMovie);
        }

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

std::pair<bool, QString> TMainConsole::deleteMiniConsole(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a miniconsole cannot have an empty string as its name")};
    }

    auto pConsole = mSubConsoleMap.take(name);
    if (pConsole) {
        mCachedWindowSizes.remove(name);

        // A UserWindow's TConsole lives *inside* a TDockWidget. Deleting only the
        // console (as for an ordinary miniconsole) leaves the dock orphaned in
        // mDockWidgetMap with a now-null widget(), which later crashes - e.g. in
        // getUserWindowSize() - when a window of the same name is recreated. Tear
        // the dock down too; it owns the console as its child widget and deletes
        // it along with itself (mirrors the shutdown path in TConsole::closeEvent).
        if (pConsole->getType() == TConsole::UserWindow) {
            if (auto pDock = mDockWidgetMap.take(name)) {
                // deleteLater() alone is sufficient: the console is the dock's
                // child widget, so destroying the dock destroys the console with
                // it. (No WA_DeleteOnClose - we delete programmatically here, not
                // in response to a close event.)
                pDock->deleteLater();
            } else {
                pConsole->deleteLater();
            }
        } else {
            // Using deleteLater() rather than delete as it seems a safer option
            // given that this item is likely to be linked to some events and
            // suchlike:
            pConsole->deleteLater();
        }

        // It remains to be seen if the miniconsole has "gone" as a result of the
        // above by the time the Lua subsystem processes the following:
        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysMiniConsoleDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, qsl("miniconsole name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::deleteCommandLine(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a command line cannot have an empty string as its name")};
    }

    auto pCmdLine = mSubCommandLineMap.value(name);
    if (pCmdLine) {
        // Deregister rather than just take() the entry: the widget outlives this
        // call until its deferred delete is delivered, and its destroyed() handler
        // must not be left armed for a console that may be gone by then.
        deregisterSubCommandLine(pCmdLine);
        // Using deleteLater() rather than delete as it seems a safer option
        // given that this item is likely to be linked to some events and
        // suchlike:
        pCmdLine->deleteLater();

        // It remains to be seen if the command line has "gone" as a result of the
        // above by the time the Lua subsystem processes the following:
        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysCommandLineDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, qsl("command line name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::deleteTextBox(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a text edit cannot have an empty string as its name")};
    }

    auto pTextBox = mTextBoxMap.take(name);
    if (pTextBox) {
        pTextBox->deleteLater();

        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysTextEditDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    return {false, qsl("text edit name '%1' not found").arg(name)};
}

std::pair<bool, QString> TMainConsole::deleteScrollBox(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a scrollbox cannot have an empty string as its name")};
    }

    auto pScrollBox = mScrollBoxMap.take(name);
    if (pScrollBox) {
        // Using deleteLater() rather than delete as it seems a safer option
        // given that this item is likely to be linked to some events and
        // suchlike:
        pScrollBox->deleteLater();

        // It remains to be seen if the scrollbox has "gone" as a result of the
        // above by the time the Lua subsystem processes the following:
        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysScrollBoxDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, qsl("scrollbox name '%1' not found").arg(name)};
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
        const QPixmap cursor_pixmap = QPixmap(pixMapLocation);
        if (cursor_pixmap.isNull()) {
            return {false, qsl("couldn't find custom cursor, is the location \"%1\" correct?").arg(pixMapLocation)};
        }
        const QCursor custom_cursor = QCursor(cursor_pixmap, hotX, hotY);
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
    auto pM = mpDockableMapWidget;
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

        if (mpHost->mpMap->mpRoomDB->isEmpty()) {
            // Don't load a map if we already have one around!
            qDebug() << "TConsole::createMapper() - restore map case 2.";
            mpHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(2) report"), true);
            const QDateTime now(QDateTime::currentDateTime());

            if (mpHost->mpMap->restore(QString())) {
                mpHost->mpMap->audit();
                mpMapper->mp2dMap->init();
                mpMapper->updateAreaComboBox();
                mpMapper->resetAreaComboBoxToPlayerRoomArea();
                mpMapper->show();
            }

            mpHost->mpMap->pushErrorMessagesToFile(tr("Loading map(2) at %1 report").arg(now.toString(Qt::ISODate)), true);
        } else {
            mpMapper->updateAreaComboBox();
            mpMapper->resetAreaComboBoxToPlayerRoomArea();
        }

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
#if defined(Q_OS_WINDOWS)
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
            pN = new TCommandLine(mpHost, name, TCommandLine::SubCommandLine, this, pS->widget());
        } else if (pW) {
            pN = new TCommandLine(mpHost, name, TCommandLine::SubCommandLine, this, pW->widget());
        } else {
            pN = new TCommandLine(mpHost, name, TCommandLine::SubCommandLine, this, mpMainFrame);
        }
        registerSubCommandLine(name, pN);
        pN->resize(width, height);
        pN->move(x, y);
        pN->show();
        return {true, QString()};
    }
    return {false, QLatin1String("couldn't create commandLine")};
}

void TMainConsole::registerSubCommandLine(const QString& name, TCommandLine* pCommandLine)
{
    if (auto pDisplaced = mSubCommandLineMap.value(name); pDisplaced && pDisplaced != pCommandLine) {
        // Would otherwise be left connected but unreachable by name
        deregisterSubCommandLine(pDisplaced);
    }
    mSubCommandLineMap[name] = pCommandLine;

    // A TCommandLine is always a child widget of something else - the miniconsole
    // it is embedded in, or the user window / scroll box it was created into - so
    // it can be destroyed without deleteCommandLine() ever being called, and this
    // map does not hold QPointers. Without this the entry outlives the widget and
    // every later lookup of the name reads freed memory; TConsole::setFont() walks
    // the whole map, so even changing the display font in Preferences hits it.
    connect(pCommandLine, &QObject::destroyed, this, [this, pCommandLine]() {
        deregisterSubCommandLine(pCommandLine);
    });
}

void TMainConsole::deregisterSubCommandLine(TCommandLine* pCommandLine)
{
    // This is the only destroyed() connection made from a command line to this
    // console, so severing all of them is severing just that one.
    disconnect(pCommandLine, &QObject::destroyed, this, nullptr);
    // Erase by value rather than by name: a replacement command line may have been
    // registered under the same name in the meantime and must be left in place.
    mSubCommandLineMap.removeIf([pCommandLine](const auto& it) {
        return it.value() == pCommandLine;
    });
}

std::pair<bool, QString> TMainConsole::createTextBox(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a text edit cannot have an empty string as its name")};
    }

    auto pT = mTextBoxMap.value(name);
    auto pW = mDockWidgetMap.value(windowname);
    auto pS = mScrollBoxMap.value(windowname);

    if (!pT) {
        if (pS) {
            pT = new TTextBox(mpHost, name, pS->widget());
        } else if (pW) {
            pT = new TTextBox(mpHost, name, pW->widget());
        } else {
            pT = new TTextBox(mpHost, name, mpMainFrame);
        }
        mTextBoxMap[name] = pT;
        pT->resize(width, height);
        pT->move(x, y);
        pT->show();
        return {true, QString()};
    }
    return {false, QLatin1String("couldn't create text edit")};
}

bool TMainConsole::setBackgroundImage(const QString& name, const QString& path)
{
    auto pL = mLabelMap.value(name);
    if (pL) {
        pL->clearSvgImage();
        if (path.endsWith(qsl(".svg"), Qt::CaseInsensitive) || path.endsWith(qsl(".svgz"), Qt::CaseInsensitive)) {
            return pL->setSvgImage(path);
        }
        pL->setPixmap(QPixmap(path));
        return true;
    }
    return false;
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
    }
    if (pL) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pL->setPalette(mainPalette);
        return true;
    }
    return false;
}

bool TMainConsole::raiseWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    auto pM = mpMapper;
    auto pN = mSubCommandLineMap.value(name);
    auto pS = mScrollBoxMap.value(name);
    auto pT = mTextBoxMap.value(name);

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
    if (pT) {
        pT->raise();
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
    auto pT = mTextBoxMap.value(name);

    if (pC) {
        pC->lower();
        lowerMainDisplay();
        return true;
    }
    if (pL) {
        pL->lower();
        lowerMainDisplay();
        return true;
    }
    if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->lower();
        lowerMainDisplay();
        return true;
    }
    if (pS) {
        pS->lower();
        lowerMainDisplay();
        return true;
    }
    if (pN) {
        pN->lower();
        lowerMainDisplay();
        return true;
    }
    if (pT) {
        pT->lower();
        lowerMainDisplay();
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
    }
    if (pL) {
        pL->show();
        return true;
    }
    return false;
}

bool TMainConsole::hideWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->hide();
        return true;
    }
    if (pL) {
        pL->hide();
        return true;
    }
    return false;
}

bool TMainConsole::printWindow(const QString& name, const QString& text)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->print(text);
        return true;
    }
    if (pL) {
        pL->setText(text);
        return true;
    }
    return false;
}

//getUserWindowSize for resizing in Geyser
QSize TMainConsole::getUserWindowSize(const QString& windowname) const
{
    auto pW = mDockWidgetMap.value(windowname);

    // Guard pW->widget(): a dock can briefly outlive its console (the console is
    // deleted via deleteLater()), during which widget() is null - dereferencing
    // it segfaults. Fall back to the main window size until the dock is gone.
    if (pW && pW->widget()) {
        const QSize windowSize = pW->widget()->size();
        const int minValidWidth = 50;

        // Mid-switch the dock can be a few pixels wide, which nothing can be laid
        // out in - hand back the last size it really had. Only a size this small
        // is refused: refusing one that has merely changed a lot would leave the
        // cache as the yardstick for every size after it, and a window shrunk to
        // under half its width could never be reported again.
        if (windowSize.width() < minValidWidth) {
            if (mCachedWindowSizes.contains(windowname)) {
                return mCachedWindowSizes.value(windowname);
            }
            return windowSize;
        }

        // Size looks valid, cache and return it
        mCachedWindowSizes[windowname] = windowSize;
        return windowSize;
    }

    return getMainWindowSize();
}

QPair<bool, QString> TMainConsole::addWordToSet(const QString& word)
{
    const QString errMsg = qsl("the word \"%1\" already seems to be in the user dictionary");
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
    const QString errMsg = qsl("the word \"%1\" does not seem to be in the user dictionary");
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
    if (newDict.isEmpty() || mLoadedSystemDictionary == newDict) {
        return;
    }

    mLoadedSystemDictionary = newDict;

    // Everywhere but macOS getMudletPath() probes for "<name>.aff" to settle
    // which directory wins, so it has to get the same name the files are then
    // loaded by.
    const QString path = mudlet::getMudletPath(enums::hunspellDictionaryPath, newDict);
    QString spell_aff = qsl("%1%2.aff").arg(path, newDict);
    QString spell_dic = qsl("%1%2.dic").arg(path, newDict);

    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
    }

#if defined(Q_OS_WINDOWS)
    // strip non-ASCII characters from the path because hunspell can't handle them
    // when compiled with MinGW 7.3.0
    mudlet::self()->sanitizeUtf8Path(spell_aff, qsl("%1.aff").arg(newDict));
    mudlet::self()->sanitizeUtf8Path(spell_dic, qsl("%1.dic").arg(newDict));
#endif

    mpHunspell_system = Hunspell_create(spell_aff.toUtf8().constData(), spell_dic.toUtf8().constData());
    if (mpHunspell_system) {
        mHunspellCodecName_system = QByteArray(Hunspell_get_dic_encoding(mpHunspell_system));
        qDebug().noquote().nospace() << "TMainConsole::setSystemSpellDictionary(\"" << newDict << "\") INFO - System Hunspell dictionary loaded for profile, it uses a \""
                                     << Hunspell_get_dic_encoding(mpHunspell_system) << "\" encoding...";
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
            mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(enums::profileDataItemPath, mProfileName, qsl("profile")), mWordSet_profile);
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
    }
    return mudlet::self()->getWordSet();
}

void TMainConsole::setProfileName(const QString& newName)
{
    TConsole::setProfileName(newName);

    for (const auto pC : std::as_const(mSubConsoleMap)) {
        pC->setProfileName(newName);
    }
}

void TMainConsole::refreshSubconsoles()
{
    for (const auto pC : std::as_const(mSubConsoleMap)) {
        if (pC) {
            pC->refreshView();
        }
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

// The title is in .second when .first is true, otherwise .second is why there
// is none. Mirrors setUserWindowTitle's checks in the same order and words, so
// that a miniconsole sharing the name is not reported as a missing window.
std::pair<bool, QString> TMainConsole::getUserWindowTitle(const QString& name) const
{
    if (name.isEmpty()) {
        return {false, qsl("a user window cannot have an empty string as its name")};
    }

    auto pC = mSubConsoleMap.value(name);
    if (!pC) {
        return {false, qsl("user window name '%1' not found").arg(name)};
    }

    if (pC->getType() != UserWindow) {
        return {false, qsl("\"%1\" is not a user window").arg(name)};
    }

    auto pD = mDockWidgetMap.value(name);
    if (!pD) {
        return {false, qsl("internal error: TConsole \"%1\" is marked as a user window but does not have a TDockWidget to contain it").arg(name)};
    }

    return {true, pD->windowTitle()};
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
    Q_ASSERT_X(mpLineEdit_networkLatency, "TMainConsole::printOnDisplay(...)", "mpLineEdit_networkLatency does not point to a valid QLineEdit");
    mProcessingTimer.restart();

    // Notify visibility manager of incoming data (for output gap detection)
    if (isFromServer) {
        getHyperlinkVisibilityManager().onDataReceived();
    }

    // feedTriggers() lands here, so this runs nested inside an outer pass that
    // is itself mid-translate; clearing the flag outright would take trigger
    // context away from the rest of that pass.
    const bool wasInTriggerEngineMode = mTriggerEngineMode;
    mTriggerEngineMode = true;
    const int beforeTranslateLastLineNumber = buffer.getLastLineNumber();
    const auto beforeTranslateLastLine = buffer.line(beforeTranslateLastLineNumber - 1);
    buffer.translateToPlainText(incomingSocketData, isFromServer);
    mTriggerEngineMode = wasInTriggerEngineMode;

    const int lastLineNumber = buffer.getLastLineNumber();
    const bool bufferChanged = lastLineNumber != beforeTranslateLastLineNumber || buffer.line(lastLineNumber - 1) != beforeTranslateLastLine;
    if (mAlertOnNewData && isFromServer && bufferChanged) {
        QApplication::alert(mudlet::self(), 0);
    }

    // dequeues MXP events and raise them through the LuaInterpreter
    // TODO: move this somewhere else more appropriate
    auto& mxpEventQueue = mpHost->mMxpClient.mMxpEvents;
    while (!mxpEventQueue.isEmpty()) {
        const auto& event = mxpEventQueue.dequeue();
        mpHost->mLuaInterpreter.signalMXPEvent(event.name, event.attrs, event.actions, event.caption);
    }

    const double processT = mProcessingTimer.elapsed() / 1000.0;
    if (mpHost->mTelnet.mGA_Driver) {
        /*:
        The first argument 'N' represents the 'N'etwork latency; the second 'S' the
        'S'ystem (processing) time
        */
        mpLineEdit_networkLatency->setText(tr("N:%1 S:%2").arg(mpHost->mTelnet.networkLatencyTime, 0, 'f', 3).arg(processT, 0, 'f', 3));
    } else {
        /*:
        The argument 'S' represents the 'S'ystem (processing) time, in this situation
        the Game Server is not sending \"GoAhead\" signals so we cannot deduce the
        network latency...
        */
        mpLineEdit_networkLatency->setText(tr("<no GA> S:%1").arg(processT, 0, 'f', 3));
    }
    // Modify the tab text if this is not the currently active host - this
    // method is only used on the "main" console so no need to filter depending
    // on TConsole types:

    emit signal_newDataAlert(mProfileName);
}

void TMainConsole::finalize()
{
    if (mUpperPane) {
        mUpperPane->showNewLines();
    }
    if (mLowerPane) {
        mLowerPane->showNewLines();
    }
}

// TODO: It may be worth considering moving the (now) three following methods
// to the TMap class...?
bool TMainConsole::saveMap(const QString& location, int saveVersion)
{
    QString filename_map = location;
    if (filename_map.isEmpty()) {
        filename_map = mudlet::getMudletPath(enums::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
    } else if (const QFileInfo fileInfo(location); fileInfo.isRelative()) {
        // Resolve the name relative to the profile home directory the way
        // TMainConsole::importMap does, rather than against whatever directory
        // Mudlet happens to have been started in:
        filename_map = QDir::cleanPath(mudlet::getMudletPath(enums::profileDataItemPath, mProfileName, fileInfo.filePath()));
    }

    const QDir dir_map(mudlet::getMudletPath(enums::profileMapsPath, mProfileName));
    if (!dir_map.exists() && !dir_map.mkpath(dir_map.path())) {
        qDebug().noquote() << "Error saving map: could not make the profile's map directory" << dir_map.path();
        return false;
    }

    QSaveFile file_map(filename_map);
    if (!file_map.open(QIODevice::WriteOnly)) {
        // Naming the file matters more than usual: a relative location is not
        // the path the caller typed
        qDebug().noquote() << "Error saving map to" << filename_map << ":" << file_map.errorString();
        return false;
    }

    QDataStream out(&file_map);
    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        out.setVersion(mudlet::scmQDataStreamFormat_5_12);
    }

    bool saved = mpHost->mpMap->serialize(out, saveVersion);
    if (saved && !file_map.commit()) {
        qDebug() << "Error saving map: " << (file_map.error() == QFile::NoError ? "issue with serializing" : file_map.errorString());
        saved = false;
    }

    if (saved) {
        mpHost->mpMap->resetUnsaved();
        mpHost->mpMap->setSaveError(false);
    } else {
        mpHost->mpMap->setSaveError(true);
    }

    return saved;
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

    // The same resolution saveMap and importMap use, so that a map written
    // under a bare name is looked for where it was written:
    QString filePathName = location;
    if (const QFileInfo fileInfo(location); !location.isEmpty() && fileInfo.isRelative()) {
        filePathName = QDir::cleanPath(mudlet::getMudletPath(enums::profileDataItemPath, mProfileName, fileInfo.filePath()));
    }

    qDebug() << "TMainConsole::loadMap() - restore map case 1.";
    pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(1) report"), true);
    const QDateTime now(QDateTime::currentDateTime());

    bool result = false;
    if (pHost->mpMap->restore(filePathName)) {
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

    if (filePathName.isEmpty()) {
        pHost->mpMap->pushErrorMessagesToFile(tr("Loading map(1) at %1 report").arg(now.toString(Qt::ISODate)), true);
    } else {
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Loading map(1) "%1" at %2 report)").arg(filePathName, now.toString(Qt::ISODate)), true);
    }

    pHost->mpMap->updateArea(-1);

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
    const QDateTime now(QDateTime::currentDateTime());

    bool result = false;

    const QFileInfo fileInfo(location);
    QString filePathNameString;
    if (!fileInfo.filePath().isEmpty()) {
        if (fileInfo.isRelative()) {
            // Resolve the name relative to the profile home directory:
            filePathNameString = QDir::cleanPath(mudlet::getMudletPath(enums::profileDataItemPath, mProfileName, fileInfo.filePath()));
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
            const QString infoMsg = tr("[ ERROR ]  - Map file not found, path and name used was:\n"
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
            const QString infoMsg = tr("[ INFO ]  - Map file located and opened, now parsing it...");
            pHost->postMessage(infoMsg);
        }

        result = pHost->mpMap->importMap(file, errMsg);

        file.close();
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Importing map(1) "%1" at %2 report)").arg(location, now.toString(Qt::ISODate)));
    } else {
        if (!errMsg) {
            const QString infoMsg = tr(R"([ INFO ]  - Map file located but it could not opened, please check permissions on:"%1".)").arg(filePathNameString);
            pHost->postMessage(infoMsg);
        } else {
            *errMsg = tr("loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" could not be opened for reading).")
                              .arg(filePathNameString);
        }
        return false;
    }

    pHost->mpMap->updateArea(-1);

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

    const QString infoMsg = tr("[ INFO ]  - Map reload request received from system...");
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

void TMainConsole::showPackageDownloadProgress(const QString& title, const QString& cancelText)
{
    auto pHost = getHost();
    if (!pHost) {
        qWarning() << "TMainConsole::showPackageDownloadProgress() WARNING - called with no host; ignoring the download-progress request.";
        return;
    }
    // A second server-triggered download can arrive mid-download (e.g. a
    // reconnect re-sends Client.GUI). QProgressDialog::close() emits canceled(),
    // so closing the superseded dialog while it is still wired to
    // slot_cancelPackageDownload() would abort the download this new dialog is
    // about to track; detach it before closing.
    if (mpPackageDownloadProgressDialog) {
        mpPackageDownloadProgressDialog->disconnect();
        mpPackageDownloadProgressDialog->close();
    }
    // placeholder range; reset by the first download-progress update
    mpPackageDownloadProgressDialog = new QProgressDialog(title, cancelText, 0, 4000000, this);
    connect(mpPackageDownloadProgressDialog, &QProgressDialog::canceled, &pHost->mTelnet, &cTelnet::slot_cancelPackageDownload);
    mpPackageDownloadProgressDialog->setAttribute(Qt::WA_DeleteOnClose);
    mpPackageDownloadProgressDialog->show();
}

void TMainConsole::updatePackageDownloadProgress(qint64 got, qint64 total)
{
    if (mpPackageDownloadProgressDialog) {
        // total is -1 for chunked HTTP responses of unknown length; a (0, 0)
        // range turns the dialog into a busy indicator
        mpPackageDownloadProgressDialog->setRange(0, total > 0 ? static_cast<int>(total) : 0);
        mpPackageDownloadProgressDialog->setValue(static_cast<int>(got));
    }
}

void TMainConsole::closePackageDownloadProgress()
{
    if (mpPackageDownloadProgressDialog) {
        mpPackageDownloadProgressDialog->close();
    }
}

void TMainConsole::createMapProgressDialog(const QString& title, const QString& label, const QString& cancelButtonText, int minimum, int maximum)
{
    if (mpMapProgressDialog) {
        mpMapProgressDialog->hide();
        mpMapProgressDialog->deleteLater();
    }
    auto pHost = getHost();
    // If canceled() cannot be wired to the map, omit the cancel button rather
    // than show one that does nothing.
    const bool cancelWirable = pHost && !pHost->mpMap.isNull();
    // Deliberately not WA_DeleteOnClose: the JSON import keeps updating this
    // dialog from a processEvents loop, so it must outlive a mid-operation
    // dismissal; we delete it explicitly instead.
    mpMapProgressDialog = new QProgressDialog(label, cancelWirable ? cancelButtonText : QString(), minimum, maximum, this);
    mpMapProgressDialog->setWindowTitle(title);
    mpMapProgressDialog->setWindowIcon(QIcon(qsl(":/icons/mudlet_map_download.png")));
    mpMapProgressDialog->setAutoClose(false);
    mpMapProgressDialog->setAutoReset(false);
    // QProgressDialog still emits canceled() on Escape or window-close even with
    // no cancel button, so only connect it when the operation is cancelable;
    // otherwise a non-cancelable import could be aborted by a spurious cancel.
    if (cancelWirable && !cancelButtonText.isEmpty()) {
        connect(mpMapProgressDialog, &QProgressDialog::canceled, pHost->mpMap.data(), &TMap::slot_mapProgressDialogCancelled);
    }
}

void TMainConsole::showMapTransferProgress(const QString& title, const QString& label, const QString& cancelButtonText)
{
    createMapProgressDialog(title, label, cancelButtonText, 0, 0);
    mpMapProgressDialog->setMinimumWidth(300);
    mpMapProgressDialog->setMinimumDuration(0);
    mpMapProgressDialog->show();
}

void TMainConsole::showMapJsonProgress(const QString& title, const QString& label, const QString& cancelButtonText, int maximum)
{
    createMapProgressDialog(title, label, cancelButtonText, 0, maximum);
    mpMapProgressDialog->setWindowModality(Qt::NonModal);
    mpMapProgressDialog->setMinimumWidth(500);
    mpMapProgressDialog->setMinimumDuration(1);
}

void TMainConsole::setMapProgressDialogLabel(const QString& text)
{
    if (mpMapProgressDialog) {
        mpMapProgressDialog->setLabelText(text);
    }
}

void TMainConsole::setMapProgressDialogRange(int minimum, int maximum)
{
    if (mpMapProgressDialog) {
        mpMapProgressDialog->setRange(minimum, maximum);
    }
}

void TMainConsole::setMapProgressDialogValue(int value)
{
    if (mpMapProgressDialog) {
        mpMapProgressDialog->setValue(value);
    }
}

void TMainConsole::disableMapProgressDialogCancel()
{
    if (mpMapProgressDialog) {
        // Taking the button away does not stop a window-close from emitting
        // canceled(), so drop the connection as well - by this point the
        // operation can no longer be stopped. Only ours goes, leaving
        // QProgressDialog's own canceled() -> cancel() wiring intact.
        if (auto pHost = getHost(); pHost && !pHost->mpMap.isNull()) {
            disconnect(mpMapProgressDialog, &QProgressDialog::canceled, pHost->mpMap.data(), &TMap::slot_mapProgressDialogCancelled);
        }
        mpMapProgressDialog->setCancelButton(nullptr);
    }
}

void TMainConsole::closeMapProgressDialog()
{
    if (mpMapProgressDialog) {
        // hide() rather than close() so we don't re-enter QProgressDialog's
        // closeEvent -> cancel() while a cancel is already being handled.
        mpMapProgressDialog->hide();
        mpMapProgressDialog->deleteLater();
        // deleteLater() leaves the QPointer set until the event loop gets to
        // run, which a synchronous JSON operation will not let it do, so forget
        // the dialog now and make any late writes to it no-ops.
        mpMapProgressDialog = nullptr;
    }
}

void TMainConsole::createMapperDock(const QString& title, const QString& objectName)
{
    mpDockableMapWidget = new QDockWidget(title);
    mpDockableMapWidget->setObjectName(objectName);
}

void TMainConsole::showMapperScriptReminder()
{
    QUiLoader loader;
    QFile file(qsl(":/ui/lacking_mapper_script.ui"));
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "TMainConsole::showMapperScriptReminder() WARNING - failed to open lacking_mapper_script.ui for reading:" << file.errorString();
        return;
    }

    auto dialog = qobject_cast<QDialog*>(loader.load(&file, mudlet::self()));
    file.close();
    if (!dialog) {
        qWarning() << "TMainConsole::showMapperScriptReminder() WARNING - could not load the mapping-script reminder dialog.";
        return;
    }

    connect(dialog, &QDialog::accepted, mudlet::self(), &mudlet::slot_openMappingScriptsPage);

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void TMainConsole::showUnpackingProgress(const QString& message, const QString& title)
{
    // deleteLater() not close(): the dialog is parentless with no WA_DeleteOnClose,
    // so closing it would leak it once we overwrite the pointer below.
    if (mpUnpackingDialog) {
        mpUnpackingDialog->deleteLater();
    }

    QUiLoader loader;
    QFile uiFile(qsl(":/ui/package_manager_unpack.ui"));
    if (!uiFile.open(QFile::ReadOnly)) {
        qWarning() << "TMainConsole::showUnpackingProgress() WARNING - failed to open package_manager_unpack.ui for reading:" << uiFile.errorString();
        return;
    }
    auto* pDialog = qobject_cast<QDialog*>(loader.load(&uiFile, nullptr));
    uiFile.close();
    if (!pDialog) {
        qWarning() << "TMainConsole::showUnpackingProgress() WARNING - could not load the unpacking progress dialog.";
        return;
    }
    mpUnpackingDialog = pDialog;

    // Trap: processEvents() below can deliver a re-entrant install (or its
    // matching hide) that replaces or clears mpUnpackingDialog and disposes of
    // this frame's dialog. Drive a local pointer, never the member, and bail if
    // our dialog is taken out from under us.
    QPointer<QDialog> dialog = pDialog;

    if (auto* pLabel = dialog->findChild<QLabel*>(qsl("label"))) {
        pLabel->setText(message);
    }
    dialog->hide(); // Must hide to change WindowModality
    dialog->setWindowTitle(title);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();
    QCoreApplication::processEvents();
    if (!dialog) {
        return;
    }
    dialog->raise();
    dialog->repaint();                 // Force a redraw
    QCoreApplication::processEvents(); // Try to ensure we are on top of any other dialogs and freshly drawn
}

void TMainConsole::closeUnpackingProgress()
{
    if (mpUnpackingDialog) {
        mpUnpackingDialog->deleteLater();
        mpUnpackingDialog = nullptr;
    }
}

void TMainConsole::setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded)
{
    setupSucceeded = false;

    if (!player || !player->mediaPlayer()) {
        return;
    }

    const QString target = player->mediaData().mediaKey();

    if (target.isEmpty()) {
        qWarning() << qsl("TMainConsole::setupVideoOutput() ERROR - 'key' not specified for video.");
        return;
    }

    QString widgetType = TMediaData::MediaWidgetLabel;
    QWidget* targetWidget = mLabelMap.value(target);

    if (!targetWidget) {
        targetWidget = mSubConsoleMap.value(target);
        if (targetWidget) {
            widgetType = TMediaData::MediaWidgetWindow;
        }
    }

    if (!targetWidget) {
        qWarning() << qsl("TMainConsole::setupVideoOutput() ERROR - No matching widget for 'key' = %1 to present video.").arg(target);
        return;
    }

    player->mediaData().setMediaWidget(widgetType);

    QVideoWidget* myVideoWidget = nullptr;
    if (widgetType == TMediaData::MediaWidgetLabel) {
        myVideoWidget = qobject_cast<TLabel*>(targetWidget)->mpVideoWidget;
    } else if (widgetType == TMediaData::MediaWidgetWindow) {
        myVideoWidget = qobject_cast<TConsole*>(targetWidget)->mpVideoWidget;
    }

    if (!myVideoWidget) {
        myVideoWidget = new QVideoWidget();
        myVideoWidget->setParent(targetWidget);
        myVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (widgetType == TMediaData::MediaWidgetLabel) {
            QObject::connect(qobject_cast<TLabel*>(targetWidget), &TLabel::resized, myVideoWidget, [targetWidget, myVideoWidget]() {
                myVideoWidget->resize(targetWidget->size());
            });
        } else if (widgetType == TMediaData::MediaWidgetWindow) {
            QObject::connect(qobject_cast<TConsole*>(targetWidget), &TConsole::resized, myVideoWidget, [targetWidget, myVideoWidget]() {
                myVideoWidget->resize(targetWidget->size());
            });
        }
    }

    if (targetWidget->isHidden()) {
        targetWidget->show();
    }

    myVideoWidget->resize(targetWidget->size());
    player->mediaPlayer()->setVideoOutput(myVideoWidget);
    myVideoWidget->show();

    setupSucceeded = true;
}

void TMainConsole::hideVideoOutput(TMediaPlayer* player)
{
    if (!player || !player->mediaPlayer()) {
        return;
    }

    auto* videoOutput = qobject_cast<QVideoWidget*>(player->mediaPlayer()->videoOutput());

    if (!videoOutput) {
        return;
    }

    QWidget* parent = videoOutput->parentWidget();

    if (parent && parent->isVisible()) {
        parent->hide();
    }
}

void TMainConsole::showStatistics()
{
    auto pHost = getHost();
    if (!pHost) {
        return;
    }

    const QString header = qsl("%1\n").arg(tr("+--------------------------------------------------------------+\n"
                                              "|                      system statistics                       |\n"
                                              "+--------------------------------------------------------------+",
                                              "Header for the system's statistics information displayed in the console, it is 64 'narrow' characters wide"));
    print(header, QColor(150, 120, 0), Qt::black);

    QStringList subjects;
    QStringList tables;
    if (pHost->mTelnet.isGMCPEnabled()) {
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("GMCP events:");
        tables << QLatin1String("gmcp");
    }
    if (pHost->mTelnet.isATCPEnabled()) {
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("ATCP events:");
        tables << QLatin1String("atcp");
    }
    if (pHost->mTelnet.isChannel102Enabled()) {
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("Channel102 events:");
        tables << QLatin1String("channel102");
    }
    if (pHost->mTelnet.isMXPEnabled()) {
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("MXP events:");
        tables << QLatin1String("mxp");
    }
    if (pHost->mTelnet.isMSSPEnabled()) {
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("MSSP events:");
        tables << QLatin1String("mssp");
    }
    if (pHost->mTelnet.isMSDPEnabled()) {
        // This might be a nil rather than an empty table if not present:
        //: Heading for the system's statistics information displayed in the console
        subjects << tr("MSDP events:");
        tables << QLatin1String("msdp");
    }

    Q_ASSERT_X(subjects.count() == tables.count(), "TMainConsole::showStatistics()", "mismatch in titles and built-in tables to show");
    for (int i = 0, total = subjects.count(); i < total; ++i) {
        mpHost->mLuaInterpreter.compileAndExecuteScript(
                qsl("setFgColor(190,150,0); setUnderline(true); echo([[\n\n%1\n]]);setUnderline(false);setFgColor(150,120,0);display( %2 );").arg(subjects.at(i), tables.at(i)));
    }

    const QString itemScript = "setFgColor(190,150,0); setUnderline(true); echo([[\n\n%1\n]]); setBold(false);setUnderline(false);setFgColor(150,120,0)";

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Telnet Options:")));
    print(pHost->mTelnet.assembleTelnetOptionsReport(), QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Trigger Report:")));
    QString itemMsg = std::get<0>(mpHost->getTriggerUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Timer Report:")));
    itemMsg = std::get<0>(mpHost->getTimerUnit()->assembleReport());
    ;
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Alias Report:")));
    itemMsg = std::get<0>(mpHost->getAliasUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Keybinding Report:")));
    itemMsg = std::get<0>(mpHost->getKeyUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Script Report:")));
    itemMsg = std::get<0>(mpHost->getScriptUnit()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    //: Heading for the system's statistics information displayed in the console
    mpHost->mLuaInterpreter.compileAndExecuteScript(itemScript.arg(tr("Gif Report:")));
    itemMsg = std::get<0>(mpHost->getGifTracker()->assembleReport());
    print(itemMsg, QColor(150, 120, 0), Qt::black);

    // Footer for the system's statistics information displayed in the console, it should be 64 'narrow' characters wide
    const QString footer = qsl("\n+--------------------------------------------------------------+\n");
    mpHost->mpConsole->print(footer, QColor(150, 120, 0), Qt::black);

    mpHost->mLuaInterpreter.compileAndExecuteScript(QLatin1String("resetFormat();"));

    mpHost->mpConsole->raise();
}

void TMainConsole::closeEvent(QCloseEvent* event)
{
    // Guard against duplicate close events during widget destruction.
    // The first close comes from explicit close(), the second can come
    // from Qt during widget deletion. Processing the event twice can
    // cause crashes in TBuffer destruction.
    if (mEnableClose) {
        event->accept();
        return;
    }

    qDebug().nospace().noquote() << "TMainConsole::closeEvent(...) INFO - received by \"" << mpHost->getName() << "\".";
    TEvent conCloseEvent{};
    conCloseEvent.mArgumentList.append(qsl("sysExitEvent"));
    conCloseEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(conCloseEvent);

    if (mpHost->mFORCE_SAVE_ON_EXIT || mpHost->isClosingForced()) {
        mudlet::self()->saveWindowLayout();
        mpHost->modulesToWrite.clear();
        // We are not checking the status result from here!
        mpHost->saveProfile();

        if (mpHost->mpMap && mpHost->mpMap->mpRoomDB) {
            // There is a map loaded - but it *could* have no rooms at all!
            if (!saveMap(QString())) {
                qWarning() << "TMainConsole::closeEvent(...) WARNING - forced close map save failed";
            }
        }
        mpHost->waitForProfileSave();
        mEnableClose = true;
        event->accept();
        return;
    }

    if (!mEnableClose) {
    ASK_PROFILE:
        const int choice = QMessageBox::question(this, tr("Save profile?"), tr("Do you want to save the profile %1?").arg(mProfileName), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (choice == QMessageBox::Cancel) {
            event->ignore();
            return;
        }

        if (choice == QMessageBox::Yes) {
            mudlet::self()->saveWindowLayout();

            mpHost->modulesToWrite.clear();
            auto [ok, filename, error] = mpHost->saveProfile();

            if (!ok) {
                QMessageBox::critical(
                        this, tr("Could not save profile"), tr("Sorry, could not save your profile as \"%1\" - got the following error: \"%2\".").arg(filename, error), QMessageBox::Retry);
                goto ASK_PROFILE;
            }

            if (mpHost->mpMap && mpHost->mpMap->mpRoomDB) {
                // There is a map loaded - but it *could* have no rooms at all!
            ASK_MAP:
                if (!saveMap(QString())) {
                    const int mapChoice = QMessageBox::warning(this,
                                                               tr("Could not save map"),
                                                               tr("Sorry, could not save the map. Would you like to retry or close without saving the map?"),
                                                               QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Cancel);
                    if (mapChoice == QMessageBox::Retry) {
                        goto ASK_MAP;
                    }
                    if (mapChoice == QMessageBox::Cancel) {
                        event->ignore();
                        return;
                    }
                    // QMessageBox::Ignore - continue without saving map
                }
            }

            mpHost->waitForProfileSave();
            mEnableClose = true;
            event->accept();
            return;
        }

        if (choice == QMessageBox::No) {
            mudlet::self()->saveWindowLayout();
            mEnableClose = true;
            event->accept();
            return;
        }

        if (!mudlet::self()->isGoingDown()) {
            QMessageBox::warning(this, "Aborting exit", "Session exit aborted on user request.");
            event->ignore();
            return;
        }

        mEnableClose = true;
        event->accept();
    }
}

bool TMainConsole::clear(const QString& name)
{
    if (name.isEmpty() || !name.compare(QLatin1String("main"))) {
        TConsole::clear();
        mUpperPane->showNewLines();
        mUpperPane->forceUpdate();
        mLowerPane->forceUpdate();
        return true;
    }

    auto pC = mSubConsoleMap.value(name);
    if (pC) {
        pC->clear();
        return true;
    }

    return false;
}
