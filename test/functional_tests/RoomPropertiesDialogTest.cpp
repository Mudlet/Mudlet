/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * dlgRoomProperties is what the 2D mapper's "Properties..." menu entry opens on
 * a room selection. It has to fold a whole selection into one form - a single
 * shared value gets a plain entry field, a mixed one gets a dropdown of what is
 * in use or a partially-checked box - and then report on OK only the fields the
 * player actually decided about, so that the ones left showing "(Multiple
 * values...)" leave every room alone.
 *
 * None of that is reachable from Lua: the dialog is a QDialog with no scripting
 * entry point, and its decisions live in widget state and in the arguments of
 * signal_save_symbol rather than in the map.
 *
 * Run with: ctest -R RoomPropertiesDialogTest -V
 */

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFontMetrics>
#include <QImage>
#include <QLabel>
#include <QLine>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScopeGuard>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "ColourValue.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgRoomProperties.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

#include <algorithm>
#include <cstdlib>

// What the dialog shows in place of a value the selection does not agree on.
// dlgRoomProperties keeps it as a private tr() string, and no translator is
// installed in a test run, so the source string is what appears.
static const QString scmMultipleValues{qsl("(Multiple values...)")};

// Everything signal_save_symbol carries, so a case can assert on the decision
// the dialog reached rather than on a room the mapper has yet to write to.
struct EmittedProperties
{
    int emitCount = 0;
    bool changeName = false;
    QString newName;
    bool changeRoomColor = false;
    int roomColorNumber = 0;
    bool changeSymbol = false;
    QString newSymbol;
    bool changeSymbolColor = false;
    QColor newSymbolColor;
    bool changeWeight = false;
    int newWeight = 0;
    bool changeLockStatus = false;
    std::optional<bool> newLockStatus;
    bool changeHiddenStatus = false;
    std::optional<bool> newHiddenStatus;
    bool changeBorderColor = false;
    QColor newBorderColor;
    bool changeBorderThickness = false;
    int newBorderThickness = 0;
    QList<int> roomIds;
};

class RoomPropertiesDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("RoomPropertiesDialog-Test");
    int mAreaId = 0;
    QPointer<dlgRoomProperties> mpDialog;
    EmittedProperties mEmitted;
    static constexpr int scmFirstRoom = 11;
    static constexpr int scmSecondRoom = 12;
    static constexpr int scmThirdRoom = 13;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TRoom* room(const int id) const { return roomDB()->getRoom(id); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildMap()
    {
        map()->mapClear();
        mAreaId = roomDB()->addArea(qsl("Properties area"));
        QVERIFY(mAreaId > 0);
        int offset = 0;
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, offset++, 0, 0));
        }
        mEmitted = EmittedProperties();
    }

    // The scan T2DMap::slot_setRoomProperties()'s caller does before it opens
    // the dialog: the dialog is handed counts, not rooms, so how a selection is
    // summarised is part of what it is being asked to display.
    dlgRoomProperties* openDialogOn(const QList<int>& roomIds)
    {
        QHash<QString, int> usedNames;
        QHash<int, int> usedColors;
        QHash<QString, int> usedSymbols;
        QHash<int, int> usedWeights;
        QHash<bool, int> usedLockStatus;
        int hiddenRoomCount = 0;
        QSet<int> rooms;

        for (const int roomId : roomIds) {
            TRoom* pRoom = room(roomId);
            if (!pRoom) {
                continue;
            }
            rooms.insert(roomId);
            if (!pRoom->name.isEmpty()) {
                ++usedNames[pRoom->name];
            }
            ++usedColors[pRoom->environment];
            ++usedSymbols[pRoom->mSymbol];
            if (pRoom->getWeight() > 0) {
                ++usedWeights[pRoom->getWeight()];
            }
            ++usedLockStatus[pRoom->isLocked];
            if (pRoom->isHidden()) {
                ++hiddenRoomCount;
            }
        }

        auto* pDlg = new dlgRoomProperties(mpHost);
        connect(pDlg,
                &dlgRoomProperties::signal_save_symbol,
                this,
                [this](bool changeName,
                       QString newName,
                       bool changeRoomColor,
                       int roomColorNumber,
                       bool changeSymbol,
                       QString newSymbol,
                       bool changeSymbolColor,
                       QColor newSymbolColor,
                       bool changeWeight,
                       int newWeight,
                       bool changeLockStatus,
                       std::optional<bool> newLockStatus,
                       bool changeHiddenStatus,
                       std::optional<bool> newHiddenStatus,
                       bool changeBorderColor,
                       QColor newBorderColor,
                       bool changeBorderThickness,
                       int newBorderThickness,
                       QSet<int> roomIds) {
                    ++mEmitted.emitCount;
                    mEmitted.roomIds = QList<int>(roomIds.cbegin(), roomIds.cend());
                    std::sort(mEmitted.roomIds.begin(), mEmitted.roomIds.end());
                    mEmitted.changeName = changeName;
                    mEmitted.newName = newName;
                    mEmitted.changeRoomColor = changeRoomColor;
                    mEmitted.roomColorNumber = roomColorNumber;
                    mEmitted.changeSymbol = changeSymbol;
                    mEmitted.newSymbol = newSymbol;
                    mEmitted.changeSymbolColor = changeSymbolColor;
                    mEmitted.newSymbolColor = newSymbolColor;
                    mEmitted.changeWeight = changeWeight;
                    mEmitted.newWeight = newWeight;
                    mEmitted.changeLockStatus = changeLockStatus;
                    mEmitted.newLockStatus = newLockStatus;
                    mEmitted.changeHiddenStatus = changeHiddenStatus;
                    mEmitted.newHiddenStatus = newHiddenStatus;
                    mEmitted.changeBorderColor = changeBorderColor;
                    mEmitted.newBorderColor = newBorderColor;
                    mEmitted.changeBorderThickness = changeBorderThickness;
                    mEmitted.newBorderThickness = newBorderThickness;
                });
        pDlg->init(usedNames, usedColors, usedSymbols, usedWeights, usedLockStatus, hiddenRoomCount, rooms);
        mpDialog = pDlg;
        return pDlg;
    }

    struct ColorSelectorOutcome
    {
        bool sawSelector = false;
        bool clickLandedOnAColor = false;
        bool sawMenu = false;
        int colorsLeftInList = -1;
    };
    ColorSelectorOutcome mColorSelectorOutcome;

    // The colour selector and its context menu each run their own event loop,
    // so the right-click is made, and "Delete room color" picked, from timers
    // inside them. A clickedRow of -1 right-clicks the empty space past the colours.
    void rightClickInTheRoomColorSelector(dlgRoomProperties* pDlg, const int currentRow, const int clickedRow)
    {
        using namespace std::chrono_literals;
        mColorSelectorOutcome = ColorSelectorOutcome();
        auto* menuAnswerer = new QTimer(this);
        menuAnswerer->setInterval(20ms);
        connect(menuAnswerer, &QTimer::timeout, this, [this, menuAnswerer]() {
            auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if (!menu) {
                return;
            }
            menuAnswerer->stop();
            mColorSelectorOutcome.sawMenu = true;
            // Its only action, so a translation cannot hide it
            const auto actions = menu->actions();
            if (actions.size() == 1) {
                actions.constFirst()->trigger();
            }
            menu->close();
        });
        auto* selectorDriver = new QTimer(this);
        selectorDriver->setInterval(20ms);
        QElapsedTimer waited;
        waited.start();
        connect(selectorDriver, &QTimer::timeout, this, [this, selectorDriver, menuAnswerer, currentRow, clickedRow, &waited]() {
            auto* selector = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            auto* list = selector ? selector->findChild<QListWidget*>() : nullptr;
            if (!list) {
                // Something else is modal, and the button click would wait on it for ever
                if (waited.hasExpired(10000)) {
                    selectorDriver->stop();
                    if (selector) {
                        selector->reject();
                    }
                }
                return;
            }
            selectorDriver->stop();
            mColorSelectorOutcome.sawSelector = true;
            list->setCurrentRow(currentRow);
            QWidget* viewport = list->viewport();
            const QPoint pos = clickedRow >= 0 ? list->visualItemRect(list->item(clickedRow)).center() : QPoint(viewport->width() - 2, viewport->height() - 2);
            mColorSelectorOutcome.clickLandedOnAColor = list->itemAt(pos);
            menuAnswerer->start();
            QTest::mouseClick(viewport, Qt::RightButton, Qt::NoModifier, pos);
            QContextMenuEvent event(QContextMenuEvent::Mouse, pos, viewport->mapToGlobal(pos));
            QApplication::sendEvent(viewport, &event);
            menuAnswerer->stop();
            mColorSelectorOutcome.colorsLeftInList = list->count();
            selector->reject();
        });
        selectorDriver->start();
        pDlg->pushButton_setRoomColor->click();
        selectorDriver->stop();
        delete selectorDriver;
        delete menuAnswerer;
    }

    // In the grabbed image's pixels, which outnumber the label's on a high-DPI screen
    struct PreviewInk
    {
        QRect contents;
        // Where the symbol was drawn, invalid when nothing of it was
        QRect ink;
    };

    // insideCircle counts only what is inside the circle a round room fills,
    // as outside it a round preview shows the dialog's background
    static PreviewInk symbolInk(QLabel* preview, const bool insideCircle = false)
    {
        const QImage shown = preview->grab().toImage();
        const qreal ratio = shown.devicePixelRatio();
        const QRect logical = preview->contentsRect();
        PreviewInk result;
        result.contents = QRect(qRound(logical.x() * ratio), qRound(logical.y() * ratio), qRound(logical.width() * ratio), qRound(logical.height() * ratio));
        const QPointF center = QRectF(result.contents).center();
        const qreal radius = result.contents.width() / 2.0 - 2 * ratio;
        const QColor background = shown.pixelColor(qRound(center.x()), result.contents.top() + qRound(2 * ratio));
        for (int y = result.contents.top(); y <= result.contents.bottom(); ++y) {
            for (int x = result.contents.left(); x <= result.contents.right(); ++x) {
                if (insideCircle && QLineF(center, QPointF(x + 0.5, y + 0.5)).length() > radius) {
                    continue;
                }
                const QColor pixel = shown.pixelColor(x, y);
                if (std::abs(pixel.red() - background.red()) + std::abs(pixel.green() - background.green()) + std::abs(pixel.blue() - background.blue()) > 48) {
                    result.ink |= QRect(x, y, 1, 1);
                }
            }
        }
        return result;
    }

    static uiDesign::ColourValue* symbolColour(const dlgRoomProperties* pDlg) { return pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesSymbolColour")); }
    static uiDesign::ColourValue* borderColour(const dlgRoomProperties* pDlg) { return pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesBorderColour")); }
    static QSpinBox* borderOpacity(const dlgRoomProperties* pDlg) { return pDlg->findChild<QSpinBox*>(qsl("roomPropertiesBorderOpacity")); }

    // The swatch is the square at the field's leading edge, inset alike from the top and the leading edge, so at the
    // field's own height its middle is half that height in from both
    static QList<QColor> swatchRow(QWidget* pField)
    {
        const QImage image = pField->grab().toImage();
        const qreal ratio = image.devicePixelRatio();
        const int middle = uiDesign::scmInputHeight / 2;
        // Inside the swatch's hairline, which spans the field's height less 2 * (hairline + 3) pixels
        QList<QColor> row;
        for (int x = middle - 8; x <= middle + 8; ++x) {
            row.append(image.pixelColor(QPointF(x * ratio, (pField->height() / 2) * ratio).toPoint()));
        }
        return row;
    }
    static QColor swatchOf(QWidget* pField) { return swatchRow(pField).at(8); }
    // A see-through colour shows the checkerboard's squares through it, which no opaque one does
    static bool showsTheBoardThrough(QWidget* pField)
    {
        const QList<QColor> row = swatchRow(pField);
        return std::any_of(row.cbegin(), row.cend(), [&row](const QColor& pixel) {
            return std::abs(pixel.lightness() - row.constFirst().lightness()) > 24;
        });
    }

    // Types into the field, ending with key: the field takes the code at Return and gives it up at Escape
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

    // The colours window is opened rather than run, so it is among the dialog's children as soon as Choose is clicked
    static bool chooseInTheColoursWindow(dlgRoomProperties* pDlg, QPushButton* pChoose, const QColor& colour)
    {
        QTest::mouseClick(pChoose, Qt::LeftButton);
        auto* pWindow = pDlg->findChild<QColorDialog*>();
        if (!pWindow || !pWindow->isVisible()) {
            return false;
        }
        pWindow->setCurrentColor(colour);
        pWindow->accept();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        return true;
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        // The other end of signal_save_symbol, where the OK lands; these are the
        // two things dlgMapper's constructor sets that the slot needs
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    // Runs after every case, a case QTest cut short at a failed QVERIFY
    // included, so nothing a failure walked away from is left for the leak
    // check to find. The dialog carries WA_DeleteOnClose, so accept()/reject()
    // may already have queued its deletion; run that before deciding there is
    // one left to do.
    void cleanup()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        delete mpDialog.data();
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void oneRoomPrefillsItsOwnValues()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->name = qsl("The Armoury");
        pRoom->mSymbol = qsl("!");
        pRoom->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->lineEdit_name->text(), qsl("The Armoury"));
        QCOMPARE(pDlg->lineEdit_roomSymbol->text(), qsl("!"));
        QVERIFY2(pDlg->comboBox_roomSymbol->isHidden(), "the symbol dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->spinBox_weight->value(), 5);
        QVERIFY2(pDlg->comboBox_weight->isHidden(), "the weight dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_hidden->isTristate());
    }

    void aSelectionThatDisagreesShowsPlaceholdersAndTristates()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        QCOMPARE(pDlg->lineEdit_name->text(), scmMultipleValues);
        QVERIFY2(pDlg->lineEdit_roomSymbol->isHidden(), "the plain symbol field cannot show two different symbols");
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(0), scmMultipleValues);
        QVERIFY2(pDlg->spinBox_weight->isHidden(), "the plain weight field cannot show two different weights");
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(0), scmMultipleValues);
        QVERIFY(pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::PartiallyChecked);
        QVERIFY(pDlg->checkBox_hidden->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::PartiallyChecked);
    }

    // The whole point of the placeholders: a field left showing one has to come
    // back as "no decision", or OK would overwrite every room in the selection
    // with the placeholder text.
    void okOnAnUntouchedMixedSelectionChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        // the set decides which rooms the save applies to, so an emit that
        // changes nothing still has to name the selection it was opened on
        QCOMPARE(mEmitted.roomIds, QList<int>({scmFirstRoom, scmSecondRoom}));
        QVERIFY2(!mEmitted.changeName, "the name placeholder was taken for a new name");
        QVERIFY2(!mEmitted.changeSymbol, "the symbol placeholder was taken for a new symbol");
        QVERIFY2(!mEmitted.changeWeight, "the weight placeholder was taken for a new weight");
        QVERIFY2(!mEmitted.changeLockStatus, "a partially checked lock box was taken for a decision");
        QVERIFY2(!mEmitted.changeHiddenStatus, "a partially checked hidden box was taken for a decision");
        QVERIFY2(!mEmitted.changeRoomColor, "the room colour was reported as changed without the colour selector being used");
        QVERIFY2(!mEmitted.changeSymbolColor, "the symbol colour was reported as changed without the colour selector being used");
    }

    void okReportsTheEditedNameSymbolWeightAndCheckboxes()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmFirstRoom)->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Old Armoury"));
        pDlg->lineEdit_roomSymbol->setText(qsl("A"));
        pDlg->spinBox_weight->setValue(12);
        pDlg->checkBox_locked->setChecked(true);
        pDlg->checkBox_hidden->setChecked(true);
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QCOMPARE(mEmitted.roomIds, QList<int>({scmFirstRoom}));
        QVERIFY(mEmitted.changeName);
        QCOMPARE(mEmitted.newName, qsl("Old Armoury"));
        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("A"));
        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 12);
        QVERIFY(mEmitted.changeLockStatus);
        QCOMPARE(mEmitted.newLockStatus, std::optional<bool>(true));
        QVERIFY(mEmitted.changeHiddenStatus);
        QCOMPARE(mEmitted.newHiddenStatus, std::optional<bool>(true));
    }

    // Cancel has to emit nothing at all, whatever was typed into the form
    void cancelReportsNothing()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Never Applied"));
        pDlg->reject();

        QCOMPARE(mEmitted.emitCount, 0);
    }

    // The dropdown entries carry a usage count for the player's benefit; the
    // count has to be stripped back off before the value is handed on.
    void theWeightDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);
        room(scmThirdRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(1), qsl("7 (count: 2)"));
        QCOMPARE(pDlg->comboBox_weight->itemText(2), qsl("3 (count: 1)"));

        pDlg->comboBox_weight->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 7);
    }

    void theSymbolDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmThirdRoom)->mSymbol = qsl("?");

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(1), qsl("? (count: 2)"));
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(2), qsl("! (count: 1)"));

        pDlg->comboBox_roomSymbol->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("?"));
    }

    // The weight dropdown is editable, so a player can type anything into it
    void anUnreadableTypedWeightIsNotAppliedToTheSelection()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(pDlg->comboBox_weight->lineEdit());
        pDlg->comboBox_weight->setCurrentText(qsl("heavy"));
        pDlg->accept();

        QVERIFY2(!mEmitted.changeWeight, "text that names no number was still applied as a room weight");
    }

    // Each colour is a field showing the room's own colour as a code over a swatch of it, beside glyph buttons that
    // choose it in the colours window and reset it
    void theColourFieldsShowTheRoomsOwnColours()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY2(pSymbol && pBorder && borderOpacity(pDlg), "the dialog has no colour fields, or no opacity for the border");
        QCOMPARE(pSymbol->text(), qsl("#FF00FF"));
        QCOMPARE(pBorder->text(), qsl("#008000"));
        QCOMPARE(swatchOf(pSymbol), QColor(Qt::magenta));
        QCOMPARE(swatchOf(pBorder), QColor(Qt::darkGreen));
        QCOMPARE(borderOpacity(pDlg)->value(), 100);
        QVERIFY2(!pDlg->findChild<QSpinBox*>(qsl("roomPropertiesSymbolOpacity")), "the symbol, which is never see-through, was given an opacity");
        for (QPushButton* pButton : {pDlg->pushButton_setSymbolColor, pDlg->pushButton_setBorderColor, pDlg->pushButton_resetSymbolColor, pDlg->pushButton_resetBorderColor}) {
            QVERIFY2(pButton->text().isEmpty() && !pButton->icon().isNull(), qPrintable(qsl("%1 is not a glyph button: '%2'").arg(pButton->objectName(), pButton->text())));
            QCOMPARE(pButton->size(), QSize(uiDesign::scmInputHeight, uiDesign::scmInputHeight));
        }
        QVERIFY(pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(pDlg->pushButton_resetBorderColor->isEnabled());
    }

    // With no colour of its own the field is empty, says why, and its swatch shows what the room is drawn with
    void withNoColourOfItsOwnTheFieldShowsWhatTheRoomIsDrawnWith()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("=");
        // A light room, so its symbol is drawn in black rather than in the white of the field round the swatch
        const int lightEnvironment = 300;
        map()->mCustomEnvColors[lightEnvironment] = QColor(240, 240, 200);
        const auto forgetEnvironment = qScopeGuard([this, lightEnvironment]() {
            map()->mCustomEnvColors.remove(lightEnvironment);
        });
        room(scmFirstRoom)->environment = lightEnvironment;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY(pSymbol && pBorder);
        QVERIFY2(pSymbol->text().isEmpty() && pBorder->text().isEmpty(), qPrintable(qsl("'%1' and '%2'").arg(pSymbol->text(), pBorder->text())));
        QCOMPARE(pSymbol->placeholderText(), qsl("Automatic"));
        QCOMPARE(pBorder->placeholderText(), qsl("Map default"));
        // The symbol is black or white, whichever stands out against the room
        QCOMPARE(swatchOf(pSymbol), QColor(Qt::black));
        QCOMPARE(pBorder->colour(), mpHost->mRoomBorderColor);
        QCOMPARE(swatchOf(pBorder), QColor(mpHost->mRoomBorderColor.rgb()));
        QCOMPARE(borderOpacity(pDlg)->value(), qRound(mpHost->mRoomBorderColor.alphaF() * 100));
        QVERIFY2(!pDlg->pushButton_resetSymbolColor->isEnabled(), "Reset is offered for a symbol colour the room does not have");
        QVERIFY2(!pDlg->pushButton_resetBorderColor->isEnabled(), "Reset is offered for a border colour the room does not have");
    }

    // Reset is there while the room has a colour to reset, and always for several rooms, as the others may have one
    void resetIsOfferedOnlyWhileThereIsAColourToReset()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QVERIFY(!pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());

        QVERIFY2(chooseInTheColoursWindow(pDlg, pDlg->pushButton_setSymbolColor, QColor(Qt::cyan)), "Choose opened no colours window for the symbol");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#00FFFF"));
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled(), "a chosen symbol colour cannot be reset");
        // The opacity follows a colour the colours window chose
        QVERIFY2(chooseInTheColoursWindow(pDlg, pDlg->pushButton_setBorderColor, QColor(10, 20, 30, 153)), "Choose opened no colours window for the border");
        QCOMPARE(borderColour(pDlg)->text(), qsl("#0A141E"));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
        QVERIFY2(pDlg->pushButton_resetBorderColor->isEnabled(), "a chosen border colour cannot be reset");

        pDlg->pushButton_resetSymbolColor->click();
        pDlg->pushButton_resetBorderColor->click();
        QVERIFY(!pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled() && pDlg->pushButton_resetBorderColor->isEnabled(), "Reset is held back for rooms other than the first, which may carry colours");
    }

    // The field takes a click and the keyboard, and only Choose opens the colours window
    void onlyChooseOpensTheColoursWindow()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QTest::mouseClick(symbolColour(pDlg), Qt::LeftButton);
        QTest::mouseClick(borderColour(pDlg), Qt::LeftButton);
        QCoreApplication::processEvents();
        QVERIFY2(!pDlg->findChild<QColorDialog*>(), "clicking a colour field opened the colours window");

        QTest::mouseClick(pDlg->pushButton_setSymbolColor, Qt::LeftButton);
        auto* pWindow = pDlg->findChild<QColorDialog*>();
        QVERIFY2(pWindow && pWindow->isVisible(), "Choose opened no colours window");
        pWindow->reject();
    }

    // A code typed and taken with Return is a chosen colour, the border's previewed on the room as the colours
    // window's is, and the short form and a missing # are both understood
    void aTypedCodeIsTakenAsAChosenColour()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), qsl("#00ff00"), Qt::Key_Return);
        QVERIFY2(pDlg->isVisible(), "Return in the colour field closed the dialog");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#00FF00"));
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled(), "a typed symbol colour cannot be reset");
        typeInto(borderColour(pDlg), qsl("0f0"), Qt::Key_Return);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#00FF00"));
        QCOMPARE(pRoom->mBorderColor, QColor(0, 255, 0));
        QVERIFY2(pDlg->pushButton_resetBorderColor->isEnabled(), "a typed border colour cannot be reset");

        pDlg->accept();
        QVERIFY(mEmitted.changeSymbolColor);
        QCOMPARE(mEmitted.newSymbolColor, QColor(0, 255, 0));
        QVERIFY(mEmitted.changeBorderColor);
        QCOMPARE(mEmitted.newBorderColor, QColor(0, 255, 0));
    }

    // A code that is not a whole colour when it is taken, and anything left at Escape, puts the colour back
    void anUnfinishedCodeOrEscapePutsTheColourBack()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), qsl("#12"), Qt::Key_Return);
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#FF00FF"));
        typeInto(borderColour(pDlg), qsl("#1234"), Qt::Key_Return);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        typeInto(symbolColour(pDlg), qsl("#123456"), Qt::Key_Escape);
        QVERIFY2(pDlg->isVisible(), "Escape in a field holding a typed code closed the dialog");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#FF00FF"));
        typeInto(borderColour(pDlg), qsl("#123456"), Qt::Key_Escape);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));

        pDlg->accept();
        QVERIFY2(!mEmitted.changeSymbolColor, "a code put back was reported as a change of symbol colour");
        QVERIFY2(!mEmitted.changeBorderColor, "a code put back was reported as a change of border colour");
    }

    // Emptying a field and taking it is what Reset does
    void anEmptiedFieldPutsTheRoomBackToTheDefault()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), QString(), Qt::Key_Return);
        QVERIFY2(symbolColour(pDlg)->showsDefault(), qPrintable(qsl("the emptied symbol field shows '%1' rather than its default").arg(symbolColour(pDlg)->text())));
        QVERIFY2(!pDlg->pushButton_resetSymbolColor->isEnabled(), "Reset is still offered once the field has reset the colour");
        typeInto(borderColour(pDlg), QString(), Qt::Key_Return);
        QVERIFY(borderColour(pDlg)->showsDefault());
        QVERIFY2(!pRoom->mBorderColor.isValid(), "emptying the border field did not reach the room");
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());

        pDlg->accept();
        QVERIFY(mEmitted.changeSymbolColor);
        QVERIFY2(!mEmitted.newSymbolColor.isValid(), "an emptied symbol field has to report an invalid QColor so the default is used");
    }

    // Only the colour is typed: how see-through the border is stays as it was
    void aTypedCodeKeepsTheOpacity()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(200, 40, 80, 153);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
        typeInto(borderColour(pDlg), qsl("#00ff00"), Qt::Key_Return);
        QCOMPARE(pRoom->mBorderColor, QColor(0, 255, 0, 153));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
    }

    // The opacity box sets how see-through the border is, keeping its colour, and the swatch shows the board through it
    void theOpacityBoxSetsHowSeeThroughTheBorderIs()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QVERIFY2(!showsTheBoardThrough(borderColour(pDlg)), "an opaque border's swatch shows the board through it");
        typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
        QVERIFY2(pDlg->isVisible(), "Return in the opacity box closed the dialog");
        QCOMPARE(pRoom->mBorderColor, QColor(0, 128, 0, 153));
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        QVERIFY2(showsTheBoardThrough(borderColour(pDlg)), "a see-through border's swatch does not show the board through it");
        QVERIFY(pDlg->pushButton_resetBorderColor->isEnabled());
    }

    // Rooms with different symbols show no one colour, and a code typed for them goes to every one
    void aCodeTypedForRoomsWithDifferentSymbolsReachesThemAll()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        QVERIFY2(pSymbol->text().isEmpty() && pSymbol->placeholderText().isEmpty() && !pSymbol->colour().isValid(), "rooms that differ were shown a colour");
        typeInto(pSymbol, qsl("#0000ff"), Qt::Key_Return);
        QCOMPARE(pSymbol->text(), qsl("#0000FF"));
        pDlg->accept();

        QCOMPARE(room(scmFirstRoom)->mSymbolColor, QColor(Qt::blue));
        QCOMPARE(room(scmSecondRoom)->mSymbolColor, QColor(Qt::blue));
        QCOMPARE(room(scmFirstRoom)->mSymbol, qsl("!"));
        QCOMPARE(room(scmSecondRoom)->mSymbol, qsl("?"));
    }

    // The border controls preview onto the rooms themselves as they are moved,
    // so that the map behind the dialog shows the result; Cancel has to put the
    // rooms back the way it found them.
    void theBorderPreviewIsLiveAndCancelUndoesIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(pDlg->spinBox_borderThickness->value(), 2);

        pDlg->spinBox_borderThickness->setValue(6);
        QCOMPARE(pRoom->mBorderThickness, 6);
        pDlg->pushButton_resetBorderColor->click();
        QVERIFY2(!pRoom->mBorderColor.isValid(), "resetting the border colour did not reach the room");

        pDlg->reject();

        QCOMPARE(pRoom->mBorderThickness, 2);
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));
    }

    void okKeepsThePreviewedBorderAndReportsIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->spinBox_borderThickness->setValue(6);
        pDlg->accept();

        QCOMPARE(pRoom->mBorderThickness, 6);
        QVERIFY(mEmitted.changeBorderThickness);
        QCOMPARE(mEmitted.newBorderThickness, 6);
        QVERIFY2(!mEmitted.changeBorderColor, "the border colour was reported as changed by a thickness edit");
    }

    // Every room in the selection previews, not just the one the dialog read its
    // starting values from
    void theBorderPreviewReachesEveryRoomInTheSelection()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        int previewCount = 0;
        connect(pDlg, &dlgRoomProperties::signal_preview_border, this, [&previewCount](QSet<int>) {
            ++previewCount;
        });

        pDlg->spinBox_borderThickness->setValue(4);

        QCOMPARE(previewCount, 1);
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 4);
        }

        pDlg->reject();
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 0);
        }
    }

    // Clearing the symbol is how a symbol is removed, and an empty field is a
    // decision rather than a "leave alone"
    void clearingTheSymbolFieldIsReportedAsAChange()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->clear();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QVERIFY(mEmitted.newSymbol.isEmpty());
    }

    void resettingTheSymbolColourIsReportedAsAnInvalidColour()
    {
        buildMap();
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->pushButton_resetSymbolColor->click();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbolColor);
        QVERIFY2(!mEmitted.newSymbolColor.isValid(), "a reset symbol colour has to be an invalid QColor so the default is used");
    }

    // The symbol and its colour are two decisions, and a selection that
    // disagrees about the symbol leaves only the first of them undecided: the
    // colour still has to reach every room, and the placeholder standing in for
    // the symbols must not be written over them (#8394)
    void aSymbolColourDecidedForRoomsWithDifferentSymbolsLeavesTheSymbolsAlone()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);
        room(scmSecondRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        QCOMPARE(pDlg->comboBox_roomSymbol->currentText(), scmMultipleValues);
        pDlg->pushButton_resetSymbolColor->click();
        pDlg->accept();

        QVERIFY2(!mEmitted.changeSymbol, "the symbols disagree, so the dialog has no symbol to report");
        QVERIFY2(mEmitted.changeSymbolColor, "the colour was reset, which is a decision about the colour alone");
        QCOMPARE(room(scmFirstRoom)->mSymbol, qsl("!"));
        QCOMPARE(room(scmSecondRoom)->mSymbol, qsl("?"));
        QVERIFY(!room(scmFirstRoom)->mSymbolColor.isValid());
        QVERIFY(!room(scmSecondRoom)->mSymbolColor.isValid());
    }

    // The dialog is not modal, so a script can delete a room in the selection
    // while it is open; Cancel must only put back the rooms that are still there
    void cancelAfterARoomIsDeletedRestoresOnlyTheRoomsLeft()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->spinBox_borderThickness->setValue(5);
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 5);

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        pDlg->reject();

        QVERIFY(!room(scmFirstRoom));
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 0);
    }

    void okAfterARoomIsDeletedAppliesOnlyToTheRoomsLeft()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));
        pDlg->spinBox_borderThickness->setValue(3);

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        pDlg->accept();

        QVERIFY(!room(scmFirstRoom));
        QCOMPARE(room(scmSecondRoom)->name, qsl("Renamed"));
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 3);
    }

    // deleteMap() and loading another map free every room the dialog was opened on
    void closingTheDialogAfterTheMapIsClearedTouchesNoRoom()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));
        pDlg->spinBox_borderThickness->setValue(3);

        map()->mapClear();
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY(!room(scmFirstRoom));
        QVERIFY(!room(scmSecondRoom));

        buildMap();
        auto* pSecondDlg = openDialogOn({scmFirstRoom});
        pSecondDlg->spinBox_borderThickness->setValue(4);
        map()->mapClear();
        pSecondDlg->reject();

        QVERIFY(!room(scmFirstRoom));
    }

    void previewAndCancelAfterARoomIdIsReusedLeaveTheNewRoomAlone()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        QVERIFY(map()->addRoom(scmFirstRoom));
        QVERIFY(map()->setRoomArea(scmFirstRoom, mAreaId));
        room(scmFirstRoom)->mBorderThickness = 7;

        pDlg->spinBox_borderThickness->setValue(5);
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 5);
        QVERIFY2(room(scmFirstRoom)->mBorderThickness == 7, "the preview reached a new room that reused the id");

        pDlg->reject();
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 0);
        QVERIFY2(room(scmFirstRoom)->mBorderThickness == 7, "Cancel restored the old room's border onto a new room that reused the id");
    }

    void okAfterARoomIdIsReusedLeavesTheNewRoomAlone()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        QVERIFY(map()->addRoom(scmFirstRoom));
        QVERIFY(map()->setRoomArea(scmFirstRoom, mAreaId));
        pDlg->accept();

        QCOMPARE(room(scmSecondRoom)->name, qsl("Renamed"));
        QVERIFY2(room(scmFirstRoom)->name.isEmpty(), "OK renamed a new room that reused the id");
        QCOMPARE(mEmitted.roomIds, QList<int>{scmSecondRoom});
    }

    void rightClickingPastTheColorsOfAnEmptyColorListOffersNothing()
    {
        buildMap();
        map()->mCustomEnvColors.clear();

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, -1, -1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY2(!mColorSelectorOutcome.sawMenu, "there is no color under the cursor to delete");
    }

    void rightClickingPastTheColorsLeavesTheCurrentOneAlone()
    {
        buildMap();
        map()->mCustomEnvColors = {{257, QColor(Qt::red)}, {258, QColor(Qt::blue)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, 0, -1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY2(!mColorSelectorOutcome.clickLandedOnAColor, "the selector is too small to have empty space past its colors");
        QVERIFY2(!mColorSelectorOutcome.sawMenu, "there is no color under the cursor to delete");
        QCOMPARE(mColorSelectorOutcome.colorsLeftInList, 2);
        QCOMPARE(map()->mCustomEnvColors.size(), 2);
    }

    void deletingARoomColorRemovesTheOneRightClicked()
    {
        buildMap();
        map()->mCustomEnvColors = {{257, QColor(Qt::red)}, {258, QColor(Qt::blue)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, 0, 1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY(mColorSelectorOutcome.clickLandedOnAColor);
        QVERIFY(mColorSelectorOutcome.sawMenu);
        QCOMPARE(mColorSelectorOutcome.colorsLeftInList, 1);
        QVERIFY(map()->mCustomEnvColors.contains(257));
        QVERIFY2(!map()->mCustomEnvColors.contains(258), "the right-clicked color was not deleted");
    }

    // The preview is a sample of the room as the map draws it, where a symbol
    // of several characters is shrunk to fit the room rather than cut off
    void aSymbolOfSeveralCharactersFitsInsideThePreview()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));

        const auto [contents, ink] = symbolInk(pDlg->label_preview);
        QVERIFY2(ink.isValid(), "nothing of the symbol was drawn");
        QVERIFY2(ink.left() > contents.left() && ink.right() < contents.right(),
                 qPrintable(qsl("the symbol, drawn across x %1 to %2, is cut off by the preview's edges at %3 and %4").arg(ink.left()).arg(ink.right()).arg(contents.left()).arg(contents.right())));
    }

    // Round rooms draw their symbol in the square inside the circle, smaller
    // than in a square room of the same size
    void aSymbolInARoundRoomIsDrawnSmallerThanInASquareOne()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        // Measured over the same circle as the round room, so only the size differs
        const QRect squareInk = symbolInk(pDlg->label_preview, true).ink;

        mpHost->mBubbleMode = true;
        auto restoreSquareRooms = qScopeGuard([this]() {
            mpHost->mBubbleMode = false;
        });
        pDlg->lineEdit_roomSymbol->clear();
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        const QRect roundInk = symbolInk(pDlg->label_preview, true).ink;

        QVERIFY2(squareInk.isValid() && roundInk.isValid(), "nothing of the symbol was drawn");
        QVERIFY2(roundInk.width() < squareInk.width(), qPrintable(qsl("the symbol is %1 pixels wide in a round room and %2 in a square one").arg(roundInk.width()).arg(squareInk.width())));
    }

    // The map fits a symbol to the room whatever point size the symbol font
    // was given, so the preview must not follow that size either
    void theSymbolFontPointSizeDoesNotChangeThePreview()
    {
        buildMap();
        const QFont originalFont = map()->mMapSymbolFont;
        auto restoreFont = qScopeGuard([this, originalFont]() {
            map()->mMapSymbolFont = originalFont;
        });
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("W"));
        const QImage atOriginalSize = pDlg->label_preview->grab().toImage();
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the symbol was drawn");

        for (const int pointSize : {6, 40}) {
            map()->mMapSymbolFont.setPointSize(pointSize);
            pDlg->lineEdit_roomSymbol->clear();
            pDlg->lineEdit_roomSymbol->setText(qsl("W"));
            QVERIFY2(pDlg->label_preview->grab().toImage() == atOriginalSize, qPrintable(qsl("the preview changed when the symbol font was set to %1pt").arg(pointSize)));
        }
    }

    // Removing the symbol must not leave the last one showing, as OK would then
    // save a room without the symbol the preview shows
    void clearingTheSymbolClearsThePreview()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the symbol was drawn");

        pDlg->lineEdit_roomSymbol->clear();

        QVERIFY2(!symbolInk(pDlg->label_preview).ink.isValid(), "the preview still shows the removed symbol");
        QVERIFY(pDlg->label_preview->pixmap().isNull());
    }

    // The "(Multiple values...)" entry decides no symbol, so it shows none
    void pickingTheMultipleValuesEntryClearsThePreview()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(0), scmMultipleValues);
        pDlg->comboBox_roomSymbol->setCurrentIndex(1);
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the picked symbol was drawn");

        pDlg->comboBox_roomSymbol->setCurrentIndex(0);

        QVERIFY2(!symbolInk(pDlg->label_preview).ink.isValid(), "the preview still shows the previously picked symbol");
        QVERIFY(pDlg->label_preview->pixmap().isNull());
    }

    // With only the symbol font allowed, the map draws a character that font
    // cannot show as the replacement character, and the preview has to too
    void aCharacterTheFontLacksIsPreviewedAsTheReplacementCharacter()
    {
        buildMap();
        map()->setOnlySymbolFontUsed(true);
        auto restoreFontMerging = qScopeGuard([this]() {
            map()->setOnlySymbolFontUsed(false);
        });
        // Unassigned in Unicode, so the symbol font has no glyph for it
        const QChar unassigned(0x0378);
        QVERIFY(!QFontMetrics(map()->mMapSymbolFont).inFontUcs4(unassigned.unicode()));

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(QString(QChar::ReplacementCharacter));
        const QImage replacement = pDlg->label_preview->grab().toImage();
        pDlg->lineEdit_roomSymbol->setText(QString(unassigned));
        const QImage missing = pDlg->label_preview->grab().toImage();

        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing was drawn for the missing character");
        QVERIFY2(missing == replacement, "the missing character was not previewed as the replacement character");
    }
};

#include "RoomPropertiesDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomPropertiesDialogTest)
