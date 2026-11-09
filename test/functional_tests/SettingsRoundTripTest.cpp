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

#include <algorithm>
#include <functional>
#include <tuple>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTimeEdit>
#include <QThread>
#include <QTimer>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "ColourValue.h"
#include "Host.h"
#include "MMCP.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "TConsole.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TelnetServerStub.h"
#include "TriggerMatchPool.h"
#include "XMLimport.h"
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
    QPointer<QTimer> mpColorDialogTimer;
#ifdef INCLUDE_FONTS
    // Bundled, so on every machine, and not the display font a new profile starts with
    const QString mEditorOwnFamily = qsl("Ubuntu Mono");
#endif

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

#ifdef INCLUDE_FONTS
    QList<int> mApplicationFontIds;

    // A test binary never copies the bundled fonts out of the resources the way main.cpp does, so
    // without this the editor font cases would pick from families that are not there
    void registerBundledFonts()
    {
        for (const QString& resourcePath : {qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf")}) {
            const int id = QFontDatabase::addApplicationFont(resourcePath);
            QVERIFY2(id != -1, qPrintable(qsl("could not register the bundled font \"%1\"").arg(resourcePath)));
            mApplicationFontIds.append(id);
        }
    }
#endif

    bool runLua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    // The player room marker's settings are the map's and the Host's both, so a case changing them puts both back
    void restorePlayerRoomLater()
    {
        const QColor priorMapOuterColor = mpHost->mpMap->mPlayerRoomOuterColor;
        const QColor priorMapInnerColor = mpHost->mpMap->mPlayerRoomInnerColor;
        const quint8 priorMapStyle = mpHost->mpMap->mPlayerRoomStyle;
        quint8 priorStyle = 0;
        quint8 priorOuter = 0;
        quint8 priorInner = 0;
        QColor priorOuterColor;
        QColor priorInnerColor;
        mpHost->getPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        restoreLater([=, this]() {
            mpHost->mpMap->mPlayerRoomOuterColor = priorMapOuterColor;
            mpHost->mpMap->mPlayerRoomInnerColor = priorMapInnerColor;
            mpHost->mpMap->mPlayerRoomStyle = priorMapStyle;
            mpHost->setPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        });
    }

    std::pair<QColor, QColor> hostPlayerRoomColours() const
    {
        quint8 style = 0;
        quint8 outer = 0;
        quint8 inner = 0;
        QColor outerColor;
        QColor innerColor;
        mpHost->getPlayerRoomStyleDetails(style, outer, inner, outerColor, innerColor);
        return {outerColor, innerColor};
    }

    uiDesign::ColourValue* ringColour(const QString& ring) const { return mpPreferences->findChild<uiDesign::ColourValue*>(qsl("settingsPlayerRoom%1Colour").arg(ring)); }
    QSpinBox* ringOpacity(const QString& ring) const { return mpPreferences->findChild<QSpinBox*>(qsl("settingsPlayerRoom%1Opacity").arg(ring)); }

    // Types into a field or a spin box, ending with key
    static void typeInto(QWidget* pField, const QString& typed, const Qt::Key key)
    {
        pField->setFocus();
        if (auto* pLineEdit = qobject_cast<QLineEdit*>(pField)) {
            pLineEdit->selectAll();
        } else if (auto* pSpinBox = qobject_cast<QSpinBox*>(pField)) {
            pSpinBox->selectAll();
        }
        if (typed.isEmpty()) {
            QTest::keyClick(pField, Qt::Key_Backspace);
        } else {
            QTest::keyClicks(pField, typed);
        }
        QTest::keyClick(pField, key);
    }

    // Across the middle of the swatch at the field's leading edge, which is as far in from that edge as from the top
    static bool showsTheBoardThrough(QWidget* pField)
    {
        const QImage image = pField->grab().toImage();
        const qreal ratio = image.devicePixelRatio();
        const int middle = pField->height() / 2;
        // Inside the swatch's hairline, which spans the field's height less 2 * (hairline + inset) pixels
        const int reach = middle - 5;
        QList<int> lightness;
        for (int x = middle - reach; x <= middle + reach; ++x) {
            lightness.append(image.pixelColor(QPointF(x * ratio, (pField->height() / 2) * ratio).toPoint()).lightness());
        }
        const auto [darkest, lightest] = std::minmax_element(lightness.cbegin(), lightness.cend());
        return *lightest - *darkest > 24;
    }

    using colourCardColour = std::pair<QString, QColor*>;
    // Every colour of the two colour cards by the name its button carries after "pushButton_", and where the profile keeps it
    QList<colourCardColour> colourCardColours() const
    {
        Host* h = mpHost;
        return {{qsl("foreground_color"), &h->mFgColor},
                {qsl("background_color"), &h->mBgColor},
                {qsl("command_line_foreground_color"), &h->mCommandLineFgColor},
                {qsl("command_line_background_color"), &h->mCommandLineBgColor},
                {qsl("command_foreground_color"), &h->mCommandFgColor},
                {qsl("command_background_color"), &h->mCommandBgColor},
                {qsl("black"), &h->mBlack},
                {qsl("lBlack"), &h->mLightBlack},
                {qsl("red"), &h->mRed},
                {qsl("lRed"), &h->mLightRed},
                {qsl("green"), &h->mGreen},
                {qsl("lGreen"), &h->mLightGreen},
                {qsl("yellow"), &h->mYellow},
                {qsl("lYellow"), &h->mLightYellow},
                {qsl("blue"), &h->mBlue},
                {qsl("lBlue"), &h->mLightBlue},
                {qsl("magenta"), &h->mMagenta},
                {qsl("lMagenta"), &h->mLightMagenta},
                {qsl("cyan"), &h->mCyan},
                {qsl("lCyan"), &h->mLightCyan},
                {qsl("white"), &h->mWhite},
                {qsl("lWhite"), &h->mLightWhite},
                {qsl("background_color_2"), &h->mBgColor_2},
                {qsl("mapInfoBg"), &h->mMapInfoBg},
                {qsl("roomBorderColor"), &h->mRoomBorderColor},
                {qsl("upperLevelColor"), &h->mUpperLevelColor},
                {qsl("roomCollisionBorderColor"), &h->mRoomCollisionBorderColor},
                {qsl("lowerLevelColor"), &h->mLowerLevelColor},
                {qsl("foreground_color_2"), &h->mFgColor_2},
                {qsl("mapGridColor"), &h->mMapGridColor},
                {qsl("black_2"), &h->mBlack_2},
                {qsl("Lblack_2"), &h->mLightBlack_2},
                {qsl("red_2"), &h->mRed_2},
                {qsl("Lred_2"), &h->mLightRed_2},
                {qsl("green_2"), &h->mGreen_2},
                {qsl("Lgreen_2"), &h->mLightGreen_2},
                {qsl("yellow_2"), &h->mYellow_2},
                {qsl("Lyellow_2"), &h->mLightYellow_2},
                {qsl("blue_2"), &h->mBlue_2},
                {qsl("Lblue_2"), &h->mLightBlue_2},
                {qsl("magenta_2"), &h->mMagenta_2},
                {qsl("Lmagenta_2"), &h->mLightMagenta_2},
                {qsl("cyan_2"), &h->mCyan_2},
                {qsl("Lcyan_2"), &h->mLightCyan_2},
                {qsl("white_2"), &h->mWhite_2},
                {qsl("Lwhite_2"), &h->mLightWhite_2}};
    }

    uiDesign::ColourValue* colourField(const QString& stem) const { return mpPreferences->findChild<uiDesign::ColourValue*>(qsl("settingsColour_%1").arg(stem)); }

    // What scripts read from color_table for that key, as the colour it holds
    QColor colorTableEntry(const char* key) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "color_table");
        lua_getfield(L, -1, key);
        QColor colour;
        if (lua_istable(L, -1)) {
            int channels[3] = {0, 0, 0};
            for (int index = 1; index <= 3; ++index) {
                lua_rawgeti(L, -1, index);
                channels[index - 1] = static_cast<int>(lua_tointeger(L, -1));
                lua_pop(L, 1);
            }
            colour = QColor(channels[0], channels[1], channels[2]);
        }
        lua_pop(L, 2);
        return colour;
    }

    // How many pixels of a widget's own drawing are exactly that colour
    static int pixelsOf(QWidget* pWidget, const QColor& colour)
    {
        const QImage image = pWidget->grab().toImage();
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                count += image.pixelColor(x, y).rgb() == colour.rgb();
            }
        }
        return count;
    }

    // Counts the times a widget is drawn on screen, which grab() never shows: it draws the widget afresh each time
    class PaintCounter : public QObject
    {
    public:
        explicit PaintCounter(QWidget* pWidget) { pWidget->installEventFilter(this); }
        int count() const { return mCount; }
        void reset() { mCount = 0; }

    protected:
        bool eventFilter(QObject* pWatched, QEvent* pEvent) override
        {
            mCount += pEvent->type() == QEvent::Paint;
            return QObject::eventFilter(pWatched, pEvent);
        }

    private:
        int mCount = 0;
    };

    // Where the words in a spin box start and end across its field, from the first and last columns holding a
    // pixel unlike the field's own ground; -1 where there are none
    static std::pair<int, int> inkAcross(QSpinBox* pBox)
    {
        const QImage image = pBox->grab().toImage();
        const qreal ratio = image.devicePixelRatio();
        const QRect field = pBox->findChild<QLineEdit*>()->geometry();
        const auto at = [&image, ratio](const int x, const int y) {
            return image.pixelColor(QPointF(x * ratio, y * ratio).toPoint());
        };
        const QColor ground = at(field.left() + 2, field.top() + 1);
        int start = -1;
        int end = -1;
        for (int x = field.left() + 2; x <= field.right(); ++x) {
            for (int y = field.top() + 1; y < field.bottom(); ++y) {
                const QColor pixel = at(x, y);
                if (qAbs(pixel.red() - ground.red()) + qAbs(pixel.green() - ground.green()) + qAbs(pixel.blue() - ground.blue()) > 120) {
                    if (start < 0) {
                        start = x;
                    }
                    end = x;
                    break;
                }
            }
        }
        return {start, end};
    }

    int settingChangedCount(const char* key) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "settingChangedCounts");
        lua_getfield(L, -1, key);
        const int count = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 2);
        return count;
    }

    // Lua type and value of the last event for that key, as "boolean:true";
    // empty when no event ever arrived for it
    QString settingChangedValue(const char* key) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "settingChangedValues");
        lua_getfield(L, -1, key);
        const char* value = lua_tostring(L, -1);
        const QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 2);
        return result;
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

