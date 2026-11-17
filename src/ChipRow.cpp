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

#include "ChipRow.h"

#include "EditorPlaceholderButton.h"
#include "FlowLayout.h"
#include "uiDesign.h"
#include "utils.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>

#include <algorithm>

namespace uiDesign {

static constexpr qreal scmChipFontScale = 0.85;
// Less on the trailing edge, where the cross stands in the gap
static constexpr int scmChipPaddingVertical = 3;
static constexpr int scmChipPaddingLeading = 9;
static constexpr int scmChipPaddingTrailing = 5;
static constexpr int scmChipGap = 4;
static constexpr qreal scmChipGlyphScale = 0.75;
static constexpr int scmChipSpacingAcross = 6;
static constexpr int scmChipSpacingDown = 6;
static constexpr int scmChipNoteMilliseconds = 2000;
static constexpr int scmSuggestionPaddingVertical = 2;
static constexpr int scmSuggestionPaddingHorizontal = 6;
static constexpr int scmSuggestionSlack = 32;
static constexpr int scmSuggestionNoteGap = 16;
static constexpr int scmSuggestionNoteRole = Qt::UserRole;

QFont chipFont(const QWidget* pOn)
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont hostFont = pOn->font();
    if (hostFont.pointSizeF() > 0.0) {
        font.setPointSizeF(hostFont.pointSizeF() * scmChipFontScale);
    } else {
        font.setPixelSize(std::max(1, qRound(hostFont.pixelSize() * scmChipFontScale)));
    }
    return font;
}

static int chipHeightOn(const QWidget* pOn)
{
    return QFontMetrics(chipFont(pOn)).height() + 2 * (scmChipPaddingVertical + scmInputBorderWidth);
}

static int chipGlyphSizeOn(const QWidget* pOn)
{
    return std::max(1, qRound(QFontMetrics(chipFont(pOn)).height() * scmChipGlyphScale));
}

Chip::Chip(const QString& name, QWidget* pParent)
: QFrame(pParent)
{
    setObjectName(qsl("editorChip"));
    setFrameShape(QFrame::NoFrame);
    // An #id rule paints this widget's background only with WA_StyledBackground set
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    auto* pRow = new QHBoxLayout(this);
    pRow->setSpacing(scmChipGap);

    mpLabel = new QLabel(this);
    mpLabel->setObjectName(qsl("editorChipLabel"));
    mpLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    pRow->addWidget(mpLabel);

    mpRemove = new QToolButton(this);
    mpRemove->setObjectName(qsl("editorChipRemove"));
    mpRemove->setAutoRaise(true);
    // Delete removes a focused chip, so Tab stops once per chip rather than twice
    mpRemove->setFocusPolicy(Qt::NoFocus);
    connect(mpRemove, &QAbstractButton::clicked, this, &Chip::removeRequested);
    pRow->addWidget(mpRemove);

    //: Accessible name of one event in a script's row of events; %1 is the event's name
    mAccessibleNameWords = tr("Event %1");
    //: Tooltip on the cross that takes one event off the row of events a script listens for; %1 is the event's name
    mRemoveTooltipWords = tr("Stop listening for %1");
    applyPadding();
    remeasure();
    setName(name);
}

void Chip::setWords(const QString& accessibleName, const QString& removeTooltip)
{
    mAccessibleNameWords = accessibleName;
    mRemoveTooltipWords = removeTooltip;
    setName(mName);
}

// Measured off the row: a stylesheet rule naming the chip stops it inheriting the
// row's font, so the chip's own font() answers with the application's
void Chip::remeasure()
{
    const QWidget* pOn = parentWidget() ? parentWidget() : this;
    mpLabel->setFont(chipFont(pOn));
    const int glyphSize = chipGlyphSizeOn(pOn);
    mpRemove->setIconSize(QSize(glyphSize, glyphSize));
    mpRemove->setFixedSize(glyphSize, glyphSize);
    // The row's line: the field shares it and can be taller than a chip
    const auto* pRow = qobject_cast<const ChipRow*>(pOn);
    setFixedHeight(pRow ? pRow->lineHeight() : chipHeightOn(pOn));
    updateGeometry();
}

