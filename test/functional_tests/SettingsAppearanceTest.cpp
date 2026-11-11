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
 * It also keeps the page-scoped field recipe off the search field and sidebar.
 *
 * Run with: ctest -R SettingsAppearanceTest -V
 */

#include <algorithm>
#include <cmath>
#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QRegularExpression>
#include <QCheckBox>
#include <QRadioButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"
#include "uiDesign.h"

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
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void writeFreshEditorThemesFile()
    {
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
    }

    static QWidget* shellOf(const dlgProfilePreferences* pDialog) { return pDialog->findChild<QWidget*>(qsl("settingsShell")); }

    QWidget* shell() const { return shellOf(mpPreferences); }

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
    static QColor paintedSurfaceOf(const dlgProfilePreferences* pDialog)
    {
        QWidget* pShell = shellOf(pDialog);
        return pixelOf(pShell, QPoint(3, pShell->height() - 3));
    }

    QColor paintedSurface() const { return paintedSurfaceOf(mpPreferences); }

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

    // The certificate warnings carry their colours as a stylesheet, so the two
    // have to be read back out of one to be measured. Qt parses a colour name
    // but not the rgb() form these are written in.
    static QColor colourFrom(const QString& styleSheet, const QString& property)
    {
        const QRegularExpression declaration(qsl("(?:^|;)\\s*%1:\\s*([^;]+)").arg(property));
        const auto declared = declaration.match(styleSheet);
        if (!declared.hasMatch()) {
            return QColor();
        }
        const QString value = declared.captured(1).trimmed();
        static const QRegularExpression rgbFunction(qsl("^rgb\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)$"));
        if (const auto rgb = rgbFunction.match(value); rgb.hasMatch()) {
            return QColor(rgb.captured(1).toInt(), rgb.captured(2).toInt(), rgb.captured(3).toInt());
        }
        return QColor::fromString(value);
    }

    static QStringList selectorsIn(const QString& styleSheet)
    {
        QStringList selectors;
        const QStringList rules = styleSheet.split(QLatin1Char('}'), Qt::SkipEmptyParts);
        for (const QString& rule : rules) {
            const QString selectorList = rule.section(QLatin1Char('{'), 0, 0);
            const QStringList parts = selectorList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                if (const QString selector = part.simplified(); !selector.isEmpty()) {
                    selectors.append(selector);
                }
            }
        }
        return selectors;
    }

    // Only for a hand-built sheet with no selectors or nesting: declarations lie between semicolons
    static QString declarationValue(const QString& styleSheet, const QString& property)
    {
        for (const QString& declaration : styleSheet.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
            if (declaration.section(QLatin1Char(':'), 0, 0).trimmed() == property) {
                return declaration.section(QLatin1Char(':'), 1).trimmed();
            }
        }
        return QString();
    }

    // The wash composited over its surface: what a word on it is actually read against
    static QColor washOver(const QColor& surface, const QString& value)
    {
        static const QRegularExpression channels(qsl("^rgba?\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*(?:,\\s*([0-9.]+)\\s*)?\\)$"));
        const QRegularExpressionMatch match = channels.match(value);
        if (!match.hasMatch()) {
            return QColor();
        }
        const qreal alpha = match.captured(4).isEmpty() ? 1.0 : match.captured(4).toDouble();
        const auto mix = [alpha](const int over, const int under) {
            return qRound(under + (over - under) * alpha);
        };
        return QColor(mix(match.captured(1).toInt(), surface.red()), mix(match.captured(2).toInt(), surface.green()), mix(match.captured(3).toInt(), surface.blue()));
    }

    static QString describe(const QColor& surface)
    {
        return qsl("the shell is painted %1 (lightness %2) while the application palette is %3 (Base %4)")
                .arg(surface.name(), QString::number(surface.lightness()), applicationIsLight() ? qsl("light") : qsl("dark"), QApplication::palette().color(QPalette::Base).name());
    }

    // A theme stylesheet that paints every widget, as dark-theme packages do, decides the shell's colours as
    // it did before the shell drew its own surfaces: otherwise each control the theme paints sits in a dark
    // box on a light card (#11839)
    void checkTheShellFollowsTheThemeStyleSheet(const QColor& themeFill = QColor(qsl("#181818")))
    {
        const QColor surface = paintedSurface();
        QVERIFY2(surface == themeFill, qPrintable(qsl("the theme paints every widget %1, yet the shell is %2").arg(themeFill.name(), surface.name())));

        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"));
        QVERIFY2(pCard, "the Appearance card this case reads its colours off is not there any more");
        const QColor cardFill = pixelOf(pCard, QPoint(pCard->width() / 2, pCard->height() - 4));
        const QColor labelFill = pixelOf(mpPreferences->label_appearance, QPoint(1, 1));
        QVERIFY2(cardFill == labelFill, qPrintable(qsl("a label is painted %1 on a card painted %2").arg(labelFill.name(), cardFill.name())));

        const QColor ink = mpPreferences->label_appearance->palette().color(mpPreferences->label_appearance->foregroundRole());
        const qreal ratio = contrastRatio(cardFill, ink);
        QVERIFY2(ratio >= 4.5, qPrintable(qsl("a card is painted %1 under %2 text, a contrast of %3:1").arg(cardFill.name(), ink.name(), QString::number(ratio, 'f', 2))));
    }

    // Exact: rules aimed at the cards and the fields paint them red and green, and the shell's recipes have to win
    void checkTheShellsSurfacesAreTheRecipes(const QColor& card, const QColor& field, const QString& when)
    {
        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"));
        QVERIFY2(pCard, "the Appearance card this case reads its colours off is not there any more");
        const QColor cardFill = pixelOf(pCard, QPoint(pCard->width() / 2, pCard->height() - 4));
        QVERIFY2(cardFill.rgb() == card.rgb(), qPrintable(qsl("%1, a card is painted %2 rather than the card recipe's %3").arg(when, cardFill.name(), card.name())));

        auto* pField = mpPreferences->lineEdit_logFileFolder;
        QVERIFY2(pField, "the log folder field this case reads is not there any more");
        // Inside the border and padding, before the text
        const QColor fieldFill = pixelOf(pField, QPoint(uiDesign::scmInputBorderWidth + 2, pField->height() / 2));
        QVERIFY2(fieldFill.rgb() == field.rgb(), qPrintable(qsl("%1, a field is painted %2 rather than the field recipe's %3").arg(when, fieldFill.name(), field.name())));
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        mudlet::self()->setAppStyleSheet(QString());
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

    // mudlet::signal_appearanceChanged fires after the mode has turned over, so a dialog
    // comparing the mode before and after its call finds it unmoved. The counts catch
    // the first dialog re-entering its slot through that signal, invisible otherwise.
    void test_aSecondDialogFollowsAThemeChangeMadeInTheFirst()
    {
        // So the move below is a real one whichever appearance the machine is in
        setAppearance(enums::Appearance::light);

        // Given what mudlet::showOptionsDialog() gives a dialog it puts on screen
        auto pSecond = std::make_unique<dlgProfilePreferences>(mudlet::self(), mpHost);
        pSecond->setStyleSheet(mpHost->mProfileStyleSheet);
        pSecond->resize(1060, 760);
        pSecond->show();
        QVERIFY(QTest::qWaitForWindowExposed(pSecond.get()));
        QVERIFY2(shellOf(pSecond.get()), "the second dialog's settings shell was never built");
        pSecond->grab();

        const QString lightPage = uiDesign::themeTokens().page.name();
        int firstStyled = mpPreferences->mShellStyleApplications;
        int secondStyled = pSecond->mShellStyleApplications;

        // Through the first dialog: the second has only the application's signal to go on
        setAppearance(enums::Appearance::dark);
        pSecond->grab();
        const QString darkPage = uiDesign::themeTokens().page.name();
        QVERIFY2(!applicationIsLight(), "the application did not go dark, so the flip below is not the one this case is about");
        QVERIFY2(darkPage != lightPage, "the two appearances lay the page in the same colour, so the sheets below cannot be told apart");
        QVERIFY2(paintedSurfaceOf(pSecond.get()).lightness() < 128,
                 qPrintable(qsl("the second dialog is painted %1 (lightness %2) while the application went dark")
                                    .arg(paintedSurfaceOf(pSecond.get()).name(), QString::number(paintedSurfaceOf(pSecond.get()).lightness()))));
        QVERIFY2(shellOf(pSecond.get())->styleSheet().contains(darkPage),
                 qPrintable(qsl("the second dialog's shell is still mixed from the light page's %1 rather than the dark page's %2").arg(lightPage, darkPage)));
        QVERIFY2(pSecond->mShellStyleApplications - secondStyled == 1,
                 qPrintable(qsl("the second dialog restyled %1 times for one appearance change").arg(pSecond->mShellStyleApplications - secondStyled)));
        QVERIFY2(mpPreferences->mShellStyleApplications - firstStyled == 1,
                 qPrintable(qsl("the dialog the change was made in restyled %1 times for it").arg(mpPreferences->mShellStyleApplications - firstStyled)));

        firstStyled = mpPreferences->mShellStyleApplications;
        secondStyled = pSecond->mShellStyleApplications;

        // And back: a fix that records the theme once could be right one way only
        setAppearance(enums::Appearance::light);
        pSecond->grab();
        QVERIFY2(applicationIsLight(), "the application did not go light, so the flip below is not the one this case is about");
        QVERIFY2(paintedSurfaceOf(pSecond.get()).lightness() >= 128,
                 qPrintable(qsl("the second dialog is painted %1 (lightness %2) while the application went light")
                                    .arg(paintedSurfaceOf(pSecond.get()).name(), QString::number(paintedSurfaceOf(pSecond.get()).lightness()))));
        QVERIFY2(shellOf(pSecond.get())->styleSheet().contains(lightPage),
                 qPrintable(qsl("the second dialog's shell is still mixed from the dark page's %1 rather than the light page's %2").arg(darkPage, lightPage)));
        QVERIFY2(pSecond->mShellStyleApplications - secondStyled == 1,
                 qPrintable(qsl("the second dialog restyled %1 times for one appearance change").arg(pSecond->mShellStyleApplications - secondStyled)));
        QVERIFY2(mpPreferences->mShellStyleApplications - firstStyled == 1,
                 qPrintable(qsl("the dialog the change was made in restyled %1 times for it").arg(mpPreferences->mShellStyleApplications - firstStyled)));
    }

    // QLabel bakes an anchor's colour into its document at setText, so a QPalette::Link
    // set afterwards is ignored; only the label's text shows the ink it really uses
    void test_everyLinkCarriesTheAccentInBothAppearances()
    {
        auto* pLink = mpPreferences->findChild<QLabel*>(qsl("settingsHeroLink"));
        QVERIFY2(pLink, "the security status card has no 'settingsHeroLink'");
        QVERIFY2(pLink->text().contains(qsl("<a ")), "the hero link carries no anchor, so nothing below is measured");

        setAppearance(enums::Appearance::dark);
        const QString onDark = uiDesign::themeTokens().accentText.name();
        const QString linkOnDark = pLink->text();

        setAppearance(enums::Appearance::light);
        const QString onLight = uiDesign::themeTokens().accentText.name();
        const QString linkOnLight = pLink->text();

        qInfo().noquote() << qsl("  the hero link reads \"%1\" on dark and \"%2\" on light").arg(linkOnDark, linkOnLight);
        QVERIFY2(onDark != onLight, "the two appearances answer the same accent ink, so the check below cannot tell them apart");
        QVERIFY2(linkOnDark.contains(qsl("color: %1").arg(onDark)), qPrintable(qsl("the hero link is not inked %1 on the dark appearance: \"%2\"").arg(onDark, linkOnDark)));
        QVERIFY2(linkOnLight.contains(qsl("color: %1").arg(onLight)),
                 qPrintable(qsl("the hero link is not inked %1 on the light appearance - an appearance change did not re-ink it: \"%2\"").arg(onLight, linkOnLight)));
    }

    // The card's fill comes from the shell stylesheet and its text does not, so the
    // two can disagree about the theme
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

    void test_theCertificateWarningReadsOnItsWashInBothAppearances()
    {
        // As the SSL error path does: restyleCertificateWarnings() rewrites only the
        // controls already carrying a sheet of their own
        const QList<QWidget*> warned{mpPreferences->checkBox_self_signed, mpPreferences->ssl_issuer_label};
        for (QWidget* pControl : warned) {
            pControl->setStyleSheet(qsl("font-weight: bold;"));
        }

        // Each measured after a real move, not after asking for the appearance already in force
        setAppearance(enums::Appearance::dark);
        setAppearance(enums::Appearance::light);
        QStringList washes;
        for (const enums::Appearance appearance : {enums::Appearance::light, enums::Appearance::dark}) {
            setAppearance(appearance);
            // The controls stand on the SSL cards, so the wash lies over the card tone
            const QColor card = uiDesign::themeTokens().card;
            for (QWidget* pControl : warned) {
                const QString sheet = pControl->styleSheet();
                const QColor ink(declarationValue(sheet, qsl("color")));
                const QColor wash = washOver(card, declarationValue(sheet, qsl("background")));
                QVERIFY2(ink.isValid() && wash.isValid(),
                         qPrintable(qsl("%1 was not restyled for the %2 appearance - its sheet reads \"%3\"")
                                            .arg(pControl->objectName(), appearance == enums::Appearance::dark ? qsl("dark") : qsl("light"), sheet)));
                washes.append(wash.name());
                const qreal ratio = contrastRatio(ink, wash);
                QVERIFY2(ratio >= uiDesign::scmTextMinimumRatio,
                         qPrintable(qsl("%1 is written %2 on a warning wash that comes out %3, a contrast of %4:1 against the %5 floor")
                                            .arg(pControl->objectName(), ink.name(), wash.name(), QString::number(ratio, 'f', 2), QString::number(uiDesign::scmTextMinimumRatio, 'f', 1))));
            }
        }
        qInfo().noquote() << qsl("  the warning wash comes out %1").arg(washes.join(qsl(", ")));
        QVERIFY2(washes.at(0) != washes.at(2), "the wash is the same colour in both appearances, so it is not being mixed against the page");
    }

    // The shell's surfaces are its own, and a profile's Lua stylesheet is
    // applied to the whole dialog - so its rules reach the controls on a page
    // but cannot repaint the cards and fields the shell draws, before a theme
    // change or after one, nor under a theme that decides the shell's colours.
    void test_aProfileStyleSheetsRulesDoNotRepaintTheShellsSurfaces()
    {
        const QString rules = qsl("QGroupBox { background-color: rgb(255, 0, 0); } QLineEdit { background-color: rgb(0, 255, 0); }");
        setAppearance(enums::Appearance::dark);
        mpPreferences->setStyleSheet(rules);
        QCoreApplication::processEvents();
        mpPreferences->grab();
        checkTheShellsSurfacesAreTheRecipes(uiDesign::themeTokens().card, uiDesign::themeTokens().field, qsl("before any theme change"));
        if (QTest::currentTestFailed()) {
            return;
        }

        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so this is not the mid-life flip");
        checkTheShellsSurfacesAreTheRecipes(uiDesign::themeTokens().card, uiDesign::themeTokens().field, qsl("after the theme change"));
        if (QTest::currentTestFailed()) {
            return;
        }

        // The theme decides the tones, and the rules beside it still do not reach the surfaces
        const QColor themeFill(qsl("#181818"));
        mpPreferences->setStyleSheet(qsl("QWidget { background-color: %1; color: #a0a0a0; } %2").arg(themeFill.name(), rules));
        QCoreApplication::processEvents();
        mpPreferences->grab();
        checkTheShellsSurfacesAreTheRecipes(themeFill, themeFill, qsl("under a theme stylesheet"));
    }

    void test_aProfileStyleSheetThemeDecidesTheShellsColours()
    {
        setAppearance(enums::Appearance::light);
        mpPreferences->setStyleSheet(qsl("QWidget { background-color: #181818; color: #a0a0a0; }"));
        QCoreApplication::processEvents();
        checkTheShellFollowsTheThemeStyleSheet();
        if (QTest::currentTestFailed()) {
            return;
        }

        // A theme change has to leave the theme stylesheet in charge
        setAppearance(enums::Appearance::dark);
        checkTheShellFollowsTheThemeStyleSheet();
        if (QTest::currentTestFailed()) {
            return;
        }

        setAppearance(enums::Appearance::light);
        checkTheShellFollowsTheThemeStyleSheet();
        if (QTest::currentTestFailed()) {
            return;
        }

        // Cleared with no theme change after it, which would restyle the shell anyway
        mpPreferences->setStyleSheet(QString());
        QCoreApplication::processEvents();
        mpPreferences->grab();
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));
    }

    // The tokens' light treatment comes from the theme's palette, not the application's
    void test_aLightThemeStyleSheetLightensTheShellOnTheDarkAppearance()
    {
        setAppearance(enums::Appearance::dark);
        const QColor themeFill(qsl("#f0f0f0"));
        mpPreferences->setStyleSheet(qsl("QWidget { background-color: %1; color: #202020; }").arg(themeFill.name()));
        QCoreApplication::processEvents();
        mpPreferences->grab();
        QVERIFY2(!applicationIsLight(), "the application did not stay dark, so the theme is not the only thing lightening the shell");
        checkTheShellFollowsTheThemeStyleSheet(themeFill);
    }

    // A see-through theme background is no colour to draw cards in
    void test_aTransparentThemeBackgroundLeavesTheShellItsOwnColours()
    {
        setAppearance(enums::Appearance::light);
        mpPreferences->setStyleSheet(qsl("QWidget { background-color: transparent; }"));
        QCoreApplication::processEvents();
        mpPreferences->grab();
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));
    }

    // ...and nor is a gradient, which Qt copies into Base as well
    void test_aGradientThemeBackgroundLeavesTheShellItsOwnColours()
    {
        setAppearance(enums::Appearance::light);
        mpPreferences->setStyleSheet(qsl("QWidget { background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #181818, stop: 1 #303030); }"));
        QCoreApplication::processEvents();
        mpPreferences->grab();
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));
    }

    void test_anAppStyleSheetThemeDecidesTheShellsColours()
    {
        setAppearance(enums::Appearance::light);
        mudlet::self()->setAppStyleSheet(qsl("QWidget { background-color: #181818; color: #a0a0a0; }"));
        QCoreApplication::processEvents();
        mpPreferences->grab();
        checkTheShellFollowsTheThemeStyleSheet();
        if (QTest::currentTestFailed()) {
            return;
        }

        mudlet::self()->setAppStyleSheet(QString());
        QCoreApplication::processEvents();
        mpPreferences->grab();
        QVERIFY2(paintedSurface().lightness() >= 128, qPrintable(describe(paintedSurface())));
    }

    // The certificate warnings carry their colours in a stylesheet rather than
    // taking them from the palette, so a theme change has to restyle them
    // explicitly: the checkbox, which named no colour of its own, was left
    // drawing the dark theme's near-white text on the bright yellow it had been
    // given, and the label kept its light-theme red (#9418).
    void test_theCertificateWarningsFollowAThemeChange()
    {
        // The dialog opens on whatever the desktop's own scheme is, and the
        // restyle only runs when the appearance crosses the light/dark line, so
        // start from the side the flip below has to come from.
        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so the flip below is not the one this case is about");

        // Only a warning already showing is restyled, and one shows only for a
        // TLS connection whose certificate is bad - which a test cannot offer.
        // So put the two in the state such a connection used to leave them in.
        mpPreferences->checkBox_self_signed->setStyleSheet(qsl("font-weight: bold; background: yellow"));
        mpPreferences->ssl_issuer_label->setStyleSheet(qsl("font-weight: bold; color: red; background: yellow"));
        const QList<QWidget*> warnings{mpPreferences->checkBox_self_signed, mpPreferences->ssl_issuer_label};
        QStringList beforeTheFlip;
        for (QWidget* pWarning : warnings) {
            beforeTheFlip << pWarning->styleSheet();
        }

        // The other two warnings are not showing anything, and a restyle has to
        // pass them by rather than paint a warning colour onto a silent widget.
        const QList<QWidget*> quiet{mpPreferences->checkBox_expired, mpPreferences->ssl_expires_label};

        const auto readable = [](QWidget* pWidget) {
            const QString styleSheet = pWidget->styleSheet();
            const QColor ink = colourFrom(styleSheet, qsl("color"));
            // The background is a translucent wash, so what the ink is read against is that wash over the card
            const QColor fill = washOver(uiDesign::themeTokens().card, declarationValue(styleSheet, qsl("background")));
            if (!ink.isValid() || !fill.isValid()) {
                return qsl("%1 is styled '%2', which does not say both what colour it is drawn in and what it is drawn on").arg(pWidget->objectName(), styleSheet);
            }
            // 3:1 rather than the 4.5:1 body text keeps: the light-mode design,
            // red on pale yellow, is only 3.9:1
            const qreal ratio = contrastRatio(fill, ink);
            return ratio >= 3.0 ? QString() : qsl("%1 is drawn %2 on %3, a contrast of %4:1").arg(pWidget->objectName(), ink.name(), fill.name(), QString::number(ratio, 'f', 2));
        };

        setAppearance(enums::Appearance::dark);
        QVERIFY2(!applicationIsLight(), "the application did not go dark, so this is not the flip the case is about");
        QStringList inDarkMode;
        for (int i = 0, total = warnings.size(); i < total; ++i) {
            QWidget* pWarning = warnings.at(i);
            // Red on yellow clears a contrast check by itself, so what catches a
            // warning the dark theme never reached is that it is still wearing
            // the stylesheet the light theme left on it.
            QVERIFY2(pWarning->styleSheet() != beforeTheFlip.at(i),
                     qPrintable(qsl("%1 kept the light theme's '%2' after the appearance went dark").arg(pWarning->objectName(), pWarning->styleSheet())));
            const QString complaint = readable(pWarning);
            QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
            inDarkMode << pWarning->styleSheet();
        }

        setAppearance(enums::Appearance::light);
        QVERIFY2(applicationIsLight(), "the application did not go light, so this is not the flip the case is about");
        for (int i = 0, total = warnings.size(); i < total; ++i) {
            const QString complaint = readable(warnings.at(i));
            QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
            QVERIFY2(warnings.at(i)->styleSheet() != inDarkMode.at(i), qPrintable(qsl("%1 is styled '%2' in both themes").arg(warnings.at(i)->objectName(), warnings.at(i)->styleSheet())));
        }
        for (QWidget* pQuiet : quiet) {
            QVERIFY2(pQuiet->styleSheet().isEmpty(), qPrintable(qsl("%1 is warning about nothing, yet a restyle gave it '%2'").arg(pQuiet->objectName(), pQuiet->styleSheet())));
        }
    }

    void test_aPagesFieldsAreDrawnFromTheSharedInputRecipe()
    {
        setAppearance(enums::Appearance::dark);
        auto* pField = mpPreferences->lineEdit_logFileFolder;
        QVERIFY2(pField, "the log folder field this case reads is not there any more");
        QVERIFY2(pField->height() >= uiDesign::scmInputHeight,
                 qPrintable(qsl("a field on a page is %1px tall, against the %2px the shared recipe asks for").arg(QString::number(pField->height()), QString::number(uiDesign::scmInputHeight))));

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        // Inside the border and padding, before the text: the field holds a temporary
        // path, so a point under the words lands on a descender or not run to run
        const QColor fill = pixelOf(pField, QPoint(uiDesign::scmInputBorderWidth + 2, pField->height() / 2));
        QVERIFY2(TestPixels::distance(fill, tokens.field) < TestPixels::distance(fill, tokens.card),
                 qPrintable(qsl("a field is painted %1, nearer the card's %2 than the field surface's %3").arg(fill.name(), tokens.card.name(), tokens.field.name())));
    }

    // Qt's stylesheet parser takes only pt and px for font-size and silently drops a
    // percentage, so the resolved font is the only proof a rule applied
    void test_theShellsHeadingsAreSetAtTheStepsTheScaleNames()
    {
        auto* pTitle = mpPreferences->findChild<QLabel*>(qsl("settingsPageTitle"));
        QVERIFY2(pTitle, "the dialog has no 'settingsPageTitle'");
        QCOMPARE(pTitle->font().pointSize(), uiDesign::typeSize(uiDesign::TypeStep::Display));

        auto* pWordmark = mpPreferences->findChild<QLabel*>(qsl("settingsWordmark"));
        QVERIFY2(pWordmark, "the dialog has no 'settingsWordmark'");
        QCOMPARE(pWordmark->font().pointSize(), uiDesign::typeSize(uiDesign::TypeStep::Title));

        // A card's words carry no size of their own, so the card's font is the body step
        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"));
        QVERIFY2(pCard, "the Appearance card this case measures the body size against is not there any more");
        QCOMPARE(pCard->font().pointSize(), uiDesign::typeSize(uiDesign::TypeStep::Body));
        QVERIFY2(pTitle->font().pointSize() > pWordmark->font().pointSize() && pWordmark->font().pointSize() > pCard->font().pointSize(),
                 qPrintable(qsl("the page title, the wordmark and a card's words are %1/%2/%3pt - the headings are not standing over the page")
                                    .arg(QString::number(pTitle->font().pointSize()), QString::number(pWordmark->font().pointSize()), QString::number(pCard->font().pointSize()))));
    }

    // The page-scoped field rules must not reach the card the fields sit on
    void test_aCardIsStillPaintedAsACard()
    {
        setAppearance(enums::Appearance::dark);
        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"));
        QVERIFY2(pCard, "the Appearance card this case reads its colours off is not there any more");
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor fill = pixelOf(pCard, QPoint(pCard->width() / 2, pCard->height() - 4));
        // Exact rather than nearest-of-two, which would pass a card left in whatever
        // the platform paints a group box
        QVERIFY2(fill.rgb() == tokens.card.rgb(), qPrintable(qsl("a card is painted %1 rather than the card tone's %2").arg(fill.name(), tokens.card.name())));
    }

    // Named on the shell, the rules would also draw the search box and the sidebar's editors
    void test_theInputRulesReachNothingOutsideThePages()
    {
        auto* pStack = mpPreferences->findChild<QStackedWidget*>(qsl("settingsStack"));
        auto* pSearchField = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(pStack && pSearchField, "the stack and the search field are the two things this case is about");
        QVERIFY2(!pStack->isAncestorOf(pSearchField), "the search field is inside the stack, so the rules scoped to the pages now draw it too");

        const QStringList inputTypes{qsl("QLineEdit"), qsl("QPlainTextEdit"), qsl("QTextEdit"), qsl("QComboBox"), qsl("QAbstractSpinBox")};
        int scopedRules = 0;
        for (const QString& selector : selectorsIn(shell()->styleSheet())) {
            const bool namesAField = std::any_of(inputTypes.cbegin(), inputTypes.cend(), [&selector](const QString& type) {
                return selector.contains(type);
            });
            if (!namesAField) {
                continue;
            }
            QVERIFY2(selector.startsWith(qsl("#settingsStack ")), qPrintable(qsl("\"%1\" draws a field from outside the stack of pages").arg(selector)));
            ++scopedRules;
        }
        QVERIFY2(scopedRules > 0, "the shell stylesheet draws no fields at all, so nothing here was checked");

        QVERIFY2(shell()->styleSheet().contains(qsl("#settingsSearchField { border: 1px solid")), "the search field lost the rule that draws it");
        QVERIFY2(shell()->styleSheet().contains(qsl("border-radius: %1px; padding-left").arg(QString::number(uiDesign::scmRadiusProminentInput))),
                 "the search field is no longer drawn with the prominent input's corner");
    }

    // Read off what the rules paint, not their text: a selector list says nothing
    // about which controls a sheet actually reaches
    void test_theInputRulesNameNothingButFields()
    {
        QWidget stack;
        stack.setObjectName(qsl("settingsStack"));
        auto* pLayout = new QVBoxLayout(&stack);
        auto* pCheck = new QCheckBox(qsl("a choice"), &stack);
        auto* pRadio = new QRadioButton(qsl("one of several"), &stack);
        auto* pBar = new QScrollBar(Qt::Vertical, &stack);
        auto* pField = new QLineEdit(qsl("typed in"), &stack);
        pLayout->addWidget(pCheck);
        pLayout->addWidget(pRadio);
        pLayout->addWidget(pBar);
        pLayout->addWidget(pField);
        // Fixed sizes: the field rule's min-height would grow a control and change
        // the grab's size whether or not anything repainted it
        pCheck->setFixedSize(180, 24);
        pRadio->setFixedSize(180, 24);
        pBar->setFixedSize(16, 90);
        pField->setFixedSize(180, 30);
        stack.resize(260, 220);
        stack.show();
        QVERIFY2(QTest::qWaitForWindowExposed(&stack), "the throwaway stack was never put on screen, so there is nothing here to grab");

        const QImage bareCheck = pCheck->grab().toImage();
        const QImage bareRadio = pRadio->grab().toImage();
        const QImage bareBar = pBar->grab().toImage();
        const QImage bareField = pField->grab().toImage();

        stack.setStyleSheet(uiDesign::inputStyleSheet(uiDesign::themeTokens(), qsl("#settingsStack")));
        QCoreApplication::processEvents();
        QTest::qWait(50);

        QVERIFY2(pField->grab().toImage() != bareField, "the input rules left the field exactly as the platform drew it, so this case is reading a sheet that reaches nothing");
        QVERIFY2(pCheck->grab().toImage() == bareCheck, "the input rules redrew a check box, which is not a field and is drawn by the card's own rules");
        QVERIFY2(pRadio->grab().toImage() == bareRadio, "the input rules redrew a radio button, which is not a field and is drawn by the card's own rules");
        QVERIFY2(pBar->grab().toImage() == bareBar, "the input rules redrew a scroll bar, which is chrome and is drawn by the shared scroll bar recipe");
    }

    // The dropped-down list is a window of its own, so the rule has to reach it
    // across that boundary from a sheet scoped to the stack
    void test_aComboBoxPopupIsTheFieldOpenedUp()
    {
        setAppearance(enums::Appearance::dark);
        auto* pCombo = mpPreferences->comboBox_appearance;
        // Shown: a list is polished against the stylesheets over it when dropped down,
        // and an unshown one keeps the palette it was made with
        pCombo->showPopup();
        QCoreApplication::processEvents();
        QAbstractItemView* pList = pCombo->view();
        QVERIFY2(pList, "the appearance combo box has no list to drop down");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        // Exact rather than nearest-of-two: unstyled, the list is the platform's base
        // colour, a few levels off the field, and would pass
        const QColor fill = pixelOf(pList->viewport(), QPoint(pList->viewport()->width() / 2, 4));
        pCombo->hidePopup();
        QVERIFY2(fill.rgb() == tokens.field.rgb(),
                 qPrintable(qsl("the popup list is painted %1 rather than the field surface's %2 - the rule scoped to the stack did not reach it").arg(fill.name(), tokens.field.name())));
    }

    // The menu hangs off the dialog, not the shell, so the shell's sheet never reaches it
    void test_theMapProfilesMenuIsDrawnFromTheDesign()
    {
        setAppearance(enums::Appearance::dark);
        QMenu* pMenu = mpPreferences->pushButton_chooseProfiles->menu();
        QVERIFY2(pMenu, "the choose-profiles button carries no menu, so there is nothing here to draw");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheet = pMenu->styleSheet();
        QVERIFY2(sheet.contains(qsl("QMenu")) && sheet.contains(tokens.card.name()),
                 qPrintable(qsl("the map profiles menu is not drawn on the design's card surface (%1): its sheet is \"%2\"").arg(tokens.card.name(), sheet.left(120))));
        QVERIFY2(pMenu->testAttribute(Qt::WA_TranslucentBackground), "the map profiles menu's window was never asked to be see-through, so its corner is opaque");
    }

    // The popup's window is filled before the list is drawn, so a radius on the list
    // alone leaves the window's square corners showing; hence a grab that keeps alpha
    void test_aComboBoxPopupIsOpenAtTheCorner()
    {
        setAppearance(enums::Appearance::dark);
        auto* pCombo = mpPreferences->comboBox_appearance;
        pCombo->showPopup();
        QCoreApplication::processEvents();
        QAbstractItemView* pList = pCombo->view();
        QVERIFY2(pList, "the appearance combo box has no list to drop down");
        QWidget* pPopup = pList->window();
        QVERIFY2(pPopup && pPopup != mpPreferences, "the list dropped down inside the dialog rather than in a window of its own");
        QVERIFY2(pPopup->testAttribute(Qt::WA_TranslucentBackground), "the popup's window was never asked to be see-through, so it is opaque behind whatever corner the list is given");

        const QImage shot = pPopup->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const qreal ratio = shot.devicePixelRatio();
        // Named off the geometry, not guessed: the window is taller than the list it holds
        const QPoint onTheFrame(shot.width() / 2, qRound(pList->geometry().top() * ratio));
        QWidget* pViewport = pList->viewport();
        const QPoint inTheList = pViewport->mapTo(pPopup, QPoint(pViewport->width() / 2, 4)) * ratio;
        pCombo->hidePopup();

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor corner = QColor::fromRgba(shot.pixel(0, 0));
        const QColor frame = QColor::fromRgba(shot.pixel(onTheFrame));
        const QColor list = QColor::fromRgba(shot.pixel(inTheList));
        QVERIFY2(corner.alpha() == 0, qPrintable(qsl("the popup's corner is painted %1 rather than cut away, so the list's radius has a square window behind it").arg(corner.name(QColor::HexArgb))));
        QVERIFY2(frame.alpha() == 255 && frame.rgb() == tokens.border.rgb(),
                 qPrintable(qsl("the top of the popup's frame is %1 rather than the hairline's %2 - the frame the corner is cut out of is not there")
                                    .arg(frame.name(QColor::HexArgb), tokens.border.name())));
        QVERIFY2(list.alpha() == 255 && list.rgb() == tokens.field.rgb(),
                 qPrintable(qsl("the popup's first row is %1 rather than the field surface's %2 - the window was opened up and took the list with it")
                                    .arg(list.name(QColor::HexArgb), tokens.field.name())));

        // An appearance change swaps the style, which re-resolves every palette after
        // the shell restyled, so the popup reasserts on show. Offscreen nothing
        // re-resolves, so the wipe is made here.
        pPopup->setPalette(QPalette());
        pCombo->showPopup();
        QCoreApplication::processEvents();
        const QColor cornerAgain = QColor::fromRgba(pPopup->grab().toImage().convertToFormat(QImage::Format_ARGB32).pixel(0, 0));
        pCombo->hidePopup();
        QVERIFY2(cornerAgain.alpha() == 0,
                 qPrintable(qsl("a palette handed back to the popup closed its corner again, painting it %1 - nothing says the frame is nothing at the one moment left, which is the popup being shown")
                                    .arg(cornerAgain.name(QColor::HexArgb))));
    }
};

#include "SettingsAppearanceTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsAppearanceTest)
