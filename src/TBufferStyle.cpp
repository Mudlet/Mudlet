/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "TBufferStyle.h"
#include <QDebug>
TBufferStyle::TBufferStyle(const TColorSettings& colorSettings)
: TColorSettings(colorSettings), mBold(false), mItalics(false), mOverline(false), mReverse(false), mStrikeOut(false), mUnderline(false), mItalicBeforeBlink(false), mIsDefaultColor(true)
{
}

TBufferStyle::TBufferStyle() : mBold(false), mItalics(false), mOverline(false), mReverse(false), mStrikeOut(false), mUnderline(false), mItalicBeforeBlink(false), mIsDefaultColor(true) {}


TChar::AttributeFlags TBufferStyle::getTCharFlags() const
{
    // clang-format off
    return ((mIsDefaultColor ? mBold : false) ? TChar::Bold : TChar::None)
            | (mItalics ? TChar::Italic : TChar::None)
            | (mOverline ? TChar::Overline : TChar::None)
            | (mReverse ? TChar::Reverse : TChar::None)
            | (mStrikeOut ? TChar::StrikeOut : TChar::None)
            | (mUnderline ? TChar::Underline : TChar::None);
    // clang-format on
}

TChar TBufferStyle::createChar() const
{
    return TChar((!mIsDefaultColor && mBold) ? mFgColorLight : mFgColor, mBgColor, getTCharFlags());
}

TChar TBufferStyle::createTransparent() const
{
    return TChar(mBgColor, mBgColor, getTCharFlags());
}


