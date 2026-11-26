/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022-2023, 2025-2026 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
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


#include "dlgRoomProperties.h"
#include "ColourValue.h"
#include "Host.h"
#include "RoomAppearance.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"
#include "uiDesign.h"
#include "utils.h"
#include "widgetutils.h"

#include <QAbstractTextDocumentLayout>
#include <QColorDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QScopedValueRollback>
#include <QShowEvent>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTabWidgetFrame>
#include <QStyledItemDelegate>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QtMath>

#include <optional>

namespace {
// The map draws from these keys itself, and the dialog edits the border ones elsewhere, so the table neither shows
// nor writes them
const QLatin1String scmMudletOwnedUserDataPrefix{"room.ui_"};

bool editedInUserDataTable(const QString& key)
{
    return !key.startsWith(scmMudletOwnedUserDataPrefix);
}

// What toPlainText() gives, but with non-breaking spaces kept: it turns them into plain ones, and a mapper script
// matching the game's description would then miss the room
QString plainTextKeepingSpaces(const QPlainTextEdit& edit)
{
    QString text = edit.document()->toRawText();
    for (QChar& character : text) {
        switch (character.unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            character = QLatin1Char('\n');
            break;
        default:
            break;
        }
    }
    return text;
}

QString firstLine(const QString& text)
{
    const auto end = std::find_if(text.cbegin(), text.cend(), [](const QChar character) {
        return character == QLatin1Char('\n') || character == QLatin1Char('\r') || character == QChar::LineSeparator || character == QChar::ParagraphSeparator;
    });
    return QString(text.cbegin(), end - text.cbegin());
}

// Set on a value cell that shows the placeholder because the selected rooms do not agree on that key
constexpr int scmMixedValueRole = Qt::UserRole;
// Set for good on a value cell that started out mixed: emptied, even after a value was typed in, it goes back to the
// placeholder rather than writing nothing to every room
constexpr int scmStartedMixedRole = Qt::UserRole + 1;

// The heading names this many rooms of a selection at most; past it, the first few and how many more
constexpr int scmMostRoomIdsNamed = 4;
// The key column opens no wider than this share of the table, so a long key cannot crowd out every value
constexpr qreal scmKeyColumnShare = 0.45;
constexpr int scmValueBoxRestLines = 3;
// What the table keeps of its rows before the value box starts to scroll
constexpr int scmTableFloorRows = 4;
// QAbstractSpinBox::sizeHint() leaves this for the blinking cursor after its widest value
constexpr int scmSpinBoxCursorRoom = 2;

// Holds the Script data tab to the height the Room tab needs. Its words wrap, and QWidgetItem asks a widget whose
// contents wrap for the height they want at a width, size policy or not, so the pane says it has no such height
class ScriptDataPane : public QWidget
{
public:
    using QWidget::QWidget;
    bool hasHeightForWidth() const override { return false; }
};

// A value cell shows one line, the first of its value: the box under the table shows it whole and is where it is
// edited, as a line editor in the cell would flatten one of several lines
class UserDataValueDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void setMixedInk(const QColor& ink) { mMixedInk = ink; }

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->text = firstLine(index.data(Qt::DisplayRole).toString());
        if (index.data(scmMixedValueRole).toBool()) {
            option->font.setItalic(true);
            option->palette.setColor(QPalette::Text, mMixedInk);
        }
    }

private:
    QColor mMixedInk;
};

// Scripts keep a table as JSON text. Whitespace outside strings is all that indenting adds, so taking it out again
// gives back the very text that was stored.
QString jsonWithoutWhitespace(const QString& text)
{
    QString result;
    result.reserve(text.size());
    bool inString = false;
    bool escaped = false;
    for (const QChar character : text) {
        if (inString) {
            result.append(character);
            if (escaped) {
                escaped = false;
            } else if (character == QLatin1Char('\\')) {
                escaped = true;
            } else if (character == QLatin1Char('"')) {
                inString = false;
            }
            continue;
        }
        switch (character.unicode()) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            continue;
        case '"':
            inString = true;
            break;
        default:
            break;
        }
        result.append(character);
    }
    return result;
}

// Never QJsonDocument::toJson(), which sorts the keys and writes the numbers anew: only whitespace is put in, outside
// the strings, so the reader sees the stored text laid out rather than a copy of it
QString jsonIndented(const QString& compact)
{
    const QString step = qsl("  ");
    QString result;
    result.reserve(compact.size() * 2);
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    const auto newLine = [&result, &step](const int indent) {
        result.append(QLatin1Char('\n'));
        result.append(step.repeated(std::max(0, indent)));
    };
    for (qsizetype i = 0; i < compact.size(); ++i) {
        const QChar character = compact.at(i);
        if (inString) {
            result.append(character);
            if (escaped) {
                escaped = false;
            } else if (character == QLatin1Char('\\')) {
                escaped = true;
            } else if (character == QLatin1Char('"')) {
                inString = false;
            }
            continue;
        }
        switch (character.unicode()) {
        case '"':
            inString = true;
            result.append(character);
            break;
        case '{':
        case '[': {
            result.append(character);
            const QChar next = i + 1 < compact.size() ? compact.at(i + 1) : QChar();
            // An empty list or table stays on its line
            if (next == QLatin1Char('}') || next == QLatin1Char(']')) {
                result.append(next);
                ++i;
                break;
            }
            newLine(++depth);
            break;
        }
        case '}':
        case ']':
            newLine(--depth);
            result.append(character);
            break;
        case ',':
            result.append(character);
            newLine(depth);
            break;
        case ':':
            result.append(character);
            result.append(QLatin1Char(' '));
            break;
        default:
            result.append(character);
            break;
        }
    }
    return result;
}

struct JsonShape
{
    bool list = false;
    qsizetype entries = 0;
};

std::optional<JsonShape> jsonShape(const QString& text)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    if (document.isArray()) {
        return JsonShape{true, document.array().size()};
    }
    if (document.isObject()) {
        return JsonShape{false, document.object().size()};
    }
    return std::nullopt;
}

// Only a list or table stored with nothing between its entries is laid out: one a script wrote with whitespace of
// its own, or a word that only looks like JSON, is shown as it is stored
bool shownIndented(const QString& value)
{
    if (!value.startsWith(QLatin1Char('[')) && !value.startsWith(QLatin1Char('{'))) {
        return false;
    }
    return jsonWithoutWhitespace(value) == value && jsonShape(value).has_value();
}

// All of it, so typing replaces it, with the cursor at the start: a field narrower than its words then shows how they
// begin rather than how they end
void selectFromTheEnd(QLineEdit* pEdit)
{
    if (pEdit) {
        pEdit->setSelection(pEdit->text().size(), -pEdit->text().size());
    }
}

// As QAbstractSpinBox::sizeHint() measures a box, but for four digits rather than its widest value
int fourDigitSpinBoxWidth(const QSpinBox* pBox)
{
    pBox->ensurePolished();
    const QFontMetrics metrics(pBox->font());
    int widestDigit = 0;
    for (const QChar digit : qsl("0123456789")) {
        widestDigit = std::max(widestDigit, metrics.horizontalAdvance(digit));
    }
    QStyleOptionSpinBox option;
    option.initFrom(pBox);
    option.buttonSymbols = pBox->buttonSymbols();
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
    option.frame = pBox->hasFrame();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    const QSize contents(4 * widestDigit + metrics.horizontalAdvance(QLatin1Char(' ')) + scmSpinBoxCursorRoom, pBox->sizeHint().height());
    return pBox->style()->sizeFromContents(QStyle::CT_SpinBox, &option, contents, pBox).width();
}
} // namespace


