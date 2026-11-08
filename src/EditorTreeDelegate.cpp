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

#include "EditorTreeDelegate.h"

#include "EditorTreeRowMetrics.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "uiDesign.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QHelpEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QToolTip>

namespace uiDesign {

namespace {
constexpr int scmTypeLabelGap = 12;
constexpr int scmTypeLabelMinimumCharacters = 4;
} // namespace

EditorTreeDelegate::EditorTreeDelegate(TTreeWidget* pTree, const TreeType treeType, Host* pHost)
: EditorTreeRowDelegate(pTree)
, mTreeType(treeType)
, mpHost(pHost)
{
    restyle();
    // A row inside a closed group is not on screen, so its change would leave the group's count stale
    if (pTree && treeType != TreeType::None) {
        connect(pTree->model(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft) {
            if (topLeft.parent().isValid() && mpTree) {
                mpTree->viewport()->update();
            }
        });
    }
}

void EditorTreeDelegate::restyleInks(const ThemeTokens& tokens)
{
    mRunningDot = stateColor(scmStateHue_ok, tokens.darkPage);
    mQuietDot = tokens.mutedText;
    // The dot alone is too small to find a switched-off row by in a long list
    mNotRunningText = tokens.disabledText;
    mMarkInk = tokens.mutedText;
    mErrorMarkInk = errorInk(tokens);
    // Must match the colour the trees' stylesheet writes a chosen row's name in
    mSelectedMarkInk = tokens.accentText;
    mPackageInk = mColouredMarks ? identityInk(scmIdentity_packages, tokens) : QColor();
    mPackageFill = mColouredMarks ? identityFill(scmIdentity_packages, tokens) : QColor();
}

void EditorTreeDelegate::clearGlyphs() const
{
    for (auto& cached : mDotGlyphs) {
        cached = QPixmap();
    }
    for (auto& cached : mMarkGlyphs) {
        cached = QPixmap();
    }
}

void EditorTreeDelegate::setNewDescriptions(const QString& item, const QString& folder)
{
    mNewItemDescription = item;
    mNewFolderDescription = folder;
}

EditorTreeDelegate::ItemState EditorTreeDelegate::stateOf(const QModelIndex& index) const
{
    ItemState state;
    if (mpHost.isNull() || !index.isValid()) {
        return state;
    }
    const QVariant idData = index.data(Qt::UserRole);
    const TreeType treeType = mTreeType == TreeType::None ? static_cast<TreeType>(index.data(scmRole_rowTreeType).toInt()) : mTreeType;
    if (!idData.isValid() || treeType == TreeType::None) {
        if (mTreeType == TreeType::None) {
            state.dotSlot = true;
            state.mark = RowMark::Folder;
        }
        return state;
    }
    const int id = idData.toInt();

    // Not checkIfNew(): mIsNew is cleared only by a save from this editor, so every item loaded
    // from the profile reports itself as new for the whole session
    const QString description = index.data(Qt::AccessibleDescriptionRole).toString();
    const bool newByDescription = !description.isEmpty() && (description == mNewItemDescription || description == mNewFolderDescription);

    const auto markFor = [this, newByDescription](const bool compiles, const bool folder, const QString& packageName, const QString& name, const RowMark kind) {
        if (!compiles) {
            return RowMark::Error;
        }
        if (newByDescription) {
            return folder ? RowMark::NewFolder : RowMark::NewItem;
        }
        if (kind != RowMark::None) {
            return kind;
        }
        // The Lua ancestors API's test for a package's top folder; a module's master folder is not a package
        if (folder && !packageName.isEmpty() && packageName == name && !mpHost->mInstalledModules.contains(packageName)) {
            return RowMark::Package;
        }
        return folder ? RowMark::Folder : RowMark::None;
    };

    bool wantedOn = false;
    bool running = false;
    RowMark mark = RowMark::None;
    switch (treeType) {
    case TreeType::Trigger: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        running = pT->isActive() && pT->ancestorsActive();
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), pT->isFilterChain() ? RowMark::Filter : RowMark::None);
        break;
    }
    case TreeType::Alias: {
        TAlias* pT = mpHost->getAliasUnit()->getAlias(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        running = pT->isActive() && pT->ancestorsActive();
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), RowMark::None);
        break;
    }
    case TreeType::Timer: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        // An offset timer's isActive() is false between firings, and ancestorsActive() walks that same
        // flag; shouldAncestorsBeActive() is what TLuaInterpreter's isActive() reads for one
        running = pT->isOffsetTimer() ? (pT->shouldBeActive() && pT->shouldAncestorsBeActive()) : (pT->isActive() && pT->ancestorsActive());
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), pT->isOffsetTimer() ? RowMark::OffsetTimer : RowMark::None);
        break;
    }
    case TreeType::Script: {
        TScript* pT = mpHost->getScriptUnit()->getScript(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        running = pT->isActive() && pT->ancestorsActive();
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), RowMark::None);
        break;
    }
    case TreeType::Action: {
        TAction* pT = mpHost->getActionUnit()->getAction(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        running = pT->isActive() && pT->ancestorsActive();
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), RowMark::None);
        break;
    }
    case TreeType::Key: {
        TKey* pT = mpHost->getKeyUnit()->getKey(id);
        if (!pT) {
            return state;
        }
        wantedOn = pT->shouldBeActive();
        running = pT->isActive() && pT->ancestorsActive();
        mark = markFor(pT->state(), pT->isFolder(), pT->mPackageName, pT->getName(), RowMark::None);
        break;
    }
    default:
        return state;
    }

    state.known = true;
    state.dot = running ? DotState::Running : (wantedOn ? DotState::WantedOn : DotState::Off);
    state.mark = mark;
    return state;
}

