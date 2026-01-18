/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "mapInfoContributorManager.h"
#include "TArea.h"
#include "TRoomDB.h"
#include "dlgMapper.h"

MapInfoContributorManager::MapInfoContributorManager(QObject* parent, Host* pH)
: QObject(parent)
, mpHost(pH)
{
    registerContributor(qsl("Short"), [=, this](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
        return shortInfo(roomID, selectionSize, areaId, displayAreaId, infoColor);
    });
    registerContributor(qsl("Full"), [=, this](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
        return fullInfo(roomID, selectionSize, areaId, displayAreaId, infoColor);
    });
}

void MapInfoContributorManager::registerContributor(const QString& name, MapInfoCallback callback)
{
    if (contributors.contains(name)) {
        ordering.removeOne(name);
    }
    ordering.append(name);
    contributors.insert(name, callback);
    emit signal_contributorsUpdated();
}

bool MapInfoContributorManager::removeContributor(const QString& name)
{
    mpHost->mMapInfoContributors.remove(name);
    ordering.removeOne(name);
    emit signal_contributorsUpdated();
    return contributors.remove(name) > 0;
}

bool MapInfoContributorManager::enableContributor(const QString &name)
{
    if (!contributors.contains(name)) {
        return false;
    }
    mpHost->mMapInfoContributors.insert(name);
    mpHost->mpMap->updateArea(-1);
    emit signal_contributorsUpdated();
    return true;
}

bool MapInfoContributorManager::disableContributor(const QString &name)
{
    if (!contributors.contains(name)) {
        return false;
    }
    mpHost->mMapInfoContributors.remove(name);
    mpHost->mpMap->updateArea(-1);
    emit signal_contributorsUpdated();
    return true;
}

MapInfoCallback MapInfoContributorManager::getContributor(const QString& name)
{
    return contributors.value(name);
}

QList<QString> &MapInfoContributorManager::getContributorKeys()
{
    return ordering;
}

MapInfoProperties MapInfoContributorManager::shortInfo(int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor)
{
    Q_UNUSED(selectionSize)
    Q_UNUSED(displayAreaId)

    QString infoText;
    TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomID);
    if (room) {
        const QString areaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        static const QRegularExpression trailingPunctuation(qsl("[.,/]+$"));
        auto roomName = QString(room->name);
        if (mpHost->mMapViewOnly) {
            roomName = roomName.remove(trailingPunctuation).trimmed();
        }
        auto roomFragment = !roomName.isEmpty() && roomName != QString::number(room->getId()) ?
            qsl("%1 / %2").arg(roomName, QString::number(room->getId())) : QString::number(room->getId());
        infoText = qsl("%1 (%2)\n").arg(roomFragment, areaName);
    }
    return MapInfoProperties{false, false, infoText, infoColor};
}

