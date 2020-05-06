//
// Created by gustavo on 06/05/2020.
//

#ifndef MUDLET_MAPDISPLAYSETTINGS_H
#define MUDLET_MAPDISPLAYSETTINGS_H


#include "TColorSettings.h"
class TMapDisplaySettings : public TColorSettings
{
public:
    TMapDisplaySettings();
    TMapDisplaySettings(const TMapDisplaySettings &other);
    // These hold values that are needed in the TMap class which are saved with
    // the profile - but which cannot be kept there as that class is not
    // necessarily instantiated when the profile is read.
    // Base color(s) for the player room in the mappers:
    // Mode selected:
    // 0 is closest to original style with adjustable outer diameter
    // 1 is Fixed red color ring with adjustable outer/inner diameter
    // 2 is fixed blue/yellow colors ring with adjustable outer/inner diameter
    // 3 is adjustable outer(primary)/inner(secondary) colors ring with adjustable outer/inner diameter
    quint8 mPlayerRoomStyle;

    // Base color(s) for the player room in the mappers:
    QColor mPlayerRoomOuterColor;
    QColor mPlayerRoomInnerColor;

    // Percentage of the room size (actually width) for the outer diameter of
    // the circular marking, integer percentage clamped in the preferences
    // between 200 and 50 - default 120:
    quint8 mPlayerRoomOuterDiameterPercentage;
    // Percentage of the outer size for the inner diameter of the circular
    // marking, integer percentage clamped in the preferences between 83 and 0,
    // with a default of 70. NOT USED FOR "Original" style marking (the 0'th
    // one):
    quint8 mPlayerRoomInnerDiameterPercentage;
};


#endif //MUDLET_MAPDISPLAYSETTINGS_H
