#include "map/handlers/SelectionRectangleHandler.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "utils.h"

#include "pre_guard.h"
#include <QMouseEvent>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QtGlobal>
#include <QTreeWidgetItem>
#include "post_guard.h"

SelectionRectangleHandler::SelectionRectangleHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool SelectionRectangleHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return context.button == Qt::LeftButton
            && !context.isCustomLineDrawing
            && !context.isRoomBeingMoved;
    case QEvent::MouseMove:
        return context.isMultiSelectionActive || context.isSizingLabel;
    case QEvent::MouseButtonRelease:
        return context.button == Qt::LeftButton
            && (context.isMultiSelectionActive || context.isSizingLabel);
    default:
        return false;
    }
}

bool SelectionRectangleHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!context.event) {
        return false;
    }

    switch (context.event->type()) {
    case QEvent::MouseButtonPress:
        return handleMousePress(context);
    case QEvent::MouseMove:
        return handleMouseMove(context);
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(context);
    default:
        return false;
    }
}

bool SelectionRectangleHandler::handleMousePress(T2DMap::MapInteractionContext& context) const
{
    if (context.button != Qt::LeftButton) {
        return false;
    }

    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    mMapWidget.mPopupMenu = false;
    mMapWidget.mMultiSelection = !mMapWidget.mMapViewOnly;
    mMapWidget.mMultiRect = QRect(context.widgetPosition, context.widgetPosition);

    if (!mMapWidget.mpMap->mpRoomDB->getRoom(mMapWidget.mRoomID)) {
        return true;
    }

    auto* area = context.area;
    if (!area) {
        return true;
    }

    if (!context.modifiers.testFlag(Qt::ControlModifier)) {
        if (!mMapWidget.mMapViewOnly) {
            mMapWidget.mHelpMsg = T2DMap::tr("Drag to select multiple rooms or labels, release to finish...");
        }
        mMapWidget.mMultiSelectionSet.clear();
    }

    mMapWidget.prepareSingleClickSelection(context);

    if (!mMapWidget.mMultiSelectionSet.empty()) {
        mMapWidget.mHelpMsg.clear();
    }

    if (mMapWidget.mMultiSelection && !mMapWidget.mMultiSelectionSet.empty() && context.modifiers.testFlag(Qt::ControlModifier)) {
        mMapWidget.mMultiSelection = false;
        mMapWidget.mHelpMsg.clear();
    }

    return false;
}

bool SelectionRectangleHandler::handleMouseMove(T2DMap::MapInteractionContext& context) const
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    if (!(mMapWidget.mMultiSelection && !mMapWidget.mRoomBeingMoved) && !mMapWidget.mSizeLabel) {
        return false;
    }

    if (mMapWidget.mNewMoveAction) {
        mMapWidget.mMultiRect = QRect(context.widgetPosition, context.widgetPosition);
        mMapWidget.mNewMoveAction = false;
    } else {
        mMapWidget.mMultiRect.setBottomLeft(context.widgetPosition);
    }

    if (!mMapWidget.mpMap->mpRoomDB->getRoom(mMapWidget.mRoomID)) {
        return true;
    }

    auto* area = mMapWidget.mpMap->mpRoomDB->getArea(mMapWidget.mAreaID);
    if (!area) {
        return true;
    }

    if (!mMapWidget.mSizeLabel) {
        mMapWidget.mMultiSelectionSet.clear();
        const float fx = mMapWidget.xspan / 2.0f * mMapWidget.mRoomWidth - mMapWidget.mRoomWidth * static_cast<float>(mMapWidget.mMapCenterX);
        const float fy = mMapWidget.yspan / 2.0f * mMapWidget.mRoomHeight - mMapWidget.mRoomHeight * static_cast<float>(mMapWidget.mMapCenterY);
        QSetIterator<int> itSelectedRoom(area->getAreaRooms());
        while (itSelectedRoom.hasNext()) {
            const int currentRoomId = itSelectedRoom.next();
            TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(currentRoomId);
            if (!room) {
                continue;
            }

            if ((room->z() != mMapWidget.mMapCenterZ) && !context.modifiers.testFlag(Qt::ShiftModifier)) {
                continue;
            }

            const float rx = static_cast<float>(room->x()) * mMapWidget.mRoomWidth + fx;
            const float ry = static_cast<float>(room->y() * -1) * mMapWidget.mRoomHeight + fy;
            QRectF dr;
            if (area->gridMode) {
                dr = QRectF(static_cast<qreal>(rx - (mMapWidget.mRoomWidth / 2.0f)), static_cast<qreal>(ry - (mMapWidget.mRoomHeight / 2.0f)), static_cast<qreal>(mMapWidget.mRoomWidth), static_cast<qreal>(mMapWidget.mRoomHeight));
            } else {
                dr = QRectF(static_cast<qreal>(rx - mMapWidget.mRoomWidth * static_cast<float>(mMapWidget.rSize / 2.0)), static_cast<qreal>(ry - mMapWidget.mRoomHeight * static_cast<float>(mMapWidget.rSize / 2.0)), static_cast<qreal>(mMapWidget.mRoomWidth) * mMapWidget.rSize, static_cast<qreal>(mMapWidget.mRoomHeight) * mMapWidget.rSize);
            }
            if (mMapWidget.mMultiRect.contains(dr)) {
                mMapWidget.mMultiSelectionSet.insert(currentRoomId);
            }
        }
        switch (mMapWidget.mMultiSelectionSet.size()) {
        case 0:
            mMapWidget.mMultiSelectionHighlightRoomId = 0;
            break;
        case 1:
            mMapWidget.mMultiSelectionHighlightRoomId = *(mMapWidget.mMultiSelectionSet.begin());
            break;
        default:
            mMapWidget.getCenterSelection();
            break;
        }

        if (mMapWidget.mMultiSelectionSet.size() > 1) {
            populateMultiSelectionWidget();
        } else {
            mMapWidget.mMultiSelectionListWidget.hide();
        }
    }

    mMapWidget.update();

    return true;
}

