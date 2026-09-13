/***************************************************************************
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

/*
 * A profile stores its editor theme by name; the file itself lives in the
 * shared theme download cache, so a profile moved to another machine can name
 * a theme that is not there. edbee's editor then paints with its fallback
 * theme, and the trigger pattern line edits must degrade the same way.
 *
 * Run with: ctest -R MissingEditorThemeTest -V
 */

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SingleLineTextEdit.h"
#include "mudlet.h"

#include "edbee/edbee.h"
#include "edbee/views/texttheme.h"

#include "GroupedTest.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

class MissingEditorThemeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_patternEditPaintsWithTheFallbackThemeWhenTheProfilesThemeIsMissing()
    {
        auto* themeManager = edbee::Edbee::instance()->themeManager();
        auto* fallback = themeManager->fallbackTheme();
        QVERIFY(fallback);
        const QString missingTheme = qsl("Theme-that-never-reached-this-machine");
        QVERIFY2(!themeManager->theme(missingTheme), "the theme this test relies on being absent was found");

        SingleLineTextEdit patternEdit;
        // The fallback is black on white, which a default palette may already
        // be, so pre-paint with a colour it does not use to prove setTheme()
        // applied it
        const QColor sentinel(Qt::magenta);
        QVERIFY(fallback->backgroundColor() != sentinel && fallback->foregroundColor() != sentinel);
        QPalette sentinelPalette = patternEdit.palette();
        sentinelPalette.setColor(QPalette::Base, sentinel);
        sentinelPalette.setColor(QPalette::Text, sentinel);
        patternEdit.setPalette(sentinelPalette);
        patternEdit.viewport()->setPalette(sentinelPalette);

        // The highlighter colours an anchor from the theme's comment scope: green
        // in the Mudlet theme the constructor applied, and the fallback theme has
        // no scopes, so afterwards the anchor can only carry its plain colours
        patternEdit.setPlainText(qsl("^"));
        auto anchorFormat = [&patternEdit]() {
            const auto ranges = patternEdit.document()->firstBlock().layout()->formats();
            return ranges.isEmpty() ? QTextCharFormat() : ranges.first().format;
        };
        QCOMPARE(anchorFormat().foreground().color(), QColor(0x00, 0x80, 0x00));

        patternEdit.setTheme(missingTheme);

        QCOMPARE(patternEdit.palette().color(QPalette::Base), fallback->backgroundColor());
        QCOMPARE(patternEdit.palette().color(QPalette::Text), fallback->foregroundColor());
        QCOMPARE(patternEdit.viewport()->palette().color(QPalette::Base), fallback->backgroundColor());
        QCOMPARE(anchorFormat().foreground().color(), fallback->foregroundColor());
        QCOMPARE(anchorFormat().background().color(), fallback->backgroundColor());
    }
};

#include "MissingEditorThemeTest.moc"
MUDLET_GROUPED_TEST_MAIN(MissingEditorThemeTest)
