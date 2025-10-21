/***************************************************************************
 *   Copyright (C) 2025 by Piotr Wilczynski - delwing@gmail.com            *
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

#include "CustomLineEditContextMenuHandler.h"

#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "utils.h"

#include "pre_guard.h"
#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include "post_guard.h"

CustomLineEditContextMenuHandler::CustomLineEditContextMenuHandler(T2DMap& mapWidget)
: mMapWidget(mapWidget)
{
}

bool CustomLineEditContextMenuHandler::matches(const T2DMap::MapInteractionContext& context) const
{
    if (!context.event || !mMapWidget.mpMap) {
        return false;
    }

    if (context.event->type() != QEvent::MouseButtonRelease) {
        return false;
    }

    if (context.button != Qt::RightButton) {
        return false;
    }

    if (context.isCustomLineDrawing) {
        return false;
    }

    return mMapWidget.mCustomLineSelectedRoom > 0;
}

bool CustomLineEditContextMenuHandler::handle(T2DMap::MapInteractionContext& context)
{
    if (!mMapWidget.mpMap || !mMapWidget.mpMap->mpRoomDB) {
        return false;
    }

    TRoom* room = mMapWidget.mpMap->mpRoomDB->getRoom(mMapWidget.mCustomLineSelectedRoom);
    if (!room) {
        return false;
    }

    auto popup = new QMenu(&mMapWidget);
    popup->setToolTipsVisible(true);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    mMapWidget.registerContextMenu(popup);

    //: 2D Mapper context menu (custom line editing) item
    auto addPoint = new QAction(T2DMap::tr("Add point"), &mMapWidget);
    if (mMapWidget.mCustomLineSelectedPoint > -1) {
        QObject::connect(addPoint, &QAction::triggered, &mMapWidget, &T2DMap::slot_customLineAddPoint);
        //: 2D Mapper context menu (custom line editing) item tooltip (enabled state)
        addPoint->setToolTip(utils::richText(T2DMap::tr("Divide segment by adding a new point mid-way along")));
    } else {
        addPoint->setEnabled(false);
        //: 2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)
        addPoint->setToolTip(utils::richText(T2DMap::tr("Select a point first, then add a new point mid-way along the segment towards room")));
    }

    //: 2D Mapper context menu (custom line editing) item
    auto removePoint = new QAction(T2DMap::tr("Remove point"), &mMapWidget);
    if (mMapWidget.mCustomLineSelectedPoint > -1) {
        if (room->customLines.value(mMapWidget.mCustomLineSelectedExit).count() > 1) {
            QObject::connect(removePoint, &QAction::triggered, &mMapWidget, &T2DMap::slot_customLineRemovePoint);
            if ((mMapWidget.mCustomLineSelectedPoint + 1) < room->customLines.value(mMapWidget.mCustomLineSelectedExit).count()) {
                //: 2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)
                removePoint->setToolTip(utils::richText(T2DMap::tr("Merge pair of segments by removing this point")));
            } else {
                //: 2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)
                removePoint->setToolTip(utils::richText(T2DMap::tr("Remove last segment by removing this point")));
            }
        } else {
            removePoint->setEnabled(false);
            //: (2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action "delete line" should match the translation for that action))
            removePoint->setToolTip(utils::richText(T2DMap::tr(R"(use "delete line" to remove the only segment ending in an editable point)")));
        }
    } else {
        removePoint->setEnabled(false);
        //: 2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)
        removePoint->setToolTip(utils::richText(T2DMap::tr("Select a point first, then remove it")));
    }

    //: 2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)
    auto lineProperties = new QAction(T2DMap::tr("Properties"), &mMapWidget);
    //: 2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise
    lineProperties->setText(T2DMap::tr("properties..."));
    lineProperties->setToolTip(utils::richText(T2DMap::tr("Change the properties of this custom line")));
    QObject::connect(lineProperties, &QAction::triggered, &mMapWidget, &T2DMap::slot_customLineProperties);

    //: 2D Mapper context menu (custom line editing) item
    auto deleteLine = new QAction(T2DMap::tr("Delete line"), &mMapWidget);
    //: 2D Mapper context menu (custom line editing) item tooltip
    deleteLine->setToolTip(utils::richText(T2DMap::tr("Delete all of this custom line")));
    QObject::connect(deleteLine, &QAction::triggered, &mMapWidget, &T2DMap::slot_deleteCustomExitLine);

    popup->addAction(addPoint);
    popup->addAction(removePoint);
    popup->addAction(lineProperties);
    popup->addAction(deleteLine);

    mMapWidget.mPopupMenu = true;
    popup->popup(mMapWidget.mapToGlobal(context.widgetPosition));

    return true;
}