int EditorTreeDelegate::rowLevelOf(const QModelIndex& index) const
{
    return treeRowLevelOf(index) + (mTreeType == TreeType::None ? 1 : 0);
}

bool EditorTreeDelegate::carriesDot(const QModelIndex& index) const
{
    return stateOf(index).known;
}

void EditorTreeDelegate::dotPressed()
{
    emit toggleRequested();
}

QPixmap EditorTreeDelegate::dotGlyph(const DotState state, const bool broken) const
{
    syncGlyphRatio();
    const qreal ratio = mGlyphRatio;

    QPixmap& glyph = mDotGlyphs[(broken ? scmDotStateCount : 0) + static_cast<int>(state)];
    if (!glyph.isNull()) {
        return glyph;
    }

    TreeDotForm form = TreeDotForm::Ring;
    if (state == DotState::Running) {
        form = TreeDotForm::Filled;
    } else if (state == DotState::WantedOn) {
        form = TreeDotForm::RingWithCentre;
    }
    const QColor ink = broken ? mErrorMarkInk : (state == DotState::Running ? mRunningDot : mQuietDot);
    glyph = treeRowDotGlyph(form, ink, ratio);
    return glyph;
}

QPixmap EditorTreeDelegate::markGlyph(const RowMark mark, const bool selected) const
{
    QPixmap& glyph = mMarkGlyphs[2 * static_cast<int>(mark) + (selected ? 1 : 0)];
    if (!glyph.isNull()) {
        return glyph;
    }

    QString file;
    switch (mark) {
    case RowMark::Folder:
        file = qsl(":/icons/editor-folder.svg");
        break;
    case RowMark::Filter:
        file = qsl(":/icons/editor-filter.svg");
        break;
    case RowMark::OffsetTimer:
        file = qsl(":/icons/editor-offset-timer.svg");
        break;
    case RowMark::Error:
        file = qsl(":/icons/editor-errors.svg");
        break;
    case RowMark::NewFolder:
        file = qsl(":/icons/editor-new-folder.svg");
        break;
    case RowMark::NewItem:
        file = qsl(":/icons/editor-new-item.svg");
        break;
    case RowMark::Package:
        file = qsl(":/icons/packages-package.svg");
        break;
    case RowMark::None:
        return glyph;
    }

    const qreal ratio = mGlyphRatio;
    QPixmap source;
    if (mark == RowMark::Package && mPackageInk.isValid()) {
        // The colour stays on a chosen row, as it does on the bars
        source = tintedGlyph(glyphFillPixmap(file), mPackageFill);
        QPainter painter(&source);
        painter.drawPixmap(0, 0, tintedGlyph(uiDesign::glyphPixmap(file), mPackageInk));
        painter.end();
    } else {
        source = tintedGlyph(uiDesign::glyphPixmap(file), mark == RowMark::Error ? mErrorMarkInk : (selected ? mSelectedMarkInk : mMarkInk));
    }
    glyph = source.scaled(QSize(scmTreeMarkSize, scmTreeMarkSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph.setDevicePixelRatio(ratio);
    return glyph;
}

EditorTreeDelegate::Decoration EditorTreeDelegate::decorationFor(const ItemState& state, const int level, const ChevronState chevron, const bool selected) const
{
    syncGlyphRatio();

    const int key = (state.known ? 1 + static_cast<int>(state.dot) : 0) | (static_cast<int>(chevron) << 2) | (static_cast<int>(state.mark) << 4) | ((selected ? 1 : 0) << 7)
                    | ((state.dotSlot ? 1 : 0) << 8) | (level << 9);
    if (const auto cached = mDecorations.constFind(key); cached != mDecorations.constEnd()) {
        return cached.value();
    }
    return composeDecoration(key, level, chevron, state.known || state.dotSlot, state.known ? dotGlyph(state.dot, state.mark == RowMark::Error) : QPixmap(), markGlyph(state.mark, selected));
}

void EditorTreeDelegate::initStyleOption(QStyleOptionViewItem* pOption, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(pOption, index);

    const ItemState state = stateOf(index);
    const int level = rowLevelOf(index);
    const ChevronState chevron = chevronOf(index, level);
    // A tree's heading keeps the glyph restyleEditorTreeHeadingIcons() put on it, at the mark size
    // so it is as tall as the rows it heads
    if (!state.known && level < 1 && chevron == ChevronState::None) {
        pOption->decorationSize = QSize(scmTreeMarkSize, scmTreeMarkSize);
        return;
    }

    const Decoration decoration = decorationFor(state, level, chevron, pOption->state & QStyle::State_Selected);

    pOption->decorationSize = decoration.size;
    pOption->icon = decoration.icon;
    pOption->features |= QStyleOptionViewItem::HasDecoration;

    // Effective only because the trees' stylesheet names no colour for an unselected row
    if (state.known && state.dot != DotState::Running && !(pOption->state & QStyle::State_Selected)) {
        pOption->palette.setColor(QPalette::Text, mNotRunningText);
        pOption->palette.setColor(QPalette::WindowText, mNotRunningText);
    }
    // Only the row's own switch: one on inside a group that is off has nothing wrong with it to find
    if (state.known && state.dot == DotState::Off) {
        pOption->font.setStrikeOut(true);
    }
}

int EditorTreeDelegate::switchedOffInside(const QModelIndex& index) const
{
    const QAbstractItemModel* pModel = index.model();
    if (!pModel) {
        return 0;
    }
    int count = 0;
    for (int row = 0, rows = pModel->rowCount(index); row < rows; ++row) {
        const QModelIndex child = pModel->index(row, 0, index);
        const ItemState state = stateOf(child);
        if (state.known && state.dot == DotState::Off) {
            ++count;
        }
        count += switchedOffInside(child);
    }
    return count;
}

QString EditorTreeDelegate::dotToolTip(const QModelIndex& index) const
{
    const ItemState state = stateOf(index);
    if (!state.known) {
        return {};
    }
    const bool broken = state.mark == RowMark::Error;
    const bool group = state.mark == RowMark::Folder || state.mark == RowMark::Package || state.mark == RowMark::NewFolder || (index.model() && index.model()->hasChildren(index));
    QString tip;
    switch (state.dot) {
    case DotState::Running:
        tip = group
                      //: Tooltip on the dot of a running group in the script editor's trees
                      ? tr("Running. Click to switch it off, and everything inside it with it.")
                      //: Tooltip on the dot of a running item in the script editor's trees
                      : tr("Running. Click to switch it off.");
        break;
    case DotState::Off:
        if (broken) {
            //: Tooltip on the dot of a switched-off item in the script editor's trees whose code has an error
            tip = tr("Switched off, and its code has an error. Click to switch it on.");
        } else {
            tip = group
                          //: Tooltip on the dot of a switched-off group in the script editor's trees
                          ? tr("Switched off, so nothing inside it runs. Click to switch it on.")
                          //: Tooltip on the dot of a switched-off item in the script editor's trees
                          : tr("Switched off. Click to switch it on.");
        }
        break;
    case DotState::WantedOn: {
        if (broken) {
            //: Tooltip on the dot of an item in the script editor's trees that is switched on but whose code has an error
            tip = tr("Switched on, but its code has an error, so it is not running. Click to switch it off.");
            break;
        }
        QString offGroup;
        for (QModelIndex parent = index.parent(); parent.isValid() && offGroup.isEmpty(); parent = parent.parent()) {
            const ItemState parentState = stateOf(parent);
            if (parentState.known && parentState.dot == DotState::Off) {
                offGroup = parent.data(Qt::DisplayRole).toString();
            }
        }
        if (!offGroup.isEmpty()) {
            //: Tooltip on the dot of an item in the script editor's trees that is switched on inside a group that is off; %1 is that group's name
            tip = tr("Switched on, but not running: the group \"%1\" is switched off. Click to switch it off.").arg(offGroup.toHtmlEscaped());
        } else if (mpHost && mpHost->mEmergencyStop) {
            //: Tooltip on the dot of an item in the script editor's trees that is switched on while the emergency stop holds every trigger, alias, timer and key
            tip = tr("Switched on, but held by the emergency stop. Click to switch it off.");
        } else {
            //: Tooltip on the dot of an item in the script editor's trees that is switched on inside a group that is off
            tip = tr("Switched on, but not running: a group it is in is switched off. Click to switch it off.");
        }
        break;
    }
    }
    // Rich text either way, so a name that looks like markup is shown as written
    return qsl("<p>%1</p>").arg(tip);
}

bool EditorTreeDelegate::helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (pEvent && pView && pEvent->type() == QEvent::ToolTip) {
        const QRect dot = dotHitRect(option, index);
        if (dot.contains(pEvent->pos())) {
            const QString tip = dotToolTip(index);
            if (!tip.isEmpty()) {
                // The dot's rectangle, so the tip goes as the pointer moves on to the name
                QToolTip::showText(pEvent->globalPos(), tip, pView->viewport(), dot);
                return true;
            }
        }
    }
    return QStyledItemDelegate::helpEvent(pEvent, pView, option, index);
}

void EditorTreeDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // The Folders view's trailing edge is the item's type
    const bool closedGroup = mTreeType != TreeType::None && index.column() == 0 && mpTree && index.model() && index.model()->hasChildren(index) && !mpTree->isExpanded(index);
    const int offInside = closedGroup ? switchedOffInside(index) : 0;
    if (offInside > 0) {
        paintRowWithOffCount(pPainter, option, index, offInside);
    } else {
        QStyledItemDelegate::paint(pPainter, option, index);
    }

    if (index.column() != 0 || option.rect.isEmpty()) {
        return;
    }
    if (mTreeType == TreeType::None) {
        paintTypeLabel(pPainter, option, index);
    }
    if (!(option.state & QStyle::State_Selected)) {
        return;
    }

    // Painted rather than a border-left, which the pill's corner radius bends into a bracket
    paintAccentBar(pPainter, option.rect, mAccentBar);
}

