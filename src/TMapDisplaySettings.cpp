//
// Created by gustavo on 06/05/2020.
//

#include "TMapDisplaySettings.h"
TMapDisplaySettings::TMapDisplaySettings()
: mPlayerRoomStyle(0), mPlayerRoomOuterColor(Qt::red), mPlayerRoomInnerColor(Qt::white), mPlayerRoomOuterDiameterPercentage(120), mPlayerRoomInnerDiameterPercentage(70)
{
}

TMapDisplaySettings::TMapDisplaySettings(const TMapDisplaySettings& other)
: mPlayerRoomStyle(other.mPlayerRoomStyle)
, mPlayerRoomOuterColor(other.mPlayerRoomOuterColor)
, mPlayerRoomInnerColor(other.mPlayerRoomInnerColor)
, mPlayerRoomOuterDiameterPercentage(other.mPlayerRoomOuterDiameterPercentage)
, mPlayerRoomInnerDiameterPercentage(other.mPlayerRoomInnerDiameterPercentage)
{
}
