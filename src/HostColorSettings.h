//
// Created by gustavo on 06/05/2020.
//

#ifndef MUDLET_HOSTCOLORSETTINGS_H
#define MUDLET_HOSTCOLORSETTINGS_H

#include "TColorSettings.h"

class HostColorSettings : public TColorSettings
{
public:
    HostColorSettings();

    QColor mCommandBgColor;
    QColor mCommandFgColor;

    QColor mCommandLineFgColor;
    QColor mCommandLineBgColor;

    virtual void reset();

};


#endif //MUDLET_HOSTCOLORSETTINGS_H
