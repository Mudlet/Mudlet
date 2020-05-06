//
// Created by gustavo on 06/05/2020.
//

#include "HostColorSettings.h"
HostColorSettings::HostColorSettings()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
: mCommandBgColor(QColorConstants::Black)
, mCommandFgColor(QColor(113, 113, 0))
, mCommandLineFgColor(QColorConstants::DarkGray)
, mCommandLineBgColor(QColorConstants::Black)
#else
: mCommandBgColor(Qt::black)
, mCommandFgColor(QColor(113, 113, 0))
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
#endif
{}

void HostColorSettings::reset() {
    TColorSettings::reset();

    mCommandLineFgColor = Qt::darkGray;
    mCommandLineBgColor = Qt::black;
    mCommandFgColor = QColor(113, 113, 0);
    mCommandBgColor = Qt::black;
}