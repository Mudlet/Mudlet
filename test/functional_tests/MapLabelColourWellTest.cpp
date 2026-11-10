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
 * The map label dialog's three colours may be see-through, so each is shown in
 * the shared alpha colour well - a swatch over a checkerboard and how opaque
 * the colour is - as the settings dialog's map colours are.
 *
 * Run with: ctest -R MapLabelColourWellTest -V
 */

#include <QPushButton>
#include <QtTest/QtTest>

#include "AlphaColourWell.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgMapLabel.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapLabelColourWellTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }
        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
    }

    void cleanupTestCase() { mSession.stop(); }

    void test_eachColourIsShownInAnAlphaWell()
    {
        auto* pDialog = new dlgMapLabel(nullptr);
        pDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDialog));

        const QList<QPair<QPushButton*, QColor>> wells{
                {pDialog->pushButton_bgColor, pDialog->getBgColor()}, {pDialog->pushButton_fgColor, pDialog->getFgColor()}, {pDialog->pushButton_outlineColor, pDialog->getOutlineColor()}};
        for (const auto& [pButton, colour] : wells) {
            const uiDesign::AlphaColourWell* pWell = uiDesign::AlphaColourWell::find(pButton);
            QVERIFY2(pWell, qPrintable(qsl("%1 is not an alpha colour well, so a see-through colour shows as a faded button").arg(pButton->objectName())));
            QCOMPARE(pWell->colour(), colour);
            QVERIFY2(pButton->text().contains(QString::number(qRound(colour.alphaF() * 100))),
                     qPrintable(qsl("%1 does not say how opaque its colour is: \"%2\"").arg(pButton->objectName(), pButton->text())));
        }

        // What a colour picked in the dialog does to the well
        pDialog->getBgColor() = QColor(10, 20, 30, 51);
        emit pDialog->updated();
        QCOMPARE(uiDesign::AlphaColourWell::find(pDialog->pushButton_bgColor)->colour(), QColor(10, 20, 30, 51));
        QVERIFY2(pDialog->pushButton_bgColor->text().contains(qsl("20%")), qPrintable(pDialog->pushButton_bgColor->text()));

        delete pDialog;
    }

    // The dialog stays open through a switch, so each well has to take the new appearance's ink and field itself
    void test_eachWellFollowsAnAppearanceSwitch()
    {
        auto* pDialog = new dlgMapLabel(nullptr);
        pDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDialog));

        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light, enums::Appearance::dark}) {
            TestPixels::setAppearanceAndWait(appearance, pDialog);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QString name = appearance == enums::Appearance::dark ? qsl("dark") : qsl("light");
            for (QPushButton* pButton : {pDialog->pushButton_bgColor, pDialog->pushButton_fgColor, pDialog->pushButton_outlineColor}) {
                QVERIFY2(pButton->styleSheet().contains(qsl("color: %1;").arg(tokens.text.name())),
                         qPrintable(qsl("on the %1 appearance %2 does not write its words in %3: %4").arg(name, pButton->objectName(), tokens.text.name(), pButton->styleSheet())));
                // The words and the swatch are the minority of the well; the rest is its field
                QHash<QRgb, int> histogram;
                const QImage face = TestPixels::shot(pButton);
                for (int y = 0; y < face.height(); ++y) {
                    for (int x = 0; x < face.width(); ++x) {
                        ++histogram[face.pixel(x, y) | 0xff000000];
                    }
                }
                QRgb commonest = 0;
                for (auto entry = histogram.cbegin(); entry != histogram.cend(); ++entry) {
                    if (entry.value() > histogram.value(commonest)) {
                        commonest = entry.key();
                    }
                }
                QVERIFY2(QColor(commonest).rgb() == tokens.field.rgb(),
                         qPrintable(qsl("on the %1 appearance %2 is filled with %3, not the field %4").arg(name, pButton->objectName(), QColor(commonest).name(), tokens.field.name())));
            }
        }

        delete pDialog;
        mudlet::self()->setAppearance(enums::Appearance::systemSetting);
    }
};

#include "MapLabelColourWellTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLabelColourWellTest)
