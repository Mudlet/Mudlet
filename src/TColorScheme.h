//
// Created by gustavo on 05/05/2020.
//

#ifndef MUDLET_SRC_TCOLORSCHEME_H
#define MUDLET_SRC_TCOLORSCHEME_H

#include "pre_guard.h"
#include <QColor>
#include "post_guard.h"


class TColorScheme
{
public:

    TColorScheme();

    QColor mBlack;
    QColor mLightBlack;
    QColor mRed;
    QColor mLightRed;
    QColor mLightGreen;
    QColor mGreen;
    QColor mLightBlue;
    QColor mBlue;
    QColor mLightYellow;
    QColor mYellow;
    QColor mLightCyan;
    QColor mCyan;
    QColor mLightMagenta;
    QColor mMagenta;
    QColor mLightWhite;
    QColor mWhite;

    QColor getColorFromAnsi(int ansiColor) const;
    QColor getColorFromEnv(int env) const;
    bool getColorPair(int tag, QColor& color, QColor& lightColor) const;

    bool setColor(quint8 colorNumber, const QColor& Color);
    void reset();
};


#endif //MUDLET_SRC_TCOLORSCHEME_H
