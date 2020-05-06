#include "TColorScheme.h"


TColorScheme::TColorScheme()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
: mBlack(QColorConstants::Black)
, mLightBlack(QColorConstants::DarkGray)
, mRed(QColorConstants::DarkRed)
, mLightRed(QColorConstants::Red)
, mLightGreen(QColorConstants::Green)
, mGreen(QColorConstants::DarkGreen)
, mLightBlue(QColorConstants::Blue)
, mBlue(QColorConstants::DarkBlue)
, mLightYellow(QColorConstants::Yellow)
, mYellow(QColorConstants::DarkYellow)
, mLightCyan(QColorConstants::Cyan)
, mCyan(QColorConstants::DarkCyan)
, mLightMagenta(QColorConstants::Magenta)
, mMagenta(QColorConstants::DarkMagenta)
, mLightWhite(QColorConstants::White)
, mWhite(QColorConstants::LightGray)
#else
        : mBlack(Qt::black)
        , mLightBlack(Qt::darkGray)
        , mRed(Qt::darkRed)
        , mLightRed(Qt::red)
        , mLightGreen(Qt::green)
        , mGreen(Qt::darkGreen)
        , mLightBlue(Qt::blue)
        , mBlue(Qt::darkBlue)
        , mLightYellow(Qt::yellow)
        , mYellow(Qt::darkYellow)
        , mLightCyan(Qt::cyan)
        , mCyan(Qt::darkCyan)
        , mLightMagenta(Qt::magenta)
        , mMagenta(Qt::darkMagenta)
        , mLightWhite(Qt::white)
        , mWhite(Qt::lightGray)
#endif
{
}


QColor TColorScheme::getColorFromAnsi(int ansiColor) const
{
    assert(ansiColor >= 0 && ansiColor < 16);

    // clang-format off
    switch (ansiColor) {
    case 0:     return mBlack;           
    case 1:     return mRed;             
    case 2:     return mGreen;           
    case 3:     return mYellow;          
    case 4:     return mBlue;            
    case 5:     return mMagenta;         
    case 6:     return mCyan;            
    case 7:     return mWhite;           
    case 8:     return mLightBlack;      
    case 9:     return mLightRed;        
    case 10:    return mLightGreen;      
    case 11:    return mLightYellow;     
    case 12:    return mLightBlue;       
    case 13:    return mLightMagenta;    
    case 14:    return mLightCyan;       
    case 15:    return mLightWhite;      
    default:
        Q_UNREACHABLE();
    }
    // clang-format on
}

bool TColorScheme::getColorPair(int tag, QColor& color, QColor& lightColor) const
{
    switch (tag) {
    case 0:
        color = mBlack;
        lightColor = mLightBlack;
        return true;
    case 1:
        color = mRed;
        lightColor = mLightRed;
        return true;
    case 2:
        color = mGreen;
        lightColor = mLightGreen;
        return true;
    case 3:
        color = mYellow;
        lightColor = mLightYellow;
        return true;
    case 4:
        color = mBlue;
        lightColor = mLightBlue;
        return true;
    case 5:
        color = mMagenta;
        lightColor = mLightMagenta;
        return true;
    case 6:
        color = mCyan;
        lightColor = mLightCyan;
        return true;
    case 7:
        color = mWhite;
        lightColor = mLightWhite;
        return true;
    }

    return false;
}


QColor TColorScheme::getColorFromEnv(int env) const
{
    assert(env >= 1 && env <= 16);

    switch (env) {
    case 1:
        return mRed;
    case 2:
        return mGreen;
    case 3:
        return mYellow;
    case 4:
        return mBlue;
    case 5:
        return mMagenta;
    case 6:
        return mCyan;
    case 7:
        return mWhite;
    case 8:
        return mBlack;
    case 9:
        return mLightRed;
    case 10:
        return mLightGreen;
    case 11:
        return mLightYellow;
    case 12:
        return mLightBlue;
    case 13:
        return mLightMagenta;
    case 14:
        return mLightCyan;
    case 15:
        return mLightWhite;
    case 16:
        return mLightBlack;
    default:
        Q_UNREACHABLE();
    }
}


bool TColorScheme::setColor(quint8 colorNumber, const QColor& color)
{
    switch (colorNumber) {
    case 0: // Black
        mBlack = color;
        return true;
    case 1: // Red
        mRed = color;
        return true;
    case 2: // Green
        mGreen = color;
        return true;
    case 3: // Yellow
        mYellow = color;
        return true;
    case 4: // Blue
        mBlue = color;
        return true;
    case 5: // Magenta
        mMagenta = color;
        return true;
    case 6: // Cyan
        mCyan = color;
        return true;
    case 7: // Light gray
        mWhite = color;
        return true;
    case 8: // Dark gray
        mLightBlack = color;
        return true;
    case 9: // Light Red
        mLightRed = color;
        return true;
    case 10: // Light Green
        mLightGreen = color;
        return true;
    case 11: // Light Yellow
        mLightYellow = color;
        return true;
    case 12: // Light Blue
        mLightBlue = color;
        return true;
    case 13: // Light Magenta
        mLightMagenta = color;
        return true;
    case 14: // Light Cyan
        mLightCyan = color;
        return true;
    case 15: // Light gray
        mLightWhite = color;
        return true;
    }
    return false;
}

void TColorScheme::reset()
{
    mBlack = Qt::black;
    mLightBlack = Qt::darkGray;
    mRed = Qt::darkRed;
    mLightRed = Qt::red;
    mGreen = Qt::darkGreen;
    mLightGreen = Qt::green;
    mBlue = Qt::darkBlue;
    mLightBlue = Qt::blue;
    mYellow = Qt::darkYellow;
    mLightYellow = Qt::yellow;
    mCyan = Qt::darkCyan;
    mLightCyan = Qt::cyan;
    mMagenta = Qt::darkMagenta;
    mLightMagenta = Qt::magenta;
    mWhite = Qt::lightGray;
    mLightWhite = Qt::white;
}
