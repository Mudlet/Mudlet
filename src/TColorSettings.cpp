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


void TColorSettings::updateColors(const QColor& fg, const QColor& bg)
{
    mFgColor = fg;
    mBgColor = bg;
}

void TColorSettings::reset()
{
    mFgColor = Qt::lightGray;
    mBgColor = Qt::black;
}