dlgRoomProperties::dlgRoomProperties(Host* pHost, QWidget* pParentWidget)
: QDialog(pParentWidget)
, mpHost(pHost)
{
    // init generated dialog
    setupUi(this);

    mpRoomColour = new uiDesign::ColourValue(tab_general);
    mpRoomColour->setObjectName(qsl("roomPropertiesRoomColour"));
    //: Room properties dialog, screen reader name of the box showing the room's color and the number the map knows that color by
    mpRoomColour->setAccessibleName(tr("Room color"));
    // A room colour is one of the map's, known by its number, so it is chosen from them and never typed
    mpRoomColour->setReadOnly(true);
    mpRoomColour->setFocusPolicy(Qt::NoFocus);
    mpRoomColour->setChooseButton(pushButton_setRoomColor);
    horizontalLayout_roomColor->insertWidget(0, mpRoomColour);
    mpSymbolColour = new uiDesign::ColourValue(widget_symbolColor);
    mpSymbolColour->setObjectName(qsl("roomPropertiesSymbolColour"));
    //: Room properties dialog, screen reader name of the field showing the room symbol's color as a code, which a code can be typed into
    mpSymbolColour->setAccessibleName(tr("Symbol color"));
    //: Room properties dialog, in the field for the room symbol's color while no color was chosen for it: the symbol is then drawn in black or white, whichever stands out against the room
    mpSymbolColour->setDefaultWord(tr("Automatic"));
    horizontalLayout_widget_symbolColor->insertWidget(horizontalLayout_widget_symbolColor->indexOf(pushButton_setSymbolColor), mpSymbolColour);
    mpBorderColour = new uiDesign::ColourValue(widget_borderColor);
    mpBorderColour->setObjectName(qsl("roomPropertiesBorderColour"));
    //: Room properties dialog, screen reader name of the field showing the room border's color as a code, which a code can be typed into
    mpBorderColour->setAccessibleName(tr("Border color"));
    horizontalLayout_widget_borderColor->insertWidget(horizontalLayout_widget_borderColor->indexOf(pushButton_setBorderColor), mpBorderColour);
    horizontalLayout_widget_borderColor->insertWidget(horizontalLayout_widget_borderColor->indexOf(pushButton_setBorderColor), mpBorderColour->opacityBox());
    mpBorderColour->opacityBox()->setObjectName(qsl("roomPropertiesBorderOpacity"));
    mpSymbolColour->setChooseButton(pushButton_setSymbolColor);
    mpBorderColour->setChooseButton(pushButton_setBorderColor);

    //: Room properties dialog, screen reader name of the table on the Script data tab listing the keys and values scripts keep with the room
    tableWidget_userData->setAccessibleName(tr("Script data"));
    //: Room properties dialog, screen reader name of the box under the Script data table that shows the chosen row's whole value and edits it
    roomPropertiesValueBox->setAccessibleName(tr("Value"));
    mpValueDelegate = new UserDataValueDelegate(tableWidget_userData);
    tableWidget_userData->setItemDelegateForColumn(1, mpValueDelegate);
    tableWidget_userData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget_userData->horizontalHeader()->setDefaultAlignment(Qt::AlignLeading | Qt::AlignVCenter);
    // A cell shows one line, so no row grows for a value of several
    tableWidget_userData->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    uiDesign::keepRoomForVerticalScrollBar(tableWidget_userData);
    uiDesign::keepRoomForVerticalScrollBar(roomPropertiesValueBox);
    mpUserDataEmpty = new QLabel(tableWidget_userData->viewport());
    mpUserDataEmpty->setObjectName(qsl("roomPropertiesScriptDataEmpty"));
    // Where a cell's words start, the margin QStyledItemDelegate leaves inside a cell
    mpUserDataEmpty->setIndent(tableWidget_userData->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, tableWidget_userData) + 1);
    mpUserDataEmpty->hide();
    label_hashWarning->hide();
    roomPropertiesJsonWarning->hide();
    label_userDataWarning->hide();

    auto* pScriptDataPane = new ScriptDataPane(tab_userData);
    pScriptDataPane->setObjectName(qsl("roomPropertiesScriptDataPane"));
    pScriptDataPane->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    verticalLayout_userData->replaceWidget(roomPropertiesScriptData, pScriptDataPane);
    auto* pScriptDataPaneLayout = new QVBoxLayout(pScriptDataPane);
    pScriptDataPaneLayout->setContentsMargins(0, 0, 0, 0);
    pScriptDataPaneLayout->addWidget(roomPropertiesScriptData);

    for (QWidget* pScaffolding : {static_cast<QWidget*>(tab_general), static_cast<QWidget*>(tab_userData), static_cast<QWidget*>(pScriptDataPane), static_cast<QWidget*>(roomPropertiesScriptData)}) {
        uiDesign::markAsShellSurface(pScaffolding);
    }
    uiDesign::prepareTabStrip(tabWidget_room->tabBar());
    dialogButtonBox->button(QDialogButtonBox::Ok)->setProperty(uiDesign::scmProp_primaryButton, true);

    // Made after setupUi() set the form's tab order, the colour fields and the opacity box would come after OK and
    // Cancel, so the whole chain goes here: each tab's controls as they stand on screen. The tab widget stays out of
    // it: setTabOrder() takes it for all it holds, and Remove would come after the tab bar
    QList<QWidget*> tabOrder{lineEdit_name,
                             comboBox_area,
                             lineEdit_hash,
                             pushButton_setRoomColor,
                             lineEdit_roomSymbol,
                             comboBox_roomSymbol,
                             mpSymbolColour,
                             pushButton_setSymbolColor,
                             pushButton_resetSymbolColor,
                             mpBorderColour,
                             mpBorderColour->opacityBox(),
                             pushButton_setBorderColor,
                             pushButton_resetBorderColor,
                             spinBox_borderThickness,
                             checkBox_hidden,
                             spinBox_weight,
                             comboBox_weight,
                             checkBox_locked,
                             tableWidget_userData,
                             roomPropertiesValueBox,
                             pushButton_addUserData,
                             pushButton_removeUserData};
    // OK and Cancel trade places between platforms
    uiDesign::collectFocusableInLayoutOrder(dialogButtonBox->layout(), tabOrder);
    for (int i = 1, total = tabOrder.size(); i < total; ++i) {
        setTabOrder(tabOrder.at(i - 1), tabOrder.at(i));
    }
    applyRoomPropertiesShellStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgRoomProperties::applyRoomPropertiesShellStyle);
    // A typed colour goes the way one chosen in the colours window does
    connect(mpSymbolColour, &uiDesign::ColourValue::colourEntered, this, &dlgRoomProperties::slot_symbolColorSelected);
    connect(mpSymbolColour, &uiDesign::ColourValue::defaultEntered, this, &dlgRoomProperties::slot_resetSymbolColor);
    connect(mpBorderColour, &uiDesign::ColourValue::colourEntered, this, &dlgRoomProperties::slot_borderColorSelected);
    connect(mpBorderColour, &uiDesign::ColourValue::opacityEntered, this, &dlgRoomProperties::slot_borderOpacitySelected);
    connect(mpBorderColour, &uiDesign::ColourValue::defaultEntered, this, &dlgRoomProperties::slot_resetBorderColor);

    connect(lineEdit_roomSymbol, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(comboBox_roomSymbol, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_symbolComboBoxItemChanged);
    connect(comboBox_weight, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgRoomProperties::slot_weightComboBoxItemChanged);
    connect(comboBox_roomSymbol, &QComboBox::currentTextChanged, this, &dlgRoomProperties::slot_updatePreview);
    connect(pushButton_setSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openSymbolColorSelector);
    connect(pushButton_resetSymbolColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetSymbolColor);
    connect(pushButton_setRoomColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openRoomColorSelector);
    connect(pushButton_setBorderColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_openBorderColorSelector);
    connect(pushButton_resetBorderColor, &QAbstractButton::released, this, &dlgRoomProperties::slot_resetBorderColor);
    connect(spinBox_borderThickness, qOverload<int>(&QSpinBox::valueChanged), this, &dlgRoomProperties::slot_borderThicknessChanged);
    connect(pushButton_addUserData, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_addUserData);
    connect(pushButton_removeUserData, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_removeUserData);
    connect(tableWidget_userData, &QTableWidget::itemChanged, this, &dlgRoomProperties::slot_userDataItemChanged);
    // A row removed above the chosen one moves it without a change of row; slot_removeUserData() shows it again itself
    connect(tableWidget_userData, &QTableWidget::currentCellChanged, this, [this](const int currentRow, int, const int previousRow) {
        // The focus coming in from the keyboard makes the first row current without selecting it, and the row whose
        // value the box shows has to look chosen
        if (currentRow >= 0 && !tableWidget_userData->selectionModel()->hasSelection()) {
            tableWidget_userData->selectionModel()->select(tableWidget_userData->model()->index(currentRow, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        if (currentRow != previousRow) {
            showChosenValue();
        }
    });
    // A key typed into an added row is the one cell edited in place; elsewhere a double-click means the value
    connect(tableWidget_userData, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.flags().testFlag(Qt::ItemIsEditable)) {
            focusValueBox();
        }
    });
    // After the view's own handling, which moves to the value cell and takes the focus back from the closed editor
    connect(tableWidget_userData->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [this](QWidget*, const QAbstractItemDelegate::EndEditHint hint) {
        if (hint == QAbstractItemDelegate::EditNextItem) {
            focusValueBox();
        }
    });
    connect(roomPropertiesValueBox, &QPlainTextEdit::textChanged, this, &dlgRoomProperties::slot_valueBoxEdited);
    connect(lineEdit_hash, &QLineEdit::textChanged, this, &dlgRoomProperties::slot_hashChanged);
    tableWidget_userData->installEventFilter(this);
    tableWidget_userData->viewport()->installEventFilter(this);
    roomPropertiesScriptData->installEventFilter(this);

    setAttribute(Qt::WA_DeleteOnClose);
}

dlgRoomProperties::~dlgRoomProperties()
{
    // ~QDialog hides the dialog once this destructor is done, and a colour field or the opacity box still holding an
    // edit takes it on losing the focus, emitting into these slots when this object is no longer a valid receiver
    widgetutils::disconnectChildSignals(this);
}

void dlgRoomProperties::init(
        QHash<QString, int> usedNames, QHash<int, int>& pColors, QHash<QString, int>& pSymbols, QHash<int, int>& pWeights, QHash<bool, int> lockStatus, int hiddenRoomCount, const QSet<int>& roomIds)
{
    // Rooms with different names show none: the field left empty renames nothing
    mNamesDiffer = usedNames.size() > 1;
    if (mNamesDiffer) {
        lineEdit_name->clear();
        lineEdit_name->setPlaceholderText(multipleValuesPlaceholder);
    } else if (usedNames.size() == 1) {
        lineEdit_name->setText(usedNames.constBegin().key());
        lineEdit_name->selectAll();
    } else {
        lineEdit_name->clear();
    }


    // Configure symbols display
    mpSymbols = pSymbols;
    mRoomIds = roomIds;

    // Store original border values for live preview restoration on cancel
    TRoom* pFirstRoom = nullptr;
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomId);
        if (!room) {
            continue;
        }
        if (!pFirstRoom) {
            pFirstRoom = room;
        }
        mRoomSerials[roomId] = room->serial();
        mOriginalBorderColors[roomId] = room->mBorderColor;
        mOriginalBorderThicknesses[roomId] = room->mBorderThickness;
    }

    if (mpSymbols.isEmpty()) {
        // show simple text-entry box empty
        lineEdit_roomSymbol->setText(QString());
        comboBox_roomSymbol->hide();
    } else if (mpSymbols.size() == 1) {
        // show simple text-entry box with the (single) existing symbol pre-filled
        lineEdit_roomSymbol->setText(mpSymbols.constBegin().key());
        // and selected
        lineEdit_roomSymbol->selectAll();
        comboBox_roomSymbol->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing symbols
        lineEdit_roomSymbol->hide();
        comboBox_roomSymbol->addItems(getComboBoxSymbolItems());
        selectFromTheEnd(comboBox_roomSymbol->lineEdit());
    }
    auto mapSymbolFont = mpHost->mpMap->mMapSymbolFont;
    mapSymbolFont.setPointSize(qApp->font().pointSize());
    if (!lineEdit_roomSymbol->isHidden()) {
        lineEdit_roomSymbol->setFont(mapSymbolFont);
    }
    if (!comboBox_roomSymbol->isHidden()) {
        comboBox_roomSymbol->setFont(mapSymbolFont);
    }

    // Configure icon display
    if (pFirstRoom) {
        selectedSymbolColor = pFirstRoom->mSymbolColor;
    }
    mRoomColorsDiffer = !pFirstRoom || pColors.size() != 1;
    if (!mRoomColorsDiffer) {
        mRoomColor = mpHost->mpMap->getColor(pFirstRoom->getId());
        mRoomColorNumber = pColors.constBegin().key();
    } else {
        mRoomColor = QColor(128, 128, 128); // theme-fixed: the preview's stand-in for rooms of different colors
    }
    showRoomColour();
    slot_updatePreview();

    //   Configure weight display
    mpWeights = pWeights;
    if (mpWeights.isEmpty()) {
        // show spin-box with default value
        spinBox_weight->setValue(1);
        spinBox_weight->selectAll();
        comboBox_weight->hide();
    } else if (mpWeights.size() == 1) {
        // show spin-box with the (single) existing weight pre-filled
        spinBox_weight->setValue(mpWeights.constBegin().key());
        spinBox_weight->selectAll();
        comboBox_weight->hide();
    } else {
        // show combined dropdown & text-entry box to host all of the (multiple) existing weights
        spinBox_weight->hide();
        comboBox_weight->addItems(getComboBoxWeightItems());
        selectFromTheEnd(comboBox_weight->lineEdit());
    }

    // Configure lock display
    // Are all locks the same or mixed status? Then show dialog in tristate.
    if (lockStatus.contains(true) && lockStatus.contains(false)) {
        checkBox_locked->setTristate(true);
        checkBox_locked->setCheckState(Qt::PartiallyChecked);
    } else if (lockStatus.contains(true)) {
        checkBox_locked->setTristate(false);
        checkBox_locked->setCheckState(Qt::Checked);
    } else { // lockStatus.contains(false)
        checkBox_locked->setTristate(false);
        checkBox_locked->setCheckState(Qt::Unchecked);
    }

    // Configure hidden display
    if (hiddenRoomCount && hiddenRoomCount < mRoomIds.size()) {
        // Some rooms are hidden
        checkBox_hidden->setTristate(true);
        checkBox_hidden->setCheckState(Qt::PartiallyChecked);
    } else {
        // Either none or all of them are hidden
        checkBox_hidden->setTristate(false);
        checkBox_hidden->setChecked(hiddenRoomCount == mRoomIds.size());
    }
    initLockAndHiddenWords();

    // Configure border display
    selectedBorderColor = pFirstRoom->mBorderColor;
    mBorderThickness = pFirstRoom->mBorderThickness;
    showBorderColour();

    // Set thickness spinbox
    spinBox_borderThickness->setValue(mBorderThickness);

    initGeneral();
    initUserData();
    alignValueBoxes();
    keepTheTabsRoomForThePages();

    // The name is what is most often changed: typed straight away, Enter renames. Set here, as the buttons not in the
    // tab order would otherwise be the first to take focus
    lineEdit_name->setFocus(Qt::OtherFocusReason);

    // Configure dialog display
    adjustSize();
}