MapInfoProperties MapInfoContributorManager::fullInfo(int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor)
{
    QString infoText;
    bool isBold = false;
    bool isItalic = false;
    QColor color = infoColor;

    TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomID);
    if (room) {
        TArea* area = mpHost->mpMap->mpRoomDB->getArea(areaId);
        const QString areaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        if (area) {
            infoText = qsl("%1\n").arg(
                /*:
                %1 is the (text) name of the area, %2 is the area ID number,
                %3 and %4 are the minimum and maximum x coordinates, %5 and %6 for y, and %7 and %8 for z.
                This text uses non-breaking spaces (Unicode U+00A0) and non-breaking hyphens
                which are used to prevent the line being split at some places it might otherwise be.
                When translating, please consider at which points the text may be divided to fit
                onto more than one line. 
                */
                           tr("Area:\u00A0%1 ID:\u00A0%2 x:\u00A0%3\u00A0<‑>\u00A0%4 y:\u00A0%5\u00A0<‑>\u00A0%6 z:\u00A0%7\u00A0<‑>\u00A0%8")
                               .arg(areaName,
                                    QString::number(areaId),
                                    QString::number(area->min_x),
                                    QString::number(area->max_x),
                                    QString::number(area->min_y),
                                    QString::number(area->max_y),
                                    QString::number(area->min_z),
                                    QString::number(area->max_z)));
        } else {
            infoText = QChar::LineFeed;
        }


        if (!room->name.isEmpty()) {
            infoText.append(qsl("%1\n").arg(tr("Room Name: %1").arg(room->name)));
        }

        // Italicise the text if the current display area {mAreaID} is not the
        // same as the displayed text information - which happens when NO
        // room is selected AND the current area is NOT the one the player
        // is in (to emphasis that the displayed data is {mostly} not about
        // the CURRENTLY VISIBLE area)... make it bold if the player room IS
        // in the displayed map

        // If one or more rooms are selected - make the text slightly orange.
        switch (selectionSize) {
        case 0:
            // The following multi-line comments for translators are deliberately vague to cover all
            // three same strings in these cases with a same comment, so translators only see them once.
            /*:
            This text is shown when room(s) are (not) selected in mapper. %1 is the room ID number, 
            and %2, %3, %4 are the x, y, and z coordinates of the current/selected room, or a room
            near the middle of the selection. %5 is a description like: Current player room.
            This text uses non-breaking spaces (Unicode \u00A0) and a non-breaking hyphen (\u2011). 
            They are used to prevent the line being split at unexpected places. When translating, 
            please consider at which points the text may be divided to fit onto more than one line.
            */
            infoText.append(tr("Room\u00A0ID:\u00A0%1 Position\u00A0on\u00A0Map: (%2,%3,%4) \u2011\u00A0%5")
                            .arg(QString::number(roomID),
                                 QString::number(room->x()),
                                 QString::number(room->y()),
                                 QString::number(room->z()),
            //: This description is shown when NO room is selected.
                                 tr("Current player location")))
                    .append(QChar::LineFeed);
            if (areaId != displayAreaId) {
                isItalic = true;
            } else {
                isBold = true;
            }
            break;
        case 1:
            /*:
            This text is shown when room(s) are (not) selected in mapper. %1 is the room ID number, 
            and %2, %3, %4 are the x, y, and z coordinates of the current/selected room, or a room
            near the middle of the selection. %5 is a description like: Current player room.
            This text uses non-breaking spaces (Unicode \u00A0) and a non-breaking hyphen (\u2011). 
            They are used to prevent the line being split at unexpected places. When translating, 
            please consider at which points the text may be divided to fit onto more than one line.
            */
            infoText.append(tr("Room\u00A0ID:\u00A0%1 Position\u00A0on\u00A0Map: (%2,%3,%4) \u2011\u00A0%5")
                            .arg(QString::number(roomID),
                                 QString::number(room->x()),
                                 QString::number(room->y()),
                                 QString::number(room->z()),
            //: This description is shown when EXACTLY ONE room is selected.
                                 tr("Selected room")))
                    .append(QChar::LineFeed);
            isBold = true;
            if (infoColor.lightness() > 127) {
                color = QColor(255, 223, 191); // Slightly orange white
            } else {
                color = QColor(96, 48, 0); // Dark, slightly orange grey
            }
            break;
        default:
            /*:
            This text is shown when room(s) are (not) selected in mapper. %1 is the room ID number, 
            and %2, %3, %4 are the x, y, and z coordinates of the current/selected room, or a room
            near the middle of the selection. %5 is a description like: Current player room.
            This text uses non-breaking spaces (Unicode U+00A0) and a non-breaking hyphen (U+2011). 
            They are used to prevent the line being split at unexpected places. When translating, 
            please consider at which points the text may be divided to fit onto more than one line.
            */
            infoText.append(tr("Room\u00A0ID:\u00A0%1 Position\u00A0on\u00A0Map: (%2,%3,%4) \u2011\u00A0%5")
                            .arg(QString::number(roomID),
                                 QString::number(room->x()),
                                 QString::number(room->y()),
                                 QString::number(room->z()),
            //: This description is shown when MORE THAN ONE room is selected.
                                 tr("Center of %n selected rooms", nullptr, selectionSize)))
                    .append(QChar::LineFeed);
            isBold = true;
            if (infoColor.lightness() > 127) {
                color = QColor(255, 223, 191); // Slightly orange white
            } else {
                color = QColor(96, 48, 0); // Dark, slightly orange grey
            }
            break;
        }
    }

    return MapInfoProperties{isBold, isItalic, infoText, color};
}