void Chip::changeEvent(QEvent* pEvent)
{
    QFrame::changeEvent(pEvent);
    if (pEvent->type() == QEvent::FontChange) {
        remeasure();
    } else if (pEvent->type() == QEvent::LayoutDirectionChange) {
        applyPadding();
    }
}

// Not in the sheet: a plain QFrame's stylesheet padding moves what is drawn without moving what the layout places. A
// layout's margins stay where they are written, so right to left the leading one goes on the right by hand.
void Chip::applyPadding()
{
    const bool rightToLeft = layoutDirection() == Qt::RightToLeft;
    layout()->setContentsMargins(
            rightToLeft ? scmChipPaddingTrailing : scmChipPaddingLeading, scmChipPaddingVertical, rightToLeft ? scmChipPaddingLeading : scmChipPaddingTrailing, scmChipPaddingVertical);
}

void Chip::setName(const QString& name)
{
    mName = name;
    mpLabel->setText(name);
    mpRemove->setToolTip(utils::richText(mRemoveTooltipWords.arg(name)));
    setAccessibleName(mAccessibleNameWords.arg(name));
    updateGeometry();
}

void Chip::setRemoveGlyph(const QIcon& glyph)
{
    mpRemove->setIcon(glyph);
}

void Chip::setOrigin(const QString& origin, const bool system)
{
    mpLabel->setToolTip(origin.isEmpty() ? QString() : utils::richText(origin));
    const bool wasSystem = property("editorChipSystem").toBool();
    setProperty("editorChipSystem", system);
    if (wasSystem == system) {
        return;
    }
    repolish(this);
    // The rule reading the property colours the word, and restyling a widget leaves its children as they were
    repolish(mpLabel);
}

void Chip::keyPressEvent(QKeyEvent* pEvent)
{
    switch (pEvent->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        emit removeRequested();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_F2:
        emit editRequested();
        return;
    default:
        break;
    }
    QFrame::keyPressEvent(pEvent);
}