void dlgRoomProperties::initLockAndHiddenWords()
{
    checkBox_locked->setText(mRoomIds.size() > 1
                                     //: Check box in the Configure room window for several rooms at once: a locked room is never used by a path or a speedwalk, neither as its end nor on the way
                                     ? tr("Locked: no path goes to or through these rooms")
                                     //: Check box in the Configure room window: a locked room is never used by a path or a speedwalk, neither as its end nor on the way
                                     : tr("Locked: no path goes to or through this room"));

    /*: Tooltip to give additional information for the checkbox to control the
 state of being hidden when the selection includes multiple rooms and they
 are not all in the same state.*/
    const QString leaveAsTheyAre = utils::richText(tr("Leave as partially checked to not change the state of the selected rooms."));
    checkBox_hidden->setToolTip(checkBox_hidden->isTristate() ? leaveAsTheyAre : QString());
    checkBox_locked->setToolTip(checkBox_locked->isTristate() ? leaveAsTheyAre : QString());
}


QStringList dlgRoomProperties::getComboBoxSymbolItems()
{
    // Obtain a set of "used" values
    QHashIterator<QString, int> itSymbolUsed(mpSymbols);
    QSet<int> symbolCountsSet;
    while (itSymbolUsed.hasNext()) {
        itSymbolUsed.next();
        symbolCountsSet.insert(itSymbolUsed.value());
    }

    // Obtains a list of those values sorted in ascending count of use
    QList<int> symbolCountsList{symbolCountsSet.begin(), symbolCountsSet.end()};
    if (symbolCountsList.size() > 1) {
        std::sort(symbolCountsList.begin(), symbolCountsList.end());
    }

    // Build a list of strings for display in descending count of use
    QStringList displayStrings;
    displayStrings.append(multipleValuesPlaceholder);
    for (int i = symbolCountsList.size() - 1; i >= 0; --i) {
        itSymbolUsed.toFront();
        while (itSymbolUsed.hasNext()) {
            itSymbolUsed.next();
            if (itSymbolUsed.value() == symbolCountsList.at(i)) {
                /*: Format for showing a room symbol with its usage count. %1 is the symbol itself (e.g., "★" or "!"),
%2 is the number of rooms using this symbol. Example output: "★ (count: 5)" or "! (count: 12)".
The word "count" and the format can be translated, but ensure the numbers remain clearly associated.*/
                displayStrings.append(tr("%1 (count: %2)").arg(itSymbolUsed.key(), QString::number(itSymbolUsed.value())));
            }
        }
    }
    return displayStrings;
}


QStringList dlgRoomProperties::getComboBoxWeightItems()
{
    // Obtain a set of "used" values
    QHashIterator<int, int> itWeightUsed(mpWeights);
    QSet<int> weightCountsSet;
    while (itWeightUsed.hasNext()) {
        itWeightUsed.next();
        weightCountsSet.insert(itWeightUsed.value());
    }

    // Obtains a list of those values sorted in ascending count of use
    QList<int> weightCountsList{weightCountsSet.begin(), weightCountsSet.end()};
    if (weightCountsList.size() > 1) {
        std::sort(weightCountsList.begin(), weightCountsList.end());
    }

    // Build a list of strings for display in descending count of use
    QStringList displayStrings;
    displayStrings.append(multipleValuesPlaceholder);
    for (int i = weightCountsList.size() - 1; i >= 0; --i) {
        itWeightUsed.toFront();
        while (itWeightUsed.hasNext()) {
            itWeightUsed.next();
            if (itWeightUsed.value() == weightCountsList.at(i)) {
                /*: Format for showing a room weight with its usage count. %1 is the weight value (e.g., "1" or "50"),
 %2 is the number of rooms with this weight. Example output: "5 (count: 3)" or "100 (count: 7)".
 The word "count" and the format can be translated, but ensure the numbers remain clearly associated.*/
                displayStrings.append(tr("%1 (count: %2)").arg(QString::number(itWeightUsed.key()), QString::number(itWeightUsed.value())));
            }
        }
    }
    return displayStrings;
}


void dlgRoomProperties::accept()
{
    // Leaving the cell commits a key still being typed in it: a button on macOS takes no focus when clicked, so the
    // editor would not have lost it, and what was typed would go unsaved
    tableWidget_userData->setCurrentIndex(QModelIndex());
    QDialog::accept();

    // Left empty over rooms with different names, the field decides nothing
    const QString newName = lineEdit_name->text();
    const bool changeName = !mNamesDiffer || !newName.isEmpty();

    // mChangeRoomColor and mRoomColorNumber were already set when the color selector closed with OK

    // Find symbol to return back
    const QString newSymbol = getNewSymbol();
    bool changeSymbol = true;
    const QColor newSymbolColor = selectedSymbolColor;
    bool changeSymbolColor = mSymbolColorWasChanged;
    if (newSymbol == multipleValuesPlaceholder) {
        // We don't want to change then
        changeSymbol = false;
    }

    // Find weight to return back
    const int newWeight = getNewWeight();
    bool changeWeight = true;
    if (newWeight <= -1) {
        // We don't want to change then
        changeWeight = false;
    }

    // Find lock status to return back
    Qt::CheckState const newCheckState = checkBox_locked->checkState();
    bool changeLockStatus = true;
    std::optional<bool> newLockStatus;
    if (newCheckState == Qt::PartiallyChecked) {
        // We don't want to change then
        changeLockStatus = false;
    } else {
        if (newCheckState == Qt::Checked) {
            newLockStatus = true;
        } else { // Qt::Unchecked
            newLockStatus = false;
        }
    }

    // Find hidden status to return back
    Qt::CheckState const newHiddenCheckState = checkBox_hidden->checkState();
    bool changeHiddenStatus = true;
    std::optional<bool> newHiddenStatus;
    if (newHiddenCheckState == Qt::PartiallyChecked) {
        // We don't want to change then
        changeHiddenStatus = false;
    } else {
        if (newHiddenCheckState == Qt::Checked) {
            newHiddenStatus = true;
        } else { // Qt::Unchecked
            newHiddenStatus = false;
        }
    }

    // Find border settings to return back. An opacity set alone for rooms that differ in colour has no one colour to
    // go with the save, and the preview has already given it to each room with that room's own colour.
    bool changeBorderColor = mBorderColorWasChanged;
    QColor newBorderColor = selectedBorderColor;
    bool changeBorderThickness = mBorderThicknessWasChanged;
    int newBorderThickness = mBorderThickness;

    emit signal_save_symbol(changeName,
                            newName,
                            mChangeRoomColor,
                            mRoomColorNumber,
                            changeSymbol,
                            newSymbol,
                            changeSymbolColor,
                            newSymbolColor,
                            changeWeight,
                            newWeight,
                            changeLockStatus,
                            newLockStatus,
                            changeHiddenStatus,
                            newHiddenStatus,
                            changeBorderColor,
                            newBorderColor,
                            changeBorderThickness,
                            newBorderThickness,
                            liveRoomIds());

    const QMap<QString, std::optional<QString>> changes = userDataChanges();
    if (!changes.isEmpty()) {
        emit signal_save_userData(changes, liveRoomIds());
    }

    const QSet<int> rooms = liveRoomIds();
    if (rooms.size() == 1 && lineEdit_hash->text() != mOriginalHash) {
        emit signal_save_hash(*rooms.cbegin(), lineEdit_hash->text());
    }

    // Last, as the map then shows the area the rooms went to
    const QVariant areaId = comboBox_area->currentData();
    if (areaId.isValid() && areaId.toInt() != mOriginalAreaId && !rooms.isEmpty()) {
        emit signal_save_area(areaId.toInt(), rooms);
    }
}


