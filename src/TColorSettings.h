//
// Created by gustavo on 05/05/2020.
//

#ifndef MUDLET__TBUFFERCOLORSETTINGS_H
#define MUDLET__TBUFFERCOLORSETTINGS_H

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
    TColorSettings(const TColorSettings &other);
    TColorSettings(const TColorScheme& scheme, const QColor& fg, const QColor& bg);

    void updateForeGroundFromTag(int tag);
    void updateBackGround(int tag);

    void updateBackGround(const QColor &color);
    void updateForeGround(const QColor& fg, const QColor& fgLight);
    void updateForeGround(const QColor& fg);
    void updateColors(QColor fg, QColor bg);
};

#endif //MUDLET__TBUFFERCOLORSETTINGS_H