#ifdef INCLUDE_FONTS
        registerBundledFonts();
#endif
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
#ifdef INCLUDE_FONTS
        for (const int id : std::as_const(mApplicationFontIds)) {
            QFontDatabase::removeApplicationFont(id);
        }
#endif
    }

    void cleanup()
    {
        delete mpColorDialogTimer;
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
        const bool priorAntiAlias = mpHost->fontsAntiAlias();
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
            mpHost->setFontsAntiAlias(priorAntiAlias);
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
        mpHost->setFontsAntiAlias(!priorAntiAlias);

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
        QCOMPARE(mpPreferences->checkBox_antiAlias->isChecked(), !priorAntiAlias);
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
        // Stored trimmed, and the username lowercased as Discord compares it
        const QString discordUserName = qsl("round_trip_user");
        const QString chatPrefix = qsl("[rt]");

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
        const bool priorColorSpaceId = mpHost->getHaveColorSpaceId();
        const bool priorAutoClear = mpHost->mAutoClearCommandLineAfterSend;
        const bool priorPasswordMasking = mpHost->disablePasswordMasking();
        const bool priorRunAllKeys = mpHost->getKeyUnit()->mRunAllKeyMatches;
        const bool priorTimestamps = mpHost->mIsLoggingTimestamps;
        const bool priorAntiAlias = mpHost->fontsAntiAlias();
        const QString priorDiscordUserName = mpHost->getRequiredDiscordUserName();
        const QString priorChatPrefix = mpHost->getMMCPChatPrefix();
        const bool priorPrefixEmotes = mpHost->getMMCPPrefixEmotes();
        const bool priorChatNewline = mpHost->getMMCPAddChatMessageNewline();
        const bool priorSnoopInMain = mpHost->getMMCPShowSnoopInMainConsole();
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
            mpHost->setHaveColorSpaceId(priorColorSpaceId);
            mpHost->mAutoClearCommandLineAfterSend = priorAutoClear;
            mpHost->setDisablePasswordMasking(priorPasswordMasking);
            mpHost->getKeyUnit()->mRunAllKeyMatches = priorRunAllKeys;
            mpHost->mIsLoggingTimestamps = priorTimestamps;
            mpHost->setFontsAntiAlias(priorAntiAlias);
            mpHost->setRequiredDiscordUserName(priorDiscordUserName);
            mpHost->setMMCPChatPrefix(priorChatPrefix);
            mpHost->setMMCPPrefixEmotes(priorPrefixEmotes);
            mpHost->setMMCPAddChatMessageNewline(priorChatNewline);
            mpHost->setMMCPShowSnoopInMainConsole(priorSnoopInMain);
        });

        QVERIFY(priorWrapAt != wrapAt);
        QVERIFY(priorWrapIndent != wrapIndent);
        QVERIFY(priorHangingIndent != hangingIndent);
        QVERIFY(priorCommandLineHeight != commandLineHeight);
        QVERIFY(priorLogFileName != logFileName);
        QVERIFY(priorDebugInterval != debugInterval);
        QVERIFY(priorEchoMode != echoMode);
        QVERIFY(priorBlankLines != blankLines);
        QVERIFY(priorDiscordUserName != discordUserName);
        QVERIFY(priorChatPrefix != chatPrefix);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);

        // The line edits are filled without taking the focus: a field being
        // typed into is deliberately left alone by an apply somebody else started
        mpPreferences->lineEdit_logFileName->setText(logFileName);
        mpPreferences->doubleclick_ignore_lineedit->setText(doubleClickIgnore);
        mpPreferences->lineEdit_discordUserName->setText(qsl("  Round_Trip_User "));
        mpPreferences->lineEdit_mmcpChatMessagePrefix->setText(qsl(" %1 ").arg(chatPrefix));

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
        mpPreferences->checkBox_expectCSpaceIdInColonLessMColorCode->setChecked(!priorColorSpaceId);
        mpPreferences->auto_clear_input_line_checkbox->setChecked(!priorAutoClear);
        mpPreferences->disable_password_masking_checkbox->setChecked(!priorPasswordMasking);
        mpPreferences->checkBox_runAllKeyBindings->setChecked(!priorRunAllKeys);
        mpPreferences->mIsLoggingTimestamps->setChecked(!priorTimestamps);
        mpPreferences->checkBox_antiAlias->setChecked(!priorAntiAlias);
        mpPreferences->checkBox_mmcpPrefixEmotes->setChecked(!priorPrefixEmotes);
        mpPreferences->checkBox_mmcpAddChatMessageNewline->setChecked(!priorChatNewline);
        mpPreferences->checkBox_mmcpSnoopInMainConsole->setChecked(!priorSnoopInMain);

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
        QCOMPARE(mpHost->getHaveColorSpaceId(), !priorColorSpaceId);
        QCOMPARE(mpHost->mAutoClearCommandLineAfterSend, !priorAutoClear);
        QCOMPARE(mpHost->disablePasswordMasking(), !priorPasswordMasking);
        QCOMPARE(mpHost->getKeyUnit()->mRunAllKeyMatches, !priorRunAllKeys);
        QCOMPARE(mpHost->mIsLoggingTimestamps, !priorTimestamps);
        QCOMPARE(mpHost->fontsAntiAlias(), !priorAntiAlias);
        QCOMPARE(mpHost->getRequiredDiscordUserName(), discordUserName);
        QCOMPARE(mpHost->getMMCPChatPrefix(), chatPrefix);
        QCOMPARE(mpHost->getMMCPPrefixEmotes(), !priorPrefixEmotes);
        QCOMPARE(mpHost->getMMCPAddChatMessageNewline(), !priorChatNewline);
        QCOMPARE(mpHost->getMMCPShowSnoopInMainConsole(), !priorSnoopInMain);

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
        QVERIFY2(mpHost->mainConsoleView(), "the profile has no console, so it has no maximum buffer size to offer");
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
        const int maximumSize = mpHost->mainConsoleView()->buffer.getMaxBufferSize();
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

    // Ticking the maximum is applied straight away, and the size the user chose
    // has to outlive that for unticking it, then or on a later visit, to put it back
    void test_untickingAnAppliedMaximumBufferSizeBringsBackTheChosenSize()
    {
        QVERIFY2(mpHost->mainConsoleView(), "the profile has no console, so it has no maximum buffer size to offer");
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        const int priorBufferSize = mpHost->getConsoleBufferSize();
        const bool priorUseMax = mpHost->getUseMaxConsoleBufferSize();
        const int priorLinesLimit = buffer.mLinesLimit;
        const int priorBatchDeleteSize = buffer.mBatchDeleteSize;
        restoreLater([=, this, &buffer]() {
            mpHost->setConsoleBufferSize(priorBufferSize);
            mpHost->setUseMaxConsoleBufferSize(priorUseMax);
            buffer.setBufferSize(priorLinesLimit, priorBatchDeleteSize);
        });

        const int chosenSize = 12345;
        mpHost->setMainConsoleBufferSize(chosenSize, 1000, false);
        const int maximumSize = buffer.getMaxBufferSize();
        QVERIFY2(maximumSize != chosenSize, "the machine's maximum buffer size happens to be the size this case picked, so the two cannot be told apart");

        // As setConsoleBufferSize("main", ..., true) does
        mpHost->setMainConsoleBufferSize(999, 1000, true);
        QCOMPARE(buffer.mLinesLimit, maximumSize);
        QCOMPARE(mpHost->getConsoleBufferSize(), chosenSize);
        mpHost->setMainConsoleBufferSize(chosenSize, 1000, false);

        openPreferences();
        QSignalSpy tickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QVERIFY2(applyAndWait(tickSpy), "ticking the maximum was never applied");
        QCOMPARE(buffer.mLinesLimit, maximumSize);

        QSignalSpy untickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), chosenSize);
        QVERIFY2(applyAndWait(untickSpy), "unticking the maximum was never applied");
        QCOMPARE(buffer.mLinesLimit, chosenSize);

        QSignalSpy retickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QVERIFY2(applyAndWait(retickSpy), "ticking the maximum again was never applied");
        closePreferences();

        openPreferences();
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), maximumSize);
        QSignalSpy laterUntickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), chosenSize);
        QVERIFY2(applyAndWait(laterUntickSpy), "unticking the maximum on a later visit was never applied");
        QCOMPARE(buffer.mLinesLimit, chosenSize);
        QCOMPARE(mpHost->getConsoleBufferSize(), chosenSize);
    }

    // Applying is debounced, so a size typed just before ticking the maximum has
    // not reached the profile yet when the tick overwrites the box with the maximum
    void test_aSizeEditedJustBeforeTickingTheMaximumIsKept()
    {
        QVERIFY2(mpHost->mainConsoleView(), "the profile has no console, so it has no maximum buffer size to offer");
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        const int priorBufferSize = mpHost->getConsoleBufferSize();
        const bool priorUseMax = mpHost->getUseMaxConsoleBufferSize();
        const int priorLinesLimit = buffer.mLinesLimit;
        const int priorBatchDeleteSize = buffer.mBatchDeleteSize;
        restoreLater([=, this, &buffer]() {
            mpHost->setConsoleBufferSize(priorBufferSize);
            mpHost->setUseMaxConsoleBufferSize(priorUseMax);
            buffer.setBufferSize(priorLinesLimit, priorBatchDeleteSize);
        });

        mpHost->setMainConsoleBufferSize(12345, 1000, false);
        const int editedSize = 23456;
        QVERIFY2(buffer.getMaxBufferSize() != editedSize, "the machine's maximum buffer size happens to be the size this case picked, so the two cannot be told apart");

        openPreferences();
        QSignalSpy tickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->console_buffer_size_spinBox->setValue(editedSize);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QVERIFY2(applyAndWait(tickSpy), "ticking the maximum was never applied");
        QCOMPARE(mpHost->getConsoleBufferSize(), editedSize);

        QSignalSpy untickSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_useMaxBufferSize->click();
        QCOMPARE(mpPreferences->console_buffer_size_spinBox->value(), editedSize);
        QVERIFY2(applyAndWait(untickSpy), "unticking the maximum was never applied");
        QCOMPARE(buffer.mLinesLimit, editedSize);
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
            mpHost->setMMCPPort(priorPort);
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

    // A picked colour goes to the map for drawing and to the Host for saving,
    // each button to its own end of the ring
    void test_aPickedPlayerRoomColourReachesTheMapAndTheHostTogether()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        const QColor priorMapOuterColor = mpHost->mpMap->mPlayerRoomOuterColor;
        const QColor priorMapInnerColor = mpHost->mpMap->mPlayerRoomInnerColor;
        const quint8 priorMapStyle = mpHost->mpMap->mPlayerRoomStyle;
        quint8 priorStyle = 0;
        quint8 priorOuter = 0;
        quint8 priorInner = 0;
        QColor priorOuterColor;
        QColor priorInnerColor;
        mpHost->getPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        restoreLater([=, this]() {
            mpHost->mpMap->mPlayerRoomOuterColor = priorMapOuterColor;
            mpHost->mpMap->mPlayerRoomInnerColor = priorMapInnerColor;
            mpHost->mpMap->mPlayerRoomStyle = priorMapStyle;
            mpHost->setPlayerRoomStyleDetails(priorStyle, priorOuter, priorInner, priorOuterColor, priorInnerColor);
        });

        openPreferences();
        // Style 3 is the only one that offers the colour buttons
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);
        QVERIFY(mpPreferences->pushButton_playerRoomPrimaryColor->isEnabled());

        const QColor outerColor(12, 34, 56);
        const QColor innerColor(65, 43, 21);
        QVERIFY(priorOuterColor != outerColor && priorInnerColor != innerColor);

        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, outerColor);
        mpPreferences->pushButton_playerRoomPrimaryColor->click();
        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, innerColor);
        mpPreferences->pushButton_playerRoomSecondaryColor->click();

        QCOMPARE(mpHost->mpMap->mPlayerRoomOuterColor, outerColor);
        QCOMPARE(mpHost->mpMap->mPlayerRoomInnerColor, innerColor);
        quint8 hostStyle = 0;
        quint8 hostOuter = 0;
        quint8 hostInner = 0;
        QColor hostOuterColor;
        QColor hostInnerColor;
        mpHost->getPlayerRoomStyleDetails(hostStyle, hostOuter, hostInner, hostOuterColor, hostInnerColor);
        QCOMPARE(hostOuterColor, outerColor);
        QCOMPARE(hostInnerColor, innerColor);
        // ...and the field beside each button shows it
        QCOMPARE(ringColour(qsl("Outer"))->text(), qsl("#0C2238"));
        QCOMPARE(ringColour(qsl("Inner"))->text(), qsl("#412B15"));
    }

    // Each ring's colour is a field showing the map's colour as a code, with its opacity beside it, then the button
    // opening the colours window; the diameters are named by labels of their own, and the colours are only offered
    // while the custom ring is the marker
    void test_thePlayerRoomColoursAreFieldsBesideTheButtonsThatChooseThem()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        restorePlayerRoomLater();
        mpHost->mpMap->mPlayerRoomOuterColor = QColor(12, 34, 56);
        mpHost->mpMap->mPlayerRoomInnerColor = QColor(200, 40, 80, 153);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        uiDesign::ColourValue* pOuter = ringColour(qsl("Outer"));
        uiDesign::ColourValue* pInner = ringColour(qsl("Inner"));
        QVERIFY2(pOuter && pInner && ringOpacity(qsl("Outer")) && ringOpacity(qsl("Inner")), "the Player room marker card has no colour fields with their opacities");
        QCOMPARE(pOuter->text(), qsl("#0C2238"));
        QCOMPARE(pInner->text(), qsl("#C82850"));
        QCOMPARE(ringOpacity(qsl("Outer"))->value(), 100);
        QCOMPARE(ringOpacity(qsl("Inner"))->value(), 60);
        for (QPushButton* pChoose : {mpPreferences->pushButton_playerRoomPrimaryColor, mpPreferences->pushButton_playerRoomSecondaryColor}) {
            QVERIFY2(pChoose->text().isEmpty() && !pChoose->icon().isNull(), qPrintable(qsl("%1 is not a glyph button: '%2'").arg(pChoose->objectName(), pChoose->text())));
        }
        QCOMPARE(mpPreferences->pushButton_playerRoomPrimaryColor->accessibleName(), qsl("Outer ring color"));
        // The compact row of the colour cards, the diameters held to it so each ring's row is one height
        for (QWidget* pPiece : QList<QWidget*>{pOuter,
                                               pInner,
                                               ringOpacity(qsl("Outer")),
                                               ringOpacity(qsl("Inner")),
                                               mpPreferences->pushButton_playerRoomPrimaryColor,
                                               mpPreferences->pushButton_playerRoomSecondaryColor,
                                               mpPreferences->spinBox_playerRoomOuterDiameter,
                                               mpPreferences->spinBox_playerRoomInnerDiameter}) {
            QVERIFY2(pPiece->height() == uiDesign::scmColourWellHeight,
                     qPrintable(qsl("%1 is %2px tall, the colour cards' rows %3px").arg(pPiece->objectName()).arg(pPiece->height()).arg(uiDesign::scmColourWellHeight)));
        }

        const QList<std::pair<QString, QSpinBox*>> diameters{{qsl("Outer"), mpPreferences->spinBox_playerRoomOuterDiameter}, {qsl("Inner"), mpPreferences->spinBox_playerRoomInnerDiameter}};
        for (const auto& [ring, pDiameter] : diameters) {
            QVERIFY2(pDiameter->prefix().isEmpty(), qPrintable(qsl("the %1 diameter still carries its name inside the box: '%2'").arg(ring, pDiameter->prefix())));
            QCOMPARE(pDiameter->suffix(), qsl("%"));
            auto* pLabel = mpPreferences->findChild<QLabel*>(qsl("settingsPlayerRoom%1DiameterLabel").arg(ring));
            QVERIFY2(pLabel && pLabel->buddy() == pDiameter && pLabel->text() == qsl("Diameter"), qPrintable(qsl("the %1 diameter has no label of its own").arg(ring)));
        }

        QComboBox* pStyle = mpPreferences->comboBox_playerRoomStyle;
        for (const int style : {1, 0, 3}) {
            pStyle->setCurrentIndex(style);
            for (const QString& ring : {qsl("Outer"), qsl("Inner")}) {
                const QString where = qsl("style %1, %2 ring").arg(QString::number(style), ring);
                QVERIFY2(ringColour(ring)->isEnabled() == (style == 3), qPrintable(qsl("%1: the colour field does not follow the style").arg(where)));
                QVERIFY2(ringOpacity(ring)->isEnabled() == (style == 3), qPrintable(qsl("%1: the opacity does not follow the style").arg(where)));
                QVERIFY2(mpPreferences->findChild<QLabel*>(qsl("settingsPlayerRoom%1RingLabel").arg(ring))->isEnabled() == (style == 3),
                         qPrintable(qsl("%1: the ring's name does not follow the style").arg(where)));
            }
            QVERIFY2(mpPreferences->findChild<QLabel*>(qsl("settingsPlayerRoomInnerDiameterLabel"))->isEnabled() == (style != 0),
                     qPrintable(qsl("style %1: the inner diameter's label does not follow its box").arg(style)));
        }
    }

    // A code typed into a ring's field goes to the map and the Host as a colour chosen in the colours window does,
    // and only the colour is typed, so the ring stays as see-through as it was
    void test_aTypedPlayerRoomColourReachesTheMapAndTheHostTogether()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        restorePlayerRoomLater();
        mpHost->mpMap->mPlayerRoomInnerColor = QColor(200, 40, 80, 153);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);
        uiDesign::ColourValue* pInner = ringColour(qsl("Inner"));
        QVERIFY(pInner && pInner->isEnabled());
        typeInto(pInner, qsl("#00ff00"), Qt::Key_Return);
        QCOMPARE(pInner->text(), qsl("#00FF00"));
        QCOMPARE(mpHost->mpMap->mPlayerRoomInnerColor, QColor(0, 255, 0, 153));
        QCOMPARE(hostPlayerRoomColours().second, QColor(0, 255, 0, 153));
    }

    // A number typed into a ring's opacity sets how see-through it is and keeps its colour, and the swatch shows the
    // board through it; the box follows a colour chosen in the colours window too
    void test_aTypedPlayerRoomOpacityReachesTheMapAndTheHostTogether()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        restorePlayerRoomLater();
        mpHost->mpMap->mPlayerRoomOuterColor = QColor(12, 34, 56);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);
        uiDesign::ColourValue* pOuter = ringColour(qsl("Outer"));
        QVERIFY(pOuter && pOuter->isEnabled());
        QVERIFY2(!showsTheBoardThrough(pOuter), "the opaque outer ring's swatch shows the board through it");
        typeInto(ringOpacity(qsl("Outer")), qsl("60"), Qt::Key_Return);
        QCOMPARE(mpHost->mpMap->mPlayerRoomOuterColor, QColor(12, 34, 56, 153));
        QCOMPARE(hostPlayerRoomColours().first, QColor(12, 34, 56, 153));
        QCOMPARE(pOuter->text(), qsl("#0C2238"));
        QVERIFY2(showsTheBoardThrough(pOuter), "the see-through outer ring's swatch does not show the board through it");

        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, QColor(1, 2, 3, 51));
        mpPreferences->pushButton_playerRoomPrimaryColor->click();
        QCOMPARE(pOuter->text(), qsl("#010203"));
        QCOMPARE(ringOpacity(qsl("Outer"))->value(), 20);
    }

    void test_aRingFieldPutsBackWhatIsNotAWholeColour_data()
    {
        QTest::addColumn<QString>("typed");
        QTest::addColumn<int>("key");
        QTest::newRow("an unfinished code") << qsl("#12") << static_cast<int>(Qt::Key_Return);
        QTest::newRow("a code left at Escape") << qsl("#123456") << static_cast<int>(Qt::Key_Escape);
        // There is no Reset on this card, so there is no default for an emptied field to go back to
        QTest::newRow("an emptied field") << QString() << static_cast<int>(Qt::Key_Return);
    }

    void test_aRingFieldPutsBackWhatIsNotAWholeColour()
    {
        QFETCH(QString, typed);
        QFETCH(int, key);
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        restorePlayerRoomLater();
        mpHost->mpMap->mPlayerRoomOuterColor = QColor(12, 34, 56);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);
        uiDesign::ColourValue* pOuter = ringColour(qsl("Outer"));
        QVERIFY(pOuter && pOuter->isEnabled());
        typeInto(pOuter, typed, static_cast<Qt::Key>(key));
        QCOMPARE(pOuter->text(), qsl("#0C2238"));
        QVERIFY2(mpPreferences->isVisible(), "the key that ended the edit closed the settings");
        QCOMPARE(mpHost->mpMap->mPlayerRoomOuterColor, QColor(12, 34, 56));
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

    // Every colour of the Main display's and the map's colour cards is a field
    // showing the profile's colour as its code, beside the button choosing it,
    // which shows only a glyph and carries the colour's name. Each colour is
    // given one of its own first, so a field filled from the wrong one shows.
    void test_everyColourOfTheColourCardsIsAFieldShowingTheProfilesColour()
    {
        const QList<colourCardColour> colours = colourCardColours();
        QList<QColor> priorColours;
        for (const auto& [stem, pHeld] : colours) {
            priorColours.append(*pHeld);
        }
        restoreLater([colours, priorColours]() {
            for (int index = 0; index < colours.size(); ++index) {
                *colours.at(index).second = priorColours.at(index);
            }
        });
        const QStringList seeThrough{qsl("background_color"), qsl("background_color_2"), qsl("mapInfoBg"), qsl("upperLevelColor"), qsl("lowerLevelColor"), qsl("mapGridColor")};
        for (int index = 0; index < colours.size(); ++index) {
            *colours.at(index).second = QColor(5 * index, 250 - 5 * index, (37 * index) % 256, seeThrough.contains(colours.at(index).first) ? 200 : 255);
        }

        openPreferences();
        for (const auto& [stem, pHeld] : colours) {
            uiDesign::ColourValue* pField = colourField(stem);
            auto* pChoose = mpPreferences->findChild<QPushButton*>(qsl("pushButton_%1").arg(stem));
            QVERIFY2(pField && pChoose, qPrintable(qsl("'%1' has no field beside its button").arg(stem)));
            QVERIFY2(pField->text() == pHeld->name(QColor::HexRgb).toUpper(), qPrintable(qsl("'%1' shows '%2' for the profile's %3").arg(stem, pField->text(), pHeld->name(QColor::HexArgb))));
            QCOMPARE(pField->colour(), *pHeld);
            QVERIFY2(pChoose->text().isEmpty() && !pChoose->icon().isNull(), qPrintable(qsl("'%1' is not a glyph button").arg(pChoose->objectName())));
            QVERIFY2(!pChoose->accessibleName().isEmpty() && pChoose->accessibleName() == pField->accessibleName(),
                     qPrintable(qsl("'%1' is named '%2' and its field '%3'").arg(stem, pChoose->accessibleName(), pField->accessibleName())));
            QCOMPARE(pChoose->toolTip(), qsl("Choose %1…").arg(pField->accessibleName()));
        }
        QCOMPARE(mpPreferences->findChildren<uiDesign::ColourValue*>(QRegularExpression(qsl("^settingsColour_"))).size(), colours.size());
        // What the .ui file said about a colour now describes the field showing it
        QVERIFY2(colourField(qsl("foreground_color"))->toolTip().contains(qsl("foreground color used by default")), qPrintable(colourField(qsl("foreground_color"))->toolTip()));
    }

    // A code typed into a console colour's field goes where one chosen in the
    // colours window goes: to the profile, and to the color_table scripts read
    void test_aTypedConsoleColourReachesTheProfileAndTheScripts()
    {
        const QColor priorLightRed = mpHost->mLightRed;
        restoreLater([this, priorLightRed]() {
            mpHost->mLightRed = priorLightRed;
            mpHost->updateAnsi16ColorsInTable();
        });

        openPreferences();
        mpPreferences->setTab(qsl("mainDisplay"));
        uiDesign::ColourValue* pLightRed = colourField(qsl("lRed"));
        QVERIFY(pLightRed && pLightRed->isEnabled());
        typeInto(pLightRed, qsl("#1a2b3c"), Qt::Key_Return);
        QCOMPARE(pLightRed->text(), qsl("#1A2B3C"));
        QCOMPARE(mpHost->mLightRed, QColor(0x1A, 0x2B, 0x3C));
        QCOMPARE(colorTableEntry("ansi_009"), QColor(0x1A, 0x2B, 0x3C));
    }

    // ...and so does one typed into one of a card's own colours, the map's
    // background telling the map's widgets as the colours window does
    void test_aTypedNamedColourReachesTheProfile()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map to announce the background to");
        const QColor priorForeground = mpHost->mFgColor;
        const QColor priorMapBackground = mpHost->mBgColor_2;
        const QColor priorMapRed = mpHost->mRed_2;
        restoreLater([this, priorForeground, priorMapBackground, priorMapRed]() {
            mpHost->mFgColor = priorForeground;
            mpHost->mBgColor_2 = priorMapBackground;
            mpHost->mRed_2 = priorMapRed;
            mpHost->mpMap->restore16ColorSet();
        });

        openPreferences();
        mpPreferences->setTab(qsl("mainDisplay"));
        typeInto(colourField(qsl("foreground_color")), qsl("#FEDCBA"), Qt::Key_Return);
        QCOMPARE(mpHost->mFgColor, QColor(0xFE, 0xDC, 0xBA));

        mpPreferences->setTab(qsl("mapper"));
        QSignalSpy announced(mpHost->mpMap.data(), &TMap::signal_mapperSettingChanged);
        typeInto(colourField(qsl("background_color_2")), qsl("#123"), Qt::Key_Return);
        QCOMPARE(mpHost->mBgColor_2, QColor(0x11, 0x22, 0x33));
        QCOMPARE(announced.count(), 1);

        // The rooms are drawn from the map's own copy of the sixteen, which the colours window renews as well
        typeInto(colourField(qsl("red_2")), qsl("#0A0B0C"), Qt::Key_Return);
        QCOMPARE(mpHost->mRed_2, QColor(0x0A, 0x0B, 0x0C));
        QCOMPARE(mpHost->mpMap->mCustomEnvColors.value(257), QColor(0x0A, 0x0B, 0x0C));
    }

    // The five map colours that may be see-through each have an opacity box,
    // and a number typed into it changes how see-through the colour is and
    // nothing else; the box follows a colour chosen in the colours window
    void test_aTypedMapOpacityKeepsTheColour()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map to announce the background to");
        const QColor priorGrid = mpHost->mMapGridColor;
        const QColor priorBackground = mpHost->mBgColor_2;
        restoreLater([this, priorGrid, priorBackground]() {
            mpHost->mMapGridColor = priorGrid;
            mpHost->mBgColor_2 = priorBackground;
        });
        mpHost->mMapGridColor = QColor(10, 20, 30, 64);
        mpHost->mBgColor_2 = QColor(70, 80, 90);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        for (const QString& stem : {qsl("background_color_2"), qsl("mapInfoBg"), qsl("upperLevelColor"), qsl("lowerLevelColor"), qsl("mapGridColor")}) {
            QVERIFY2(mpPreferences->findChild<QSpinBox*>(qsl("settingsOpacity_%1").arg(stem)), qPrintable(qsl("'%1' has no opacity box").arg(stem)));
        }
        auto* pOpacity = mpPreferences->findChild<QSpinBox*>(qsl("settingsOpacity_mapGridColor"));
        QCOMPARE(pOpacity->value(), 25);
        typeInto(pOpacity, qsl("50"), Qt::Key_Return);
        QCOMPARE(mpHost->mMapGridColor, QColor(10, 20, 30, 128));
        QCOMPARE(colourField(qsl("mapGridColor"))->text(), qsl("#0A141E"));

        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, QColor(1, 2, 3, 51));
        mpPreferences->pushButton_mapGridColor->click();
        QCOMPARE(mpHost->mMapGridColor, QColor(1, 2, 3, 51));
        QCOMPARE(colourField(qsl("mapGridColor"))->text(), qsl("#010203"));
        QCOMPARE(pOpacity->value(), 20);

        // The map's background tells the map's widgets, as one chosen in the colours window does
        QSignalSpy announced(mpHost->mpMap.data(), &TMap::signal_mapperSettingChanged);
        typeInto(mpPreferences->findChild<QSpinBox*>(qsl("settingsOpacity_background_color_2")), qsl("60"), Qt::Key_Return);
        QCOMPARE(mpHost->mBgColor_2, QColor(70, 80, 90, 153));
        QCOMPARE(colourField(qsl("background_color_2"))->text(), qsl("#46505A"));
        QCOMPARE(announced.count(), 1);
    }

    // ...and so has the Main display's background
    void test_aTypedDisplayBackgroundOpacityKeepsTheColour()
    {
        const QColor priorBackground = mpHost->mBgColor;
        restoreLater([this, priorBackground]() {
            mpHost->mBgColor = priorBackground;
            mpHost->mainConsoleView()->changeColors();
        });
        mpHost->mBgColor = QColor(40, 50, 60);

        openPreferences();
        mpPreferences->setTab(qsl("mainDisplay"));
        auto* pOpacity = mpPreferences->findChild<QSpinBox*>(qsl("settingsOpacity_background_color"));
        QVERIFY2(pOpacity, "the Main display's background has no opacity box");
        QCOMPARE(pOpacity->value(), 100);
        typeInto(pOpacity, qsl("40"), Qt::Key_Return);
        QCOMPARE(mpHost->mBgColor, QColor(40, 50, 60, 102));
        QCOMPARE(colourField(qsl("background_color"))->text(), qsl("#28323C"));
    }

    // The preview beside the sixteen colours is drawn in the profile's colours
    // and drawn again whenever one changes - typed, chosen in the colours window
    // or put back by Reset all - so what it shows is never the colour before
    void test_thePreviewsFollowTheColours()
    {
        const QColor priorCommandBackground = mpHost->mCommandBgColor;
        const QColor priorMapGreen = mpHost->mGreen_2;
        restoreLater([this, priorCommandBackground, priorMapGreen]() {
            mpHost->mCommandBgColor = priorCommandBackground;
            mpHost->mGreen_2 = priorMapGreen;
            mpHost->mpMap->restore16ColorSet();
        });

        openPreferences();
        const QList<std::tuple<QString, QString, QString, QPushButton*, QPushButton*>> previews{
                {qsl("mainDisplay"), qsl("settingsDisplayColoursPreview"), qsl("command_background_color"), mpPreferences->pushButton_command_background_color, mpPreferences->pushButton_resetColors},
                {qsl("mapper"), qsl("settingsMapColoursPreview"), qsl("green_2"), mpPreferences->pushButton_green_2, mpPreferences->reset_colors_button_2}};
        for (const auto& [page, previewName, stem, pChoose, pReset] : previews) {
            mpPreferences->setTab(page);
            QCoreApplication::processEvents();
            auto* pPreview = mpPreferences->findChild<QWidget*>(previewName);
            QVERIFY2(pPreview && pPreview->isVisible(), qPrintable(qsl("there is no preview on show on the '%1' page").arg(page)));
            // A widget scrolled out of sight is never drawn, however often it asks to be
            TestSettings::pageOf(mpPreferences, page)->ensureWidgetVisible(pPreview, 0, 0);
            QCoreApplication::processEvents();
            PaintCounter paints(pPreview);
            // The window draws itself again as the colours window hands the activation back, a second or so later, and
            // that is no answer to a colour; counting starts once the preview has gone a while without being drawn
            const auto settleThenCount = [&paints]() {
                for (int wait = 0; wait < 20; ++wait) {
                    paints.reset();
                    QTest::qWait(300);
                    if (paints.count() == 0) {
                        return;
                    }
                }
            };
            settleThenCount();

            const QColor typed(0x12, 0xAB, 0x34);
            typeInto(colourField(stem), typed.name(), Qt::Key_Return);
            QTRY_VERIFY2(paints.count() > 0, qPrintable(qsl("the '%1' preview was not drawn again for a typed colour").arg(page)));
            QVERIFY2(pixelsOf(pPreview, typed) > 20, qPrintable(qsl("the '%1' preview does not show the typed %2").arg(page, typed.name())));

            const QColor chosen(0xAB, 0x12, 0x34);
            settleThenCount();
            mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, chosen);
            pChoose->click();
            QTRY_VERIFY2(paints.count() > 0, qPrintable(qsl("the '%1' preview was not drawn again for a chosen colour").arg(page)));
            QCOMPARE(pixelsOf(pPreview, typed), 0);
            QVERIFY2(pixelsOf(pPreview, chosen) > 20, qPrintable(qsl("the '%1' preview does not show the chosen %2").arg(page, chosen.name())));

            settleThenCount();
            pReset->click();
            QTRY_VERIFY2(paints.count() > 0, qPrintable(qsl("the '%1' preview was not drawn again on Reset all").arg(page)));
            QCOMPARE(pixelsOf(pPreview, chosen), 0);
        }
    }

    // The line over the preview's command line is the one the console draws over its own, in the command line's colours
    void test_theDisplayPreviewSeamMatchesTheConsole()
    {
        const QColor priorBackground = mpHost->mCommandLineBgColor;
        const QColor priorForeground = mpHost->mCommandLineFgColor;
        restoreLater([this, priorBackground, priorForeground]() {
            mpHost->mCommandLineBgColor = priorBackground;
            mpHost->mCommandLineFgColor = priorForeground;
            mpHost->mainConsoleView()->changeColors();
        });

        openPreferences();
        mpPreferences->setTab(qsl("mainDisplay"));
        QCoreApplication::processEvents();
        typeInto(colourField(qsl("command_line_background_color")), qsl("#000064"), Qt::Key_Return);
        typeInto(colourField(qsl("command_line_foreground_color")), qsl("#fa0000"), Qt::Key_Return);
        QCoreApplication::processEvents();
        auto* pSeam = mpHost->mainConsoleView()->findChild<QWidget*>(qsl("consoleFootSeam"));
        QVERIFY2(pSeam, "the console has no seam over its command line");
        const QColor seam = pSeam->palette().color(QPalette::Window);
        QVERIFY2(seam != QColor(0x00, 0x00, 0x64) && seam != QColor(0xfa, 0x00, 0x00), qPrintable(seam.name()));

        auto* pPreview = mpPreferences->findChild<QWidget*>(qsl("settingsDisplayColoursPreview"));
        QVERIFY2(pPreview && pPreview->isVisible(), "there is no display preview on show");
        TestSettings::pageOf(mpPreferences, qsl("mainDisplay"))->ensureWidgetVisible(pPreview, 0, 0);
        QCoreApplication::processEvents();
        const int seamPixels = pixelsOf(pPreview, seam);
        QVERIFY2(seamPixels >= pPreview->width() / 2, qPrintable(qsl("the preview shows %1 pixels of the console's seam colour %2 across %3").arg(seamPixels).arg(seam.name()).arg(pPreview->width())));
    }

    // Every colour field's swatch stands for the Choose beside it, so a click on a swatch picks in the colours window
    void test_aClickOnASwatchChoosesInTheColoursWindow()
    {
        const QColor priorCommandBackground = mpHost->mCommandBgColor;
        restoreLater([this, priorCommandBackground]() {
            mpHost->mCommandBgColor = priorCommandBackground;
        });
        openPreferences();
        const QList<uiDesign::ColourValue*> fields = mpPreferences->findChildren<uiDesign::ColourValue*>();
        QCOMPARE(fields.size(), 48);
        for (const uiDesign::ColourValue* pField : fields) {
            QVERIFY2(pField->chooseButton(), qPrintable(qsl("%1 has no Choose for its swatch to stand for").arg(pField->objectName())));
        }
        QCOMPARE(ringColour(qsl("Outer"))->chooseButton(), mpPreferences->pushButton_playerRoomPrimaryColor);

        mpPreferences->setTab(qsl("mainDisplay"));
        QCoreApplication::processEvents();
        uiDesign::ColourValue* pField = colourField(qsl("command_background_color"));
        TestSettings::pageOf(mpPreferences, qsl("mainDisplay"))->ensureWidgetVisible(pField, 0, 0);
        QCoreApplication::processEvents();
        const QColor chosen(0x21, 0x43, 0x65);
        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, chosen);
        const int middle = pField->height() / 2;
        QTest::mouseClick(pField, Qt::LeftButton, Qt::NoModifier, QPoint(middle, middle));
        QTRY_COMPARE(mpHost->mCommandBgColor, chosen);
        QCOMPARE(pField->text(), chosen.name().toUpper());
    }

    // The map preview's grid is drawn with the map's own pen: as wide as the grid width setting makes it against a
    // room, so a thin grid in the map is a thin grid in the preview
    void test_theMapPreviewGridFollowsTheGridWidth()
    {
        const QColor priorGrid = mpHost->mMapGridColor;
        const QColor priorBackground = mpHost->mBgColor_2;
        const double priorWidth = mpHost->mMapGridLineSize;
        const bool priorAntiAlias = mpHost->mMapperUseAntiAlias;
        restoreLater([this, priorGrid, priorBackground, priorWidth, priorAntiAlias]() {
            mpHost->mMapGridColor = priorGrid;
            mpHost->mBgColor_2 = priorBackground;
            mpHost->mMapGridLineSize = priorWidth;
            mpHost->mMapperUseAntiAlias = priorAntiAlias;
        });
        mpHost->mBgColor_2 = QColor(Qt::black);
        mpHost->mMapperUseAntiAlias = true;

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        QCoreApplication::processEvents();
        auto* pPreview = mpPreferences->findChild<QWidget*>(qsl("settingsMapColoursPreview"));
        QVERIFY2(pPreview && pPreview->isVisible(), "there is no map preview on show");
        TestSettings::pageOf(mpPreferences, qsl("mapper"))->ensureWidgetVisible(pPreview, 0, 0);
        QCoreApplication::processEvents();
        // Everything but the grid stays put, so what a grid adds is what the picture gains over one without it
        const auto brightness = [pPreview]() {
            const QImage picture = pPreview->grab().toImage();
            qint64 sum = 0;
            for (int y = 0; y < picture.height(); ++y) {
                for (int x = 0; x < picture.width(); ++x) {
                    sum += qGray(picture.pixel(x, y));
                }
            }
            return sum;
        };
        mpHost->mMapGridColor = QColor(255, 255, 255, 0);
        const qint64 noGrid = brightness();
        mpHost->mMapGridColor = QColor(Qt::white);

        mpPreferences->doubleSpinBox_gridSize->setValue(0.5);
        QCoreApplication::processEvents();
        const qint64 thin = brightness() - noGrid;
        mpPreferences->doubleSpinBox_gridSize->setValue(5.0);
        QCoreApplication::processEvents();
        const qint64 thick = brightness() - noGrid;
        QVERIFY2(thin > 0, "the map preview shows no grid");
        QVERIFY2(thick > 3 * thin, qPrintable(qsl("the grid at width 5 adds %1 to the preview's brightness, at width 0.5 %2").arg(thick).arg(thin)));
    }

    // The map preview's rooms are measured as the map measures them: as large as the room size setting makes them
    // against a cell, bordered as thickly as the border size makes it, and bordered only while borders are shown
    void test_theMapPreviewRoomsFollowTheFeatureSizes()
    {
        const QColor priorGreen = mpHost->mGreen_2;
        const QColor priorBorder = mpHost->mRoomBorderColor;
        const double priorRoomSize = mpHost->mRoomSize;
        const double priorBorderSize = mpHost->mRoomBorderSize;
        const bool priorShowBorders = mpHost->mMapperShowRoomBorders;
        const bool priorAntiAlias = mpHost->mMapperUseAntiAlias;
        restoreLater([this, priorGreen, priorBorder, priorRoomSize, priorBorderSize, priorShowBorders, priorAntiAlias]() {
            mpHost->mGreen_2 = priorGreen;
            mpHost->mRoomBorderColor = priorBorder;
            mpHost->mRoomSize = priorRoomSize;
            mpHost->mRoomBorderSize = priorBorderSize;
            mpHost->mMapperShowRoomBorders = priorShowBorders;
            mpHost->mMapperUseAntiAlias = priorAntiAlias;
            mpHost->mpMap->restore16ColorSet();
        });
        const QColor green(0x12, 0xAB, 0x34);
        const QColor border(0xAB, 0x34, 0x12);
        mpHost->mGreen_2 = green;
        mpHost->mRoomBorderColor = border;
        // Every edge on whole pixels, so the colours can be counted exactly
        mpHost->mMapperUseAntiAlias = false;

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        QCoreApplication::processEvents();
        auto* pPreview = mpPreferences->findChild<QWidget*>(qsl("settingsMapColoursPreview"));
        QVERIFY2(pPreview && pPreview->isVisible(), "there is no map preview on show");
        mpPreferences->checkbox_mMapperShowRoomBorders->setChecked(true);

        mpPreferences->spinBox_roomSize->setValue(3);
        const int smallRoom = pixelsOf(pPreview, green);
        mpPreferences->spinBox_roomSize->setValue(9);
        const int largeRoom = pixelsOf(pPreview, green);
        QVERIFY2(smallRoom > 0 && largeRoom > 4 * smallRoom, qPrintable(qsl("a room covers %1 pixels at room size 3 and %2 at room size 9").arg(smallRoom).arg(largeRoom)));

        mpPreferences->spinBox_borderSize->setValue(1);
        const int thinBorders = pixelsOf(pPreview, border);
        mpPreferences->spinBox_borderSize->setValue(11);
        const int thickBorders = pixelsOf(pPreview, border);
        QVERIFY2(thinBorders > 0 && thickBorders > 2 * thinBorders, qPrintable(qsl("the borders cover %1 pixels at border size 1 and %2 at border size 11").arg(thinBorders).arg(thickBorders)));

        mpPreferences->checkbox_mMapperShowRoomBorders->setChecked(false);
        QCOMPARE(pixelsOf(pPreview, border), 0);
    }

    // Each of the sixteen environment colours has a room on show in the map preview, so a change to any of them shows
    void test_theMapPreviewShowsEveryEnvironmentColour()
    {
        const QList<std::pair<QString, QColor Host::*>> environment{{qsl("mBlack_2"), &Host::mBlack_2},
                                                                    {qsl("mLightBlack_2"), &Host::mLightBlack_2},
                                                                    {qsl("mRed_2"), &Host::mRed_2},
                                                                    {qsl("mLightRed_2"), &Host::mLightRed_2},
                                                                    {qsl("mGreen_2"), &Host::mGreen_2},
                                                                    {qsl("mLightGreen_2"), &Host::mLightGreen_2},
                                                                    {qsl("mYellow_2"), &Host::mYellow_2},
                                                                    {qsl("mLightYellow_2"), &Host::mLightYellow_2},
                                                                    {qsl("mBlue_2"), &Host::mBlue_2},
                                                                    {qsl("mLightBlue_2"), &Host::mLightBlue_2},
                                                                    {qsl("mMagenta_2"), &Host::mMagenta_2},
                                                                    {qsl("mLightMagenta_2"), &Host::mLightMagenta_2},
                                                                    {qsl("mCyan_2"), &Host::mCyan_2},
                                                                    {qsl("mLightCyan_2"), &Host::mLightCyan_2},
                                                                    {qsl("mWhite_2"), &Host::mWhite_2},
                                                                    {qsl("mLightWhite_2"), &Host::mLightWhite_2}};
        QList<QColor> priorColours;
        for (const auto& [name, pHeld] : environment) {
            priorColours.append(mpHost->*pHeld);
        }
        const bool priorAntiAlias = mpHost->mMapperUseAntiAlias;
        restoreLater([this, environment, priorColours, priorAntiAlias]() {
            for (int index = 0; index < environment.size(); ++index) {
                mpHost->*environment.at(index).second = priorColours.at(index);
            }
            mpHost->mMapperUseAntiAlias = priorAntiAlias;
            mpHost->mpMap->restore16ColorSet();
        });
        // Every edge on whole pixels, so a room's own colour is there to count
        mpHost->mMapperUseAntiAlias = false;

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        QCoreApplication::processEvents();
        auto* pPreview = mpPreferences->findChild<QWidget*>(qsl("settingsMapColoursPreview"));
        QVERIFY2(pPreview && pPreview->isVisible(), "there is no map preview on show");
        const QColor marker(0x13, 0xA7, 0x5D);
        QVERIFY2(pixelsOf(pPreview, marker) == 0, "SETUP: the map preview already shows the colour standing out");
        QStringList unseen;
        for (const auto& [name, pHeld] : environment) {
            const QColor prior = mpHost->*pHeld;
            mpHost->*pHeld = marker;
            if (pixelsOf(pPreview, marker) == 0) {
                unseen.append(name);
            }
            mpHost->*pHeld = prior;
        }
        QVERIFY2(unseen.isEmpty(), qPrintable(qsl("the map preview shows no room in %1").arg(unseen.join(qsl(", ")))));
    }

    // The diameters and the opacities are percentages standing one over the
    // other: each pair one width whatever it holds, every value right-aligned so
    // the percent signs line up, and the widest a box can hold in its middle
    void test_thePlayerRoomPercentagesLineUp()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so the player room controls are never enabled");
        restorePlayerRoomLater();
        mpHost->mpMap->mPlayerRoomOuterColor = QColor(12, 34, 56);
        mpHost->mpMap->mPlayerRoomInnerColor = QColor(200, 40, 80, 153);

        openPreferences();
        mpPreferences->setTab(qsl("mapper"));
        mpPreferences->comboBox_playerRoomStyle->setCurrentIndex(3);
        QSpinBox* pOuter = mpPreferences->spinBox_playerRoomOuterDiameter;
        QSpinBox* pInner = mpPreferences->spinBox_playerRoomInnerDiameter;
        pOuter->setValue(pOuter->maximum());
        pInner->setValue(80);
        QCoreApplication::processEvents();
        QCOMPARE(pOuter->size(), pInner->size());
        const QList<std::pair<QSpinBox*, QSpinBox*>> pairs{{pOuter, pInner}, {ringOpacity(qsl("Outer")), ringOpacity(qsl("Inner"))}};
        QCOMPARE(pairs.last().first->value(), 100);
        QCOMPARE(pairs.last().second->value(), 60);
        for (const auto& [pWidest, pShorter] : pairs) {
            QCOMPARE(pWidest->alignment() & Qt::AlignHorizontal_Mask, Qt::AlignRight);
            const auto [widestStart, widestEnd] = inkAcross(pWidest);
            const auto [shorterStart, shorterEnd] = inkAcross(pShorter);
            QVERIFY2(widestStart >= 0 && shorterStart >= 0, qPrintable(qsl("no words found in '%1' or '%2'").arg(pWidest->objectName(), pShorter->objectName())));
            QVERIFY2(qAbs(widestEnd - shorterEnd) <= 1,
                     qPrintable(qsl("'%1' ends at %2px and '%3' at %4px, so their percent signs do not line up").arg(pWidest->text()).arg(widestEnd).arg(pShorter->text()).arg(shorterEnd)));
            QVERIFY2(shorterStart > widestStart, qPrintable(qsl("'%1' does not start after the wider '%2'").arg(pShorter->text(), pWidest->text())));
            const QRect field = pWidest->findChild<QLineEdit*>()->geometry();
            const int leading = widestStart - field.left();
            const int trailing = field.right() - widestEnd;
            QVERIFY2(qAbs(leading - trailing) <= 2, qPrintable(qsl("'%1' stands %2px from the field's leading edge and %3px from its trailing one").arg(pWidest->text()).arg(leading).arg(trailing)));
        }
    }

    // sysSettingChanged is what a script watches to follow a setting it does not
    // own. A Lua spec can only reach setConfig; the dialog is the other writer,
    // and it writes twice - the live slot as the box is ticked, then the
    // debounced apply, which rewrites every control it finds dirty. The second
    // write must not raise a second event.
    void test_theAccessibilityCheckboxesEachRaiseOneSettingChangedEvent()
    {
        const bool priorClosedCaption = mpHost->mEnableClosedCaption;
        const bool priorScreenReader = mpHost->mAdvertiseScreenReader;
        const bool priorAnnounce = mpHost->mAnnounceIncomingText;
        restoreLater([=, this]() {
            mpHost->mEnableClosedCaption = priorClosedCaption;
            mpHost->mAdvertiseScreenReader = priorScreenReader;
            mpHost->mAnnounceIncomingText = priorAnnounce;
        });

        QVERIFY(runLua(qsl("settingChangedCounts = {}\n"
                           "settingChangedValues = {}\n"
                           "settingChangedHandler = registerAnonymousEventHandler('sysSettingChanged', function(_, key, value)\n"
                           "  settingChangedCounts[key] = (settingChangedCounts[key] or 0) + 1\n"
                           "  settingChangedValues[key] = type(value) .. ':' .. tostring(value)\n"
                           "end)\n")));
        restoreLater([this]() {
            runLua(qsl("killAnonymousEventHandler(settingChangedHandler)"));
        });

        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);

        // Nothing spins the event loop between these three, so the debounce
        // cannot have run by the time the counts below are read
        mpPreferences->checkBox_enableClosedCaption->setChecked(!priorClosedCaption);
        mpPreferences->checkBox_advertiseScreenReader->setChecked(!priorScreenReader);
        mpPreferences->checkBox_announceIncomingText->setChecked(!priorAnnounce);

        QCOMPARE(settingChangedCount("enableClosedCaption"), 1);
        QCOMPARE(settingChangedValue("enableClosedCaption"), priorClosedCaption ? qsl("boolean:false") : qsl("boolean:true"));
        QCOMPARE(settingChangedCount("advertiseScreenReader"), 1);
        QCOMPARE(settingChangedValue("advertiseScreenReader"), priorScreenReader ? qsl("boolean:false") : qsl("boolean:true"));
        QCOMPARE(settingChangedCount("announceIncomingText"), 0);

        QVERIFY2(applyAndWait(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(settingChangedCount("announceIncomingText"), 1);
        QCOMPARE(settingChangedValue("announceIncomingText"), priorAnnounce ? qsl("boolean:false") : qsl("boolean:true"));
        QVERIFY2(settingChangedCount("enableClosedCaption") == 1, "the apply raised enableClosedCaption again for a value the live slot had already stored");
        QVERIFY2(settingChangedCount("advertiseScreenReader") == 1, "the apply raised advertiseScreenReader again for a value the live slot had already stored");
    }

    // App-wide rather than the profile's: read from and written to Mudlet.ini, and the running pool is
    // reconfigured at once rather than at the next start
    void test_theTriggerMatchPoolSettingsAreReadFromMudletIniAndAppliedAtOnce()
    {
        if (QThread::idealThreadCount() < 2) {
            QSKIP("the pool needs a second core before it starts a helper");
        }
        for (const char* name : {"MUDLET_MATCH_THREADS", "MUDLET_MATCH_THRESHOLD", "MUDLET_MATCH_MISSES_PER_MATCH", "MUDLET_MATCH_FLOOD_LINES", "MUDLET_MATCH_SPIN_US"}) {
            if (qEnvironmentVariableIsSet(name)) {
                QSKIP("a MUDLET_MATCH_* environment variable overrides the setting under test");
            }
        }
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings);
        const QStringList keys{qsl("triggerMatchThreads"), qsl("triggerMatchThreshold"), qsl("triggerMatchMissesPerMatch"), qsl("triggerMatchFloodLines"), qsl("triggerMatchSpinMicroseconds")};
        QMap<QString, QVariant> prior;
        for (const auto& key : keys) {
            prior.insert(key, settings->value(key));
        }
        restoreLater([settings, prior]() {
            qunsetenv("MUDLET_MATCH_THRESHOLD");
            for (auto it = prior.cbegin(); it != prior.cend(); ++it) {
                it.value().isValid() ? settings->setValue(it.key(), it.value()) : settings->remove(it.key());
            }
            TriggerMatchPool::instance().configure(TriggerMatchPool::settingsFromConfig());
        });
        TriggerMatchPool& pool = TriggerMatchPool::instance();
        for (const auto& key : keys) {
            settings->remove(key);
        }
        pool.configure(TriggerMatchPool::settingsFromConfig());
        QVERIFY2(pool.workerCount() == 0, "the pool should be off until the player switches it on");

        settings->setValue(qsl("triggerMatchThreshold"), 77);
        // Below its minimum, so the pool uses the default and the dialog has to show that
        settings->setValue(qsl("triggerMatchFloodLines"), 0);
        openPreferences();
        auto* pEnable = mpPreferences->findChild<QCheckBox*>(qsl("checkBox_triggerMatchPool"));
        auto* pThreads = mpPreferences->findChild<QSpinBox*>(qsl("spinBox_triggerMatchThreads"));
        auto* pThreshold = mpPreferences->findChild<QSpinBox*>(qsl("spinBox_triggerMatchThreshold"));
        auto* pMissesPerMatch = mpPreferences->findChild<QSpinBox*>(qsl("spinBox_triggerMatchMissesPerMatch"));
        auto* pFloodLines = mpPreferences->findChild<QSpinBox*>(qsl("spinBox_triggerMatchFloodLines"));
        auto* pSpin = mpPreferences->findChild<QSpinBox*>(qsl("spinBox_triggerMatchSpinMicroseconds"));
        QVERIFY(pEnable && pThreads && pThreshold && pMissesPerMatch && pFloodLines && pSpin);
        QVERIFY(!pEnable->isChecked());
        QVERIFY2(!pThreshold->isEnabled(), "tuning a pool that is off should not be offered");
        QCOMPARE(pThreshold->value(), 77);
        QCOMPARE(pThreads->value(), TriggerMatchPool::automaticThreads() - 1);
        const TriggerMatchPool::Settings defaults;
        QCOMPARE(pMissesPerMatch->value(), defaults.missesPerMatch);
        QCOMPARE(pFloodLines->value(), defaults.floodChunkLines);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pEnable->setChecked(true);
        QVERIFY(pThreshold->isEnabled());
        QVERIFY(applyAndWait(applySpy));
        QCOMPARE(settings->value(qsl("triggerMatchThreads")).toInt(), TriggerMatchPool::automaticThreads());
        QCOMPARE(pool.workerCount(), TriggerMatchPool::automaticThreads());
        QCOMPARE(pool.threshold(), 77);
        for (const auto& key : {qsl("triggerMatchMissesPerMatch"), qsl("triggerMatchSpinMicroseconds")}) {
            QVERIFY2(!settings->contains(key), qPrintable(qsl("%1 was written although the player never changed it").arg(key)));
        }
        QCOMPARE(settings->value(qsl("triggerMatchFloodLines")).toInt(), 0);
        QCOMPARE(pool.floodChunkLines(), defaults.floodChunkLines);

        // The field counts the helpers; the setting counts the main thread too. The default can sit at
        // either end of the range, depending on the machine's cores.
        if (pThreads->maximum() > pThreads->minimum()) {
            const int helpers = pThreads->value() == pThreads->minimum() ? pThreads->maximum() : pThreads->minimum();
            applySpy.clear();
            pThreads->setValue(helpers);
            QVERIFY(applyAndWait(applySpy));
            QCOMPARE(settings->value(qsl("triggerMatchThreads")).toInt(), helpers + 1);
            QCOMPARE(pool.workerCount(), helpers + 1);
        }

        applySpy.clear();
        pThreshold->setValue(64);
        pMissesPerMatch->setValue(5);
        pFloodLines->setValue(9);
        pSpin->setValue(33);
        QVERIFY(applyAndWait(applySpy));
        QCOMPARE(settings->value(qsl("triggerMatchThreshold")).toInt(), 64);
        QCOMPARE(settings->value(qsl("triggerMatchMissesPerMatch")).toInt(), 5);
        QCOMPARE(settings->value(qsl("triggerMatchFloodLines")).toInt(), 9);
        QCOMPARE(settings->value(qsl("triggerMatchSpinMicroseconds")).toInt(), 33);
        QCOMPARE(pool.threshold(), 64);
        QCOMPARE(pool.missesPerMatch(), 5);
        QCOMPARE(pool.floodChunkLines(), 9);
        QCOMPARE(pool.settings().spinMicroseconds, 33);

        // An environment variable still beats what the dialog wrote
        qputenv("MUDLET_MATCH_THRESHOLD", "200");
        applySpy.clear();
        pThreshold->setValue(65);
        QVERIFY(applyAndWait(applySpy));
        QCOMPARE(settings->value(qsl("triggerMatchThreshold")).toInt(), 65);
        QCOMPARE(pool.threshold(), 200);
        qunsetenv("MUDLET_MATCH_THRESHOLD");

        applySpy.clear();
        pEnable->setChecked(false);
        QVERIFY(applyAndWait(applySpy));
        QCOMPARE(settings->value(qsl("triggerMatchThreads")).toInt(), 0);
        QCOMPARE(pool.workerCount(), 0);
    }

    // The box is tri-state, and what the apply has to write is the box's own
    // state rather than the profile's. While the setting was reachable only
    // from setConfig() the apply read the profile and wrote it straight back,
    // so a box put back on the dialog over that apply would be inert (#8148).
    void test_theAmbiguousEastAsianWidthBoxReachesTheProfile()
    {
        const Qt::CheckState prior = mpHost->getWideAmbiguousEAsianGlyphsControlState();
        restoreLater([this, prior]() {
            mpHost->setWideAmbiguousEAsianGlyphs(prior);
        });

        openPreferences();
        QCheckBox* pBox = mpPreferences->checkBox_useWideAmbiguousEastAsianGlyphs;
        QCOMPARE(pBox->checkState(), prior);
        const Qt::CheckState wanted = (prior == Qt::Checked) ? Qt::Unchecked : Qt::Checked;

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pBox->setCheckState(wanted);
        QVERIFY2(applyAndWait(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(mpHost->getWideAmbiguousEAsianGlyphsControlState(), wanted);
        QCOMPARE(mpHost->wideAmbiguousEAsianGlyphs(), wanted == Qt::Checked);

        // The third state is the one the box is tri-state for - it says "decide
        // from the encoding" rather than yes or no - and it has to reach the
        // profile as itself rather than as whichever of the two it looks like.
        QSignalSpy backSpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pBox->setCheckState(Qt::PartiallyChecked);
        QVERIFY2(applyAndWait(backSpy), "the debounce never wrote the settings back");
        QCOMPARE(mpHost->getWideAmbiguousEAsianGlyphsControlState(), Qt::PartiallyChecked);
    }

    // The script editor reads code in the display font until the box is unticked; the two controls
    // under it show what following gives, greyed out, and pick the editor's own font once it is
    void test_theEditorFontIsTheDisplayFontUntilTheBoxIsUnticked()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so the bundled \"Ubuntu Mono\" this gives the editor as a font of its own is not in the resources");
#else
        QVERIFY(mpHost->editorFontFollowsDisplay());
        restoreLater([this]() {
            mpHost->setEditorFontFollowsDisplay(true);
        });
        const QFont display = mpHost->getDisplayFont();
        QVERIFY(display.family() != mEditorOwnFamily);

        openPreferences();
        QCheckBox* pFollow = mpPreferences->checkBox_editorFontFollowsDisplay;
        QFontComboBox* pFamily = mpPreferences->fontComboBox_editorFont;
        QSpinBox* pSize = mpPreferences->spinBox_editorFontSize;
        QVERIFY(pFollow->isChecked());
        QVERIFY2(!pFamily->isEnabled() && !pSize->isEnabled(), "the editor font can be picked while it follows the display font");
        QCOMPARE(pFamily->currentFont().family(), display.family());
        QCOMPARE(pSize->value(), display.pointSize());

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pFollow->click();
        QVERIFY2(pFamily->isEnabled() && pSize->isEnabled(), "unticking the box left the editor font greyed out");
        QVERIFY2(applyAndWait(applySpy), "the debounce never wrote the settings back");
        QVERIFY(!mpHost->editorFontFollowsDisplay());
        // ...starting from what it showed
        QCOMPARE(mpHost->getEditorFont().family(), display.family());
        QCOMPARE(mpHost->getEditorFont().pointSize(), display.pointSize());

        applySpy.clear();
        pFamily->setCurrentFont(QFont(mEditorOwnFamily));
        QVERIFY2(applyAndWait(applySpy), "picking a family never wrote the settings back");
        QCOMPARE(mpHost->getEditorFont().family(), mEditorOwnFamily);
        QCOMPARE(mpHost->getEditorFont().pointSize(), display.pointSize());

        applySpy.clear();
        pSize->stepBy(1);
        QVERIFY2(applyAndWait(applySpy), "stepping the size never wrote the settings back");
        QCOMPARE(mpHost->getEditorFont().family(), mEditorOwnFamily);
        QCOMPARE(mpHost->getEditorFont().pointSize(), display.pointSize() + 1);
        QVERIFY2(TFontAttributes(mpHost->getDisplayFont()) == TFontAttributes(display), "the editor's own font reached the display font");

        applySpy.clear();
        pFollow->click();
        QVERIFY2(applyAndWait(applySpy), "ticking the box never wrote the settings back");
        QVERIFY(mpHost->editorFontFollowsDisplay());
        QCOMPARE(mpHost->getEditorFont().family(), display.family());
        QVERIFY2(!pFamily->isEnabled() && !pSize->isEnabled(), "ticking the box again left the editor font open to picking");
        QCOMPARE(pFamily->currentFont().family(), display.family());
        QCOMPARE(pSize->value(), display.pointSize());
#endif
    }

    // Both are the profile's, so they outlast the session
    void test_theEditorFontIsSavedWithTheProfileAndReadBack()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so the bundled \"Ubuntu Mono\" this gives the editor as a font of its own is not in the resources");