QString dlgRoomProperties::getNewSymbol()
{
    if (mpSymbols.size() <= 1) {
        return lineEdit_roomSymbol->text();
    }
    QString newSymbolText = comboBox_roomSymbol->currentText();
    // Parse the initial text before the parentheses containing count
    const QRegularExpression countStripper(qsl("^(.*) \\(.*\\)$"));
    const QRegularExpressionMatch match = countStripper.match(newSymbolText);
    if (match.hasMatch() && match.lastCapturedIndex() > 0) {
        return match.captured(1);
    }
    return newSymbolText;
}


int dlgRoomProperties::getNewWeight()
{
    if (mpWeights.size() <= 1) {
        return spinBox_weight->value();
    }
    const QString newWeightText = comboBox_weight->currentText();
    if (newWeightText == multipleValuesPlaceholder) {
        return -1; // User did not want to select any weight, so we will do no change
    }
    // Parse an initial number out of what was selected or typed
    const QRegularExpression countStripper(qsl("^\\s*(\\d+)"));
    const QRegularExpressionMatch match = countStripper.match(newWeightText);
    if (match.hasMatch() && match.lastCapturedIndex() > 0) {
        return match.captured(1).toInt();
    }
    if (newWeightText.toInt() > 0) {
        return newWeightText.toInt();
    }
    return -2; // Maybe some other input we did not understand, so we will do no change
}


void dlgRoomProperties::slot_updatePreview()
{
    auto realSymbolColor = selectedSymbolColor.isValid() ? selectedSymbolColor : defaultSymbolColor();
    QString newSymbol = getNewSymbol();
    const bool symbolsDiffer = newSymbol == multipleValuesPlaceholder;
    if (symbolsDiffer) {
        newSymbol = QString();
    }
    // Rooms with different symbols show no one colour until one is decided for all of them
    if (symbolsDiffer && !mSymbolColorWasChanged) {
        mpSymbolColour->clear();
    } else if (selectedSymbolColor.isValid()) {
        mpSymbolColour->setColour(selectedSymbolColor);
    } else {
        mpSymbolColour->setDefault(realSymbolColor);
    }
    pushButton_resetSymbolColor->setEnabled(resetOffered(selectedSymbolColor));
    const bool roundRoom = mpHost->mBubbleMode;
    // theme-fixed: the room's own color and the map's room border, drawn as the map draws them
    label_preview->setStyleSheet(qsl("background-color: %1; border: %2; border-radius: %3px;")
                                         .arg(mRoomColor.name(),
                                              mpHost->mMapperShowRoomBorders ? qsl("1px solid %1").arg(mpHost->mRoomBorderColor.name()) : qsl("none"),
                                              QString::number(roundRoom ? label_preview->width() / 2 : 0)));
    label_preview->setAccessibleName(newSymbol);
    if (newSymbol.isEmpty()) {
        label_preview->clear();
        return;
    }
    // Drawn as the map draws it, which shrinks a symbol to fit the room, and
    // a round room's to fit the square inside its circle
    const QSize size = label_preview->contentsRect().size();
    QRect symbolRect(QPoint(), roundRoom ? size * 0.707 : size);
    symbolRect.moveCenter(QRect(QPoint(), size).center());
    const qreal ratio = label_preview->devicePixelRatioF();
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    ushort fontSize = 1;
    RoomAppearance::paintSymbol(painter, symbolRect, newSymbol, realSymbolColor, mpHost->mpMap->mMapSymbolFont, mpHost->mpMap->mMapSymbolFontFudgeFactor, fontSize);
    painter.end();
    label_preview->setPixmap(pixmap);
}


void dlgRoomProperties::slot_openSymbolColorSelector()
{
    auto* dialog = new QColorDialog(selectedSymbolColor.isValid() ? selectedSymbolColor : defaultSymbolColor(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Set symbol color"));
    dialog->open(this, SLOT(slot_symbolColorSelected(const QColor&)));
}


void dlgRoomProperties::slot_symbolColorSelected(const QColor& color)
{
    selectedSymbolColor = color;
    mSymbolColorWasChanged = true;
    slot_updatePreview();
}


void dlgRoomProperties::slot_resetSymbolColor()
{
    selectedSymbolColor = QColor();
    mSymbolColorWasChanged = true;
    slot_updatePreview();
}

// Sheets go on the shell, not the dialog: a profile's Lua stylesheet may be given to the dialog
void dlgRoomProperties::applyRoomPropertiesShellStyle()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QString hairline = QString::number(uiDesign::scmInputBorderWidth);
    const QString captionSize = QString::number(uiDesign::typeSize(uiDesign::TypeStep::Caption));
    const QColor warningInk = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_warning, tokens.darkPage), tokens.text, uiDesign::scmTextMinimumRatio);
    const QString table = qsl("QTableWidget#tableWidget_userData");

    const QStringList headings{qsl("#label_roomIdValue"), qsl("#roomPropertiesMapHeadingText"), qsl("#roomPropertiesPathHeadingText")};
    const QStringList headingRows{qsl("#roomPropertiesRoomHeading"), qsl("#roomPropertiesMapHeading"), qsl("#roomPropertiesPathHeading")};
    const QStringList labels{qsl("#label_name"),
                             qsl("#label_area"),
                             qsl("#label_hash"),
                             qsl("#roomPropertiesRoomColorLabel"),
                             qsl("#roomPropertiesSymbolLabel"),
                             qsl("#label_borderColor"),
                             qsl("#label_borderThickness"),
                             qsl("#roomPropertiesWeightLabel"),
                             qsl("#roomPropertiesValueLabel")};
    const QStringList captions{qsl("#roomPropertiesHashCaption"), qsl("#roomPropertiesWeightCaption"), qsl("#label_userDataInstructions")};
    const QStringList warnings{qsl("#label_hashWarning"), qsl("#roomPropertiesJsonWarning"), qsl("#label_userDataWarning")};

    // The input recipe stops at the value box on the Script data tab: further down, it would draw the key typed into
    // an added row as a field taller than the row it is typed in
    const QString rules =
            qsl("#roomPropertiesShell { background-color: %1; }").arg(tokens.page.name()) + qsl("QWidget[settingsSurface=\"true\"] { background-color: transparent; border: none; }")
            + uiDesign::tabBarStyleSheet(qsl("#tabWidget_room"), tokens) + uiDesign::inputStyleSheet(tokens, qsl("#tab_general"))
            + uiDesign::inputStyleSheet(tokens, qsl("#roomPropertiesScriptData >")) + uiDesign::choiceStyleSheet(tokens, qsl("#tab_general"))
            + uiDesign::buttonStyleSheet(tokens, qsl("#roomPropertiesShell")) + headings.join(qsl(", "))
            + qsl(" { color: %1; background: transparent; font-size: %2pt; font-weight: 600; }").arg(tokens.mutedText.name(), captionSize) + headingRows.join(qsl(", "))
            + qsl(" { border-bottom: %1px solid %2; }").arg(hairline, tokens.separator.name()) + labels.join(qsl(", ")) + qsl(" { color: %1; background: transparent; }").arg(tokens.mutedText.name())
            + captions.join(qsl(", ")) + qsl(" { color: %1; background: transparent; font-size: %2pt; }").arg(tokens.mutedText.name(), captionSize) + warnings.join(qsl(", "))
            + qsl(" { color: %1; background: transparent; font-size: %2pt; }").arg(warningInk.name(), captionSize)
            // The table has no recipe of its own: it takes the field's surface, hairline and corner, its header the
            // chrome's words, and its chosen row the accent's wash as a chosen row of a list does
            + table
            + qsl(" { background-color: %1; color: %2; border: %3px solid %4; border-radius: %5px; selection-background-color: %6; selection-color: %7; outline: none; }")
                      .arg(tokens.field.name(), tokens.text.name(), hairline, tokens.border.name(), QString::number(uiDesign::scmRadiusInput), tokens.accentSoft, tokens.accentText.name())
            + table + qsl(" QHeaderView { background: transparent; border: none; color: %1; }").arg(tokens.mutedText.name()) + table
            + qsl(" QHeaderView::section { background: transparent; color: %1; border: none; border-bottom: %2px solid %3; padding: %4px %5px; font-size: %6pt; }")
                      .arg(tokens.mutedText.name(),
                           hairline,
                           tokens.separator.name(),
                           QString::number(uiDesign::scmInputPaddingVertical),
                           QString::number(uiDesign::scmInputPaddingHorizontal),
                           captionSize)
            + table
            + qsl(" QHeaderView::section:first { border-right: %1px solid %2; }").arg(hairline, tokens.separator.name())
            // A label, not a placeholder, so held to the text contrast floor
            + qsl("#roomPropertiesScriptDataEmpty { color: %1; background: transparent; }")
                      .arg(uiDesign::readableOn(tokens.field, tokens.disabledText, tokens.text, uiDesign::scmTextMinimumRatio).name())
            + uiDesign::scrollBarStyleSheet(table, tokens, tokens.field) + uiDesign::scrollBarStyleSheet(qsl("#roomPropertiesValueBox"), tokens, tokens.field);
    roomPropertiesShell->setStyleSheet(rules);

    static_cast<UserDataValueDelegate*>(mpValueDelegate)->setMixedInk(uiDesign::readableOn(tokens.field, tokens.disabledText, tokens.text, uiDesign::scmQuietMinimumRatio));
    tableWidget_userData->viewport()->update();

    uiDesign::setColourButtonGlyph(pushButton_setRoomColor, uiDesign::ColourButton::Choose, tokens);
    for (QPushButton* pChoose : {pushButton_setSymbolColor, pushButton_setBorderColor}) {
        uiDesign::setColourButtonGlyph(pChoose, uiDesign::ColourButton::Choose, tokens);
    }
    for (QPushButton* pReset : {pushButton_resetSymbolColor, pushButton_resetBorderColor}) {
        uiDesign::setColourButtonGlyph(pReset, uiDesign::ColourButton::Reset, tokens);
    }

    // The base style measures a spin box and a combo box taller than a line of text, so each is held to the field's height
    for (QWidget* pControl : {static_cast<QWidget*>(lineEdit_name),
                              static_cast<QWidget*>(comboBox_area),
                              static_cast<QWidget*>(lineEdit_hash),
                              static_cast<QWidget*>(lineEdit_roomSymbol),
                              static_cast<QWidget*>(comboBox_roomSymbol),
                              static_cast<QWidget*>(spinBox_borderThickness),
                              static_cast<QWidget*>(spinBox_weight),
                              static_cast<QWidget*>(comboBox_weight)}) {
        uiDesign::holdHeight(pControl, uiDesign::scmInputHeight);
    }

    // A styled spin box starts its words at its frame, where a line field insets them, so they would not stand in a column
    for (const QSpinBox* pBox : {spinBox_borderThickness, spinBox_weight}) {
        if (auto* pEdit = pBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly)) {
            const bool rightToLeft = pEdit->layoutDirection() == Qt::RightToLeft;
            pEdit->setTextMargins(rightToLeft ? 0 : uiDesign::scmInputPaddingHorizontal, 0, rightToLeft ? uiDesign::scmInputPaddingHorizontal : 0, 0);
        }
    }

    uiDesign::keepClickFocusOffControls(roomPropertiesShell);
    uiDesign::letPopupsTakeTheFieldsCorner(roomPropertiesShell);
    spaceTheSections();
    alignValueBoxes();
    keepTheTabsRoomForThePages();
    fitScriptData();
}