// QStyledItemDelegate::paint() with the name elided short of the count, so a long name never runs under it
void EditorTreeDelegate::paintRowWithOffCount(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const int offInside) const
{
    QStyleOptionViewItem rowOption(option);
    initStyleOption(&rowOption, index);
    const QWidget* pWidget = rowOption.widget ? rowOption.widget : mpTree;
    QStyle* pStyle = pWidget ? pWidget->style() : QApplication::style();

    //: Script editor tree, at the right edge of a closed group: how many items inside it are switched off
    const QString count = tr("%n off", nullptr, offInside);
    QFont countFont = rowOption.font;
    countFont.setStrikeOut(false);
    countFont.setPointSize(typeSize(TypeStep::Caption));
    const QFontMetrics countMetrics(countFont);
    const QFontMetrics nameMetrics(rowOption.font);
    // QCommonStyle draws a row's words inside the text rectangle less this much on either side
    const int textMargin = pStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, &rowOption, pWidget) + 1;
    const QRect textRect = pStyle->subElementRect(QStyle::SE_ItemViewItemText, &rowOption, pWidget).adjusted(textMargin, 0, -textMargin, 0);
    const int nameRoom = textRect.width() - countMetrics.horizontalAdvance(count) - scmTypeLabelGap;
    const bool countFits = nameRoom >= scmTypeLabelMinimumCharacters * nameMetrics.horizontalAdvance(QLatin1Char('0'));
    if (countFits) {
        rowOption.text = nameMetrics.elidedText(rowOption.text, rowOption.textElideMode, nameRoom);
    }
    pStyle->drawControl(QStyle::CE_ItemViewItem, &rowOption, pPainter, pWidget);
    if (!countFits) {
        return;
    }
    pPainter->save();
    pPainter->setClipRect(rowOption.rect);
    pPainter->setFont(countFont);
    pPainter->setPen((rowOption.state & QStyle::State_Selected) ? mSelectedMarkInk : mNotRunningText);
    pPainter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, count);
    pPainter->restore();
}