void TBufferStyle::updateColorSettings(const TColorSettings& colors)
{
    mBlack = colors.mBlack;
    mLightBlack = colors.mLightBlack;
    mRed = colors.mRed;
    mLightRed = colors.mLightRed;
    mLightGreen = colors.mLightGreen;
    mGreen = colors.mGreen;
    mLightBlue = colors.mLightBlue;
    mBlue = colors.mBlue;
    mLightYellow = colors.mLightYellow;
    mYellow = colors.mYellow;
    mLightCyan = colors.mLightCyan;
    mCyan = colors.mCyan;
    mLightMagenta = colors.mLightMagenta;
    mMagenta = colors.mMagenta;
    mLightWhite = colors.mLightWhite;
    mWhite = colors.mWhite;

    TColorSettings::updateColors(colors.mFgColor, colors.mBgColor);
}
/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint on
      3 italics on
      4 underline on
      5 blink on slow
      6 blink on fast
      7 inverse on
      9 strikethrough on
      10 ? TODO
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      25 blink off
      26 RESERVED (for proportional spacing)
      27 inverse off
      29 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 fg default
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg default
      50 RESERVED (for proportional spacing)
      51 framed on
      52 encircled on
      53 overlined on
      54 framed / encircled off
      55 overlined off

      Notes for code 38/48:
      38:0 implementation defined (48:0 is NOT allowed)

      38:1 transparent foreground
      48:1 transparent background

      sequences for 24(32 for '4')-bit Color support:
      38:2:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space foreground color
      48:2:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space background color
      38:3:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space foreground color
      48:3:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space background color
      38:4:???:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space foreground color
      48:4:???:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space background color
      The third parameter is the color space id but this is expected to be the
      "default" value which is an empty string. The seventh parameter may be used
      to specify a tolerance value (an integer) and parameter eight may be used
      to specify a colour space associated with the tolerance (0 for CIELUV,
      1 for CIELAB).

      sequences for (indexed) 256 Color support:
      38:5:0-256 (indexed) foreground color
      48:5:0-256 (indexed) background color:
          0x00-0x07:   0 -   7 standard colors (as in ESC [ 30–37 m)
          0x08-0x0F:   8 -  15 high intensity colors (as in ESC [ 90–97 m)
          0x10-0xE7:  16 - 231 6 × 6 × 6 = 216 colors: 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
          0xE8-0xFF: 232 - 255 grayscale from black to white in 24 steps

      Also note that for the 38 and 48 codes the parameter elements SHOULD be
      separated by ':' but some interpretations erroneously use ';'.  Also
      "empty" parameter elements represent a default value and that empty
      elements at the end can be omitted.
 */
void TBufferStyle::decodeSGR(const QString& sequence, bool haveColorSpaceId, const TColorSettings& hostColorSettings)
{
    QStringList parameterStrings = sequence.split(QChar(';'));
    for (int paraIndex = 0, total = parameterStrings.count(); paraIndex < total; ++paraIndex) {
        QString allParameterElements = parameterStrings.at(paraIndex);
        if (allParameterElements.contains(QLatin1String(":"))) {
            /******************************************************************
             * Parameter string with colon separated Parameter (sub) elements *
             ******************************************************************/
            // We have colon separated parameter elements, so we must have at least 2 members
            QStringList parameterElements(allParameterElements.split(QChar(':')));
            if (parameterElements.at(0) == QLatin1String("38")) {
                if (parameterElements.count() >= 2) {
                    decodeSGR38(parameterElements, true);

                } else {
                    // We only have a single element in this parameterString,
                    // so we will need to steal the needed number from the
                    // remainder - this is falling back to using a semicolon
                    // separated list rather than a colon separated one
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);     // "38"
                    madeElements << parameterStrings.at(paraIndex + 1); // "2" or "5" hopefully
                    bool isOk = false;
                    int sgr38_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr38_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr38_type) {
                    case 5: // Needs just one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        decodeSGR38(madeElements, false);
                        // Move the index to consume the used values
                        paraIndex += 2;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        decodeSGR38(madeElements, false);
                        // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[fallthrough]];
                    default:
                        break;
                    }
                }
                // End of if (parameterElements.at(0) == QLatin1String("38"))
            } else if (parameterElements.at(0) == QLatin1String("48")) {
                if (parameterElements.count() >= 2) {
                    decodeSGR48(parameterElements, true);

                } else {
                    // We only have a single element in this parameterString,
                    // so we will need to steal the needed number from the
                    // remainder - this is falling back to using a semicolon
                    // separated list rather than a colon separated one
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr48_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr48_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr48_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values
                        decodeSGR48(madeElements, false);
                        paraIndex += 2;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values
                        decodeSGR48(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[fallthrough]];
                    default:
                        break;
                    }
                }
                // End of if (parameterElements.at(0) == QLatin1String("48"))
            } else if (parameterElements.at(0) == QLatin1String("4")) {
                // New way of controlling underline
                bool isOk = false;
                int value = parameterElements.at(1).toInt(&isOk);
                if (!isOk) {
                    // missing value
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence
                                                 << "\") ERROR - failed to detect underline parameter element (the second part) in a SGR...;4:?;..m sequence assuming it is a zero!";
                }
                switch (value) {
                case 0: // Underline off
                    mUnderline = false;
                    break;
                case 1: // Underline on
                    mUnderline = true;
                    break;
                case 2: // Double underline - not supported, treat as single
                    [[fallthrough]];
                case 3: // Wavey underline - not supported, treat as single
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence
                                                 << "\") ERROR - unsupported underline parameter element (the second part) in a SGR...;4:" << parameterElements.at(1)
                                                 << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                default: // Something unexpected
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence
                                                 << "\") ERROR - unexpected underline parameter element (the second part) in a SGR...;4:" << parameterElements.at(1)
                                                 << ";../m sequence treating it as a zero!";
                    mUnderline = false;
                    break;
                }
            } else if (parameterElements.at(0) == QLatin1String("3")) {
                // New way of controlling italics
                bool isOk = false;
                int value = parameterElements.at(1).toInt(&isOk);
                if (!isOk) {
                    // missing value
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence
                                                 << "\") ERROR - failed to detect italic parameter element (the second part) in a SGR...;3:?;../m sequence assuming it is a zero!";
                }
                switch (value) {
                case 0: // Italics/Slant off
                    mItalics = false;
                    break;
                case 1: // Italics on
                    mItalics = true;
                    break;
                case 2: // Slant on - not supported, treat as italics
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence
                                                 << "\") ERROR - unsupported italic parameter element (the second part) in a SGR...;3:" << parameterElements.at(1)
                                                 << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                default: // Something unexpected
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - unexpected italic parameter element (the second part) in a SGR...;3:" << parameterElements.at(1)
                                                 << ";../m sequence treating it as a zero!";
                    mUnderline = false;
                    break;
                }
            } else {
                qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - parameter string with an unexpected initial parameter element in a SGR...;"
                                             << parameterElements.at(0) << ":" << parameterElements.at(1) << "...;.../m sequence, ignoring it!";
            }
        } else {
            /******************************************************************
             *             Parameter string with no sub-elements              *
             ******************************************************************/
            // We do not have a colon separated string so we must just have a
            // number:
            bool isOk = false;
            int tag = 0;
            if (!allParameterElements.isEmpty()) {
                tag = allParameterElements.toInt(&isOk);
            } else {
                // Allow for an empty parameter to be treated as valid and equal to 0:
                isOk = true;
            }
            if (isOk) {
                switch (tag) {
                case 0:
                    mIsDefaultColor = true;
                    mFgColor = hostColorSettings.mFgColor;
                    mBgColor = hostColorSettings.mBgColor;
                    mBold = false;
                    mItalics = false;
                    mOverline = false;
                    mReverse = false;
                    mStrikeOut = false;
                    mUnderline = false;
                    break;
                case 1:
                    mBold = true;
                    break;
                case 2:
                    // Technically this should be faint (i.e. decreased
                    // intensity compared to normal and 22 should be
                    // the reset to "normal" intensity):
                    mBold = false;
                    break;
                case 3:
                    // There is a proposal by the "VTE" terminal
                    // emulator to use a (sub)parameter entry to
                    // destinguish between italics and slanted text by
                    // using ESC[...;3:1;...m and ESC[...;3:2;...m
                    // respectively - that is handled above in the colon
                    // sub-string separated part:
                    mItalics = true;
                    break;
                case 4:
                    // There is a implimention by some terminal
                    // emulators ("Kitty" and "VTE") to use a
                    // (sub)parameter entry of 3 for a wavy underline
                    // {presumably 2 would be a double underline and 1
                    // the normal single underline) by sending e.g.:
                    // ESC[...;4:3;...m - that is handled above in the colon
                    // sub-string separated part:
                    mUnderline = true;
                    break;
                case 5:
                    if (mItalics) {
                        mItalicBeforeBlink = true;
                    }
                    mItalics = true;
                    break; //slow-blinking, represented as italics instead
                case 6:
                    if (mItalics) {
                        mItalicBeforeBlink = true;
                    }
                    mItalics = true;
                    break; //fast blinking, represented as italics instead
                case 7:
                    mReverse = true;
                    break;
                // case 8: // Concealed characters (set foreground to be the same as background?)
                //    break;
                case 9:
                    mStrikeOut = true;
                    break;
                // case 10:
                //    break; //default font
                // case 21: // Double underline according to specs
                //    break;
                case 22:
                    mBold = false;
                    break;
                case 23:
                    mItalics = false;
                    break;
                case 24:
                    mUnderline = false;
                    break;
                case 25:
                    if (!mItalicBeforeBlink) {
                        mItalics = false;
                    }
                    mItalicBeforeBlink = false;
                    break; // blink off
                case 27:
                    mReverse = false;
                    break;
                // case 28: // Revealed characters (undoes the effect of "8")
                //    break;
                case 29:
                    mStrikeOut = false;
                    break;
                case 30:
                    mFgColor = mBlack;
                    mFgColorLight = mLightBlack;
                    mIsDefaultColor = false;
                    break;
                case 31:
                    mFgColor = mRed;
                    mFgColorLight = mLightRed;
                    mIsDefaultColor = false;
                    break;
                case 32:
                    mFgColor = mGreen;
                    mFgColorLight = mLightGreen;
                    mIsDefaultColor = false;
                    break;
                case 33:
                    mFgColor = mYellow;
                    mFgColorLight = mLightYellow;
                    mIsDefaultColor = false;
                    break;
                case 34:
                    mFgColor = mBlue;
                    mFgColorLight = mLightBlue;
                    mIsDefaultColor = false;
                    break;
                case 35:
                    mFgColor = mMagenta;
                    mFgColorLight = mLightMagenta;
                    mIsDefaultColor = false;
                    break;
                case 36:
                    mFgColor = mCyan;
                    mFgColorLight = mLightCyan;
                    mIsDefaultColor = false;
                    break;
                case 37:
                    mFgColor = mWhite;
                    mFgColorLight = mLightWhite;
                    mIsDefaultColor = false;
                    break;
                case 38: {
                    // We only have single elements so we will need to steal the
                    // needed number from the remainder:
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr38_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr38_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr38_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values
                        decodeSGR38(madeElements, false);
                        paraIndex += 2;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values LESS
                        // the one that the for loop will handle - even if it
                        // goes past end
                        decodeSGR38(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[fallthrough]];
                    default:
                        break;
                    }
                } break;
                case 39: //default foreground color
                    mFgColor = hostColorSettings.mFgColor;
                    break;
                case 40:
                    mBgColor = mBlack;
                    break;
                case 41:
                    mBgColor = mRed;
                    break;
                case 42:
                    mBgColor = mGreen;
                    break;
                case 43:
                    mBgColor = mYellow;
                    break;
                case 44:
                    mBgColor = mBlue;
                    break;
                case 45:
                    mBgColor = mMagenta;
                    break;
                case 46:
                    mBgColor = mCyan;
                    break;
                case 47:
                    mBgColor = mWhite;
                    break;
                case 48: {
                    // We only have single elements so we will need to steal the
                    // needed number from the remainder:
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr48_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr48_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr48_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values
                        decodeSGR48(madeElements, false);
                        paraIndex += 2;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values
                        decodeSGR48(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[fallthrough]];
                    default:
                        break;
                    }
                } break;
                case 49: // default background color
                    mBgColor = hostColorSettings.mBgColor;
                    break;
                // case 51: // Framed
                //    break;
                // case 52: // Encircled
                //    break;
                case 53:
                    mOverline = true;
                    break;
                // case 54: // Not framed, not encircled
                //    break;
                case 55:
                    mOverline = false;
                    break;
                // 56 to 59 reserved for future standardization
                // case 60: // ideogram underline or right side line
                //    break;
                // case 61: // ideogram double underline or double right side line
                //    break;
                // case 62: // ideogram overline or left side line
                //    break;
                // case 63: // ideogram double overline or double left side line
                //    break;
                // case 64: // ideogram stress marking
                //    break;
                // case 65: // cancels the effects of 60 to 64
                //    break;
                case 90:
                    mFgColor = mLightBlack;
                    mFgColorLight = mLightBlack;
                    mIsDefaultColor = false;
                    break;
                case 91:
                    mFgColor = mLightRed;
                    mFgColorLight = mLightRed;
                    mIsDefaultColor = false;
                    break;
                case 92:
                    mFgColor = mLightGreen;
                    mFgColorLight = mLightGreen;
                    mIsDefaultColor = false;
                    break;
                case 93:
                    mFgColor = mLightYellow;
                    mFgColorLight = mLightYellow;
                    mIsDefaultColor = false;
                    break;
                case 94:
                    mFgColor = mLightBlue;
                    mFgColorLight = mLightBlue;
                    mIsDefaultColor = false;
                    break;
                case 95:
                    mFgColor = mLightMagenta;
                    mFgColorLight = mLightMagenta;
                    mIsDefaultColor = false;
                    break;
                case 96:
                    mFgColor = mLightCyan;
                    mFgColorLight = mLightCyan;
                    mIsDefaultColor = false;
                    break;
                case 97:
                    mFgColor = mLightWhite;
                    mFgColorLight = mLightWhite;
                    mIsDefaultColor = false;
                    break;
                case 100:
                    mBgColor = mLightBlack;
                    break;
                case 101:
                    mBgColor = mLightRed;
                    break;
                case 102:
                    mBgColor = mLightGreen;
                    break;
                case 103:
                    mBgColor = mLightYellow;
                    break;
                case 104:
                    mBgColor = mLightBlue;
                    break;
                case 105:
                    mBgColor = mLightMagenta;
                    break;
                case 106:
                    mBgColor = mLightCyan;
                    break;
                case 107:
                    mBgColor = mLightWhite;
                    break;
                default:
                    qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Unhandled single SGR code sequence CSI " << tag << " m received, Mudlet will ignore it.";
                }
            }
        }
    }
}