void dlgRoomProperties::spaceTheSections()
{
    const int line = fontMetrics().height();
    // The hairline under a heading stands a little below its words, and a section stands apart from the one above
    horizontalLayout_roomHeading->setContentsMargins(0, 0, 0, line / 4);
    horizontalLayout_mapHeading->setContentsMargins(0, line / 2, 0, line / 4);
    horizontalLayout_pathHeading->setContentsMargins(0, line / 2, 0, line / 4);
    // Set rather than left to the style, which on macOS answers per pair of controls and so cannot be added up
    verticalLayout_scriptData->setSpacing(line / 2);
}

// The tab strip's sheet holds a page scmTabPaneInset in from every side of the pane, and QTabWidget adds only the base
// style's own frame to a page's size when it asks for its own, so the Room tab would come out short of what it needs
void dlgRoomProperties::keepTheTabsRoomForThePages()
{
    tabWidget_room->setMinimumSize(0, 0);
    QStyleOptionTabWidgetFrame option;
    option.initFrom(tabWidget_room);
    const QSize frame = tabWidget_room->style()->sizeFromContents(QStyle::CT_TabWidget, &option, QSize(0, 0), tabWidget_room);
    const QSize shortfall = (QSize(2 * uiDesign::scmTabPaneInset, 2 * uiDesign::scmTabPaneInset) - frame).expandedTo(QSize(0, 0));
    tabWidget_room->setMinimumSize(tabWidget_room->sizeHint() + shortfall);
}

void dlgRoomProperties::alignValueBoxes()
{
    int width = spinBox_borderThickness->sizeHint().width();
    for (const uiDesign::ColourValue* pColour : {mpRoomColour, mpSymbolColour, mpBorderColour}) {
        width = std::max(width, pColour->sizeHint().width());
    }
    for (QWidget* pBox : {static_cast<QWidget*>(mpRoomColour),
                          static_cast<QWidget*>(lineEdit_roomSymbol),
                          static_cast<QWidget*>(comboBox_roomSymbol),
                          static_cast<QWidget*>(mpSymbolColour),
                          static_cast<QWidget*>(mpBorderColour),
                          static_cast<QWidget*>(spinBox_borderThickness)}) {
        pBox->setFixedWidth(width);
    }
    // A weight is rarely past four digits, and one that is still shows, scrolled
    spinBox_weight->setFixedWidth(fourDigitSpinBoxWidth(spinBox_weight));
}

// The field shows the first room's colour, and the other rooms of a selection may carry colours of their own
bool dlgRoomProperties::resetOffered(const QColor& chosen) const
{
    return mRoomIds.size() > 1 || chosen.isValid();
}

bool dlgRoomProperties::borderColoursDiffer() const
{
    const QList<QColor> colours = mOriginalBorderColors.values();
    return std::any_of(colours.cbegin(), colours.cend(), [&colours](const QColor& colour) {
        return colour != colours.constFirst();
    });
}

void dlgRoomProperties::showBorderColour()
{
    // A room with no border colour of its own is drawn in the map's, so the field shows that code, not a word
    mpBorderColour->setDefaultWord(mpHost->mRoomBorderColor.name(QColor::HexRgb).toUpper());
    if (selectedBorderColor.isValid()) {
        mpBorderColour->setColour(selectedBorderColor);
    } else {
        mpBorderColour->setDefault(mpHost->mRoomBorderColor);
    }
    pushButton_resetBorderColor->setEnabled(resetOffered(selectedBorderColor));
}

// The number is what the map and scripts know the colour by (setRoomEnv), so it stands where a colour code would
void dlgRoomProperties::showRoomColour()
{
    if (mRoomColorsDiffer && !mChangeRoomColor) {
        //: Room properties dialog, in the box for the room color when the chosen rooms have different colors
        mpRoomColour->setDefaultWord(tr("Multiple"));
        mpRoomColour->setDefault(QColor());
        return;
    }
    mpRoomColour->setDefaultWord(QString::number(mRoomColorNumber));
    mpRoomColour->setDefault(mRoomColor);
}

QColor dlgRoomProperties::backgroundBasedColor(QColor background)
{
    return background.lightness() > 127 ? Qt::black : Qt::white; // theme-fixed: the symbol color the map picks against the room
}

QColor dlgRoomProperties::defaultSymbolColor()
{
    return backgroundBasedColor(mRoomColor);
}


void dlgRoomProperties::slot_defineNewColor()
{
    auto color = QColorDialog::getColor(mpHost->mRed, this, tr("Define new room color"));
    if (color.isValid()) {
        auto environmentId = mpHost->mpMap->mCustomEnvColors.size() + 257 + 16;
        if (mpHost->mpMap->mCustomEnvColors.contains(environmentId)) {
            // find a new environment ID to use, starting with the latest
            // 'safe' number so the new environment is last in the dialog
            do {
                environmentId++;
            } while (mpHost->mpMap->mCustomEnvColors.contains(environmentId));
        }

        mpHost->mpMap->mCustomEnvColors[environmentId] = color;
        slot_openRoomColorSelector();
    }
    repaint();
    mpHost->mpMap->setUnsaved(__func__);
}


