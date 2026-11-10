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
 * Guards the module manager's design-language shell: the sheet on the shell
 * rather than the dialog, the buttons' recipe and glyphs, and the shared notice
 * an import failure is reported in.
 *
 * Run with: ctest -R ModuleManagerShellTest -V
 */

#include <QAbstractButton>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QToolButton>
#include <QtTest/QtTest>

#include "Host.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgModuleManager.h"
#include "dlgSystemMessageArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ModuleManagerShellTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgModuleManager* mpDialog = nullptr;
    const QString mProfileName = qsl("ModuleManagerShell-Test-Profile");

    QWidget* shell() const { return mpDialog->findChild<QWidget*>(qsl("modulesShell")); }
    dlgSystemMessageArea* notice() const { return mpDialog->findChild<dlgSystemMessageArea*>(qsl("modulesNotice")); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mpDialog = new dlgModuleManager(nullptr, mpHost);
        mpDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpDialog));
        TestPixels::settle();
    }

    void cleanupTestCase()
    {
        if (mpDialog) {
            mpDialog->close();
            mpDialog = nullptr;
        }
        QCoreApplication::processEvents();
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // As in every window that has adopted the design, so a profile stylesheet given to the dialog cannot wipe it
    void test_everyControlIsOnTheShellAndTheSheetIsSetThere()
    {
        QWidget* pShell = shell();
        QVERIFY2(pShell, "there is no widget named 'modulesShell' - the window is still drawn by the platform");
        for (QWidget* pWidget : {static_cast<QWidget*>(mpDialog->moduleTable),
                                 static_cast<QWidget*>(mpDialog->textEdit),
                                 static_cast<QWidget*>(mpDialog->installButton),
                                 static_cast<QWidget*>(mpDialog->uninstallButton),
                                 static_cast<QWidget*>(mpDialog->helpButton)}) {
            QVERIFY2(pShell->isAncestorOf(pWidget), qPrintable(qsl("%1 is not inside the shell, so the shell's sheet does not reach it").arg(pWidget->objectName())));
        }
        QVERIFY2(mpDialog->styleSheet().isEmpty(), "the dialog carries a sheet of its own, which a profile stylesheet given to the dialog would replace");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QVERIFY2(pShell->styleSheet().contains(uiDesign::buttonStyleSheet(tokens, qsl("#modulesShell"))), "the shell's sheet does not carry the shared button recipe");
        QVERIFY2(pShell->styleSheet().contains(tokens.page.name()), qPrintable(qsl("the shell's sheet does not name the page tone %1").arg(tokens.page.name())));
    }

    void test_theButtonsCarryTheirGlyphs()
    {
        for (QPushButton* pButton : {mpDialog->installButton, mpDialog->uninstallButton, mpDialog->helpButton}) {
            QVERIFY2(!pButton->icon().isNull(), qPrintable(qsl("%1 has no glyph").arg(pButton->objectName())));
            // A right-to-left button draws its glyph after its words, unlike the package manager's
            QVERIFY2(pButton->layoutDirection() == QGuiApplication::layoutDirection(), qPrintable(qsl("%1 is not laid out in the application's direction").arg(pButton->objectName())));
        }
        QVERIFY2(mpDialog->helpButton->x() < mpDialog->installButton->x() && mpDialog->installButton->x() < mpDialog->uninstallButton->x(), "the foot no longer reads Module Help, Install, Uninstall");
    }

    // The table stretches, so without the .ui's minimum the explanation would shrink to a scrolling sliver
    void test_theExplanationKeepsItsHeight()
    {
        mpDialog->resize(600, 481);
        TestPixels::settle();
        QVERIFY2(mpDialog->textEdit->height() >= 230, qPrintable(qsl("the explanation is %1px tall, short of the 230px the .ui gives it").arg(mpDialog->textEdit->height())));
    }

    // Longer than any status-line timeout, so only the cross can have hidden it
    void test_anImportFailureStaysInTheSharedNoticeUntilDismissed()
    {
        dlgSystemMessageArea* pNotice = notice();
        QVERIFY2(pNotice, "there is no dlgSystemMessageArea named 'modulesNotice' - the window reports failures with a bare label");
        QVERIFY2(shell()->isAncestorOf(pNotice), "the notice is not inside the shell");
        QVERIFY2(!pNotice->isVisible(), "the notice is on show with nothing to report");

        const QString message = qsl("Failed to import: broken<v2>.mpackage");
        mpDialog->showImportStatus(message);
        QTest::qWait(4500ms);
        QVERIFY2(pNotice->isVisible(), "the import failure hid itself before the player dismissed it");
        QCOMPARE(pNotice->notificationAreaMessageBox->text(), message);
        QCOMPARE(pNotice->notificationAreaMessageBox->textFormat(), Qt::PlainText);
        QVERIFY2(!mpDialog->label_importStatus->isVisible(), "the label the notice replaced is on show as well");

        pNotice->messageAreaCloseButton->click();
        QVERIFY2(!pNotice->isVisible(), "the notice's cross does not dismiss it");
    }

    void test_aNewInstallClearsAnEarlierFailure()
    {
        dlgSystemMessageArea* pNotice = notice();
        QVERIFY(pNotice);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        mpDialog->installModules({dir.filePath(qsl("missing<v2>.xml"))});
        QVERIFY2(pNotice->isVisible(), "a module that could not be imported is not reported");
        QCOMPARE(pNotice->notificationAreaMessageBox->text(), qsl("Failed to import: missing<v2>.xml"));

        const QString moduleName = qsl("module-manager-shell-install");
        QFile module(dir.filePath(moduleName + qsl(".xml")));
        QVERIFY(module.open(QIODevice::WriteOnly));
        module.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n<MudletPackage version=\"1.001\">\n</MudletPackage>\n");
        module.close();
        mpDialog->installModules({module.fileName()});
        QVERIFY2(mpHost->mInstalledModules.contains(moduleName), "the test module was not installed, so this proves nothing about a successful install");
        QVERIFY2(!pNotice->isVisible(), "the earlier failure is still on show after an install that succeeded");

        QVERIFY(mpHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI));
    }

    // Nothing else restyles this window, so the appearance signal alone has to move it
    void test_anAppearanceChangeRestylesTheShell()
    {
        QMap<QString, QString> sheets;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpDialog);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            QVERIFY2(shell()->styleSheet().contains(tokens.page.name()),
                     qPrintable(qsl("on the %1 appearance the shell's sheet does not name the page tone %2 that appearance mixes").arg(appearance.first, tokens.page.name())));
            QVERIFY2(!mpDialog->installButton->icon().isNull(), qPrintable(qsl("on the %1 appearance Install lost its glyph").arg(appearance.first)));
            sheets.insert(appearance.first, shell()->styleSheet());
        }
        QVERIFY2(sheets.value(qsl("dark")) != sheets.value(qsl("light")), "both appearances leave the shell with the same sheet, so the pass is not being re-run");
    }
};

#include "ModuleManagerShellTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleManagerShellTest)
