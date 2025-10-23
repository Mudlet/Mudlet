#pragma once

#include <QColor>
#include "I_TDebug.h"
#include "TDebug.h"
#include "Host.h"

class TDebugAdapter : public I_TDebug {
public:
    TDebugAdapter(const QString& fg, const QString& bg)
        : mDebug(QColor(fg), QColor(bg)) {}

    I_TDebug& operator<<(const QString& text) override { mDebug << text; return *this; }
    I_TDebug& operator<<(const QChar& ch) override { mDebug << ch; return *this; }
    I_TDebug& operator<<(int value) override { mDebug << value; return *this; }
    I_TDebug& operator>>(I_Host* host) override { mDebug >> dynamic_cast<Host*>(host); return *this; }

private:
    TDebug mDebug;
};