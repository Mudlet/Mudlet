/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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
 * Guards the primary button's three states: under the pointer it takes more of
 * the accent rather than less, pressed reads apart from hovered, its inks read
 * on both, and the plain button beside it keeps states of its own.
 *
 * Run with: ctest -R PrimaryButtonStatesTest -V
 */

#include <QHBoxLayout>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QWidget>
#include <QtTest/QtTest>

#include <memory>

#include "DarkTheme.h"
#include "MudletProxyStyle.h"
#include "PixelTestHelper.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class PrimaryButtonStatesTest : public QObject
{
    Q_OBJECT

private:
    enum class Drawn { Rest, Hover, Pressed };

    struct States
    {
        QColor rest;
        QColor hover;
        QColor pressed;
    };

    // Summed over the three channels, as TestPixels::distance() measures
    static constexpr int scmStatesApart = 12;

    std::unique_ptr<QWidget> mpContainer;
    QPushButton* mpPrimary = nullptr;
    QPushButton* mpPlain = nullptr;
    uiDesign::ThemeTokens mTokens;

    static void addBaseStyles()
    {
        QTest::addColumn<bool>("dark");
        // Fusion is what Ubuntu runs, where the missing states were reported
        QTest::newRow("fusion") << false;
        QTest::newRow("dark") << true;
    }

    // Returns through QVERIFY, so a caller checks QTest::currentTestFailed() after it
    void showButtons(const bool dark)
    {
        if (dark) {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            qApp->setStyle(new DarkTheme);
        } else {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            qApp->setStyle(new MudletProxyStyle(qsl("Fusion")));
        }
        QCoreApplication::processEvents();
        mTokens = uiDesign::themeTokens();
        qInfo().noquote()
                << qsl("  darkPage %1, page %2, accent %3, accentText %4, pressed ink %5")
                           .arg(mTokens.darkPage ? qsl("true") : qsl("false"), mTokens.page.name(), mTokens.accent.name(), mTokens.accentText.name(), uiDesign::primaryPressedInk(mTokens).name());
        if (dark) {
            QVERIFY2(mTokens.darkPage, "SETUP: the dark style left the page light, so the dark appearance is not the one read");
        }

        mpContainer = std::make_unique<QWidget>();
        auto* pLayout = new QHBoxLayout(mpContainer.get());
        // First in the focus chain, so showing the window leaves both buttons without the focus hairline
        auto* pField = new QLineEdit(mpContainer.get());
        mpPrimary = new QPushButton(qsl("Connect"), mpContainer.get());
        mpPrimary->setProperty(uiDesign::scmProp_primaryButton, true);
        mpPlain = new QPushButton(qsl("Cancel"), mpContainer.get());
        pLayout->addWidget(pField);
        pLayout->addWidget(mpPrimary);
        pLayout->addWidget(mpPlain);
        mpContainer->setStyleSheet(uiDesign::buttonStyleSheet(mTokens));
        mpContainer->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpContainer.get()));
        pField->setFocus();
        QCoreApplication::processEvents();
        QVERIFY2(!mpPrimary->hasFocus() && !mpPlain->hasFocus(), "SETUP: a button holds the focus, so its focus hairline is drawn into every state");
    }

    // Drawn as QPushButton::paintEvent draws, with the state on the option: QTest::mouseMove does not set
    // WA_UnderMouse, so a hover sent from a test does not reliably reach the sheet's :hover
    QColor fillOf(QPushButton* pButton, const Drawn drawn) const
    {
        QImage image(pButton->size(), QImage::Format_ARGB32);
        image.setDevicePixelRatio(1.0);
        image.fill(mTokens.page);
        QStyleOptionButton option;
        option.initFrom(pButton);
        option.rect = pButton->rect();
        option.text = pButton->text();
        if (pButton->isDefault()) {
            option.features |= QStyleOptionButton::DefaultButton;
        }
        option.state &= ~(QStyle::State_MouseOver | QStyle::State_Sunken | QStyle::State_Raised | QStyle::State_HasFocus);
        switch (drawn) {
        case Drawn::Rest:
            option.state |= QStyle::State_Raised;
            break;
        case Drawn::Hover:
            option.state |= QStyle::State_MouseOver | QStyle::State_Raised;
            break;
        case Drawn::Pressed:
            option.state |= QStyle::State_MouseOver | QStyle::State_Sunken;
            break;
        }
        {
            QPainter painter(&image);
            pButton->style()->drawControl(QStyle::CE_PushButton, &option, &painter, pButton);
        }
        // Inside the leading padding: clear of the centred word and below the rounded corner
        return image.pixelColor(QPoint(uiDesign::scmInputBorderWidth + 3, image.height() / 2));
    }

    States statesOf(QPushButton* pButton) const
    {
        const States states{fillOf(pButton, Drawn::Rest), fillOf(pButton, Drawn::Hover), fillOf(pButton, Drawn::Pressed)};
        QString line = qsl("  %1: rest %2, hover %3, pressed %4").arg(pButton->text(), states.rest.name(), states.hover.name(), states.pressed.name());
        if (pButton->property(uiDesign::scmProp_primaryButton).toBool()) {
            line += qsl(", pressed ink %1").arg(uiDesign::primaryPressedInk(mTokens).name());
        }
        qInfo().noquote() << line;
        return states;
    }