void dlgRoomProperties::slot_openRoomColorSelector()
{
    auto dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Set room color"));
    auto vboxLayout = new QVBoxLayout;
    dialog->setLayout(vboxLayout);
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    dialog->setContentsMargins(0, 0, 0, 0);
    auto listWidget = new QListWidget(dialog);
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);

    // Held apart from mRoomColorNumber until OK, so a Cancel discards the click
    int clickedColorNumber = mRoomColorNumber;
    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, &QDialog::accept);
    connect(listWidget, &QListWidget::itemClicked, dialog, [&clickedColorNumber](QListWidgetItem* pI) {
        clickedColorNumber = pI->text().toInt();
    });
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        // A right-click on empty space leaves the current item as it was, so
        // only the color under the cursor is offered for deletion
        QListWidgetItem* clickedItem = listWidget->itemAt(pos);
        if (!clickedItem) {
            return;
        }
        QMenu menu;
        //: This action deletes a color from the list of all room colors
        menu.addAction(tr("Delete room color"), this, [=, this]() {
            const int colorNumber = clickedItem->text().toInt();
            delete clickedItem;

            mpHost->mpMap->mCustomEnvColors.remove(colorNumber);
            repaint();
            mpHost->mpMap->setUnsaved(__func__);
        });

        menu.exec(QCursor::pos());
    });

    vboxLayout->addWidget(listWidget);
    auto pButtonBar = new QWidget(dialog);

    auto hboxLayout = new QHBoxLayout;
    pButtonBar->setLayout(hboxLayout);
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    auto pB_newColor = new QPushButton(pButtonBar);
    pB_newColor->setText(tr("Define new room color"));

    connect(pB_newColor, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(pB_newColor, &QAbstractButton::clicked, this, &dlgRoomProperties::slot_defineNewColor);

    hboxLayout->addWidget(pB_newColor);

    auto pB_ok = new QPushButton(pButtonBar);
    //: confirm room color selection dialog
    pB_ok->setText(tr("OK"));
    hboxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    //: cancel room color selection dialog
    pB_abort->setText(tr("Cancel"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    hboxLayout->addWidget(pB_abort);
    vboxLayout->addWidget(pButtonBar);

    QMapIterator<int, QColor> it(mpHost->mpMap->mCustomEnvColors);
    while (it.hasNext()) {
        it.next();
        QColor c;
        c = it.value();
        auto pI = new QListWidgetItem(listWidget);
        QPixmap pix = QPixmap(50, 50);
        pix.fill(c);
        const QIcon mi(pix);
        pI->setIcon(mi);
        pI->setText(QString::number(it.key()));
        listWidget->addItem(pI);
    }
    listWidget->sortItems();

    if (dialog->exec() == QDialog::Accepted && mpHost->mpMap->mCustomEnvColors.contains(clickedColorNumber)) {
        mRoomColorNumber = clickedColorNumber;
        mChangeRoomColor = true;
        mRoomColor = mpHost->mpMap->mCustomEnvColors.value(mRoomColorNumber);
        showRoomColour();
        slot_updatePreview();
    }
}

void dlgRoomProperties::slot_symbolComboBoxItemChanged(const int index)
{
    if (index < 0 || index >= comboBox_roomSymbol->count() || !comboBox_roomSymbol->lineEdit()) {
        return;
    }

    selectFromTheEnd(comboBox_roomSymbol->lineEdit());
}

void dlgRoomProperties::slot_weightComboBoxItemChanged(const int index)
{
    if (index < 0 || index >= comboBox_weight->count() || !comboBox_weight->lineEdit()) {
        return;
    }

    selectFromTheEnd(comboBox_weight->lineEdit());
}

void dlgRoomProperties::slot_openBorderColorSelector()
{
    QColor initialColor = selectedBorderColor.isValid() ? selectedBorderColor : mpHost->mRoomBorderColor;
    auto* dialog = new QColorDialog(initialColor, this);
    dialog->setOption(QColorDialog::ShowAlphaChannel, true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    //: Title for the color picker dialog when selecting a room border color
    dialog->setWindowTitle(tr("Set border color"));
    connect(dialog, &QColorDialog::currentColorChanged, this, &dlgRoomProperties::slot_borderColorSelected);
    connect(dialog, &QColorDialog::colorSelected, this, &dlgRoomProperties::slot_borderColorSelected);
    dialog->open();
}

void dlgRoomProperties::slot_borderColorSelected(const QColor& color)
{
    selectedBorderColor = color;
    mBorderColorWasChanged = true;
    showBorderColour();
    emitBorderPreview();
}

// The field shows the first room's colour, so it is everyone's only while the rooms agree or once one was decided for
// them all; until then each room takes the opacity alone
void dlgRoomProperties::slot_borderOpacitySelected(const QColor& colour)
{
    if (mBorderColorWasChanged || !borderColoursDiffer()) {
        slot_borderColorSelected(colour);
        return;
    }
    // What the field shows, for the colours window to open on
    selectedBorderColor = colour;
    mBorderAlphaOnly = colour.alpha();
    emitBorderPreview();
}

void dlgRoomProperties::slot_resetBorderColor()
{
    selectedBorderColor = QColor();
    mBorderColorWasChanged = true;
    showBorderColour();
    emitBorderPreview();
}

void dlgRoomProperties::slot_borderThicknessChanged(int value)
{
    mBorderThickness = value;
    mBorderThicknessWasChanged = true;
    emitBorderPreview();
}

void dlgRoomProperties::initGeneral()
{
    QList<const TRoom*> rooms;
    for (const int roomId : std::as_const(mRoomIds)) {
        if (const TRoom* room = liveRoom(roomId)) {
            rooms << room;
        }
    }
    const bool single = rooms.size() == 1;

    QList<int> ids;
    for (const TRoom* room : std::as_const(rooms)) {
        ids << room->getId();
    }
    std::sort(ids.begin(), ids.end());
    if (single) {
        //: Heading of the Configure room window's Room tab, naming the room being configured by its ID
        label_roomIdValue->setText(tr("Room %1").arg(ids.constFirst()));
        label_roomIdValue->setToolTip(QString());
    } else {
        const bool cut = ids.size() > scmMostRoomIdsNamed;
        QStringList named;
        for (const int id : ids.mid(0, cut ? scmMostRoomIdsNamed - 1 : ids.size())) {
            named << QString::number(id);
        }
        QStringList all;
        for (const int id : std::as_const(ids)) {
            all << QString::number(id);
        }
        if (cut) {
            //: The last entry of the list of room IDs heading the Configure room window when it was opened on too many rooms to name: how many more there are, as in "8 rooms: 478, 479, 512 and 5 more"
            named << tr("%n more", nullptr, static_cast<int>(ids.size() - named.size()));
        }
        //: Heading of the Configure room window's Room tab when several rooms are configured at once: how many, then their IDs as a list, as in "3 rooms: 478, 479 and 512"
        label_roomIdValue->setText(tr("%n rooms: %1", nullptr, static_cast<int>(ids.size())).arg(locale().createSeparatedList(named)));
        label_roomIdValue->setToolTip(cut ? utils::richText(locale().createSeparatedList(all)) : QString());
    }

    // A game ID belongs to one room only, so it cannot be given to a selection
    const QSignalBlocker hashBlocker(lineEdit_hash);
    mOriginalHash = single ? mpHost->mpMap->mpRoomDB->roomIDToHash.value(rooms.first()->getId()) : QString();
    lineEdit_hash->setText(mOriginalHash);
    lineEdit_hash->setEnabled(single);
    //: Placeholder of Configure room's game ID field, which is disabled when several rooms are being configured, as a game ID belongs to one room
    lineEdit_hash->setPlaceholderText(single ? QString() : tr("One room at a time"));
    roomPropertiesHashCaption->setText(single
                                               //: Caption under the game ID field of the Configure room window: the game's own ID for the room, which scripts look the room up by
                                               ? tr("What the game calls this room. Scripts find the room by it.")
                                               //: Caption under the game ID field of the Configure room window when several rooms are configured at once, which disables the field
                                               : tr("A hash belongs to one room."));
    roomPropertiesHashCaption->show();
    label_hashWarning->hide();
    label_userDataWarning->hide();

    QSet<int> roomAreas;
    for (const TRoom* room : std::as_const(rooms)) {
        roomAreas.insert(room->getArea());
    }
    mOriginalAreaId = roomAreas.size() == 1 ? std::optional<int>(*roomAreas.cbegin()) : std::nullopt;
    QList<QPair<QString, int>> areas;
    const QMap<int, QString>& areaNames = mpHost->mpMap->mpRoomDB->getAreaNamesMap();
    for (auto it = areaNames.cbegin(); it != areaNames.cend(); ++it) {
        areas.append({it.value(), it.key()});
    }
    std::sort(areas.begin(), areas.end(), [](const auto& left, const auto& right) {
        const int byName = QString::localeAwareCompare(left.first, right.first);
        return byName ? byName < 0 : left.second < right.second;
    });
    const QSignalBlocker areaBlocker(comboBox_area);
    comboBox_area->clear();
    if (!mOriginalAreaId.has_value()) {
        // No area id, so leaving it picked moves no room
        comboBox_area->addItem(multipleValuesPlaceholder);
    }
    for (const auto& [name, id] : std::as_const(areas)) {
        comboBox_area->addItem(qsl("%1 (%2)").arg(name, QString::number(id)), id);
    }
    comboBox_area->setCurrentIndex(mOriginalAreaId.has_value() ? comboBox_area->findData(*mOriginalAreaId) : 0);
}

void dlgRoomProperties::slot_hashChanged(const QString& hash)
{
    // Counted as initGeneral() counts them when it enables the field, so the room's own game ID is not taken for another's
    const QSet<int> rooms = liveRoomIds();
    const int roomId = rooms.size() == 1 ? *rooms.cbegin() : 0;
    const int holder = hash.isEmpty() ? -1 : mpHost->mpMap->mpRoomDB->hashToRoomID.value(hash, -1);
    if (holder < 1 || holder == roomId) {
        label_hashWarning->hide();
        roomPropertiesHashCaption->show();
        return;
    }
    //: Warning under Configure room's game ID field when the ID typed in already belongs to another room; %1 is that room's ID
    label_hashWarning->setText(tr("Room %1 has this game ID now and will lose it.").arg(holder));
    roomPropertiesHashCaption->hide();
    label_hashWarning->show();
}

void dlgRoomProperties::initUserData()
{
    QMap<QString, QSet<QString>> valuesByKey;
    QMap<QString, int> roomsByKey;
    int roomCount = 0;
    for (const int roomId : std::as_const(mRoomIds)) {
        const TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        ++roomCount;
        for (auto it = room->userData.cbegin(); it != room->userData.cend(); ++it) {
            if (!editedInUserDataTable(it.key())) {
                continue;
            }
            valuesByKey[it.key()].insert(it.value());
            ++roomsByKey[it.key()];
        }
    }

    mOriginalUserData.clear();
    for (auto it = valuesByKey.cbegin(); it != valuesByKey.cend(); ++it) {
        // A key only some of the rooms hold is as undecided as one they hold different values for
        const bool agreed = it.value().size() == 1 && roomsByKey.value(it.key()) == roomCount;
        mOriginalUserData.insert(it.key(), agreed ? std::optional<QString>(*it.value().cbegin()) : std::nullopt);
    }

    {
        const QSignalBlocker blocker(tableWidget_userData);
        tableWidget_userData->setRowCount(0);
        for (auto it = mOriginalUserData.cbegin(); it != mOriginalUserData.cend(); ++it) {
            addUserDataRow(it.key(), it.value(), false);
        }
    }
    mpUserDataEmpty->setText(roomCount > 1
                                     //: Shown in the empty table on the Script data tab of the Configure room window, opened on several rooms
                                     ? tr("No script has kept anything with these rooms.")
                                     //: Shown in the empty table on the Script data tab of the Configure room window
                                     : tr("No script has kept anything with this room."));
    showUserDataEmpty();
    mKeyColumnFitted = false;
    tableWidget_userData->resizeColumnToContents(0);
    showChosenValue();
}

void dlgRoomProperties::addUserDataRow(const QString& key, const std::optional<QString>& value, const bool keyEditable)
{
    const int row = tableWidget_userData->rowCount();
    tableWidget_userData->insertRow(row);

    auto* keyItem = new QTableWidgetItem(key);
    if (!keyEditable) {
        // Renaming a key would be a removal and an addition in one, which Remove and Add already say plainly
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
    }
    tableWidget_userData->setItem(row, 0, keyItem);

    auto* valueItem = new QTableWidgetItem(value.value_or(QString()));
    // Edited in the value box, which keeps the line breaks a cell's line editor would lose
    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
    if (!value.has_value()) {
        valueItem->setData(scmStartedMixedRole, true);
        showUserDataValueMixed(valueItem, true);
    }
    tableWidget_userData->setItem(row, 1, valueItem);
}

void dlgRoomProperties::showUserDataValueMixed(QTableWidgetItem* item, const bool mixed)
{
    item->setData(scmMixedValueRole, mixed);
    if (mixed) {
        item->setText(multipleValuesPlaceholder);
    }
}

void dlgRoomProperties::showUserDataEmpty()
{
    mpUserDataEmpty->setGeometry(tableWidget_userData->viewport()->rect());
    mpUserDataEmpty->setVisible(tableWidget_userData->rowCount() == 0);
}

QMap<QString, std::optional<QString>> dlgRoomProperties::userDataChanges() const
{
    QMap<QString, std::optional<QString>> changes;
    QSet<QString> keysLeft;
    for (int row = 0; row < tableWidget_userData->rowCount(); ++row) {
        const QTableWidgetItem* keyItem = tableWidget_userData->item(row, 0);
        const QTableWidgetItem* valueItem = tableWidget_userData->item(row, 1);
        if (!valueItem) {
            continue;
        }
        // A row listed from the rooms, whose key cannot be edited, is kept whatever its key: setRoomUserData()
        // accepts blank ones. Only an added row needs a key worth saving, and one typed with a space around it is
        // the key without
        const bool addedRow = keyItem && keyItem->flags().testFlag(Qt::ItemIsEditable);
        const QString key = !keyItem ? QString() : addedRow ? keyItem->text().trimmed() : keyItem->text();
        // The rows listed from the rooms come first, so an added row repeating a key is the one left out
        if (addedRow && (key.isEmpty() || !editedInUserDataTable(key) || keysLeft.contains(key))) {
            continue;
        }
        keysLeft.insert(key);
        if (valueItem->data(scmMixedValueRole).toBool()) {
            continue;
        }
        const auto original = mOriginalUserData.constFind(key);
        if (original != mOriginalUserData.cend() && original.value() == valueItem->text()) {
            continue;
        }
        changes.insert(key, valueItem->text());
    }
    for (auto it = mOriginalUserData.cbegin(); it != mOriginalUserData.cend(); ++it) {
        if (!keysLeft.contains(it.key())) {
            changes.insert(it.key(), std::nullopt);
        }
    }
    return changes;
}

void dlgRoomProperties::slot_addUserData()
{
    {
        const QSignalBlocker blocker(tableWidget_userData);
        addUserDataRow(QString(), QString(), true);
    }
    showUserDataEmpty();
    const int row = tableWidget_userData->rowCount() - 1;
    tableWidget_userData->setCurrentCell(row, 0);
    tableWidget_userData->editItem(tableWidget_userData->item(row, 0));
}

void dlgRoomProperties::slot_removeUserData()
{
    QList<int> rows;
    const auto selected = tableWidget_userData->selectionModel()->selectedRows();
    for (const QModelIndex& index : selected) {
        rows << index.row();
    }
    if (rows.isEmpty() && tableWidget_userData->currentRow() >= 0) {
        rows << tableWidget_userData->currentRow();
    }
    // From the bottom up, so removing a row does not move the ones still to go
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (const int row : std::as_const(rows)) {
        tableWidget_userData->removeRow(row);
    }
    showUserDataEmpty();
    updateUserDataKeyWarning();
    showChosenValue();
}

void dlgRoomProperties::updateUserDataKeyWarning()
{
    QStringList repeatedKeys;
    bool mapKey = false;
    // Read the way userDataChanges() reads them, which saves the first row of a key and leaves the others out
    QSet<QString> keysSeen;
    for (int row = 0; row < tableWidget_userData->rowCount(); ++row) {
        const QTableWidgetItem* keyItem = tableWidget_userData->item(row, 0);
        if (!keyItem) {
            continue;
        }
        // Only an added row's key can be typed; one listed from the rooms is shown only if the table edits it
        if (!keyItem->flags().testFlag(Qt::ItemIsEditable)) {
            keysSeen.insert(keyItem->text());
            continue;
        }
        const QString key = keyItem->text().trimmed();
        if (key.isEmpty()) {
            continue;
        }
        if (!editedInUserDataTable(key)) {
            mapKey = true;
        } else if (keysSeen.contains(key)) {
            if (!repeatedKeys.contains(key)) {
                repeatedKeys << key;
            }
        } else {
            keysSeen.insert(key);
        }
    }
    QStringList warnings;
    for (const QString& key : std::as_const(repeatedKeys)) {
        //: Warning under the table on Configure room's Script data tab; %1 is a key typed into a new row that an earlier row already has
        warnings << tr("\"%1\" is already in the table, so this row will not be saved.").arg(key);
    }
    if (mapKey) {
        //: Warning under the table on Configure room's Script data tab, when a key typed into a new row starts with room.ui_, which the map keeps for itself; do not translate room.ui_
        warnings << tr("Keys starting with room.ui_ belong to the map, so such a row will not be saved.");
    }
    label_userDataWarning->setText(warnings.join(QLatin1Char(' ')));
    label_userDataWarning->setVisible(!warnings.isEmpty());
    fitScriptData();
}

void dlgRoomProperties::slot_userDataItemChanged(QTableWidgetItem* item)
{
    if (!item || item->column() != 0) {
        return;
    }
    updateUserDataKeyWarning();
    if (item->row() == tableWidget_userData->currentRow()) {
        showValueLabel();
    }
}

void dlgRoomProperties::showChosenValue()
{
    const int row = tableWidget_userData->currentRow();
    QTableWidgetItem* pItem = row >= 0 ? tableWidget_userData->item(row, 1) : nullptr;
    const bool mixed = pItem && pItem->data(scmMixedValueRole).toBool();
    mShownValue = pItem && !mixed ? pItem->text() : QString();
    mShownAsJson = shownIndented(mShownValue);
    {
        const QSignalBlocker blocker(roomPropertiesValueBox);
        if (!pItem) {
            //: Placeholder of the value box on the Script data tab of the Configure room window while no row of the table above it is chosen
            roomPropertiesValueBox->setPlaceholderText(tr("Choose a row to see its value"));
        } else if (mixed) {
            roomPropertiesValueBox->setPlaceholderText(multipleValuesPlaceholder);
        } else {
            //: Placeholder of the value box on the Script data tab of the Configure room window when the chosen row's value is empty
            roomPropertiesValueBox->setPlaceholderText(tr("No value yet"));
        }
        roomPropertiesValueBox->setPlainText(mShownAsJson ? jsonIndented(mShownValue) : mShownValue);
        roomPropertiesValueBox->setReadOnly(!pItem);
        roomPropertiesValueBox->setFont(mShownAsJson ? uiDesign::fixedPitchFont(font()) : QFont());
    }
    roomPropertiesJsonWarning->hide();
    pushButton_removeUserData->setEnabled(pItem != nullptr);
    showValueLabel();
    fitScriptData();
}

void dlgRoomProperties::showValueLabel()
{
    const int row = tableWidget_userData->currentRow();
    const QTableWidgetItem* pKey = row >= 0 ? tableWidget_userData->item(row, 0) : nullptr;
    const QTableWidgetItem* pValue = row >= 0 ? tableWidget_userData->item(row, 1) : nullptr;
    if (!pKey || !pValue) {
        //: Label over the value box on the Script data tab of the Configure room window while no row is chosen
        roomPropertiesValueLabel->setText(tr("Value").toHtmlEscaped());
        return;
    }
    const QString key = qsl("<code>%1</code>").arg(pKey->text().toHtmlEscaped());
    const std::optional<JsonShape> shape = mShownAsJson ? jsonShape(pValue->text()) : std::nullopt;
    if (shape && shape->list) {
        //: Label over the value box on the Script data tab of the Configure room window when the value is a JSON list; %1 is the key, %n how many entries the list has
        roomPropertiesValueLabel->setText(tr("Value of %1, a list of %n", nullptr, static_cast<int>(shape->entries)).toHtmlEscaped().arg(key));
    } else if (shape) {
        //: Label over the value box on the Script data tab of the Configure room window when the value is a JSON table (object); %1 is the key, %n how many entries the table has
        roomPropertiesValueLabel->setText(tr("Value of %1, a table of %n", nullptr, static_cast<int>(shape->entries)).toHtmlEscaped().arg(key));
    } else {
        //: Label over the value box on the Script data tab of the Configure room window; %1 is the key of the chosen row
        roomPropertiesValueLabel->setText(tr("Value of %1").toHtmlEscaped().arg(key));
    }
}

void dlgRoomProperties::slot_valueBoxEdited()
{
    const int row = tableWidget_userData->currentRow();
    QTableWidgetItem* pItem = row >= 0 ? tableWidget_userData->item(row, 1) : nullptr;
    if (!pItem) {
        return;
    }
    QString value = plainTextKeepingSpaces(*roomPropertiesValueBox);
    // The box reads "\r\n" as one line break and gives back "\n": a value stored with them gets them back, so a
    // mapper script matching it as the game sent it still finds the room
    if (mShownValue.contains(qsl("\r\n"))) {
        value.replace(QLatin1Char('\n'), qsl("\r\n"));
    }
    if (mShownAsJson) {
        // Saved in the shape it was stored in, the layout taken out again; once it no longer reads as JSON there is
        // no telling layout from content, so it is saved as typed
        const bool stillJson = jsonShape(value).has_value();
        roomPropertiesJsonWarning->setVisible(!stillJson);
        if (stillJson) {
            value = jsonWithoutWhitespace(value);
        }
    }
    {
        const QSignalBlocker blocker(tableWidget_userData);
        // Emptied again, a value that started out mixed goes back to the rooms' differing ones
        if (pItem->data(scmStartedMixedRole).toBool() && value.isEmpty()) {
            showUserDataValueMixed(pItem, true);
        } else {
            showUserDataValueMixed(pItem, false);
            pItem->setText(value);
        }
    }
    showValueLabel();
    fitScriptData();
}

void dlgRoomProperties::focusValueBox()
{
    if (tableWidget_userData->currentRow() < 0) {
        return;
    }
    roomPropertiesValueBox->setFocus(Qt::OtherFocusReason);
}

// The tab is only as tall as the Room tab needs, so the table and the value box share what is left of it: each as
// tall as it needs while that fits, then the table gives way down to a few rows, and only then the box scrolls
void dlgRoomProperties::fitScriptData()
{
    if (mFittingScriptData || !roomPropertiesScriptData->isVisible()) {
        return;
    }
    const QScopedValueRollback<bool> fitting(mFittingScriptData, true);
    QTableWidget* pTable = tableWidget_userData;
    QPlainTextEdit* pBox = roomPropertiesValueBox;
    QLayout* pLayout = roomPropertiesScriptData->layout();
    const QLayoutItem* pTableItem = pLayout->itemAt(pLayout->indexOf(pTable));
    const QLayoutItem* pBoxItem = pLayout->itemAt(pLayout->indexOf(pBox));
    if (!pTableItem || !pBoxItem) {
        return;
    }
    // Everything else at the height the layout gives it at this width, the wrapping words included
    const int whole = pLayout->hasHeightForWidth() ? pLayout->totalHeightForWidth(roomPropertiesScriptData->width()) : pLayout->totalSizeHint().height();
    const int room = roomPropertiesScriptData->height() - (whole - pTableItem->sizeHint().height() - pBoxItem->sizeHint().height());

    // The frame, the header and a scroll bar, none of which changes with the table's height
    const int tableChrome = pTable->height() - pTable->viewport()->height();
    const int rowHeight = pTable->verticalHeader()->defaultSectionSize();
    int rows = 0;
    for (int row = 0; row < pTable->rowCount(); ++row) {
        rows += pTable->rowHeight(row);
    }
    // An empty table keeps one line, for the words saying it is empty
    const int tableNatural = tableChrome + (pTable->rowCount() ? rows : rowHeight);
    const int tableFloor = std::min(tableNatural, tableChrome + scmTableFloorRows * rowHeight);

    const int boxChrome = pBox->height() - pBox->viewport()->height();
    const int margin = qCeil(pBox->document()->documentMargin());
    const int line = QFontMetrics(pBox->font()).lineSpacing();
    // The last block's rectangle carries the bottom margin, and QPlainTextEdit wants a pixel more than the lines
    // before it calls them all in sight
    int contents = margin + 1;
    const QAbstractTextDocumentLayout* pDocumentLayout = pBox->document()->documentLayout();
    for (QTextBlock block = pBox->document()->firstBlock(); block.isValid(); block = block.next()) {
        contents += qCeil(pDocumentLayout->blockBoundingRect(block).height());
    }
    const int boxRest = boxChrome + 2 * margin + scmValueBoxRestLines * line;
    const int boxLeast = boxChrome + 2 * margin + line;
    const int boxNatural = std::max(boxRest, boxChrome + contents);

    int tableHeight = tableNatural;
    int boxHeight = boxNatural;
    if (tableHeight + boxHeight > room) {
        tableHeight = std::max(tableFloor, room - boxHeight);
        boxHeight = std::max(boxLeast, room - tableHeight);
    }
    pTable->setFixedHeight(tableHeight);
    pBox->setFixedHeight(boxHeight);
    pLayout->activate();
    if (pTable->currentIndex().isValid()) {
        pTable->scrollTo(pTable->currentIndex());
    }
}

void dlgRoomProperties::fitKeyColumn()
{
    if (mKeyColumnFitted || !tableWidget_userData->isVisible()) {
        return;
    }
    mKeyColumnFitted = true;
    tableWidget_userData->resizeColumnToContents(0);
    const int cap = qRound(tableWidget_userData->viewport()->width() * scmKeyColumnShare);
    if (tableWidget_userData->columnWidth(0) > cap) {
        tableWidget_userData->setColumnWidth(0, cap);
    }
}

bool dlgRoomProperties::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched == roomPropertiesScriptData) {
        // The layout has given the tab's children their place by now: it sees the event before any filter does
        if (pEvent->type() == QEvent::Resize || pEvent->type() == QEvent::Show) {
            fitScriptData();
        }
    } else if (pWatched == tableWidget_userData->viewport()) {
        if (pEvent->type() == QEvent::Resize) {
            mpUserDataEmpty->setGeometry(tableWidget_userData->viewport()->rect());
        }
    } else if (pWatched == tableWidget_userData) {
        if (pEvent->type() == QEvent::Show && !mKeyColumnFitted) {
            // Once the tab's layout has given the table its width
            QTimer::singleShot(0, this, &dlgRoomProperties::fitKeyColumn);
        } else if (pEvent->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent*>(pEvent)->key();
            // Rather than the dialog's OK: the value is what a row is chosen for
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && tableWidget_userData->currentRow() >= 0) {
                focusValueBox();
                return true;
            }
        }
    }
    return QDialog::eventFilter(pWatched, pEvent);
}