void Chip::mousePressEvent(QMouseEvent* pEvent)
{
    if (pEvent->button() != Qt::LeftButton) {
        QFrame::mousePressEvent(pEvent);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    emit editRequested();
}

ChipRow::ChipRow(QWidget* pParent)
: QWidget(pParent)
{
    mpFlow = new FlowLayout(this, scmChipSpacingAcross, scmChipSpacingDown);
    mpFlow->setContentsMargins(0, 0, 0, 0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    mpAdd = new PlaceholderButton(this);
    mpAdd->setObjectName(qsl("editorChipAdd"));
    //: Button at the end of a script's row of event chips that opens a field for a new event name
    mpAdd->setText(tr("Add event"));
    //: Tooltip on the button that opens the field for another event a script should listen for
    mpAdd->setToolTip(utils::richText(tr("Listen for another event")));
    mpAdd->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // Auto-raise would have the style draw a second frame over the button's own under the mouse
    mpAdd->setAutoRaise(false);
    mpAdd->setFrameRadius(scmRadiusChip);
    connect(mpAdd, &QAbstractButton::clicked, this, &ChipRow::beginAdd);

    mpField = new QLineEdit(this);
    mpField->setObjectName(qsl("editorChipEditor"));
    //: Placeholder in the field a new event name for a script is typed into
    mpField->setPlaceholderText(tr("event name"));
    mpField->installEventFilter(this);
    mpField->hide();
    remeasure();
    connect(mpField, &QLineEdit::textEdited, this, [this](const QString& text) {
        const int comma = text.indexOf(QLatin1Char(','));
        if (comma < 0) {
            return;
        }
        const QString rest = text.mid(comma + 1);
        mpField->setText(text.left(comma));
        commitField(true);
        if (mFieldOpen) {
            mpField->setText(rest);
        }
    });

    mpNote = new QLabel(this);
    mpNote->setObjectName(qsl("editorChipNote"));
    mpNote->hide();

    mpNoteTimer = new QTimer(this);
    mpNoteTimer->setObjectName(qsl("editorChipNoteTimer"));
    mpNoteTimer->setSingleShot(true);
    mpNoteTimer->setInterval(scmChipNoteMilliseconds);
    connect(mpNoteTimer, &QTimer::timeout, this, &ChipRow::hideNote);

    rebuild();
}

QStringList ChipRow::items() const
{
    QStringList names;
    names.reserve(mChips.size());
    for (const Chip* pChip : mChips) {
        names << pChip->name();
    }
    return names;
}

void ChipRow::setItems(const QStringList& items)
{
    closeField();
    hideNote();
    qDeleteAll(mChips);
    mChips.clear();
    QStringList taken;
    for (const QString& name : items) {
        // The user cannot type an empty or repeated name here, but a profile file can carry either; neither registers
        // twice. Not cleaned like a typed name: a stored "ready," is its own event, and saving the row writes it back.
        if (name.isEmpty() || taken.contains(name)) {
            continue;
        }
        taken << name;
        mChips.append(makeChip(name));
    }
    rebuild();
}

int ChipRow::count() const
{
    return mChips.size();
}

QWidget* ChipRow::chipAt(const int index) const
{
    return index >= 0 && index < mChips.size() ? mChips.at(index) : nullptr;
}

int ChipRow::indexOf(const QString& name) const
{
    for (int i = 0; i < mChips.size(); ++i) {
        if (mChips.at(i)->name() == name) {
            return i;
        }
    }
    return -1;
}

void ChipRow::focusItem(const int index)
{
    if (Chip* pChip = qobject_cast<Chip*>(chipAt(index)); pChip) {
        pChip->setFocus(Qt::OtherFocusReason);
    }
}

void ChipRow::beginAdd()
{
    openField(-1);
}

void ChipRow::setChipWords(const QString& accessibleName, const QString& removeTooltip)
{
    mChipAccessibleName = accessibleName;
    mChipRemoveTooltip = removeTooltip;
    for (Chip* pChip : std::as_const(mChips)) {
        pChip->setWords(accessibleName, removeTooltip);
    }
}

void ChipRow::setAddWords(const QString& button, const QString& tooltip, const QString& fieldPrompt)
{
    mpAdd->setText(button);
    mpAdd->setToolTip(utils::richText(tooltip));
    mpField->setPlaceholderText(fieldPrompt);
    remeasure();
}

int ChipRow::lineHeight() const
{
    const int fieldHeight = mpField ? mpField->sizeHint().height() : 0;
    return std::max(chipHeightOn(this), fieldHeight);
}

QSize ChipRow::sizeHint() const
{
    const QSize hint = QWidget::sizeHint();
    // Whatever holds the row reads its hint rather than heightForWidth(), so without the
    // wrapped height here a second line of chips is drawn over whatever is under it
    if (width() <= 0) {
        return hint;
    }
    return QSize(hint.width(), std::max(0, heightForWidth(width())));
}

bool ChipRow::hasHeightForWidth() const
{
    return true;
}

int ChipRow::heightForWidth(int width) const
{
    return mpFlow->totalHeightForWidth(width);
}

void ChipRow::remeasure()
{
    mpAdd->setFont(chipFont(this));
    const int line = lineHeight();
    mpAdd->setFixedHeight(line);
    mpField->setFixedHeight(line);
    const int glyphSize = chipGlyphSizeOn(this);
    mpAdd->setIconSize(QSize(glyphSize, glyphSize));
    for (Chip* pChip : mChips) {
        pChip->remeasure();
    }
    measureSuggestions();
    updateGeometry();
}

void ChipRow::changeEvent(QEvent* pEvent)
{
    QWidget::changeEvent(pEvent);
    // A sheet landing on the form changes the field's height, and the line with it
    if (pEvent->type() == QEvent::FontChange || pEvent->type() == QEvent::StyleChange) {
        remeasure();
    }
}

void ChipRow::resizeEvent(QResizeEvent* pEvent)
{
    QWidget::resizeEvent(pEvent);
    if (pEvent->oldSize().width() != pEvent->size().width()) {
        // A new width can wrap to a new number of lines, and the holder only re-reads the hint when told
        updateGeometry();
    }
}

bool ChipRow::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (mpCompleter && pWatched == mpCompleter->popup()) {
        // QCompleter forwards keys it does not want by calling widget->event() directly, which
        // bypasses the field's filter below. With nothing highlighted Return commits the typed
        // name, so an unlisted one is still one Return away; a highlighted row comes via activated().
        if (pEvent->type() == QEvent::KeyPress) {
            auto* pKey = static_cast<QKeyEvent*>(pEvent);
            const bool returning = pKey->key() == Qt::Key_Return || pKey->key() == Qt::Key_Enter;
            if (returning && !mpCompleter->popup()->currentIndex().isValid()) {
                mpCompleter->popup()->hide();
                commitField(true);
                return true;
            }
        }
        return QWidget::eventFilter(pWatched, pEvent);
    }
    if (pWatched != mpField) {
        return QWidget::eventFilter(pWatched, pEvent);
    }
    if (pEvent->type() == QEvent::KeyPress) {
        auto* pKey = static_cast<QKeyEvent*>(pEvent);
        switch (pKey->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            commitField(true);
            return true;
        case Qt::Key_Escape:
            closeField();
            return true;
        default:
            break;
        }
    } else if (pEvent->type() == QEvent::FocusOut && mFieldOpen && !mCommitting) {
        if (cleaned(mpField->text()).isEmpty()) {
            closeField();
        } else {
            commitField(false);
        }
    }
    return QWidget::eventFilter(pWatched, pEvent);
}

Chip* ChipRow::makeChip(const QString& name)
{
    auto* pChip = new Chip(name, this);
    if (!mChipAccessibleName.isEmpty()) {
        pChip->setWords(mChipAccessibleName, mChipRemoveTooltip);
    }
    pChip->setRemoveGlyph(mRemoveGlyph);
    markOrigin(pChip);
    connect(pChip, &Chip::removeRequested, this, [this, pChip]() {
        removeAt(mChips.indexOf(pChip));
    });
    connect(pChip, &Chip::editRequested, this, [this, pChip]() {
        openField(mChips.indexOf(pChip));
    });
    return pChip;
}

void ChipRow::rebuild()
{
    while (QLayoutItem* pItem = mpFlow->takeAt(0)) {
        delete pItem;
    }

    bool fieldPlaced = false;
    for (int i = 0; i < mChips.size(); ++i) {
        Chip* pChip = mChips.at(i);
        const bool standingIn = mFieldOpen && i == mEditingIndex;
        if (standingIn) {
            mpFlow->addWidget(mpField);
            fieldPlaced = true;
        }
        pChip->setVisible(!standingIn);
        mpFlow->addWidget(pChip);
    }
    if (mFieldOpen && !fieldPlaced) {
        mpFlow->addWidget(mpField);
    }
    mpFlow->addWidget(mpAdd);
    mpFlow->addWidget(mpNote);

    mpField->setVisible(mFieldOpen);
    mpAdd->setVisible(!mFieldOpen);

    QWidget* pPrevious = nullptr;
    for (Chip* pChip : mChips) {
        if (pPrevious) {
            setTabOrder(pPrevious, pChip);
        }
        pPrevious = pChip;
    }
    if (pPrevious) {
        setTabOrder(pPrevious, mpAdd);
    }

    mpFlow->invalidate();
    updateGeometry();
}

void ChipRow::openField(const int index)
{
    emit fieldOpening();
    hideNote();
    mEditingIndex = index;
    mFieldOpen = true;
    refreshSuggestionModel();
    const Chip* pChip = qobject_cast<Chip*>(chipAt(index));
    mpField->setText(pChip ? pChip->name() : QString());
    rebuild();
    mpField->setFocus(Qt::OtherFocusReason);
    mpField->selectAll();
}

void ChipRow::closeField(const bool keepNote)
{
    if (!mFieldOpen) {
        return;
    }
    mFieldOpen = false;
    mEditingIndex = -1;
    mpField->clear();
    // The popup is a window of its own and stays up when the field hides
    if (mpCompleter) {
        mpCompleter->popup()->hide();
    }
    if (!keepNote) {
        hideNote();
    }
    rebuild();
}

// stillTyping is Return or a typed comma, which keep the field open; false is a focus-out
void ChipRow::commitField(const bool stillTyping)
{
    if (!mFieldOpen || mCommitting) {
        return;
    }
    const QString name = cleaned(mpField->text());
    const int editing = mEditingIndex;
    Chip* pEdited = qobject_cast<Chip*>(chipAt(editing));

    if (name.isEmpty()) {
        closeField();
        return;
    }
    if (pEdited && pEdited->name() == name) {
        closeField();
        return;
    }
    for (int i = 0; i < mChips.size(); ++i) {
        if (i != editing && mChips.at(i)->name() == name) {
            showNote(name);
            emit duplicateRefused(name);
            if (!stillTyping) {
                // Left open after a focus-out, the field would stand unfocused with the add button
                // hidden behind it. The note stays, as it is what says why nothing was added.
                closeField(true);
            }
            return;
        }
    }

    mCommitting = true;
    if (pEdited) {
        pEdited->setName(name);
        markOrigin(pEdited);
        mCommitting = false;
        closeField();
        emit itemsChanged();
        return;
    }

    mChips.append(makeChip(name));
    if (stillTyping) {
        mpField->clear();
        // The new chip leaves the offer now, and the popup was answering the old text
        if (mpCompleter) {
            mpCompleter->popup()->hide();
        }
        refreshSuggestionModel();
        rebuild();
        mpField->setFocus(Qt::OtherFocusReason);
    } else {
        mFieldOpen = false;
        mEditingIndex = -1;
        mpField->clear();
        rebuild();
    }
    mCommitting = false;
    emit itemsChanged();
}

void ChipRow::removeAt(const int index)
{
    if (index < 0 || index >= mChips.size()) {
        return;
    }
    // The cross takes no focus, so an open field still holds the keyboard and the focus move below would commit through its FocusOut.
    // Settled first; a rename keeps the list's length, so index still holds.
    if (mFieldOpen) {
        if (cleaned(mpField->text()).isEmpty()) {
            closeField();
        } else {
            commitField(false);
        }
    }

    // The cross that asked for this is the chip's child and its signal is still on the stack
    Chip* pGoing = mChips.takeAt(index);
    pGoing->hide();
    pGoing->deleteLater();
    rebuild();

    // Something must hold the keyboard, or a run of Delete presses stops after the first
    mCommitting = true;
    if (!mChips.isEmpty()) {
        mChips.at(std::min(static_cast<qsizetype>(index), mChips.size() - 1))->setFocus(Qt::OtherFocusReason);
    } else if (!mFieldOpen) {
        mpAdd->setFocus(Qt::OtherFocusReason);
    }
    mCommitting = false;
    emit itemsChanged();
}

void ChipRow::showNote(const QString& name)
{
    //: Note beside a script's event field when the name typed is already one of its events
    mpNote->setText(tr("%1 is already listed").arg(name));
    mpNote->show();
    mpNoteTimer->start();
    rebuild();
    mpField->selectAll();
}

void ChipRow::hideNote()
{
    mpNoteTimer->stop();
    if (!mpNote->isHidden()) {
        mpNote->hide();
        rebuild();
    }
}

QString ChipRow::cleaned(const QString& name)
{
    QString trimmed = name.trimmed();
    if (trimmed.endsWith(QLatin1Char(','))) {
        trimmed.chop(1);
    }
    return trimmed.trimmed();
}

// A stylesheet draws a list row as one string, so the name and its note are painted here
// over the background and selection wash the style still draws
class SuggestionDelegate final : public QStyledItemDelegate
{
public:
    Q_DISABLE_COPY(SuggestionDelegate)
    explicit SuggestionDelegate(QObject* pParent)
    : QStyledItemDelegate(pParent)
    {
    }

    void setNoteColors(const QColor& resting, const QColor& chosen)
    {
        mNote = resting;
        mNoteChosen = chosen;
    }

    void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QString name = opt.text;
        // Cleared so the style paints only the row; both words are drawn below in their own inks
        opt.text.clear();
        const QWidget* pOn = opt.widget;
        QStyle* pStyle = pOn ? pOn->style() : QApplication::style();
        pStyle->drawControl(QStyle::CE_ItemViewItem, &opt, pPainter, pOn);

        const bool chosen = opt.state.testFlag(QStyle::State_Selected);
        const QRect inside = opt.rect.adjusted(scmSuggestionPaddingHorizontal, scmSuggestionPaddingVertical, -scmSuggestionPaddingHorizontal, -scmSuggestionPaddingVertical);
        const QFontMetrics metrics(opt.font);
        const QString note = index.data(scmSuggestionNoteRole).toString();

        pPainter->save();
        pPainter->setFont(opt.font);
        int noteRoom = 0;
        if (!note.isEmpty()) {
            noteRoom = metrics.horizontalAdvance(note) + scmSuggestionNoteGap;
            const QColor ink = chosen ? mNoteChosen : mNote;
            if (ink.isValid()) {
                pPainter->setPen(ink);
                pPainter->drawText(inside, Qt::AlignRight | Qt::AlignVCenter, note);
            }
        }
        const QRect nameRoom = inside.adjusted(0, 0, -noteRoom, 0);
        pPainter->setPen(opt.palette.color(QPalette::Normal, chosen ? QPalette::HighlightedText : QPalette::Text));
        pPainter->drawText(nameRoom, Qt::AlignLeft | Qt::AlignVCenter, metrics.elidedText(name, Qt::ElideRight, nameRoom.width()));
        pPainter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QFontMetrics metrics(opt.font);
        const QString note = index.data(scmSuggestionNoteRole).toString();
        int width = metrics.horizontalAdvance(opt.text) + 2 * scmSuggestionPaddingHorizontal;
        if (!note.isEmpty()) {
            width += scmSuggestionNoteGap + metrics.horizontalAdvance(note);
        }
        return QSize(width, metrics.height() + 2 * scmSuggestionPaddingVertical);
    }

private:
    QColor mNote;
    QColor mNoteChosen;
};

