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
 * Guards the connection dialog's design-language styling: fields, buttons, the
 * notice and the games-list chips. A field the validator objects to is marked
 * by a property a rule paints from, since a sheet's background beats a palette.
 *
 * Run with: ctest -R ConnectionDialogStyleTest -V
 */

#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "uiDesign.h"

#include <QtTest/QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QFontMetrics>
#include <QHoverEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyleOptionToolButton>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogStyleTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;

    // A saved profile keeps the first-launch tutorial, which hides the whole form, out of the way
    const QString mProfile = qsl("Aardvark-ConnDialogFieldStyle");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    // The fields only carry text, and the validator only runs, once a profile is being edited
    void selectTheTestProfile() const
    {
        auto* pDialog = dialog();
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, mProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The test profile is missing from the games list");
        pDialog->listWidget_profiles->setCurrentItem(items.first());
        QCoreApplication::processEvents();
    }

    static QString fieldState(const QLineEdit* pField) { return pField->property("connectionFieldState").toString(); }

    // A rounded edge is antialiased against what is behind it, and a screen
    // that doubles its pixels draws the one line over two rows
    static constexpr int scmHairlineInkSlack = 24;

    // A screen that doubles its pixels draws one logical row over two device
    // rows, and an antialiased edge splits its ink between them
    static int nearestToAccent(const QImage& shot, const QPoint& inDialog, const QColor& accent, QColor* pRead)
    {
        const qreal ratio = shot.devicePixelRatio();
        int nearest = 3 * 255;
        for (int line = 0, lines = qMax(1, qRound(ratio)); line < lines; ++line) {
            const QPoint inShot(qRound(inDialog.x() * ratio), qRound(inDialog.y() * ratio) + line);
            if (!shot.rect().contains(inShot)) {
                continue;
            }
            const QColor sample = shot.pixelColor(inShot);
            const int distance = TestPixels::distance(sample, accent);
            if (distance < nearest) {
                nearest = distance;
                *pRead = sample;
            }
        }
        return nearest;
    }

    static constexpr int scmHaloRoom = ProfileChipDelegate::scmChipHaloGap + ProfileChipDelegate::scmChipHaloWidth;

    static QString tabFontMismatch(const dlgConnectionProfiles* pDialog)
    {
        const QFont expected = pDialog->font();
        int checked = 0;
        for (const QWidget* pPage : {pDialog->tab_connection_info, pDialog->tab_options}) {
            for (const QWidget* pWidget : pPage->findChildren<QWidget*>()) {
                if (!qobject_cast<const QLabel*>(pWidget) && !qobject_cast<const QLineEdit*>(pWidget) && !qobject_cast<const QComboBox*>(pWidget) && !qobject_cast<const QCheckBox*>(pWidget)) {
                    continue;
                }
                ++checked;
                const QFont font = pWidget->font();
                if (font.family() != expected.family() || font.pointSizeF() != expected.pointSizeF()) {
                    return qsl("%1 reads in %2 at %3pt against the dialog's %4 at %5pt")
                            .arg(pWidget->objectName(), font.family(), QString::number(font.pointSizeF()), expected.family(), QString::number(expected.pointSizeF()));
                }
            }
        }
        return checked ? QString() : qsl("there is no label, field or checkbox on either tab to read");
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet({mProfile}), qPrintable(mSession.why()));

        // The macOS theme registers its own QLabel and QCheckBox fonts, offscreen
        // none. Built rather than copied from the application font, or Qt takes
        // it for that font.
        const QFont classFont(QApplication::font().family(), qRound(QApplication::font().pointSizeF()));
        QVERIFY(!classFont.isCopyOf(QApplication::font()));
        QApplication::setFont(classFont, "QLabel");
        QApplication::setFont(classFont, "QCheckBox");

        mudlet::self()->startAutoLogin({});
        // the dialog is only shown from a queued lambda, so the pointer turning
        // up is not enough
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000));
    }

    void cleanupTestCase() { mSession.stop(); }

    void test_theTabsFieldsAreDrawnFromTheDesign()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        const QString field = uiDesign::themeTokens().field.name();
        QVERIFY2(pDialog->tab_connection_info->styleSheet().contains(field),
                 qPrintable(qsl("the Connect-to tab is not drawn on the design's field surface (%1): its sheet is \"%2\"").arg(field, pDialog->tab_connection_info->styleSheet().left(120))));
        QVERIFY2(pDialog->tab_options->styleSheet().contains(field), "the Options tab is not drawn on the design's field surface");

        for (const QLineEdit* pField : {pDialog->profile_name_entry, pDialog->host_name_entry, pDialog->port_entry}) {
            QVERIFY2(pField->font().pointSize() == pDialog->font().pointSize(),
                     qPrintable(qsl("%1 is drawn at %2pt against the dialog's own %3pt")
                                        .arg(pField->objectName(), QString::number(pField->font().pointSize()), QString::number(pDialog->font().pointSize()))));
        }

        const QFontMetrics fm(pDialog->port_entry->font());
        const int leastNeeded = fm.horizontalAdvance(qsl("65535")) + 2 * (uiDesign::scmInputPaddingHorizontal + uiDesign::scmInputBorderWidth);
        QVERIFY2(pDialog->port_entry->minimumWidth() >= leastNeeded,
                 qPrintable(qsl("the port field is %1px wide, against the %2px five digits need in it").arg(QString::number(pDialog->port_entry->minimumWidth()), QString::number(leastNeeded))));
    }

    void test_theInformationBoxIsAField()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();
        QVERIFY2(pDialog->informationArea->isVisible(), "the information box is not on show");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheet = pDialog->informationArea->styleSheet();
        QVERIFY2(sheet.contains(qsl("QPlainTextEdit")) && sheet.contains(tokens.field.name()),
                 qPrintable(qsl("the information box's sheet does not draw its text edit on the field surface (%1): \"%2\"").arg(tokens.field.name(), sheet.left(160))));

        // The top edge's middle is a straight run of the hairline; the very
        // corner is outside the rounded rectangle and reads what lies behind it
        QPlainTextEdit* pField = pDialog->mud_description_textedit;
        QVERIFY2(pField->isVisible() && pField->width() > 4 * uiDesign::scmRadiusInput, "the description field is not on show to be read");
        const QImage shot = pDialog->grab().toImage();
        const qreal ratio = shot.devicePixelRatio();
        const auto readAt = [&](const QPoint& inField) {
            const QPoint at = pField->mapTo(pDialog, inField);
            return shot.pixelColor(qRound(at.x() * ratio), qRound(at.y() * ratio));
        };
        const QColor edge = readAt(QPoint(pField->width() / 2, 0));
        const QColor corner = readAt(QPoint(0, 0));
        qInfo().noquote() << qsl("  the description field's top edge reads %1 and its corner pixel %2, against a hairline of %3").arg(edge.name(), corner.name(), tokens.border.name());
        QVERIFY2(TestPixels::distance(edge, tokens.border) <= scmHairlineInkSlack,
                 qPrintable(qsl("the description field's top edge reads %1 rather than the design's hairline %2").arg(edge.name(), tokens.border.name())));
        QVERIFY2(TestPixels::distance(corner, edge) > scmHairlineInkSlack,
                 qPrintable(qsl("the description field's corner pixel reads %1, the same as its edge, so the corner is square rather than cut to the field's radius").arg(corner.name())));
    }

    void test_theButtonsAreDrawnFromTheDesign()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();
        // The validator is what lets Connect out of its disabled state, and the
        // primary fill is only claimed by an enabled default button
        pDialog->host_name_entry->setText(qsl("mudlet.org"));
        pDialog->port_entry->setText(qsl("23"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        for (const QWidget* pContainer : {static_cast<QWidget*>(pDialog->profileAdminArea), static_cast<QWidget*>(pDialog->widget_bottom)}) {
            QVERIFY2(
                    pContainer->styleSheet().contains(qsl("QPushButton")),
                    qPrintable(qsl("%1 carries no push button rules, so its buttons are still the platform's: its sheet is \"%2\"").arg(pContainer->objectName(), pContainer->styleSheet().left(120))));
        }

        QVERIFY2(pDialog->copy_profile_toolbutton->property(uiDesign::scmProp_menuButton).toBool(),
                 "the Copy button does not say it is a tool button standing on a form as a button, so no rule reaches it");
        QVERIFY2(pDialog->profileAdminArea->styleSheet().contains(qsl("QToolButton[%1").arg(QString::fromLatin1(uiDesign::scmProp_menuButton))),
                 "the admin row's sheet draws push buttons only, so the Copy button is left to the platform");
        QCOMPARE(pDialog->copy_profile_toolbutton->popupMode(), QToolButton::MenuButtonPopup);
        // The actions stay on the button so it is their associated widget, where
        // the accessible names are set. Named rather than counted: setMenu() puts
        // the menu's own action on the button as well.
        QStringList carried;
        for (const QAction* pAction : pDialog->copy_profile_toolbutton->actions()) {
            carried << pAction->objectName();
        }
        QVERIFY2(carried.contains(qsl("copyProfile")) && carried.contains(qsl("copyProfileSettingsOnly")),
                 qPrintable(qsl("the Copy button no longer carries both of its actions, so the accessible names set on their widget have nowhere to land: it holds %1").arg(carried.join(qsl(", ")))));

        // Asked of the style rather than clicked: a popup menu on macOS runs an
        // event loop of its own that a test cannot get back out of
        QStyleOptionToolButton copyOption;
        copyOption.initFrom(pDialog->copy_profile_toolbutton);
        copyOption.rect = pDialog->copy_profile_toolbutton->rect();
        copyOption.features = QStyleOptionToolButton::Menu | QStyleOptionToolButton::MenuButtonPopup;
        copyOption.subControls = QStyle::SC_ToolButton | QStyle::SC_ToolButtonMenu;
        copyOption.text = pDialog->copy_profile_toolbutton->text();
        const QRect menuHalf = pDialog->copy_profile_toolbutton->style()->subControlRect(QStyle::CC_ToolButton, &copyOption, QStyle::SC_ToolButtonMenu, pDialog->copy_profile_toolbutton);
        qInfo().noquote()
                << qsl("  the Copy button is %1px wide and hands its trailing %2px to the menu, at x %3").arg(pDialog->copy_profile_toolbutton->width()).arg(menuHalf.width()).arg(menuHalf.left());
        QVERIFY2(menuHalf.width() > 0 && menuHalf.height() > 0, "the Copy button hands no rectangle at all to its menu, so the split is gone");
        QVERIFY2(menuHalf.left() > pDialog->copy_profile_toolbutton->width() / 2, "the half that opens the Copy menu is not the trailing one");

        auto* pConnect = pDialog->findChild<QPushButton*>(qsl("connectButton"));
        QVERIFY2(pConnect, "the Connect button is not there to be read");
        QVERIFY2(pConnect->isDefault(), "Connect is not the dialog's default button, so Return no longer reaches it");
        QVERIFY2(pDialog->widget_bottom->styleSheet().contains(qsl(":default")), "the button row's sheet says nothing about the default button, so Connect reads as one control among four");

        // A platform bevel is a gradient that lands nowhere near this tone; the
        // top edge's middle is a straight run of the line, not its rounded corner
        QPushButton* pNew = pDialog->new_profile_button;
        QVERIFY2(pNew->isVisible() && pNew->width() > 8, "the New button is not on show to be measured");
        const QImage shot = pDialog->grab().toImage();
        const qreal shotRatio = shot.devicePixelRatio();
        const QPoint topMiddle = pNew->mapTo(pDialog, QPoint(pNew->width() / 2, 0));
        const QColor border = uiDesign::themeTokens().border;
        int nearest = 3 * 255;
        QColor read = border;
        for (int row = 0, rows = qMax(1, qRound(shotRatio)); row < rows; ++row) {
            const QPoint inShot(qRound(topMiddle.x() * shotRatio), qRound(topMiddle.y() * shotRatio) + row);
            if (!shot.rect().contains(inShot)) {
                continue;
            }
            const QColor sample = shot.pixelColor(inShot);
            const int distance = TestPixels::distance(sample, border);
            if (distance < nearest) {
                nearest = distance;
                read = sample;
            }
        }
        qInfo().noquote() << qsl("  the New button's top edge reads %1 against a hairline of %2, %3 away").arg(read.name(), border.name()).arg(nearest);
        QVERIFY2(nearest <= scmHairlineInkSlack, qPrintable(qsl("the New button's outline reads %1 rather than the design's %2").arg(read.name(), border.name())));
    }

    void test_theCopyButtonShowsItsTwoHalves()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();
        QCoreApplication::processEvents();

        QToolButton* pButton = pDialog->copy_profile_toolbutton;
        QVERIFY2(pButton->isVisible() && pButton->width() > 3 * uiDesign::scmInputDropDownWidth,
                 qPrintable(qsl("the Copy button is %1px wide and %2on show, which is nothing to read").arg(pButton->width()).arg(pButton->isVisible() ? QString() : qsl("not "))));

        const QString sheet = pDialog->profileAdminArea->styleSheet();
        const int arrowAt = sheet.indexOf(qsl("::menu-arrow"));
        QVERIFY2(arrowAt >= 0, "the admin row's sheet says nothing about the chevron on the menu half, so the platform's own mark is still on it");
        QVERIFY2(sheet.mid(arrowAt, 80).contains(qsl("image:")), qPrintable(qsl("the menu half's chevron is named without a picture: \"%1\"").arg(sheet.mid(arrowAt, 80))));
        QVERIFY2(sheet.contains(qsl("::menu-button:hover")), "the menu half has no hover of its own, so it lights only as part of the button");

        QStyleOptionToolButton option;
        option.initFrom(pButton);
        option.rect = pButton->rect();
        option.text = pButton->text();
        option.icon = pButton->icon();
        option.iconSize = pButton->iconSize();
        option.toolButtonStyle = pButton->toolButtonStyle();
        option.features = QStyleOptionToolButton::Menu | QStyleOptionToolButton::MenuButtonPopup;
        option.subControls = QStyle::SC_ToolButton | QStyle::SC_ToolButtonMenu;
        const QRect half = pButton->style()->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButtonMenu, pButton);
        QVERIFY2(half.width() > 4 && half.height() > 8, qPrintable(qsl("the style hands the menu a %1x%2 rectangle, which is nothing to read").arg(half.width()).arg(half.height())));

        const QImage shot = pDialog->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const qreal ratio = shot.devicePixelRatio();
        const QPoint corner = pButton->mapTo(pDialog, half.topLeft());
        const int seamAt = qRound(corner.x() * ratio);
        const int top = qRound(corner.y() * ratio);
        const int height = qRound(half.height() * ratio);
        QVERIFY2(shot.rect().contains(QPoint(seamAt, top + height / 2)), "the Copy button's menu half does not land inside the grab of the window");

        const QColor border = uiDesign::themeTokens().border;
        int onTheLine = 0;
        int rows = 0;
        QColor readAs;
        for (int y = top + height / 5; y <= top + height - height / 5; ++y) {
            if (!shot.rect().contains(QPoint(seamAt, y))) {
                continue;
            }
            ++rows;
            const QColor sample = shot.pixelColor(seamAt, y);
            if (!readAs.isValid()) {
                readAs = sample;
            }
            const int distance = TestPixels::distance(sample, border);
            if (distance <= scmHairlineInkSlack) {
                ++onTheLine;
            }
        }
        const QColor justInside = shot.pixelColor(seamAt + qMax(3, qRound(3 * ratio)), top + height / 3);
        const int insideDistance = TestPixels::distance(justInside, border);
        qInfo().noquote() << qsl("  the Copy button's menu half starts at x %1; its leading edge reads %2 against the hairline's %3 on %4 of %5 rows, and %6 a few pixels in")
                                     .arg(half.left())
                                     .arg(readAs.name(), border.name())
                                     .arg(onTheLine)
                                     .arg(rows)
                                     .arg(justInside.name());
        QVERIFY2(rows > 2 && onTheLine * 5 >= rows * 4,
                 qPrintable(qsl("the half's leading edge carries no seam: %1 of %2 rows read the design's hairline of %3, the first of them %4")
                                    .arg(onTheLine)
                                    .arg(rows)
                                    .arg(border.name(), readAs.name())));
        QVERIFY2(insideDistance > scmHairlineInkSlack,
                 qPrintable(qsl("the half is filled with the hairline's own tone (%1 a few pixels in), so what was read at its edge is a fill rather than a seam").arg(justInside.name())));
    }

    void test_aBadPortMarksTheFieldAndAGoodOneClearsIt()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();

        // so that the port is the only thing the validator can object to
        pDialog->host_name_entry->setText(qsl("mudlet.org"));
        QCoreApplication::processEvents();

        // "12a" takes the validator's non-digit branch
        pDialog->port_entry->setText(qsl("12a"));
        QCoreApplication::processEvents();
        QCOMPARE(fieldState(pDialog->port_entry), qsl("error"));

        pDialog->port_entry->setText(qsl("23"));
        QCoreApplication::processEvents();
        QVERIFY2(fieldState(pDialog->port_entry).isEmpty(), qPrintable(qsl("a port of 23 left the field marked \"%1\"").arg(fieldState(pDialog->port_entry))));
    }

    void test_anEmptyServerAddressMarksTheField()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();
        pDialog->port_entry->setText(qsl("23"));
        QCoreApplication::processEvents();

        // Whitespace rather than empty: slot_updateUrl() takes an empty field as
        // the user still typing and never reaches the validator, which trims this to nothing
        pDialog->host_name_entry->setText(qsl(" "));
        QCoreApplication::processEvents();
        QCOMPARE(fieldState(pDialog->host_name_entry), qsl("error"));

        pDialog->host_name_entry->setText(qsl("mudlet.org"));
        QCoreApplication::processEvents();
        QVERIFY2(fieldState(pDialog->host_name_entry).isEmpty(), qPrintable(qsl("a server address left the field marked \"%1\"").arg(fieldState(pDialog->host_name_entry))));
    }

    // A menu of its own, not the temporary one Qt builds from the button's
    // actions on every press: only a menu that exists between presses can be styled
    void test_theCopyMenuIsDrawnFromTheDesign()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        QMenu* pMenu = pDialog->copy_profile_toolbutton->menu();
        QVERIFY2(pMenu, "the Copy button carries no menu of its own, so nothing the style pass writes can reach one");
        const QList<QAction*> rows = pMenu->actions();
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(0)->objectName(), qsl("copyProfile"));
        QCOMPARE(rows.at(1)->objectName(), qsl("copyProfileSettingsOnly"));

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheet = pDialog->profileAdminArea->styleSheet();
        QVERIFY2(sheet.contains(qsl("QMenu")), qPrintable(qsl("the admin row's sheet says nothing about a QMenu, so Copy's menu is the platform's: its sheet is \"%1\"").arg(sheet.left(120))));
        QVERIFY2(sheet.contains(tokens.card.name()), qPrintable(qsl("the admin row's sheet never names the card surface (%1) the menu is drawn on").arg(tokens.card.name())));
        QVERIFY2(pMenu->testAttribute(Qt::WA_TranslucentBackground), "the Copy menu's window was never asked to be see-through, so its corner is opaque");

        // Popped up rather than clicked: a menu run through exec() takes an event
        // loop of its own that a case cannot get back out of
        pMenu->popup(pDialog->copy_profile_toolbutton->mapToGlobal(QPoint(0, pDialog->copy_profile_toolbutton->height())));
        QVERIFY(QTest::qWaitForWindowExposed(pMenu));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QImage shot = pMenu->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        pMenu->hide();
        QCoreApplication::processEvents();

        const QColor corner = QColor::fromRgba(shot.pixel(0, 0));
        const QColor surface = QColor::fromRgba(shot.pixel(qRound(2 * shot.devicePixelRatio()), shot.height() / 2));
        QVERIFY2(corner.alpha() == 0, qPrintable(qsl("the Copy menu's corner is painted %1 rather than cut away, so its radius has a square window behind it").arg(corner.name(QColor::HexArgb))));
        const int distance = TestPixels::distance(surface, tokens.card);
        QVERIFY2(surface.alpha() == 255 && distance <= scmHairlineInkSlack,
                 qPrintable(qsl("the Copy menu is painted %1 rather than the card surface's %2").arg(surface.name(QColor::HexArgb), tokens.card.name())));
    }

    void test_theNoticeIsDrawnAsTheEditorsBanner()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        selectTheTestProfile();

        // The validator's non-digit branch puts the notice up with its error picture
        pDialog->host_name_entry->setText(qsl("mudlet.org"));
        QCoreApplication::processEvents();
        pDialog->port_entry->setText(qsl("12a"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QVERIFY2(pDialog->notificationArea->isVisible(), "a port of \"12a\" left the notice hidden, so there is nothing to read");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheet = pDialog->notificationArea->styleSheet();
        QVERIFY2(sheet.contains(tokens.accentSoft), qPrintable(qsl("the notice is not washed with the accent (%1): its sheet is \"%2\"").arg(tokens.accentSoft, sheet.left(160))));
        QVERIFY2(sheet.contains(tokens.accent.name()), qPrintable(qsl("the notice carries no accent hairline (%1): its sheet is \"%2\"").arg(tokens.accent.name(), sheet.left(160))));

        // The .ui sets 16pt on the frame and 8pt on the words
        QCOMPARE(pDialog->notificationArea->font().pointSize(), pDialog->font().pointSize());
        QVERIFY2(pDialog->notificationAreaMessageBox->font().pointSize() == pDialog->font().pointSize(),
                 qPrintable(qsl("the notice's words are set at %1pt against the dialog's own %2pt")
                                    .arg(QString::number(pDialog->notificationAreaMessageBox->font().pointSize()), QString::number(pDialog->font().pointSize()))));

        QLabel* pGlyphLabel = pDialog->notificationAreaIconLabelError;
        QVERIFY2(pGlyphLabel->isVisible(), "the error notice is not showing the picture that says which reading it is");
        const QPixmap glyph = pGlyphLabel->pixmap(Qt::ReturnByValue);
        QVERIFY2(!glyph.isNull(), "the error notice's label carries no picture at all");
        const qreal glyphRatio = glyph.devicePixelRatio();
        const QSize drawnAt(qRound(glyph.width() / glyphRatio), qRound(glyph.height() / glyphRatio));
        qInfo().noquote() << qsl("  the error notice's picture is %1x%2 at a ratio of %3, so it is drawn %4x%5")
                                     .arg(glyph.width())
                                     .arg(glyph.height())
                                     .arg(glyphRatio)
                                     .arg(drawnAt.width())
                                     .arg(drawnAt.height());
        QVERIFY2(drawnAt == QSize(uiDesign::scmNoticeGlyphSize, uiDesign::scmNoticeGlyphSize),
                 qPrintable(qsl("the notice's picture is drawn %1x%2 rather than at the shared %3px square").arg(drawnAt.width()).arg(drawnAt.height()).arg(uiDesign::scmNoticeGlyphSize)));

        // A platform Box frame is the palette's dark line, nowhere near the
        // accent; the top edge's middle is a straight run, not the rounded corner
        const QImage shot = pDialog->grab().toImage();
        const qreal shotRatio = shot.devicePixelRatio();
        const QPoint topMiddle = pDialog->notificationArea->mapTo(pDialog, QPoint(pDialog->notificationArea->width() / 2, 0));
        const QColor accent = tokens.accent;
        int nearest = 3 * 255;
        QColor read = accent;
        for (int row = 0, rows = qMax(1, qRound(shotRatio)); row < rows; ++row) {
            const QPoint inShot(qRound(topMiddle.x() * shotRatio), qRound(topMiddle.y() * shotRatio) + row);
            if (!shot.rect().contains(inShot)) {
                continue;
            }
            const QColor sample = shot.pixelColor(inShot);
            const int distance = TestPixels::distance(sample, accent);
            if (distance < nearest) {
                nearest = distance;
                read = sample;
            }
        }
        qInfo().noquote() << qsl("  the notice's top edge reads %1 against an accent hairline of %2, %3 away").arg(read.name(), accent.name()).arg(nearest);
        QVERIFY2(nearest <= scmHairlineInkSlack, qPrintable(qsl("the notice's outline reads %1 rather than the design's accent %2").arg(read.name(), accent.name())));

        pDialog->port_entry->setText(qsl("23"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QVERIFY2(!pDialog->notificationArea->isVisible(), "a port of 23 is valid, and the notice about the invalid one is still up");
    }

    // Three games to a row, at the size the dialog opens with and at the narrowest it can be dragged to
    void test_theGamesListShowsThreeGamesToARow()
    {
        auto* pDialog = dialog();
        auto* pTabBar = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY(pTabBar);
        const int shownTab = pTabBar->currentIndex();
        auto restoreTab = qScopeGuard([pTabBar, shownTab]() {
            pTabBar->setCurrentIndex(shownTab);
        });
        // All games
        pTabBar->setCurrentIndex(1);
        auto* pList = pDialog->listWidget_profiles;
        QTRY_VERIFY2(pList->count() > 3, "SETUP: the All games tab lists too few games to fill a row");

        const auto gamesOnTheFirstRow = [pList]() {
            const int top = pList->visualRect(pList->model()->index(0, 0)).top();
            int games = 0;
            while (games < pList->count() && pList->visualRect(pList->model()->index(games, 0)).top() == top) {
                ++games;
            }
            return games;
        };
        const auto report = [pDialog, pList]() {
            return qsl("dialog %1px, list %2px, viewport %3px").arg(QString::number(pDialog->width()), QString::number(pList->width()), QString::number(pList->viewport()->width()));
        };
        QTRY_VERIFY2(gamesOnTheFirstRow() == 3, qPrintable(qsl("%1 games to the first row at the opening size: %2").arg(QString::number(gamesOnTheFirstRow()), report())));

        const QSize opening = pDialog->size();
        auto restoreSize = qScopeGuard([pDialog, opening]() {
            pDialog->resize(opening);
        });
        pDialog->resize(1, pDialog->height());
        QTRY_COMPARE(pDialog->width(), pDialog->minimumWidth());
        QTRY_VERIFY2(gamesOnTheFirstRow() == 3, qPrintable(qsl("%1 games to the first row at the narrowest: %2").arg(QString::number(gamesOnTheFirstRow()), report())));
        QVERIFY2(pList->width() == pList->minimumWidth(),
                 qPrintable(qsl("SETUP: the list has %1px over its narrowest at the dialog's narrowest, so this checks nothing tight").arg(pList->width() - pList->minimumWidth())));
    }

    void test_theGamesListChipsHaveRoundedCorners()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        const auto items = pDialog->findData(*pDialog->listWidget_profiles, mProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The test profile is missing from the games list");
        const QIcon icon = items.first()->icon();
        QVERIFY2(!icon.isNull(), "the test profile's row carries no picture at all");

        const QList<QSize> sizes = icon.availableSizes();
        QVERIFY2(!sizes.isEmpty(), "the test profile's picture reports no size, so there is nothing to read it at");
        const QImage chip = icon.pixmap(sizes.first()).toImage().convertToFormat(QImage::Format_ARGB32);
        qInfo().noquote() << qsl("  the games list chip is %1x%2").arg(chip.width()).arg(chip.height());
        QVERIFY2(chip.width() > 4 * uiDesign::scmRadiusChip && chip.height() > 2 * uiDesign::scmRadiusChip, "the chip is too small for a corner to be read out of it");

        const QColor corner = QColor::fromRgba(chip.pixel(0, 0));
        const QColor middle = QColor::fromRgba(chip.pixel(chip.width() / 2, chip.height() / 2));
        // The chip's build ratio turns the logical radius into image pixels; the
        // chip is 120 logical pixels wide
        const qreal ratio = qMax(1.0, chip.width() / static_cast<qreal>(120));
        const int justInside = qRound((uiDesign::scmRadiusChip + 2) * ratio);
        const QColor diagonal = QColor::fromRgba(chip.pixel(justInside, justInside));
        qInfo().noquote() << qsl("  its corner reads %1, its middle %2 and %3px in along the diagonal %4")
                                     .arg(corner.name(QColor::HexArgb), middle.name(QColor::HexArgb))
                                     .arg(justInside)
                                     .arg(diagonal.name(QColor::HexArgb));

        QVERIFY2(corner.alpha() == 0, qPrintable(qsl("the chip's corner is painted %1 rather than cut away, so it is still a square block").arg(corner.name(QColor::HexArgb))));
        QVERIFY2(middle.alpha() == 255, qPrintable(qsl("the middle of the chip is %1 rather than opaque, so the clip took the picture with it").arg(middle.name(QColor::HexArgb))));
        QVERIFY2(diagonal.alpha() == 255,
                 qPrintable(qsl("the chip is still transparent %1px in along its diagonal (%2), so its corner is cut far wider than the design's %3px")
                                    .arg(justInside)
                                    .arg(diagonal.name(QColor::HexArgb))
                                    .arg(uiDesign::scmRadiusChip)));

        // Shipped banners come in on a path of their own; the All games tab shows them all
        auto* pTabs = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY2(pTabs && pTabs->count() == 2, "the games list's two tabs are not there to switch between");
        pTabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QStringList squareOnes;
        int read = 0;
        for (int i = 0, total = pDialog->listWidget_profiles->count(); i < total; ++i) {
            const QListWidgetItem* pRow = pDialog->listWidget_profiles->item(i);
            const QList<QSize> rowSizes = pRow->icon().availableSizes();
            if (rowSizes.isEmpty()) {
                continue;
            }
            ++read;
            const QImage rowChip = pRow->icon().pixmap(rowSizes.first()).toImage().convertToFormat(QImage::Format_ARGB32);
            if (QColor::fromRgba(rowChip.pixel(0, 0)).alpha() != 0) {
                squareOnes << pRow->text();
            }
        }
        pTabs->setCurrentIndex(0);
        QCoreApplication::processEvents();
        qInfo().noquote() << qsl("  %1 games on the All games tab carry a picture; %2 of them still square").arg(read).arg(squareOnes.size());
        QVERIFY2(read > 10, qPrintable(qsl("only %1 games on the All games tab carry a picture, which is not the shipped list").arg(read)));
        QVERIFY2(squareOnes.isEmpty(), qPrintable(qsl("%1 shipped banner(s) are still square blocks rather than chips: %2").arg(squareOnes.size()).arg(squareOnes.join(qsl(", ")))));
    }

    void test_aHoveredChipCarriesTheAccentRing()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        auto* pList = pDialog->listWidget_profiles;
        auto* pDelegate = qobject_cast<ProfileChipDelegate*>(pList->itemDelegate());
        QVERIFY2(pDelegate, "the games list carries no chip delegate, so nothing is there to draw the halo");
        // Hover events reach only a widget with WA_Hover, measured off on this
        // viewport by default; without it the view never learns what is pointed at
        QVERIFY2(pList->viewport()->testAttribute(Qt::WA_Hover), "the games list's viewport asks for no hover events, so no pointer ever reaches the delegate");

        // Every shipped game is on the All games tab, so one chip can be pointed at while another is chosen
        auto* pTabs = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY2(pTabs && pTabs->count() == 2, "the games list's two tabs are not there to switch between");
        pTabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QVERIFY2(pList->count() > 2, qPrintable(qsl("the All games tab carries %1 rows, which is not enough to hover one and choose another").arg(pList->count())));

        // A different row is chosen, so the hovered chip shows the ring rather than the frame
        pList->setCurrentItem(pList->item(1));
        QCoreApplication::processEvents();

        const auto onShow = [&](const int row) {
            const QRect chip = pDelegate->chipRect(pList, pList->indexFromItem(pList->item(row)));
            return !chip.isEmpty() && pList->viewport()->rect().contains(chip) ? chip : QRect();
        };
        const QRect chip = onShow(0);
        QVERIFY2(!chip.isEmpty(), "the first game's chip is not on show in the list, so there is nothing to point at");
        QRect quietChip;
        int quietRow = -1;
        for (int i = 2; i < pList->count(); ++i) {
            quietChip = onShow(i);
            if (!quietChip.isEmpty()) {
                quietRow = i;
                break;
            }
        }
        QVERIFY2(!quietChip.isEmpty(), "no second chip is on show in the list, so nothing says the ring is only on the row being pointed at");

        // haloInk(), not the raw accent: the delegate walks it to the quiet floor against the list
        const QColor accent = pDelegate->haloInk();
        // The top edge's middle is a straight run of the ring; 1px above the
        // picture is the gap and 1px past the gap is the ring
        const auto abovePicture = [&](const QRect& target, const int by) {
            return pList->viewport()->mapTo(pDialog, QPoint(target.center().x(), target.top() - by));
        };
        const QPoint ringPoint = abovePicture(chip, ProfileChipDelegate::scmChipHaloGap + 1);
        const QPoint gapPoint = abovePicture(chip, 1);
        const QPoint quietPoint = abovePicture(quietChip, ProfileChipDelegate::scmChipHaloGap + 1);

        QColor beforeRead;
        const QImage beforeShot = pDialog->grab().toImage();
        const int beforeHover = nearestToAccent(beforeShot, ringPoint, accent, &beforeRead);

        // A hover event rather than a mouse move: QAbstractItemView answers
        // HoverMove with setHoverIndex() and ignores a plain move, so
        // QTest::mouseMove() leaves the chip reading its own fill
        const QPoint centre = chip.center();
        QHoverEvent arrive(QEvent::HoverMove, QPointF(centre), pList->viewport()->mapToGlobal(QPointF(centre)), QPointF(-1, -1));
        QApplication::sendEvent(pList->viewport(), &arrive);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QColor ringRead;
        QColor gapRead;
        QColor quietRead;
        const QImage hoveredShot = pDialog->grab().toImage();
        const int ring = nearestToAccent(hoveredShot, ringPoint, accent, &ringRead);
        const int gap = nearestToAccent(hoveredShot, gapPoint, accent, &gapRead);
        const int quiet = nearestToAccent(hoveredShot, quietPoint, accent, &quietRead);
        qInfo().noquote()
                << qsl("  the hovered chip is %1x%2 at %3,%4; %5px over it reads %6 (%7 from the accent %8), 1px over it %9 (%10), and the same place over the chip on row %11 reads %12 (%13)")
                           .arg(chip.width())
                           .arg(chip.height())
                           .arg(chip.left())
                           .arg(chip.top())
                           .arg(ProfileChipDelegate::scmChipHaloGap + 1)
                           .arg(ringRead.name())
                           .arg(ring)
                           .arg(accent.name(), gapRead.name())
                           .arg(gap)
                           .arg(quietRow)
                           .arg(quietRead.name())
                           .arg(quiet);

        QVERIFY2(beforeHover > scmHairlineInkSlack,
                 qPrintable(qsl("the chip already reads the accent %1 over it with no pointer on it, so the reading says nothing about the ring").arg(beforeRead.name())));
        QVERIFY2(ring <= scmHairlineInkSlack,
                 qPrintable(qsl("%1px over the hovered chip reads %2 rather than the design's accent %3, %4 away, so there is no ring standing off it")
                                    .arg(ProfileChipDelegate::scmChipHaloGap + 1)
                                    .arg(ringRead.name(), accent.name())
                                    .arg(ring)));
        QVERIFY2(gap > scmHairlineInkSlack,
                 qPrintable(qsl("1px over the hovered chip reads the accent %1 too, so the ring sits on the picture's own edge rather than standing %2px off it")
                                    .arg(gapRead.name())
                                    .arg(ProfileChipDelegate::scmChipHaloGap)));
        QVERIFY2(quiet > scmHairlineInkSlack,
                 qPrintable(qsl("the chip on row %1 reads the accent %2 over it with no pointer on it, so the ring is on every chip rather than the one being pointed at")
                                    .arg(quietRow)
                                    .arg(quietRead.name())));

        QHoverEvent leave(QEvent::HoverLeave, QPointF(-1, -1), pList->viewport()->mapToGlobal(QPoint(-1, -1)), QPointF(centre));
        QApplication::sendEvent(pList->viewport(), &leave);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QColor leftRead;
        const QImage leftShot = pDialog->grab().toImage();
        const int left = nearestToAccent(leftShot, ringPoint, accent, &leftRead);
        qInfo().noquote() << qsl("  with the pointer gone it reads %1, %2 from the accent").arg(leftRead.name()).arg(left);
        QVERIFY2(left > scmHairlineInkSlack, qPrintable(qsl("the chip still reads the accent %1 over it with the pointer off the list, so the ring is not hover only").arg(leftRead.name())));

        pTabs->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    // The hover ring's shape and place in green: a solid frame filling the gap met the picture's rounded corner,
    // and the list showed through both antialiased edges there as a dark seam
    void test_theChosenChipCarriesTheGreenRing()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");
        auto* pList = pDialog->listWidget_profiles;
        auto* pDelegate = qobject_cast<ProfileChipDelegate*>(pList->itemDelegate());
        QVERIFY2(pDelegate, "the games list carries no chip delegate, so nothing is there to draw the ring");

        selectTheTestProfile();
        const auto items = pDialog->findData(*pList, mProfile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "The test profile is missing from the games list");
        pList->scrollToItem(items.first());
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        const QRect row = pList->visualItemRect(items.first());
        const QRect chip = pDelegate->chipRect(pList, pList->indexFromItem(items.first()));
        QVERIFY2(!chip.isEmpty() && pList->viewport()->rect().contains(chip), "the chosen profile's chip is not on show in the list, so there is nothing to read");

        const QColor green = pDelegate->chosenInk();
        QVERIFY2(qAbs(green.hueF() - uiDesign::scmStateHue_update) < 0.05,
                 qPrintable(qsl("the chosen ring's ink %1 has hue %2, not the update green's %3").arg(green.name()).arg(green.hueF()).arg(uiDesign::scmStateHue_update)));
        QVERIFY2(TestPixels::distance(green, pDelegate->haloInk()) > 3 * scmHairlineInkSlack,
                 qPrintable(qsl("the chosen ring's %1 is too near the hover ring's %2 to tell the two apart").arg(green.name(), pDelegate->haloInk().name())));

        const auto offPicture = [&](const int above, const int below) {
            return pList->viewport()->mapTo(pDialog, QPoint(chip.center().x(), above ? chip.top() - above : chip.bottom() + below));
        };
        const int ringOffset = ProfileChipDelegate::scmChipHaloGap + 1;
        const QPoint ringPoint = offPicture(ringOffset, 0);
        const QPoint gapPoint = offPicture(1, 0);
        const QPoint underRingPoint = offPicture(0, ringOffset);
        // Past the ring's room is where the platform's selection highlight would fill the row
        const QPoint pastTheRingPoint = offPicture(0, scmHaloRoom + 3);

        QColor ringRead;
        QColor gapRead;
        QColor underRead;
        QColor pastRead;
        const QImage shot = pDialog->grab().toImage();
        const int ring = nearestToAccent(shot, ringPoint, green, &ringRead);
        const int gap = nearestToAccent(shot, gapPoint, green, &gapRead);
        const int under = nearestToAccent(shot, underRingPoint, green, &underRead);
        const int past = nearestToAccent(shot, pastTheRingPoint, green, &pastRead);
        qInfo().noquote() << qsl("  the chosen row is %1x%2 and its chip %3x%4; %5px over the picture reads %6 (%7 from the green %8), 1px over it %9 (%10), %5px under it %11 (%12) and %13px "
                                 "under it %14 (%15)")
                                     .arg(row.width())
                                     .arg(row.height())
                                     .arg(chip.width())
                                     .arg(chip.height())
                                     .arg(ringOffset)
                                     .arg(ringRead.name())
                                     .arg(ring)
                                     .arg(green.name(), gapRead.name())
                                     .arg(gap)
                                     .arg(underRead.name())
                                     .arg(under)
                                     .arg(scmHaloRoom + 3)
                                     .arg(pastRead.name())
                                     .arg(past);

        // A view repaints only an item's own rect as the pointer moves on or off
        // it, so a ring drawn outside it is left behind as a fragment
        const QSize needed = pList->iconSize() + QSize(2 * scmHaloRoom, 2 * scmHaloRoom);
        QVERIFY2(row.width() >= needed.width() && row.height() >= needed.height(),
                 qPrintable(qsl("the chosen row is %1x%2, which does not hold the %3x%4 its ring needs, so the ring is painted outside the rectangle the view repaints")
                                    .arg(row.width())
                                    .arg(row.height())
                                    .arg(needed.width())
                                    .arg(needed.height())));
        QVERIFY2(ring <= scmHairlineInkSlack,
                 qPrintable(qsl("%1px over the chosen chip reads %2 rather than the green %3, %4 away, so it carries no ring").arg(ringOffset).arg(ringRead.name(), green.name()).arg(ring)));
        QVERIFY2(gap > scmHairlineInkSlack,
                 qPrintable(qsl("1px over the chosen chip reads the green %1 too, so the ring sits on the picture's own edge rather than standing %2px off it as the hover ring does")
                                    .arg(gapRead.name())
                                    .arg(ProfileChipDelegate::scmChipHaloGap)));
        QVERIFY2(under <= scmHairlineInkSlack,
                 qPrintable(qsl("%1px under the chosen chip reads %2 rather than the green %3, %4 away, so the ring does not go all the way round the picture")
                                    .arg(ringOffset)
                                    .arg(underRead.name(), green.name())
                                    .arg(under)));
        QVERIFY2(past > scmHairlineInkSlack,
                 qPrintable(qsl("%1px under the chosen chip still reads the green %2, so something is painted under the row past the ring's own %3px")
                                    .arg(scmHaloRoom + 3)
                                    .arg(pastRead.name())
                                    .arg(scmHaloRoom)));
    }

    void test_anAppearanceChangeRedrawsTheFields()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        const bool wasDark = MudletApp::darkMode();
        const QString before = pDialog->tab_connection_info->styleSheet();

        TestPixels::setAppearanceAndWait(wasDark ? enums::Appearance::light : enums::Appearance::dark);

        const QString after = pDialog->tab_connection_info->styleSheet();
        QVERIFY2(after != before, "the appearance moved and the tab's sheet did not, so nothing re-ran the style pass");
        QVERIFY2(after.contains(uiDesign::themeTokens().field.name()), "the tab's sheet was rebuilt without the field surface of the appearance it was rebuilt in");

        TestPixels::setAppearanceAndWait(wasDark ? enums::Appearance::dark : enums::Appearance::light);
    }

    // setupUi() polishes the Connect-to page before its sheet is set and the
    // Options page after, with the dialog; a label or checkbox polished under a
    // sheet took its class font's face rather than the dialog's
    void test_bothTabsReadInTheDialogsFont()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        QString mismatch = tabFontMismatch(pDialog);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("with the Connect-to tab showing: %1").arg(mismatch)));

        pDialog->tabWidget_connectionInfo->setCurrentWidget(pDialog->tab_options);
        QCoreApplication::processEvents();
        mismatch = tabFontMismatch(pDialog);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("on the Options tab: %1").arg(mismatch)));

        pDialog->tabWidget_connectionInfo->setCurrentWidget(pDialog->tab_connection_info);
        QCoreApplication::processEvents();
        mismatch = tabFontMismatch(pDialog);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("back on the Connect-to tab: %1").arg(mismatch)));

        // The validator marks a field, and clears the mark, by polishing it again
        selectTheTestProfile();
        pDialog->host_name_entry->setText(qsl("mudlet.org"));
        pDialog->port_entry->setText(qsl("12a"));
        QCoreApplication::processEvents();
        QCOMPARE(fieldState(pDialog->port_entry), qsl("error"));
        mismatch = tabFontMismatch(pDialog);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("with the port marked: %1").arg(mismatch)));
        pDialog->port_entry->setText(qsl("23"));
        QCoreApplication::processEvents();
        QVERIFY(fieldState(pDialog->port_entry).isEmpty());
        mismatch = tabFontMismatch(pDialog);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("with the mark cleared: %1").arg(mismatch)));

        // A new appearance sets the pages' sheets again, which re-polishes their labels and checkboxes
        const bool wasDark = MudletApp::darkMode();
        TestPixels::setAppearanceAndWait(wasDark ? enums::Appearance::light : enums::Appearance::dark);
        mismatch = tabFontMismatch(pDialog);
        TestPixels::setAppearanceAndWait(wasDark ? enums::Appearance::dark : enums::Appearance::light);
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("in the other appearance: %1").arg(mismatch)));
    }

    // A platform hands its class fonts out anew with the application font, so those go first
    void test_bothTabsFollowANewApplicationFont()
    {
        auto* pDialog = dialog();
        QVERIFY2(pDialog, "No connection dialog to test against");

        const QFont original = QApplication::font();
        const QFont originalLabel = QApplication::font("QLabel");
        const QFont originalCheckBox = QApplication::font("QCheckBox");
        const QString family = original.family();
        const int larger = qRound(original.pointSizeF()) + 4;
        QApplication::setFont(QFont(family, larger), "QLabel");
        QApplication::setFont(QFont(family, larger), "QCheckBox");
        QApplication::setFont(QFont(family, larger));
        // The dialog answers its own font change with a call posted for later,
        // which one turn of the loop does not reach
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const qreal dialogSize = pDialog->font().pointSizeF();
        const QString mismatch = tabFontMismatch(pDialog);

        QApplication::setFont(originalLabel, "QLabel");
        QApplication::setFont(originalCheckBox, "QCheckBox");
        QApplication::setFont(original);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QCOMPARE(dialogSize, static_cast<qreal>(larger));
        QVERIFY2(mismatch.isEmpty(), qPrintable(qsl("after the interface font grew: %1").arg(mismatch)));
    }
};

MUDLET_GROUPED_TEST_MAIN(ConnectionDialogStyleTest)
#include "ConnectionDialogStyleTest.moc"