void TBufferStyle::decodeSGR38(const QStringList& parameters, bool isColonSeparated)
{
#if defined(DEBUG_SGR_PROCESSING)
    qDebug() << "    TBuffer::decodeSGR38(" << parameters << "," << isColonSeparated << ") INFO - called";
#endif
    if (parameters.at(1) == QLatin1String("5")) {
        int tag = 0;
        if (parameters.count() > 2) {
            bool isOk = false;
            tag = parameters.at(2).toInt(&isOk);
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - failed to parse color index parameter element (the third part) in a SGR...;38:5:" << parameters.at(2)
                                                 << ":...;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - failed to parse color index parameter string (the third part) in a SGR...;38;5;" << parameters.at(2)
                                                 << ";...m sequence treating it as a zero!";
                }
            }
#endif
        } else {
            // Missing last parameter - so it is treated as a zero
#if defined(DEBUG_SGR_PROCESSING)
            if (isColonSeparated) {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - missing color index parameter element (the third part) in a SGR...;38:5;...m sequence treating it as a zero!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - missing color index parameter string (the third part) in a SGR...;38;5;;m sequence treating it as a zero!";
            }
#endif
        }

        if (tag < 16) { // ANSI 0-15 colors
            if (tag >= 8) {
                tag -= 8;
                mBold = true;
            } else {
                mBold = false;
            }
            mIsDefaultColor = false;

            mFgColor = getColorFromAnsi(tag);
            mFgColorLight = getColorFromAnsi(tag + 8);
        } else if (tag < 232) { // 6x6x6 RGB color space
            // because color 0-15 behave like normal ANSI colors
            mFgColor = from6x6x6(tag - 16);
            mFgColorLight = mFgColor;
        } else if (tag < 256) { // grayscale
            mFgColor = fromFgGrayscale(tag);
            mFgColorLight = mFgColor;
        } else {
            mFgColor = QColor(192, 192, 192);
            mFgColorLight = mFgColor;
#if defined(DEBUG_SGR_PROCESSING)
            if (isColonSeparated) {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;38:5:" << parameters.at(2)
                                             << ";..m sequence treating it as 192!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;38;5;" << parameters.at(2)
                                             << ";..m sequence treating it as 192!";
            }
#endif
        }

    } else if (parameters.at(1) == QLatin1String("2")) {
        mFgColor = fromComponents(parameters);
        mFgColorLight = mFgColor;

        if (parameters.count() >= 3 && !parameters.at(2).isEmpty()) {
            if (!isColonSeparated) {
#if !defined(DEBUG_SGR_PROCESSING)
                qDebug() << "Unhandled color space identifier in a SGR...;38;2;" << parameters.at(2)
                         << ";...m sequence - if 16M colors items are missing blue elements you may have checked the \"Expect Color Space Id in SGR...(3|4)8;2;....m codes\" option on the Special "
                            "Options tab of the preferences when it is not needed!";
#else
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unhandled color space identifier in a SGR...;38;2;" << parameters.at(2)
                                             << ";...m sequence treating it as the default (empty) case!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unhandled color space identifier in a SGR...;38:2:" << parameters.at(2)
                                             << ":...;...m sequence treating it as the default (empty) case!";
#endif
            }
        }
    } else if (parameters.at(1) == QLatin1String("4") || parameters.at(1) == QLatin1String("3") || parameters.at(1) == QLatin1String("1") || parameters.at(1) == QLatin1String("0")) {
#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unhandled SGR code: SGR...;38:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unhandled SGR code: SGR...;38;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif

    } else {
#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unexpect SGR code: SGR...;38:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR38(...) WARNING - unexpect SGR code: SGR...;38;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif
    }
}