// Not in the constructor: Qt runs the filter installed last first, and the filter QCompleter
// installs in setWidget() must swallow the FocusOut the popup causes before the row's own
// filter commits the half-typed name on it
void ChipRow::makeCompleter()
{
    if (mpCompleter) {
        return;
    }
    mpCompleter = new QCompleter(this);
    mpSuggestionModel = new QStandardItemModel(mpCompleter);
    mpCompleter->setModel(mpSuggestionModel);
    mpCompleter->setCompletionMode(QCompleter::PopupCompletion);
    // What the user remembers of an event is as often its end as its beginning
    mpCompleter->setFilterMode(Qt::MatchContains);
    mpCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    // Explicit because each row also carries its note in another role
    mpCompleter->setCompletionRole(Qt::DisplayRole);
    mpField->setCompleter(mpCompleter);

    QAbstractItemView* pPopup = mpCompleter->popup();
    pPopup->installEventFilter(this);
    if (!mSuggestionSheet.isEmpty()) {
        pPopup->setStyleSheet(mSuggestionSheet);
    }
    // After setCompleter(), which creates the popup and puts QCompleter's own delegate on it
    mpSuggestionDelegate = new SuggestionDelegate(pPopup);
    pPopup->setItemDelegate(mpSuggestionDelegate);
    inkSuggestionNotes();

    // Queued: QLineEdit's own direct connection to this signal puts the chosen name in the
    // field, and anything reading the field has to come after it
    connect(
            mpCompleter,
            qOverload<const QString&>(&QCompleter::activated),
            this,
            [this](const QString& name) {
                if (!mFieldOpen) {
                    return;
                }
                mpField->setText(name);
                commitField(true);
            },
            Qt::QueuedConnection);
}

