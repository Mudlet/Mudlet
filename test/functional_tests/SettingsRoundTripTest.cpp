/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

/*
 * The settings round trip: a control comes up holding what the profile holds,
 * an edit to it reaches the profile, and the controls that only make sense
 * alongside another setting follow that setting.
 *
 * Run with: ctest -R SettingsRoundTripTest -V
 */

#include <functional>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTimeEdit>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MMCP.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "TMap.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsRoundTripTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsRoundTrip-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");
    // One Host serves the whole class, so anything a case moves is put back by
    // cleanup() - which runs whether or not the case reached its own end
    QList<std::function<void()>> mRestorers;

    void restoreLater(std::function<void()> restorer) { mRestorers.append(std::move(restorer)); }

    // Nothing here reaches the network, but the dialog is only fully wired once
    // it has been shown - buildShell() finishes moving controls between cards on
    // the first resize
    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    // An apply is what carries a change from a control to the Host, and
    // signal_preferencesSaved is emitted however much of one there turned out
    // to be
    bool applyAndWait(QSignalSpy& spy) { return TestSettings::waitForApply(spy); }

    void closePreferences()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // The chat port has no setter on the Host, so the only way to put one back
    // is the field that moved it
    void writeChatPortThroughADialog(const quint16 port)
    {
        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->lineEdit_mmcpPort->setText(QString::number(port));
        emit mpPreferences->lineEdit_mmcpPort->editingFinished();
        applyAndWait(applySpy);
        closePreferences();
    }

    static bool comboItemEnabled(const QComboBox* pComboBox, const int row)
    {
        const auto* pModel = qobject_cast<const QStandardItemModel*>(pComboBox->model());
        return pModel && pModel->item(row) && pModel->item(row)->isEnabled();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        closePreferences();
        // Reverse order, so a case that moved one setting twice ends on the
        // value it started with
        while (!mRestorers.isEmpty()) {
            mRestorers.takeLast()();
        }
        if (mpHost) {
            mpHost->waitForProfileSave();
        }
    }

    // The population pass reads the Host rather than showing the .ui file's
    // defaults, so every value below is one no default could supply
    void test_everyControlComesUpHoldingTheProfilesValue()
    {
        const int wrapAt = 137;
        const int wrapIndent = 7;
        const int hangingIndent = 3;
        const int undoWrapWidth = 123;
        const int commandLineHeight = 42;
        const int bufferSize = 12345;
        const QString commandSeparator = qsl(";;;");
        const QString logFileName = qsl("round-trip-log");
        const QTime debugInterval(0, 0, 7, 250);
        const auto echoMode = Host::CommandEchoMode::Always;
        const auto blankLines = Host::BlankLineBehaviour::Hide;

        // Every one of these has to differ from what the profile holds, or the
        // dialog could be showing the old value and still look right
        QVERIFY(mpHost->mWrapAt != wrapAt);
        QVERIFY(mpHost->mWrapIndentCount != wrapIndent);
        QVERIFY(mpHost->mWrapHangingIndentCount != hangingIndent);
        QVERIFY(mpHost->mUndoServerWrapWidth != undoWrapWidth);
        QVERIFY(mpHost->commandLineMinimumHeight != commandLineHeight);
        QVERIFY(mpHost->getConsoleBufferSize() != bufferSize);
        QVERIFY(mpHost->getCommandSeparator() != commandSeparator);
        QVERIFY(mpHost->mLogFileName != logFileName);
        QVERIFY(mpHost->mTimerDebugOutputSuppressionInterval != debugInterval);
        QVERIFY(mpHost->mCommandEchoMode != echoMode);
        QVERIFY(mpHost->mBlankLineBehaviour != blankLines);

        const int priorWrapAt = mpHost->mWrapAt;
        const int priorWrapIndent = mpHost->mWrapIndentCount;
        const int priorHangingIndent = mpHost->mWrapHangingIndentCount;
        const int priorUndoWrapWidth = mpHost->mUndoServerWrapWidth;
        const int priorCommandLineHeight = mpHost->commandLineMinimumHeight;
        const int priorBufferSize = mpHost->getConsoleBufferSize();
        const bool priorUseMaxBuffer = mpHost->getUseMaxConsoleBufferSize();
        const QString priorCommandSeparator = mpHost->getCommandSeparator();
        const QString priorLogFileName = mpHost->mLogFileName;
        const QTime priorDebugInterval = mpHost->mTimerDebugOutputSuppressionInterval;
        const auto priorEchoMode = mpHost->mCommandEchoMode;
        const auto priorBlankLines = mpHost->mBlankLineBehaviour;
        const bool priorAlertOnNewData = mpHost->mAlertOnNewData;
        const bool priorUnixEol = mpHost->mUSE_UNIX_EOL;
        const bool priorEchoLuaErrors = mpHost->mEchoLuaErrors;
        const bool priorTimestamps = mpHost->mIsLoggingTimestamps;
        restoreLater([=, this]() {
            mpHost->mWrapAt = priorWrapAt;
            mpHost->mWrapIndentCount = priorWrapIndent;
            mpHost->mWrapHangingIndentCount = priorHangingIndent;
            mpHost->mUndoServerWrapWidth = priorUndoWrapWidth;
            mpHost->commandLineMinimumHeight = priorCommandLineHeight;
            mpHost->setConsoleBufferSize(priorBufferSize);
            mpHost->setUseMaxConsoleBufferSize(priorUseMaxBuffer);
            mpHost->mCommandSeparator = priorCommandSeparator;
            mpHost->mLogFileName = priorLogFileName;
            mpHost->mTimerDebugOutputSuppressionInterval = priorDebugInterval;
            mpHost->mCommandEchoMode = priorEchoMode;
            mpHost->mBlankLineBehaviour = priorBlankLines;
            mpHost->mAlertOnNewData = priorAlertOnNewData;
            mpHost->mUSE_UNIX_EOL = priorUnixEol;
            mpHost->mEchoLuaErrors = priorEchoLuaErrors;
            mpHost->mIsLoggingTimestamps = priorTimestamps;
        });

        mpHost->mWrapAt = wrapAt;
        mpHost->mWrapIndentCount = wrapIndent;
        mpHost->mWrapHangingIndentCount = hangingIndent;
        mpHost->mUndoServerWrapWidth = undoWrapWidth;
        mpHost->commandLineMinimumHeight = commandLineHeight;
        mpHost->setUseMaxConsoleBufferSize(false);
        mpHost->setConsoleBufferSize(bufferSize);
        mpHost->mCommandSeparator = commandSeparator;
        mpHost->mLogFileName = logFileName;
        mpHost->mTimerDebugOutputSuppressionInterval = debugInterval;
        mpHost->mCommandEchoMode = echoMode;
        mpHost->mBlankLineBehaviour = blankLines;
        mpHost->mAlertOnNewData = !priorAlertOnNewData;
        mpHost->mUSE_UNIX_EOL = !priorUnixEol;
        mpHost->mEchoLuaErrors = !priorEchoLuaErrors;
        mpHost->mIsLoggingTimestamps = !priorTimestamps;

        openPreferences();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapAt);
        QCOMPARE(mpPreferences->indent_wrapped_spinBox->value(), wrapIndent);
        QCOMPARE(mpPreferences->hanging_indent_wrapped_spinBox->value(), hangingIndent);
        QCOMPARE(mpPreferences->undo_server_wrap_width_spinBox->value(), undoWrapWidth);
        QCOMPARE(mpPreferences->commandLineMinimumHeight->value(), commandLineHeight);
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), bufferSize);
        QCOMPARE(mpPreferences->command_separator_lineedit->text(), commandSeparator);
        QCOMPARE(mpPreferences->lineEdit_logFileName->text(), logFileName);
        QCOMPARE(mpPreferences->timeEdit_timerDebugOutputMinimumInterval->time(), debugInterval);
        QCOMPARE(mpPreferences->show_sent_text_combobox->currentIndex(), static_cast<int>(echoMode));
        QCOMPARE(mpPreferences->comboBox_blankLinesBehaviour->currentIndex(), static_cast<int>(blankLines));
        QCOMPARE(mpPreferences->mAlertOnNewData->isChecked(), !priorAlertOnNewData);
        QCOMPARE(mpPreferences->USE_UNIX_EOL->isChecked(), !priorUnixEol);
        QCOMPARE(mpPreferences->checkBox_echoLuaErrors->isChecked(), !priorEchoLuaErrors);
        QCOMPARE(mpPreferences->mIsLoggingTimestamps->isChecked(), !priorTimestamps);
    }

    // ...and the other direction: what the user leaves in a control is what the
    // profile ends up holding. Every control here is edited in the same dialog
    // and written by one apply, which is how a burst of changes reaches the
    // Host in practice.
    void test_anEditToEveryKindOfControlIsWrittenBackToTheProfile()
    {
        openPreferences();

        const int wrapAt = 111;
        const int wrapIndent = 9;
        const int hangingIndent = 5;
        const int commandLineHeight = 55;
        const QString logFileName = qsl("written-back-log");
        const QString doubleClickIgnore = qsl("<>{}");
        const QTime debugInterval(0, 0, 3, 500);
        const auto echoMode = Host::CommandEchoMode::Never;
        const auto blankLines = Host::BlankLineBehaviour::ReplaceWithSpace;

        const int priorWrapAt = mpHost->mWrapAt;
        const int priorWrapIndent = mpHost->mWrapIndentCount;
        const int priorHangingIndent = mpHost->mWrapHangingIndentCount;
        const int priorCommandLineHeight = mpHost->commandLineMinimumHeight;
        const QString priorLogFileName = mpHost->mLogFileName;
        const QSet<QChar> priorDoubleClickIgnore = mpHost->mDoubleClickIgnore;
        const QTime priorDebugInterval = mpHost->mTimerDebugOutputSuppressionInterval;
        const auto priorEchoMode = mpHost->mCommandEchoMode;
        const auto priorBlankLines = mpHost->mBlankLineBehaviour;
        const bool priorUnixEol = mpHost->mUSE_UNIX_EOL;
        const bool priorGaOff = mpHost->mFORCE_GA_OFF;
        const bool priorNoCompression = mpHost->mFORCE_NO_COMPRESSION;
        const bool priorAlertOnNewData = mpHost->mAlertOnNewData;
        const bool priorEchoLuaErrors = mpHost->mEchoLuaErrors;
        const bool priorForceLf = mpHost->mUSE_FORCE_LF_AFTER_PROMPT;
        const bool priorIreBugfix = mpHost->mUSE_IRE_DRIVER_BUGFIX;
        const bool priorAutoClear = mpHost->mAutoClearCommandLineAfterSend;
        const bool priorPasswordMasking = mpHost->mDisablePasswordMasking;
        const bool priorRunAllKeys = mpHost->getKeyUnit()->mRunAllKeyMatches;
        const bool priorTimestamps = mpHost->mIsLoggingTimestamps;
        restoreLater([=, this]() {
            mpHost->mWrapAt = priorWrapAt;
            mpHost->mWrapIndentCount = priorWrapIndent;
            mpHost->mWrapHangingIndentCount = priorHangingIndent;
            mpHost->commandLineMinimumHeight = priorCommandLineHeight;
            mpHost->mLogFileName = priorLogFileName;
            mpHost->mDoubleClickIgnore = priorDoubleClickIgnore;
            mpHost->mTimerDebugOutputSuppressionInterval = priorDebugInterval;
            mpHost->mCommandEchoMode = priorEchoMode;
            mpHost->mBlankLineBehaviour = priorBlankLines;
            mpHost->mUSE_UNIX_EOL = priorUnixEol;
            mpHost->mFORCE_GA_OFF = priorGaOff;
            mpHost->mFORCE_NO_COMPRESSION = priorNoCompression;
            mpHost->mAlertOnNewData = priorAlertOnNewData;
            mpHost->mEchoLuaErrors = priorEchoLuaErrors;
            mpHost->mUSE_FORCE_LF_AFTER_PROMPT = priorForceLf;
            mpHost->set_USE_IRE_DRIVER_BUGFIX(priorIreBugfix);
            mpHost->mAutoClearCommandLineAfterSend = priorAutoClear;
            mpHost->mDisablePasswordMasking = priorPasswordMasking;
            mpHost->getKeyUnit()->mRunAllKeyMatches = priorRunAllKeys;
            mpHost->mIsLoggingTimestamps = priorTimestamps;
        });

        QVERIFY(priorWrapAt != wrapAt);
        QVERIFY(priorWrapIndent != wrapIndent);
        QVERIFY(priorHangingIndent != hangingIndent);
        QVERIFY(priorCommandLineHeight != commandLineHeight);
        QVERIFY(priorLogFileName != logFileName);
        QVERIFY(priorDebugInterval != debugInterval);
        QVERIFY(priorEchoMode != echoMode);
        QVERIFY(priorBlankLines != blankLines);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);

        // The line edits are filled without taking the focus: a field being
        // typed into is deliberately left alone by an apply somebody else started
        mpPreferences->lineEdit_logFileName->setText(logFileName);
        mpPreferences->doubleclick_ignore_lineedit->setText(doubleClickIgnore);

        mpPreferences->wrap_at_spinBox->setValue(wrapAt);
        mpPreferences->indent_wrapped_spinBox->setValue(wrapIndent);
        mpPreferences->hanging_indent_wrapped_spinBox->setValue(hangingIndent);
        mpPreferences->commandLineMinimumHeight->setValue(commandLineHeight);
        mpPreferences->timeEdit_timerDebugOutputMinimumInterval->setTime(debugInterval);
        mpPreferences->show_sent_text_combobox->setCurrentIndex(static_cast<int>(echoMode));
        mpPreferences->comboBox_blankLinesBehaviour->setCurrentIndex(static_cast<int>(blankLines));

        mpPreferences->USE_UNIX_EOL->setChecked(!priorUnixEol);
        mpPreferences->mFORCE_GA_OFF->setChecked(!priorGaOff);
        mpPreferences->mFORCE_MCCP_OFF->setChecked(!priorNoCompression);
        mpPreferences->mAlertOnNewData->setChecked(!priorAlertOnNewData);
        mpPreferences->checkBox_echoLuaErrors->setChecked(!priorEchoLuaErrors);
        mpPreferences->checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked(!priorForceLf);
        mpPreferences->checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(!priorIreBugfix);
        mpPreferences->auto_clear_input_line_checkbox->setChecked(!priorAutoClear);
        mpPreferences->disable_password_masking_checkbox->setChecked(!priorPasswordMasking);
        mpPreferences->checkBox_runAllKeyBindings->setChecked(!priorRunAllKeys);
        mpPreferences->mIsLoggingTimestamps->setChecked(!priorTimestamps);

        // Nothing has reached the Host yet: these controls are written by the
        // apply rather than by a slot of their own
        QCOMPARE(mpHost->mWrapAt, priorWrapAt);
        QCOMPARE(mpHost->mCommandEchoMode, priorEchoMode);
        QCOMPARE(mpHost->mUSE_UNIX_EOL, priorUnixEol);

        QVERIFY2(applyAndWait(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(mpHost->mWrapAt, wrapAt);
        QCOMPARE(mpHost->mWrapIndentCount, wrapIndent);
        QCOMPARE(mpHost->mWrapHangingIndentCount, hangingIndent);
        QCOMPARE(mpHost->commandLineMinimumHeight, commandLineHeight);
        QCOMPARE(mpHost->mLogFileName, logFileName);
        QCOMPARE(mpHost->mTimerDebugOutputSuppressionInterval, debugInterval);
        QCOMPARE(mpHost->mCommandEchoMode, echoMode);
        QCOMPARE(mpHost->mBlankLineBehaviour, blankLines);
        QCOMPARE(mpHost->mUSE_UNIX_EOL, !priorUnixEol);
        QCOMPARE(mpHost->mFORCE_GA_OFF, !priorGaOff);
        QCOMPARE(mpHost->mFORCE_NO_COMPRESSION, !priorNoCompression);
        QCOMPARE(mpHost->mAlertOnNewData, !priorAlertOnNewData);
        QCOMPARE(mpHost->mEchoLuaErrors, !priorEchoLuaErrors);
        QCOMPARE(mpHost->mUSE_FORCE_LF_AFTER_PROMPT, !priorForceLf);
        QCOMPARE(mpHost->mUSE_IRE_DRIVER_BUGFIX, !priorIreBugfix);
        QCOMPARE(mpHost->mAutoClearCommandLineAfterSend, !priorAutoClear);
        QCOMPARE(mpHost->mDisablePasswordMasking, !priorPasswordMasking);
        QCOMPARE(mpHost->getKeyUnit()->mRunAllKeyMatches, !priorRunAllKeys);
        QCOMPARE(mpHost->mIsLoggingTimestamps, !priorTimestamps);

        // The one field that is not stored as it is typed: each character
        // becomes a member of the set of characters a double click stops at
        QSet<QChar> expectedIgnore;
        for (const QChar character : doubleClickIgnore) {
            expectedIgnore.insert(character);
        }
        QCOMPARE(mpHost->mDoubleClickIgnore, expectedIgnore);
    }

    // The buffer size is either a number the user picks or whatever this machine
    // can hold, and the spin box says which by being usable or not
    void test_theMaximumBufferSizeTakesOverTheBufferSizeSpinBox()
    {
        QVERIFY2(mpHost->mpConsole, "the profile has no console, so it has no maximum buffer size to offer");
        const int priorBufferSize = mpHost->getConsoleBufferSize();
        const bool priorUseMax = mpHost->getUseMaxConsoleBufferSize();
        restoreLater([=, this]() {
            mpHost->setConsoleBufferSize(priorBufferSize);
            mpHost->setUseMaxConsoleBufferSize(priorUseMax);
        });

        const int chosenSize = 12345;
        mpHost->setUseMaxConsoleBufferSize(false);
        mpHost->setConsoleBufferSize(chosenSize);

        openPreferences();
        const int maximumSize = mpHost->mpConsole->buffer.getMaxBufferSize();
        QVERIFY2(maximumSize != chosenSize, "the machine's maximum buffer size happens to be the size this case picked, so the two cannot be told apart");
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), chosenSize);
        QVERIFY(mpPreferences->console_buffer_size_spinBox->isEnabled());
        QVERIFY(!mpPreferences->checkBox_useMaxBufferSize->isChecked());

        // Both clicks land well inside the 400ms debounce, so the size the
        // second one puts back is the one the dialog was opened with rather than
        // anything an apply has since stored
        mpPreferences->checkBox_useMaxBufferSize->click();
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), maximumSize);
        QVERIFY2(!mpPreferences->console_buffer_size_spinBox->isEnabled(), "the buffer size can still be edited while the maximum is in charge of it");

        mpPreferences->checkBox_useMaxBufferSize->click();
        QVERIFY2(mpPreferences->console_buffer_size_spinBox->isEnabled(), "the buffer size stayed uneditable after the maximum stopped being in charge of it");
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), chosenSize);
    }

    // The width to undo a server's wrapping at is only a setting while the
    // undoing is on (#10165's neighbour: an experimental option says so, too)
    void test_theUndoWrapWidthIsOnlyReachableWhileUndoingIsOn()
    {
        const bool priorUndo = mpHost->mUndoServerWrap;
        restoreLater([=, this]() {
            mpHost->mUndoServerWrap = priorUndo;
        });
        mpHost->mUndoServerWrap = false;

        openPreferences();
        QVERIFY(!mpPreferences->checkBox_undoServerWrap->isChecked());
        QVERIFY2(!mpPreferences->undo_server_wrap_width_spinBox->isEnabled(), "the wrap width was editable with the option that uses it turned off");
        // isHidden() rather than isVisible(): the card this note sits on is not
        // the page the dialog opens on, so it is unseen either way
        QVERIFY2(mpPreferences->label_undo_server_wrap_experimental->isHidden(), "the experimental note was shown to someone not running the option");

        mpPreferences->checkBox_undoServerWrap->click();
        QVERIFY2(mpPreferences->undo_server_wrap_width_spinBox->isEnabled(), "turning the option on left its width uneditable");
        QVERIFY2(!mpPreferences->label_undo_server_wrap_experimental->isHidden(), "turning the option on did not bring out the experimental note");

        mpPreferences->checkBox_undoServerWrap->click();
        QVERIFY2(!mpPreferences->undo_server_wrap_width_spinBox->isEnabled(), "turning the option back off left its width editable");
        QVERIFY2(mpPreferences->label_undo_server_wrap_experimental->isHidden(), "turning the option back off left the experimental note up");
    }

    // Hiding both the menu bar and the tool bar would leave a window with no way
    // back to either, so whichever is set to "Never" greys the other's "Never"
    // out rather than letting it be picked and silently snapped back (#7079)
    void test_neitherTheMenuBarNorTheToolBarCanBeTheSecondOneSetToNever()
    {
        const auto priorMenu = mudlet::self()->menuBarVisibility();
        const auto priorToolBar = mudlet::self()->toolBarVisibility();
        restoreLater([=]() {
            mudlet::self()->setMenuBarVisibility(priorMenu);
            mudlet::self()->setToolBarVisibility(priorToolBar);
        });

        openPreferences();
        QComboBox* pMenuBar = mpPreferences->comboBox_menuBarVisibility;
        QComboBox* pToolBar = mpPreferences->comboBox_toolBarVisibility;

        // "Never" is the first entry of both, and with neither on it both are
        // there to be picked
        pMenuBar->setCurrentIndex(2);
        pToolBar->setCurrentIndex(2);
        QVERIFY2(comboItemEnabled(pMenuBar, 0), "the menu bar could not be set to Never with the tool bar always shown");
        QVERIFY2(comboItemEnabled(pToolBar, 0), "the tool bar could not be set to Never with the menu bar always shown");

        pMenuBar->setCurrentIndex(0);
        QVERIFY2(!comboItemEnabled(pToolBar, 0), "the tool bar could still be set to Never with the menu bar already there");
        QVERIFY2(comboItemEnabled(pMenuBar, 0), "the menu bar's own Never was greyed out by choosing it");

        // ...and one arriving anyway - a script, or a settings file written by
        // hand - is put back rather than obeyed
        pToolBar->setCurrentIndex(0);
        QCOMPARE(pToolBar->currentIndex(), 1);
        QCOMPARE(pMenuBar->currentIndex(), 0);

        pMenuBar->setCurrentIndex(2);
        QVERIFY2(comboItemEnabled(pToolBar, 0), "the tool bar's Never stayed greyed out after the menu bar stopped being hidden");
    }

    // Only the "Named file" format concatenates into a file the user names, so
    // it is the only one that has a name to show
    void test_onlyTheNamedFileFormatOffersAFileNameToFillIn()
    {
        openPreferences();
        QComboBox* pFormat = mpPreferences->comboBox_logFileNameFormat;
        const int namedFileRow = pFormat->findData(QString());
        QVERIFY2(namedFileRow >= 0, "the log format list has no named-file entry");
        const int datedRow = pFormat->findData(qsl("yyyy-MM-dd"));
        QVERIFY2(datedRow >= 0, "the log format list has no daily entry");

        pFormat->setCurrentIndex(datedRow);
        QVERIFY2(!mpPreferences->lineEdit_logFileName->isVisible(), "a dated log format still asked for a file name");
        QVERIFY(!mpPreferences->label_logFileName->isVisible());
        QVERIFY(!mpPreferences->label_logFileNameExtension->isVisible());

        pFormat->setCurrentIndex(namedFileRow);
        QVERIFY2(mpPreferences->lineEdit_logFileName->isVisible(), "the named-file format did not ask for a file name");
        QVERIFY(mpPreferences->label_logFileName->isVisible());
        QVERIFY(mpPreferences->label_logFileNameExtension->isVisible());
    }

    // The log is written as HTML or as text, and every example the format list
    // shows has to end in the extension that choice will really produce
    void test_theLogFormatExamplesFollowTheHtmlChoice()
    {
        const bool priorHtml = mpHost->mIsNextLogFileInHtmlFormat;
        restoreLater([=, this]() {
            mpHost->mIsNextLogFileInHtmlFormat = priorHtml;
        });
        mpHost->mIsNextLogFileInHtmlFormat = false;

        openPreferences();
        QComboBox* pFormat = mpPreferences->comboBox_logFileNameFormat;
        QCOMPARE(mpPreferences->label_logFileNameExtension->text(), qsl(".txt"));
        QVERIFY2(pFormat->itemText(pFormat->findData(qsl("yyyy-MM-dd"))).endsWith(qsl(".txt)")), "a text log's format example did not end in .txt");

        mpPreferences->mIsToLogInHtml->click();
        QCOMPARE(mpPreferences->label_logFileNameExtension->text(), qsl(".html"));
        for (const QString& format : {qsl("yyyy-MM-dd#HH-mm-ss"), qsl("yyyy-MM-ddTHH-mm-ss"), qsl("yyyy-MM-dd"), qsl("yyyy-MM")}) {
            const int row = pFormat->findData(format);
            QVERIFY2(row >= 0, qPrintable(qsl("the log format list lost its '%1' entry").arg(format)));
            QVERIFY2(pFormat->itemText(row).endsWith(qsl(".html)")), qPrintable(qsl("the '%1' example still offered a .txt file after HTML was chosen").arg(format)));
        }

        mpPreferences->mIsToLogInHtml->click();
        QCOMPARE(mpPreferences->label_logFileNameExtension->text(), qsl(".txt"));
        QVERIFY2(pFormat->itemText(pFormat->findData(qsl("yyyy-MM"))).endsWith(qsl(".txt)")), "going back to a text log left an .html example behind");
    }

    // A chat name is sent to other clients inside a field-separated message, so
    // one carrying a separator is refused both on the way in and on the way out
    void test_aChatNameCarryingAFieldSeparatorIsRefused()
    {
        const QString priorName = mpHost->getMMCPChatName();
        restoreLater([=, this]() {
            mpHost->setMMCPChatName(priorName);
        });
        QVERIFY(mpHost->setMMCPChatName(qsl("Roundtrip")));

        openPreferences();
        QLineEdit* pName = mpPreferences->lineEdit_mmcpChatName;
        QCOMPARE(pName->text(), qsl("Roundtrip"));

        // Typing one is not accepted by the field at all
        pName->setFocus();
        pName->selectAll();
        QTest::keyClicks(pName, qsl("Ka,pi~ta"));
        QCOMPARE(pName->text(), qsl("Kapita"));
        QTest::keyClick(pName, Qt::Key_Return);
        QCOMPARE(mpHost->getMMCPChatName(), qsl("Kapita"));

        // ...and one arriving another way is put back rather than stored
        pName->clearFocus();
        pName->setText(qsl("Bad,Name"));
        emit pName->editingFinished();
        QCOMPARE(mpHost->getMMCPChatName(), qsl("Kapita"));
        QCOMPARE(pName->text(), qsl("Kapita"));
    }

    // The chat port is a free-text field, so anything that is not a port number
    // has to land on the default rather than on zero
    void test_aChatPortThatIsNotANumberFallsBackToTheDefault()
    {
        const quint16 priorPort = mpHost->getMMCPPort();
        restoreLater([=, this]() {
            if (mpHost->getMMCPPort() != priorPort) {
                writeChatPortThroughADialog(priorPort);
            }
        });

        const quint16 chosenPort = 4711;
        QVERIFY2(chosenPort != csDefaultMMCPHostPort, "the port this case picks is the default, so it cannot tell a fallback from a pass-through");

        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->lineEdit_mmcpPort->setText(QString::number(chosenPort));
        emit mpPreferences->lineEdit_mmcpPort->editingFinished();
        QVERIFY2(applyAndWait(applySpy), "a port the field could parse was never written back");
        QCOMPARE(mpHost->getMMCPPort(), chosenPort);

        // ...and the dialog offers it again the next time it is opened
        closePreferences();
        openPreferences();
        QCOMPARE(mpPreferences->lineEdit_mmcpPort->text(), QString::number(chosenPort));

        QSignalSpy secondApplySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->lineEdit_mmcpPort->setText(qsl("not a port"));
        emit mpPreferences->lineEdit_mmcpPort->editingFinished();
        QVERIFY2(applyAndWait(secondApplySpy), "a port the field could not parse was never answered at all");
        QCOMPARE(mpHost->getMMCPPort(), csDefaultMMCPHostPort);
    }

    // Four ways to mark the player's room, and each decides which of the three
    // controls beneath it there is any point in offering
    void test_thePlayerRoomStyleDecidesWhichOfItsControlsAreOffered()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        const quint8 priorMapStyle = mpHost->mpMap->mPlayerRoomStyle;
        quint8 priorStyle = 0;
        quint8 priorOuter = 0;
        quint8 priorInner = 0;
        QColor priorOuterColor;
        QColor priorInnerColor;
        mpHost->getPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        restoreLater([=, this]() {
            mpHost->mpMap->mPlayerRoomStyle = priorMapStyle;
            mpHost->setPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        });

        openPreferences();
        QVERIFY2(mpPreferences->groupBox_playerRoomStyle->isEnabled(), "the player room card is greyed out for a profile that has a map");

        // Style 0 draws the mark itself, 1 and 2 are fixed-colour rings with a
        // hole to size, and only 3 lets the colours be picked
        struct StyleExpectation
        {
            int style;
            bool colorsOffered;
            bool innerDiameterOffered;
        };
        const QList<StyleExpectation> expectations{{0, false, false}, {1, false, true}, {2, false, true}, {3, true, true}};

        QComboBox* pStyle = mpPreferences->comboBox_playerRoomStyle;
        for (const StyleExpectation& expectation : expectations) {
            // Choosing the style already showing emits nothing, so each row has
            // to be arrived at from a different one
            if (pStyle->currentIndex() == expectation.style) {
                pStyle->setCurrentIndex((expectation.style + 1) % expectations.count());
            }
            pStyle->setCurrentIndex(expectation.style);
            const QString where = qsl("player room style %1").arg(expectation.style);
            QCOMPARE(mpHost->mpMap->mPlayerRoomStyle, static_cast<quint8>(expectation.style));
            quint8 hostStyle = 0;
            quint8 hostOuter = 0;
            quint8 hostInner = 0;
            QColor hostOuterColor;
            QColor hostInnerColor;
            mpHost->getPlayerRoomStyleDetails(hostStyle, hostOuter, hostInner, hostOuterColor, hostInnerColor);
            QCOMPARE(hostStyle, static_cast<quint8>(expectation.style));
            QVERIFY2(mpPreferences->pushButton_playerRoomPrimaryColor->isEnabled() == expectation.colorsOffered, qPrintable(qsl("%1 offers the wrong outer colour button").arg(where)));
            QVERIFY2(mpPreferences->pushButton_playerRoomSecondaryColor->isEnabled() == expectation.colorsOffered, qPrintable(qsl("%1 offers the wrong inner colour button").arg(where)));
            QVERIFY2(mpPreferences->spinBox_playerRoomInnerDiameter->isEnabled() == expectation.innerDiameterOffered, qPrintable(qsl("%1 offers the wrong inner diameter").arg(where)));
        }
    }

    // The two diameters are stored as quint8, so the dialog will not pass on a
    // value that would wrap round on the way in
    void test_aPlayerRoomDiameterReachesTheMapAndTheHostTogether()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        const quint8 priorMapOuter = mpHost->mpMap->mPlayerRoomOuterDiameterPercentage;
        const quint8 priorMapInner = mpHost->mpMap->mPlayerRoomInnerDiameterPercentage;
        const quint8 priorMapStyle = mpHost->mpMap->mPlayerRoomStyle;
        quint8 priorStyle = 0;
        quint8 priorOuter = 0;
        quint8 priorInner = 0;
        QColor priorOuterColor;
        QColor priorInnerColor;
        mpHost->getPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        restoreLater([=, this]() {
            mpHost->mpMap->mPlayerRoomOuterDiameterPercentage = priorMapOuter;
            mpHost->mpMap->mPlayerRoomInnerDiameterPercentage = priorMapInner;
            mpHost->mpMap->mPlayerRoomStyle = priorMapStyle;
            mpHost->setPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        });

        openPreferences();
        // Style 3 is the one that leaves both diameters editable
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);

        const int outer = 137;
        const int inner = 61;
        QVERIFY(mpHost->mpMap->mPlayerRoomOuterDiameterPercentage != outer);
        QVERIFY(mpHost->mpMap->mPlayerRoomInnerDiameterPercentage != inner);

        mpPreferences->spinBox_playerRoomOuterDiameter->setValue(outer);
        mpPreferences->spinBox_playerRoomInnerDiameter->setValue(inner);

        QCOMPARE(mpHost->mpMap->mPlayerRoomOuterDiameterPercentage, static_cast<quint8>(outer));
        QCOMPARE(mpHost->mpMap->mPlayerRoomInnerDiameterPercentage, static_cast<quint8>(inner));
        // The map's copy is what the mapper draws from and the Host's is what
        // the profile is saved from, so both have to move
        quint8 hostStyle = 0;
        quint8 hostOuter = 0;
        quint8 hostInner = 0;
        QColor hostOuterColor;
        QColor hostInnerColor;
        mpHost->getPlayerRoomStyleDetails(hostStyle, hostOuter, hostInner, hostOuterColor, hostInnerColor);
        QCOMPARE(hostOuter, static_cast<quint8>(outer));
        QCOMPARE(hostInner, static_cast<quint8>(inner));
    }

    // Resetting the colours puts back every one of the sixteen ANSI colours and
    // the six the console is drawn with, not just the ones the buttons show
    void test_resettingTheColoursPutsBackEveryOneOfThem()
    {
        struct ColorReset
        {
            const char* name;
            QColor* pColor;
            QColor expected;
        };
        // slot_resetColors() assigns each of these on its own, so a colour left
        // out of the list below is one nothing here would notice going missing
        const QVector<ColorReset> resets = {{"mCommandLineFgColor", &mpHost->mCommandLineFgColor, QColor(Qt::darkGray)},
                                            {"mCommandLineBgColor", &mpHost->mCommandLineBgColor, QColor(Qt::black)},
                                            {"mCommandFgColor", &mpHost->mCommandFgColor, QColor(113, 113, 0)},
                                            {"mCommandBgColor", &mpHost->mCommandBgColor, QColor(Qt::black)},
                                            {"mFgColor", &mpHost->mFgColor, QColor(Qt::lightGray)},
                                            {"mBgColor", &mpHost->mBgColor, QColor(Qt::black)},
                                            {"mBlack", &mpHost->mBlack, QColor(Qt::black)},
                                            {"mLightBlack", &mpHost->mLightBlack, QColor(Qt::darkGray)},
                                            {"mRed", &mpHost->mRed, QColor(Qt::darkRed)},
                                            {"mLightRed", &mpHost->mLightRed, QColor(Qt::red)},
                                            {"mGreen", &mpHost->mGreen, QColor(Qt::darkGreen)},
                                            {"mLightGreen", &mpHost->mLightGreen, QColor(Qt::green)},
                                            {"mBlue", &mpHost->mBlue, QColor(Qt::darkBlue)},
                                            {"mLightBlue", &mpHost->mLightBlue, QColor(Qt::blue)},
                                            {"mYellow", &mpHost->mYellow, QColor(Qt::darkYellow)},
                                            {"mLightYellow", &mpHost->mLightYellow, QColor(Qt::yellow)},
                                            {"mCyan", &mpHost->mCyan, QColor(Qt::darkCyan)},
                                            {"mLightCyan", &mpHost->mLightCyan, QColor(Qt::cyan)},
                                            {"mMagenta", &mpHost->mMagenta, QColor(Qt::darkMagenta)},
                                            {"mLightMagenta", &mpHost->mLightMagenta, QColor(Qt::magenta)},
                                            {"mWhite", &mpHost->mWhite, QColor(Qt::lightGray)},
                                            {"mLightWhite", &mpHost->mLightWhite, QColor(Qt::white)}};

        QVector<QColor> priorColors;
        for (const ColorReset& reset : resets) {
            priorColors.append(*reset.pColor);
        }
        restoreLater([resets, priorColors]() {
            for (int i = 0; i < resets.size(); ++i) {
                *resets.at(i).pColor = priorColors.at(i);
            }
        });

        openPreferences();
        const QColor wrong(17, 34, 51);
        for (const ColorReset& reset : resets) {
            QVERIFY2(reset.expected != wrong, reset.name);
            *reset.pColor = wrong;
        }

        mpPreferences->slot_resetColors();

        for (const ColorReset& reset : resets) {
            QVERIFY2(*reset.pColor == reset.expected, reset.name);
        }
    }
};

#include "SettingsRoundTripTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsRoundTripTest)