void TBufferStyle::decodeSGR48(const QStringList& parameters, bool isColonSeparated)
{
#if defined(DEBUG_SGR_PROCESSING)
    qDebug() << "    TBufferStyle::decodeSGR48(" << parameters << "," << isColonSeparated << ") INFO - called";
#endif
    bool useLightColor = false;

    if (parameters.at(1) == QLatin1String("5")) {
        int tag = 0;
        if (parameters.count() > 2) {
            bool isOk = false;
            tag = parameters.at(2).toInt(&isOk);
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - failed to parse color index parameter element (the third part) in a SGR...;48:5:" << parameters.at(2)
                                                 << ":...;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - failed to parse color index parameter string (the third part) in a SGR...;48;5;" << parameters.at(2)
                                                 << ";...m sequence treating it as a zero!";
                }
            }
#endif
        } else {
            // Missing last parameter - so it is treated as a zero
#if defined(DEBUG_SGR_PROCESSING)
            if (isColonSeparated) {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - missing color index parameter element (the third part) in a SGR...;48:5;...m sequence treating it as a zero!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - missing color index parameter string (the third part) in a SGR...;48;5;;m sequence treating it as a zero!";
            }
#endif
        }

        if (tag < 16) {
            mIsDefaultColor = false;
            mBgColor = getColorFromAnsi(tag);
        } else if (tag < 232) {
            // because color 1-15 behave like normal ANSI colors
            mBgColor = from6x6x6(tag - 16);
        } else if (tag < 256) {
            mBgColor = fromBgGrayscale(tag);
        } else {
            mBgColor = QColor(64, 64, 64);
#if defined(DEBUG_SGR_PROCESSING)
            if (isColonSeparated) {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;48:5:" << parameters.at(2)
                                             << ";..m sequence treating it as 64!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;48;5;" << parameters.at(2)
                                             << ";..m sequence treating it as 64!";
            }
#endif
        }

    } else if (parameters.at(1) == QLatin1String("2")) {
        mBgColor = fromComponents(parameters);

        if (parameters.count() >= 3 && !parameters.at(2).isEmpty()) {
            if (!isColonSeparated) {
#if !defined(DEBUG_SGR_PROCESSING)
                qDebug() << "Unhandled color space identifier in a SGR...;48;2;" << parameters.at(2)
                         << ";...m sequence - if 16M colors items are missing blue elements you may have checked the \"Expect Color Space Id in SGR...(3|4)8;2;....m codes\" option on the Special "
                            "Options tab of the preferences when it is not needed!";
#else
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unhandled color space identifier in a SGR...;48;2;" << parameters.at(2)
                                             << ";...m sequence treating it as the default (empty) case!";
            } else {
                qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unhandled color space identifier in a SGR...;48:2:" << parameters.at(2)
                                             << ":...;...m sequence treating it as the default (empty) case!";
#endif
            }
        }

    } else if (parameters.at(1) == QLatin1String("4") || parameters.at(1) == QLatin1String("3") || parameters.at(1) == QLatin1String("1") || parameters.at(1) == QLatin1String("0")) {
#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unhandled SGR code: SGR...;48:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unhandled SGR code: SGR...;48;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif

    } else {
#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unexpect SGR code: SGR...;48:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBufferStyle::decodeSGR48(...) WARNING - unexpect SGR code: SGR...;48;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif
    }
}
QColor TBufferStyle::from6x6x6(int tag)
{
    quint8 r = tag / 36;
    quint8 g = (tag - (r * 36)) / 6;
    quint8 b = (tag - (r * 36)) - (g * 6);
    // Did use 42 as a factor but that isn't right
    // as it yields:
    // 0:0; 1:42; 2:84; 3:126; 4:168; 5:210
    // 6 x 42 DOES equal 252 BUT IT IS OUT OF RANGE
    // Instead we use 51:
    // 0:0; 1:51; 2:102; 3:153; 4:204: 5:255
    return QColor(r * 51, g * 51, b * 51);
}