void dlgRoomProperties::showEvent(QShowEvent* pEvent)
{
    // The size the tabs ask for settles only as the window is made to be shown, after init() sized it; taken again
    // before QDialog::showEvent() centres the window by its size
    if (!pEvent->spontaneous() && !mSizedForShow) {
        mSizedForShow = true;
        keepTheTabsRoomForThePages();
        adjustSize();
    }
    QDialog::showEvent(pEvent);
}

void dlgRoomProperties::changeEvent(QEvent* pEvent)
{
    QDialog::changeEvent(pEvent);
    // Not before the constructor has built what the pass styles
    if (!mpValueDelegate) {
        return;
    }
    // A reader following the system's appearance gets no signal_appearanceChanged when it changes, only these
    if (pEvent->type() == QEvent::StyleChange || pEvent->type() == QEvent::PaletteChange || pEvent->type() == QEvent::FontChange) {
        applyRoomPropertiesShellStyle();
    }
}

// The dialog is not modal, so a script may have deleted any of these rooms,
// or replaced one with a new room under the same id, while it was open
TRoom* dlgRoomProperties::liveRoom(const int roomId) const
{
    TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomId);
    if (!room || !mRoomSerials.contains(roomId) || room->serial() != mRoomSerials.value(roomId)) {
        return nullptr;
    }
    return room;
}