bool SelectionRectangleHandler::handleMouseRelease(T2DMap::MapInteractionContext& context) const
{
    if (context.button != Qt::LeftButton) {
        return false;
    }

    mMapWidget.mMultiSelection = false;
    mMapWidget.mHelpMsg.clear();

    if (mMapWidget.mSizeLabel) {
        mMapWidget.mSizeLabel = false;
        const QRectF labelRect = mMapWidget.mMultiRect;
        mMapWidget.createLabel(labelRect);
    }

    mMapWidget.mMultiRect = QRect(0, 0, 0, 0);
    mMapWidget.updateSelectionWidget();
    mMapWidget.update();

    return true;
}

void SelectionRectangleHandler::populateMultiSelectionWidget() const
{
    mMapWidget.mMultiSelectionListWidget.blockSignals(true);
    mMapWidget.mIsSelectionSorting = mMapWidget.mMultiSelectionListWidget.isSortingEnabled();
    mMapWidget.mIsSelectionSortByNames = (mMapWidget.mMultiSelectionListWidget.sortColumn() == 1);
    mMapWidget.mMultiSelectionListWidget.clear();
    mMapWidget.mMultiSelectionListWidget.setSortingEnabled(false);
    QSetIterator<int> itRoom = mMapWidget.mMultiSelectionSet;
    mMapWidget.mIsSelectionUsingNames = false;
    while (itRoom.hasNext()) {
        auto item = new QTreeWidgetItem;
        const int multiSelectionRoomId = itRoom.next();
        item->setText(0, qsl("%1").arg(multiSelectionRoomId, mMapWidget.mMaxRoomIdDigits));
        item->setTextAlignment(0, Qt::AlignRight);
        TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
        if (room) {
            const QString roomName = room->name;
            if (!roomName.isEmpty()) {
                item->setText(1, roomName);
                item->setTextAlignment(1, Qt::AlignLeft);
                mMapWidget.mIsSelectionUsingNames = true;
            }
        }
        mMapWidget.mMultiSelectionListWidget.addTopLevelItem(item);
    }

    mMapWidget.mMultiSelectionListWidget.setColumnHidden(1, !mMapWidget.mIsSelectionUsingNames);
    if ((!mMapWidget.mIsSelectionUsingNames) && mMapWidget.mIsSelectionSortByNames && mMapWidget.mIsSelectionSorting) {
        mMapWidget.mIsSelectionSortByNames = false;
    }
    mMapWidget.mMultiSelectionListWidget.sortByColumn(mMapWidget.mIsSelectionSortByNames ? 1 : 0, Qt::AscendingOrder);
    mMapWidget.mMultiSelectionListWidget.setSortingEnabled(mMapWidget.mIsSelectionSorting);
    mMapWidget.resizeMultiSelectionWidget();
    mMapWidget.mMultiSelectionListWidget.selectAll();
    mMapWidget.mMultiSelectionListWidget.blockSignals(false);
    mMapWidget.mMultiSelectionListWidget.show();
}
