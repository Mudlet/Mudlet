//
// Created by gustavo on 05/05/2020.
//

#ifndef MUDLET_COLORSETTINGS_H
#define MUDLET_COLORSETTINGS_H

#include "TColorScheme.h"

class TColorSettings : public TColorScheme
{
public:
    // These three replace three sets of three integers that were used to hold
    // colour components during the parsing of SGR sequences, they were called:
    // fgColor{R|G|B}, fgColorLight{R|G|B} and bgColor{R|G|B} apart from
    // anything else, the first and last sets had the same names as arguments
    // to several of the methods which meant the latter shadowed and masked
    // them off!
    QColor mFgColor;
    QColor mFgColorLight;
    QColor mBgColor;

    TColorSettings();
    TColorSettings(const TColorSettings& other);

    void updateColors(const QColor& fg, const QColor& bg);

    virtual void reset();
};

#endif //MUDLET_COLORSETTINGS_H
