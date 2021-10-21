/***************************************************************************
 *   Copyright (C) 2021 by Piotr Wilczynski - delwing@gmail.com            *
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

MapInfoContributorManager::MapInfoContributorManager(QObject* parent, Host* pH) : QObject(parent), mpHost(pH)
{
    registerContributor(QStringLiteral("Short"), [=](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
        return shortInfo(roomID, selectionSize, areaId, displayAreaId, infoColor);
    });
    registerContributor(QStringLiteral("Full"), [=](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
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

bool MapInfoContributorManager::enableContributor(const QString &name) {
    if (!contributors.contains(name)) {
        return false;
    }
    mpHost->mMapInfoContributors.insert(name);
    mpHost->mpMap->update();
    emit signal_contributorsUpdated();
    return true;
}

bool MapInfoContributorManager::disableContributor(const QString &name) {
    if (!contributors.contains(name)) {
        return false;
    }
    mpHost->mMapInfoContributors.remove(name);
    mpHost->mpMap->update();
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
    Q_UNUSED(selectionSize);
    Q_UNUSED(displayAreaId);

    QString infoText;
    TRoom* room = mpHost->mpMap->mpRoomDB->getRoom(roomID);
    if (room) {
        QString areaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        static const QRegularExpression trailingPunctuation(QStringLiteral("[.,/]+$"));
        auto roomName = QString(room->name);
        if (mpHost->mMapViewOnly) {
            roomName = roomName.remove(trailingPunctuation).trimmed();
        }
        auto roomFragment = !roomName.isEmpty() && roomName != QString::number(room->getId()) ?
            QStringLiteral("%1 / %2").arg(roomName, QString::number(room->getId())) : QString::number(room->getId());
        infoText = QStringLiteral("%1 (%2)\n").arg(roomFragment, areaName);
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
        QString areaName = mpHost->mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        if (area) {
            infoText = tr("Area:%1%2 ID:%1%3 x:%1%4%1<‑>%1%5 y:%1%6%1<‑>%1%7 z:%1%8%1<‑>%1%9\n",
                          // Intentional separator
                          "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle "
                          "them literally in raw strings) and non-breaking hyphens which are used to "
                          "prevent the line being split at some places it might otherwise be; when "
                          "translating please consider at which points the text may be divided to fit onto "
                          "more than one line. "
                          "%2 is the (text) name of the area, %3 is the number for it, "
                          "%4 to %9 are pairs (min <-> max) of extremes for each of x,y and z coordinates")
                               .arg(QChar(160),
                                    areaName,
                                    QString::number(areaId),
                                    QString::number(area->min_x),
                                    QString::number(area->max_x),
                                    QString::number(area->min_y),
                                    QString::number(area->max_y),
                                    QString::number(area->min_z),
                                    QString::number(area->max_z));
        } else {
            infoText = QChar::LineFeed;
        }


        if (!room->name.isEmpty()) {
            infoText.append(tr("Room Name: %1\n").arg(room->name));
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
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle "
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when NO rooms are selected, %3 is the room number "
                               "of, and %4-%6 are the x,y and z coordinates for, the current player's room.")
                                    .arg(QChar(160), QString::number(roomID), QString::number(room->x), QString::number(room->y), QString::number(room->z)));
            if (areaId != displayAreaId) {
                isItalic = true;
            } else {
                isBold = true;
            }
            break;
        case 1:
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle "
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when ONE room is selected, %3 is the room number "
                               "of, and %4-%6 are the x,y and z coordinates for, the selected Room.")
                                    .arg(QChar(160), QString::number(roomID), QString::number(room->x), QString::number(room->y), QString::number(room->z)));
            isBold = true;
            if (infoColor.lightness() > 127) {
                color = QColor(255, 223, 191); // Slightly orange white
            } else {
                color = QColor(96, 48, 0); // Dark, slightly orange grey
            }
            break;
        default:
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle "
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when TWO or MORE rooms are selected; %1 is the room "
                               "number for which %2-%4 are the x,y and z coordinates of the room nearest the "
                               "middle of the selection. This room has the yellow cross-hairs. %n is the count "
                               "of rooms selected and will ALWAYS be greater than 1 in this situation. It is "
                               "provided so that non-English translations can select required plural forms as "
                               "needed.",
                               selectionSize)
                                    .arg(QChar(160), QString::number(roomID), QString::number(room->x), QString::number(room->y), QString::number(room->z)));
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