void ChipRow::refreshSuggestionModel()
{
    if (!mpSuggestionModel) {
        return;
    }
    QStringList taken;
    taken.reserve(mChips.size());
    for (int i = 0; i < mChips.size(); ++i) {
        // So a rename can keep or re-pick its own name
        if (i != mEditingIndex) {
            taken << mChips.at(i)->name();
        }
    }
    mpSuggestionModel->removeRows(0, mpSuggestionModel->rowCount());
    for (const Suggestion& suggestion : mSuggestions) {
        if (taken.contains(suggestion.name)) {
            continue;
        }
        auto* pRow = new QStandardItem(suggestion.name);
        pRow->setData(suggestion.note, scmSuggestionNoteRole);
        pRow->setEditable(false);
        mpSuggestionModel->appendRow(pRow);
    }
}

void ChipRow::setSuggestions(const QList<Suggestion>& suggestions)
{
    mSuggestions = suggestions;
    // The chips can be made before the names they hold are known
    for (Chip* pChip : mChips) {
        markOrigin(pChip);
    }
    makeCompleter();
    refreshSuggestionModel();
    measureSuggestions();
}

void ChipRow::setUnlistedOrigin(const QString& origin)
{
    mUnlistedOrigin = origin;
    for (Chip* pChip : mChips) {
        markOrigin(pChip);
    }
}