QColor TBufferStyle::fromComponents(const QStringList& parameters)
{
    int r = 0, g = 0, b = 0;

    if (parameters.count() >= 4) {
        r = qBound(0, parameters.at(3).toInt(), 255);
    }

    if (parameters.count() >= 5) {
        g = qBound(0, parameters.at(4).toInt(), 255);
    }

    if (parameters.count() >= 6) {
        b = qBound(0, parameters.at(5).toInt(), 255);
    }

    return QColor(r, g, b);
}

QColor TBufferStyle::fromFgGrayscale(int tag)
{
    assert(tag >= 232 && tag < 256);

    // black + 23 tone grayscale from dark to light
    // gray. Similar to RGB case the multiplier was
    // a bit off we had been using 10 but:
    // 23 x 10 = 230
    // whereas 23 should map to 255, this requires
    // a non-integer multiplier, instead of
    // multiplying and rounding we, for speed, can
    // use a look-up table:
    int value = 0;
    // clang-format off
    switch (tag) {
    case 232:   value =   0; break; //   0.000
    case 233:   value =  11; break; //  11.087
    case 234:   value =  22; break; //  22.174
    case 235:   value =  33; break; //  33.261
    case 236:   value =  44; break; //  44.348
    case 237:   value =  55; break; //  55.435
    case 238:   value =  67; break; //  66.522
    case 239:   value =  78; break; //  77.609
    case 240:   value =  89; break; //  88.696
    case 241:   value = 100; break; //  99.783
    case 242:   value = 111; break; // 110.870
    case 243:   value = 122; break; // 121.957
    case 244:   value = 133; break; // 133.043
    case 245:   value = 144; break; // 144.130
    case 246:   value = 155; break; // 155.217
    case 247:   value = 166; break; // 166.304
    case 248:   value = 177; break; // 177.391
    case 249:   value = 188; break; // 188.478
    case 250:   value = 200; break; // 199.565
    case 251:   value = 211; break; // 210.652
    case 252:   value = 222; break; // 221.739
    case 253:   value = 233; break; // 232.826
    case 254:   value = 244; break; // 243.913
    case 255:   value = 255; break; // 255.000
    default:
        Q_UNREACHABLE();
    }
    // clang-format on

    return QColor(value, value, value);
}