void EditorTreeDelegate::paintTypeLabel(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString label = index.siblingAtColumn(1).data(Qt::DisplayRole).toString();
    if (!mpTree || label.isEmpty()) {
        return;
    }
    QStyleOptionViewItem labelOption(option);
    initStyleOption(&labelOption, index);
    const QWidget* pWidget = labelOption.widget ? labelOption.widget : mpTree;
    QStyle* pStyle = pWidget->style();
    // QCommonStyle draws a row's words inside the text rectangle less this much on either side
    const int textMargin = pStyle->pixelMetric(QStyle::PM_FocusFrameHMargin, &labelOption, pWidget) + 1;
    const QRect textRect = pStyle->subElementRect(QStyle::SE_ItemViewItemText, &labelOption, pWidget).adjusted(textMargin, 0, -textMargin, 0);
    const QFontMetrics metrics(labelOption.font);
    const int room = textRect.width() - metrics.horizontalAdvance(labelOption.text) - scmTypeLabelGap;
    if (room < scmTypeLabelMinimumCharacters * metrics.horizontalAdvance(QLatin1Char('0'))) {
        return;
    }
    pPainter->save();
    pPainter->setClipRect(labelOption.rect);
    // The type is not what is switched off
    QFont labelFont = labelOption.font;
    labelFont.setStrikeOut(false);
    pPainter->setFont(labelFont);
    pPainter->setPen((labelOption.state & QStyle::State_Selected) ? mSelectedMarkInk : mMarkInk);
    pPainter->drawText(QRect(textRect.right() - room + 1, textRect.top(), room, textRect.height()), Qt::AlignRight | Qt::AlignVCenter, metrics.elidedText(label, Qt::ElideRight, room));
    pPainter->restore();
}

} // namespace uiDesign