void ChipRow::markOrigin(Chip* pChip) const
{
    for (const Suggestion& suggestion : mSuggestions) {
        if (suggestion.name == pChip->name()) {
            pChip->setOrigin(suggestion.origin, suggestion.system);
            return;
        }
    }
    pChip->setOrigin(mUnlistedOrigin, false);
}

void ChipRow::measureSuggestions()
{
    if (!mpCompleter) {
        return;
    }
    const QFont font = chipFont(this);
    mpCompleter->popup()->setFont(font);

    // The popup takes the narrow field's width unless given a minimum, which QCompleter's
    // showPopup() respects and keeps on screen. Widest name plus widest note, as the two are rarely on one row.
    const QFontMetrics metrics(font);
    int widestName = 0;
    int widestNote = 0;
    for (const Suggestion& suggestion : mSuggestions) {
        widestName = std::max(widestName, metrics.horizontalAdvance(suggestion.name));
        widestNote = std::max(widestNote, metrics.horizontalAdvance(suggestion.note));
    }
    const int widest = widestName + (widestNote > 0 ? scmSuggestionNoteGap + widestNote : 0);
    mpCompleter->popup()->setMinimumWidth(widest + scmSuggestionSlack);
}

void ChipRow::inkSuggestionNotes()
{
    if (mpSuggestionDelegate && mSuggestionNote.isValid()) {
        mpSuggestionDelegate->setNoteColors(mSuggestionNote, mSuggestionNoteChosen);
    }
}