QColor TBufferStyle::fromBgGrayscale(int tag)
{
    assert(tag >= 232 && tag < 256);

    // black + 23 tone grayscale from dark to light
    // gray. Similar to RGB case the multiplier was
    // a bit off we had been using 10 but:
    // 23 x 10 = 230
    // whereas 23 should map to 255, this requires
    // a non-integer multiplier, instead of
    // multiplying and rounding we, for speed, can
    // use a look-up table:
    int value = 0;
    // clang-format off
    switch (tag) {
    case 232:   value =   0; break; //   0.000
    case 233:   value =  11; break; //  11.087
    case 234:   value =  22; break; //  22.174
    case 235:   value =  33; break; //  33.261
    case 236:   value =  44; break; //  44.348
    case 237:   value =  55; break; //  55.435
    case 238:   value =  67; break; //  66.522
    case 239:   value =  78; break; //  77.609
    case 240:   value =  89; break; //  88.696
    case 241:   value = 100; break; //  99.783
    case 242:   value = 111; break; // 110.870
    case 243:   value = 122; break; // 121.957
    case 244:   value = 133; break; // 133.043
    case 245:   value = 144; break; // 144.130
    case 246:   value = 155; break; // 155.217
    case 247:   value = 166; break; // 166.304
    case 248:   value = 177; break; // 177.391
    case 249:   value = 188; break; // 188.478
    case 250:   value = 200; break; // 199.565
    case 251:   value = 211; break; // 210.652
    case 252:   value = 222; break; // 221.739
    case 253:   value = 233; break; // 232.826
    case 254:   value = 244; break; // 243.913
    case 255:   value = 255; break; // 255.000
    default:
        Q_UNREACHABLE();
    }
    // clang-format on

    return QColor(value, value, value);
}