private slots:
    void cleanup()
    {
        mpContainer.reset();
        mpPrimary = nullptr;
        mpPlain = nullptr;
    }

    void test_thePointerDeepensThePrimaryWash_data() { addBaseStyles(); }

    void test_thePointerDeepensThePrimaryWash()
    {
        QFETCH(bool, dark);
        showButtons(dark);
        if (QTest::currentTestFailed()) {
            return;
        }

        QVERIFY2(mpPrimary->testAttribute(Qt::WA_Hover), "the primary button is not given hover events, so a real pointer never reaches its :hover rule");
        const QColor chosenRow = uiDesign::blend(mTokens.page, mTokens.accent, uiDesign::scmAccentWashStrength);
        const qreal chosenRowRatio = uiDesign::contrastRatio(mTokens.accentText, chosenRow);
        QVERIFY2(chosenRowRatio >= uiDesign::scmTextMinimumRatio,
                 qPrintable(qsl("accentText %1 reads at %2:1 on the accent wash %3 a chosen row is filled with")
                                    .arg(mTokens.accentText.name(), QString::number(chosenRowRatio, 'f', 2), chosenRow.name())));
        const States primary = statesOf(mpPrimary);
        QVERIFY2(TestPixels::distance(primary.hover, primary.rest) > scmStatesApart,
                 qPrintable(qsl("the primary button reads %1 under the pointer and %2 at rest, so the pointer changes nothing").arg(primary.hover.name(), primary.rest.name())));
        QVERIFY2(TestPixels::distance(primary.hover, mTokens.accent) < TestPixels::distance(primary.rest, mTokens.accent),
                 qPrintable(qsl("the primary button reads %1 under the pointer, further from the accent %2 than its resting %3, so the pointer dims it")
                                    .arg(primary.hover.name(), mTokens.accent.name(), primary.rest.name())));
        const qreal ratio = uiDesign::contrastRatio(mTokens.accentText, primary.hover);
        qInfo().noquote() << qsl("  accentText %1 reads at %2:1 on the hovered fill").arg(mTokens.accentText.name(), QString::number(ratio, 'f', 2));
        QVERIFY2(ratio >= uiDesign::scmTextMinimumRatio,
                 qPrintable(qsl("accentText %1 reads at %2:1 on the hovered fill %3").arg(mTokens.accentText.name(), QString::number(ratio, 'f', 2), primary.hover.name())));
    }

    void test_aPressedPrimaryReadsAsItsOwn_data() { addBaseStyles(); }

    void test_aPressedPrimaryReadsAsItsOwn()
    {
        QFETCH(bool, dark);
        showButtons(dark);
        if (QTest::currentTestFailed()) {
            return;
        }

        const States primary = statesOf(mpPrimary);
        QVERIFY2(TestPixels::distance(primary.pressed, primary.hover) > scmStatesApart,
                 qPrintable(qsl("the primary button reads %1 pressed and %2 under the pointer, so a press looks like a hover").arg(primary.pressed.name(), primary.hover.name())));
        QVERIFY2(TestPixels::distance(primary.pressed, primary.rest) > scmStatesApart,
                 qPrintable(qsl("the primary button reads %1 pressed and %2 at rest").arg(primary.pressed.name(), primary.rest.name())));
        const QColor pressedInk = uiDesign::primaryPressedInk(mTokens);
        const qreal ratio = uiDesign::contrastRatio(pressedInk, primary.pressed);
        qInfo().noquote() << qsl("  pressed ink %1 reads at %2:1 on the pressed fill").arg(pressedInk.name(), QString::number(ratio, 'f', 2));
        QVERIFY2(ratio >= uiDesign::scmTextMinimumRatio,
                 qPrintable(qsl("the pressed ink %1 reads at %2:1 on the pressed fill %3").arg(pressedInk.name(), QString::number(ratio, 'f', 2), primary.pressed.name())));
    }

    void test_aPlainButtonKeepsItsOwnStates_data() { addBaseStyles(); }

    void test_aPlainButtonKeepsItsOwnStates()
    {
        QFETCH(bool, dark);
        showButtons(dark);
        if (QTest::currentTestFailed()) {
            return;
        }

        const States plain = statesOf(mpPlain);
        QVERIFY2(TestPixels::distance(plain.hover, plain.rest) > scmStatesApart,
                 qPrintable(qsl("the plain button reads %1 under the pointer and %2 at rest").arg(plain.hover.name(), plain.rest.name())));
        QVERIFY2(TestPixels::distance(plain.pressed, plain.hover) > scmStatesApart,
                 qPrintable(qsl("the plain button reads %1 pressed and %2 under the pointer").arg(plain.pressed.name(), plain.hover.name())));
    }
};

#include "PrimaryButtonStatesTest.moc"
MUDLET_GROUPED_TEST_MAIN(PrimaryButtonStatesTest)