void ChipRow::restyleSuggestions(const ThemeTokens& tokens)
{
    // No item padding: the delegate pads the words itself, and sheet padding would move the
    // background without moving them
    mSuggestionSheet = qsl("QListView { background-color: %1; color: %2; border: 1px solid %3;"
                           " selection-background-color: %4; selection-color: %5; outline: none; }")
                               .arg(tokens.field.name(), tokens.text.name(), tokens.border.name(), tokens.accentSoft, tokens.accentText.name());
    mSuggestionNote = tokens.mutedText;
    mSuggestionNoteChosen = tokens.accentText;
    if (mpCompleter) {
        mpCompleter->popup()->setStyleSheet(mSuggestionSheet);
    }
    inkSuggestionNotes();
}

void ChipRow::restyleGlyphs(const ThemeTokens& tokens)
{
    mRemoveGlyph = tintedIcon(qsl(":/icons/editor-clear.svg"), tokens);
    for (Chip* pChip : mChips) {
        pChip->setRemoveGlyph(mRemoveGlyph);
    }
    mpAdd->setIcon(tintedIcon(qsl(":/icons/editor-add.svg"), tokens));
    // A control's contrast rather than a chip's hairline, matching the Add pattern button's frame
    mpAdd->setFrameColors(blend(tokens.card, tokens.text, 0.45), tokens.accent, blend(tokens.card, tokens.text, 0.22));
}

QString ChipRow::styleSheetFor(const ThemeTokens& tokens)
{
    // The error red was measured on a darker strip than the page, so it is walked to the floor here
    const QColor noteColor = readableOn(tokens.page, errorInk(tokens), tokens.text, scmTextMinimumRatio);
    return qsl("#editorChip { background-color: %1; border: 1px solid %2; border-radius: %3px; }"
               "#editorChip QLabel { background: transparent; color: %4; }"
               "#editorChip[editorChipSystem=\"true\"] QLabel { color: %5; }"
               "#editorChip:focus { border: 1px solid %6; }"
               "#editorChipRemove { border: none; border-radius: %3px; background: transparent; padding: 0px; }"
               "#editorChipRemove:hover { background-color: %7; }"
               // PlaceholderButton paints the dashes. The extra padding pixel stands in for a
               // chip's hairline, so the dashes land on it and both boxes come out one size.
               "#editorChipAdd { border: none; border-radius: %3px; background: transparent; color: %5;"
               " padding: %8px %9px %8px %10px; }"
               "#editorChipAdd:hover { color: %4; }"
               "#editorChipNote { background: transparent; color: %11; }")
            .arg(tokens.card.name(),
                 tokens.border.name(),
                 QString::number(scmRadiusChip),
                 tokens.text.name(),
                 tokens.mutedText.name(),
                 tokens.accent.name(),
                 tokens.hoverSoft,
                 QString::number(scmChipPaddingVertical + 1),
                 QString::number(scmChipPaddingTrailing + 1))
            .arg(QString::number(scmChipPaddingLeading + 1), noteColor.name());
}

} // namespace uiDesign