QSet<int> dlgRoomProperties::liveRoomIds() const
{
    QSet<int> result;
    for (const int roomId : std::as_const(mRoomIds)) {
        if (liveRoom(roomId)) {
            result.insert(roomId);
        }
    }
    return result;
}

void dlgRoomProperties::emitBorderPreview()
{
    // Apply current border settings directly to rooms for live preview
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        if (mBorderColorWasChanged) {
            room->mBorderColor = selectedBorderColor;
        } else if (mBorderAlphaOnly) {
            // A room with no colour of its own keeps following the map's
            QColor colour = mOriginalBorderColors.value(roomId);
            if (colour.isValid()) {
                colour.setAlpha(*mBorderAlphaOnly);
            }
            room->mBorderColor = colour;
        }
        if (mBorderThicknessWasChanged) {
            room->mBorderThickness = mBorderThickness;
        }
    }
    emit signal_preview_border(mRoomIds);
}

void dlgRoomProperties::restoreOriginalBorders()
{
    for (const int roomId : std::as_const(mRoomIds)) {
        TRoom* room = liveRoom(roomId);
        if (!room) {
            continue;
        }
        room->mBorderColor = mOriginalBorderColors.value(roomId);
        room->mBorderThickness = mOriginalBorderThicknesses.value(roomId);
    }
}

void dlgRoomProperties::reject()
{
    // Hiding the dialog takes the focus from a colour field, which takes the code typed in it, so restore after that
    QDialog::reject();
    restoreOriginalBorders();
    emit signal_preview_border(mRoomIds);
}