#else
        restoreLater([this]() {
            mpHost->setEditorFontFollowsDisplay(true);
        });
        const int ownSize = 17;
        QVERIFY(mpHost->getDisplayFont().pointSize() != ownSize);

        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_editorFontFollowsDisplay->click();
        mpPreferences->fontComboBox_editorFont->setCurrentFont(QFont(mEditorOwnFamily));
        mpPreferences->spinBox_editorFontSize->setValue(ownSize);
        QVERIFY2(applyAndWait(applySpy), "the debounce never wrote the settings back");
        QVERIFY(!mpHost->editorFontFollowsDisplay());
        QCOMPARE(mpHost->getEditorFont().family(), mEditorOwnFamily);
        QCOMPARE(mpHost->getEditorFont().pointSize(), ownSize);

        QTemporaryDir saveDir;
        QVERIFY(saveDir.isValid());
        mpHost->waitForProfileSave();
        const auto [saved, xmlPath, saveError] = mpHost->saveProfile(saveDir.path(), qsl("editorFont"));
        QVERIFY2(saved, qPrintable(saveError));
        mpHost->waitForProfileSave();

        QFile file(xmlPath);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        const QString xml = QString::fromUtf8(file.readAll());
        QVERIFY2(xml.contains(qsl("<mEditorFont>%1,%2,").arg(mEditorOwnFamily).arg(ownSize)), "the editor's own font is not in the profile's save");
        QVERIFY2(xml.contains(qsl("<mEditorFontFollowsDisplay>no</mEditorFontFollowsDisplay>")), "the save says the editor follows the display font");

        // Moved off both, or reading the save back would prove nothing
        mpHost->setEditorFont(QFont(Host::scmDefaultFontFamily, ownSize + 4));
        mpHost->setEditorFontFollowsDisplay(true);
        QVERIFY2(mpPreferences->checkBox_editorFontFollowsDisplay->isChecked() && mpPreferences->spinBox_editorFontSize->value() != ownSize, "the dialog did not hear the editor font move behind it");
        file.seek(0);
        XMLimport importer(mpHost);
        const auto [imported, importMessage] = importer.importPackage(&file);
        QVERIFY2(imported, qPrintable(importMessage));
        QVERIFY2(!mpHost->editorFontFollowsDisplay(), "reading the save back left the editor following the display font");
        QCOMPARE(mpHost->getEditorFont().family(), mEditorOwnFamily);
        QCOMPARE(mpHost->getEditorFont().pointSize(), ownSize);

        // ...and the dialog, still open, shows what was read
        QVERIFY(!mpPreferences->checkBox_editorFontFollowsDisplay->isChecked());
        QVERIFY(mpPreferences->fontComboBox_editorFont->isEnabled());
        QCOMPARE(mpPreferences->fontComboBox_editorFont->currentFont().family(), mEditorOwnFamily);
        QCOMPARE(mpPreferences->spinBox_editorFontSize->value(), ownSize);
#endif
    }
};

#include "SettingsRoundTripTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsRoundTripTest)
