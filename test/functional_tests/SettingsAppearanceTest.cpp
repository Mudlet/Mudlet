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
 * Appearance is the one setting that repaints the dialog it is changed from.
 * The shell draws its own surfaces - the pages, the sidebar, the cards - from
 * a stylesheet built out of a palette, so a theme change that does not reach
 * that stylesheet leaves the whole of the dialog in the previous theme while
 * the text on it turns over to the new one.
 *
 * Run with: ctest -R SettingsAppearanceTest -V
 */

#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsAppearanceTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdgCache;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsAppearance-Test");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void writeFreshEditorThemesFile()
    {
        const QString file = MudletPaths::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
    }

    QWidget* shell() const { return mpPreferences->findChild<QWidget*>(qsl("settingsShell")); }

    // mudlet::showOptionsDialog() assigns the profile's Lua stylesheet to the
    // dialog on every show, so a dialog built by hand here is not the one the
    // application puts on screen until it has one too.
    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->setStyleSheet(mpHost->mProfileStyleSheet);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(shell(), "the settings shell was never built");
    }

    void setAppearance(const enums::Appearance state)
    {
        mpPreferences->comboBox_appearance->setCurrentIndex(state);
        QCoreApplication::processEvents();
        // A dialog on screen paints itself between one change and the next, and
        // painting is what settles a widget's palette against the application's.
        // Under the offscreen platform nothing paints unless it is asked to, so
        // without this a case would measure a dialog no user could be looking at.
        mpPreferences->grab();
    }

    static QColor pixelOf(QWidget* pWidget, const QPoint& point)
    {
        const QPixmap shot = pWidget->grab();
        return shot.toImage().pixelColor(point);
    }

    // The sidebar keeps a 16px margin under its last item, so the bottom left
    // of the shell is one of its own surfaces rather than anything on a page -
    // and the shell and the sidebar are painted the same colour, so this reads
    // the page colour whichever of the two the pixel lands on.
    QColor paintedSurface() const { return pixelOf(shell(), QPoint(3, shell()->height() - 3)); }

    // Which side of the light/dark line the application has moved to. The shell
    // has to be on the same one, whatever it was painted in a moment ago.
    static bool applicationIsLight() { return QApplication::palette().color(QPalette::Base).lightness() >= 128; }

    static qreal relativeLuminance(const QColor& colour)
    {
        const auto channel = [](const qreal value) {
            return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * channel(colour.redF()) + 0.7152 * channel(colour.greenF()) + 0.0722 * channel(colour.blueF());
    }

    static qreal contrastRatio(const QColor& one, const QColor& other)
    {
        const qreal first = relativeLuminance(one);
        const qreal second = relativeLuminance(other);
        return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
    }

    static QString describe(const QColor& surface)
    {
        return qsl("the shell is painted %1 (lightness %2) while the application palette is %3 (Base %4)")
                .arg(surface.name(), QString::number(surface.lightness()), applicationIsLight() ? qsl("light") : qsl("dark"), QApplication::palette().color(QPalette::Base).name());
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
        QVERIFY(mCacheDir.isValid());
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        writeFreshEditorThemesFile();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
    }

    void init() { openPreferences(); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        mudlet::self()->setAppearance(enums::Appearance::systemSetting);
    }

    // The dialog opens in one theme and is asked for the other while it is
    // still on screen. Nothing here reads a colour this test chose: the shell
    // is only required to end up on the same side of the light/dark line as
    // the application palette it is supposed to be drawn from.
    void test_theShellFollowsAThemeChangeMadeWhileItIsOpen()
    {
        setAppearance(enums::Appearance::dark);
        QVERIFY2(!applicationIsLight(), "the application did not go dark, so the flip below is not the one this case is about");
        QVERIFY2(paintedSurface().lightness() < 128, qPrintable(describe(paintedSurface())));

        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so the flip below is not the one this case is about");
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));
    }

    // ...and the same the other way round, since a fix that reads the theme
    // once could be right in one direction and wrong in the other
    void test_theShellFollowsAThemeChangeBackToDark()
    {
        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so the flip below is not the one this case is about");
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));

        setAppearance(enums::Appearance::dark);
        QVERIFY2(!applicationIsLight(), "the application did not go dark, so the flip below is not the one this case is about");
        QVERIFY2(paintedSurface().lightness() < 128, qPrintable(describe(paintedSurface())));
    }

    // A card is filled by the shell stylesheet and the text on it is not, so when
    // the two stop agreeing about the theme the result is a dark card under dark
    // text - 1.2:1 before this was fixed, against the 4.5:1 text should keep
    void test_aCardsTextStaysReadableAfterAThemeChange()
    {
        setAppearance(enums::Appearance::dark);
        setAppearance(enums::Appearance::light);

        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"));
        QVERIFY2(pCard, "the Appearance card this case reads its colours off is not there any more");
        auto* pLabel = mpPreferences->label_appearance;
        const QColor fill = pixelOf(pCard, QPoint(pCard->width() / 2, pCard->height() - 4));
        const QColor ink = pLabel->palette().color(pLabel->foregroundRole());
        const qreal ratio = contrastRatio(fill, ink);
        QVERIFY2(ratio >= 4.5, qPrintable(qsl("a card is painted %1 under %2 text, a contrast of %3:1").arg(fill.name(), ink.name(), QString::number(ratio, 'f', 2))));
    }

    // The shell's surfaces are its own, and a profile's Lua stylesheet is
    // applied to the whole dialog - so the one must not be able to repaint the
    // other, before a theme change or after one.
    void test_aProfileStyleSheetDoesNotTakeOverTheShellsSurfaces()
    {
        setAppearance(enums::Appearance::dark);
        mpPreferences->setStyleSheet(qsl("QWidget { background-color: rgb(255, 0, 0); }"));
        QCoreApplication::processEvents();
        const QColor beforeTheFlip = paintedSurface();
        QVERIFY2(!(beforeTheFlip.red() > 200 && beforeTheFlip.green() < 60), qPrintable(qsl("the profile stylesheet painted the shell %1 before any theme change").arg(beforeTheFlip.name())));

        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so this is not the mid-life flip");
        const QColor surface = paintedSurface();
        QVERIFY2(!(surface.red() > 200 && surface.green() < 60), qPrintable(qsl("the profile stylesheet painted the shell %1 after the theme change").arg(surface.name())));
        QVERIFY2(surface.lightness() >= 128, qPrintable(describe(surface)));
    }
};

#include "SettingsAppearanceTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsAppearanceTest)
