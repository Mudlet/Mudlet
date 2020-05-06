#include "TColorSettings.h"

TColorSettings::TColorSettings()
: TColorScheme()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
, mFgColor(QColorConstants::LightGray)
, mBgColor(QColorConstants::Black)
#else
, mFgColor(Qt::lightGray)
, mBgColor(Qt::black)
#endif
{
}

TColorSettings::TColorSettings(const TColorSettings& other) : TColorScheme(other), mFgColor(other.mFgColor), mBgColor(other.mBgColor) {}


TColorSettings::TColorSettings(const TColorScheme& scheme, const QColor& fg, const QColor& bg) : TColorScheme(scheme), mFgColor(fg), mFgColorLight(fg), mBgColor(bg) {}

void TColorSettings::updateForeGroundFromTag(int tag)
{
    getColorPair(tag, mFgColor, mFgColorLight);
}

void TColorSettings::updateForeGround(const QColor& fg)
{
    updateForeGround(fg, fg);
}
void TColorSettings::updateForeGround(const QColor& fg, const QColor& fgLight)
{
    mFgColor = fg;
    mFgColorLight = fgLight;
}


void TColorSettings::updateBackGround(int tag)
{
    mBgColor = getColorFromAnsi(tag);
}

void TColorSettings::updateBackGround(const QColor& color)
{
    mBgColor = color;
}
void TColorSettings::updateColors(QColor fg, QColor bg)
{
    mFgColor = fg;
    mBgColor = bg;
}