/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TBuffer.h"

#include <ghostty/vt/sgr.h>

#include <array>
#include <atomic>

namespace {

std::atomic<bool> sGhosttySgrEngine{qEnvironmentVariable("MUDLET_SGR_ENGINE") != qsl("native")};

// A parser handle is not safe to share, and decodeSGR() also runs on TIngestPipeline's worker
class ThreadSgrParser
{
public:
    ThreadSgrParser() { ghostty_sgr_new(nullptr, &mParser); }
    ~ThreadSgrParser() { ghostty_sgr_free(mParser); }
    ThreadSgrParser(const ThreadSgrParser&) = delete;
    ThreadSgrParser& operator=(const ThreadSgrParser&) = delete;
    GhosttySgrParser get() const { return mParser; }

private:
    GhosttySgrParser mParser = nullptr;
};

GhosttySgrParser threadSgrParser()
{
    thread_local ThreadSgrParser parser;
    return parser.get();
}

// decodeSGR38()/decodeSGR48() take the parameters as text; these outlive every call into them
const QString& indexText(const int index)
{
    static const std::array<QString, 256> texts = [] {
        std::array<QString, 256> result;
        for (int i = 0; i < 256; ++i) {
            result[i] = QString::number(i);
        }
        return result;
    }();
    return texts[index];
}

// The colon-separator bit set behind ghostty_sgr_set_params() holds this many entries
constexpr size_t csGhosttyMaxSeparatedParameters = 24;
constexpr size_t csInlineParameters = 32;

} // namespace

bool TBuffer::ghosttySgrEngine()
{
    return sGhosttySgrEngine.load(std::memory_order_relaxed);
}

void TBuffer::setGhosttySgrEngine(const bool state)
{
    sGhosttySgrEngine.store(state, std::memory_order_relaxed);
}

bool TBuffer::decodeSGRWithGhostty(const QStringView sequence, const QColor& hostForegroundColor, const QColor& hostBackgroundColor)
{
    QVarLengthArray<uint16_t, csInlineParameters> parameters;
    QVarLengthArray<char, csInlineParameters> separators;
    bool anyColon = false;
    bool anyEmpty = false;
    bool haveDigit = false;
    uint32_t value = 0;
    for (const QChar ch : sequence) {
        const char16_t unit = ch.unicode();
        if (unit >= u'0' && unit <= u'9') {
            value = std::min<uint32_t>(value * 10 + (unit - u'0'), std::numeric_limits<uint16_t>::max());
            haveDigit = true;
        } else if (unit == u';' || unit == u':') {
            anyEmpty |= !haveDigit;
            haveDigit = false;
            parameters.append(static_cast<uint16_t>(value));
            separators.append(static_cast<char>(unit));
            anyColon |= (unit == u':');
            value = 0;
        } else {
            // '<', '=', '>' and '?' mid-sequence: what Mudlet does with those is its own
            return false;
        }
    }
    parameters.append(static_cast<uint16_t>(value));
    separators.append(';');
    anyEmpty |= !haveDigit;

    // libghostty reads an empty argument as 0, where Mudlet leaves the colour of a 38;5; alone
    if (anyEmpty && !sequence.isEmpty()) {
        return false;
    }
    if (anyColon && static_cast<size_t>(parameters.size()) > csGhosttyMaxSeparatedParameters) {
        return false;
    }

    GhosttySgrParser parser = threadSgrParser();
    if (!parser || ghostty_sgr_set_params(parser, parameters.constData(), anyColon ? separators.constData() : nullptr, static_cast<size_t>(parameters.size())) != GHOSTTY_SUCCESS) {
        return false;
    }

    const std::array<const QColor*, 8> normal{&mBlack, &mRed, &mGreen, &mYellow, &mBlue, &mMagenta, &mCyan, &mWhite};
    const std::array<const QColor*, 8> light{&mLightBlack, &mLightRed, &mLightGreen, &mLightYellow, &mLightBlue, &mLightMagenta, &mLightCyan, &mLightWhite};
    const auto setUnderline = [this](const bool on, const bool wavy, const bool dotted, const bool dashed) {
        mUnderline = on;
        mUnderlineWavy = wavy;
        mUnderlineDotted = dotted;
        mUnderlineDashed = dashed;
    };

    GhosttySgrAttribute attribute;
    while (ghostty_sgr_next(parser, &attribute)) {
        const GhosttySgrAttributeValue& v = attribute.value;
        switch (attribute.tag) {
        case GHOSTTY_SGR_ATTR_UNSET:
            mIsDefaultColor = true;
            mForeGroundColor = hostForegroundColor;
            mBackGroundColor = hostBackgroundColor;
            mBold = false;
            mBoldAsked = false;
            mItalics = false;
            mOverline = false;
            mReverse = false;
            mStrikeOut = false;
            setUnderline(false, false, false, false);
            mBlink = false;
            mFastBlink = false;
            mConcealed = false;
            mAltFont = 0;
            break;
        case GHOSTTY_SGR_ATTR_BOLD:
            mBold = true;
            mBoldAsked = true;
            break;
        // Mudlet has never drawn faint text, and has always read SGR 2 as the end of bold
        case GHOSTTY_SGR_ATTR_FAINT:
        case GHOSTTY_SGR_ATTR_RESET_BOLD:
            mBold = false;
            mBoldAsked = false;
            break;
        case GHOSTTY_SGR_ATTR_ITALIC:
            mItalics = true;
            break;
        case GHOSTTY_SGR_ATTR_RESET_ITALIC:
            mItalics = false;
            break;
        case GHOSTTY_SGR_ATTR_UNDERLINE:
            switch (v.underline) {
            case GHOSTTY_SGR_UNDERLINE_NONE:
                setUnderline(false, false, false, false);
                break;
            case GHOSTTY_SGR_UNDERLINE_CURLY:
                setUnderline(true, true, false, false);
                break;
            case GHOSTTY_SGR_UNDERLINE_DOTTED:
                setUnderline(true, false, true, false);
                break;
            case GHOSTTY_SGR_UNDERLINE_DASHED:
                setUnderline(true, false, false, true);
                break;
            default:
                // Mudlet has no double underline to draw
                setUnderline(true, false, false, false);
                break;
            }
            break;
        case GHOSTTY_SGR_ATTR_OVERLINE:
            mOverline = true;
            break;
        case GHOSTTY_SGR_ATTR_RESET_OVERLINE:
            mOverline = false;
            break;
        case GHOSTTY_SGR_ATTR_BLINK:
            // libghostty reports SGR 5 and SGR 6 alike, so fast blink cannot be told apart
            mBlink = true;
            mFastBlink = false;
            break;
        case GHOSTTY_SGR_ATTR_RESET_BLINK:
            mBlink = false;
            mFastBlink = false;
            break;
        case GHOSTTY_SGR_ATTR_INVERSE:
            mReverse = true;
            break;
        case GHOSTTY_SGR_ATTR_RESET_INVERSE:
            mReverse = false;
            break;
        case GHOSTTY_SGR_ATTR_INVISIBLE:
            mConcealed = true;
            break;
        case GHOSTTY_SGR_ATTR_RESET_INVISIBLE:
            mConcealed = false;
            break;
        case GHOSTTY_SGR_ATTR_STRIKETHROUGH:
            mStrikeOut = true;
            break;
        case GHOSTTY_SGR_ATTR_RESET_STRIKETHROUGH:
            mStrikeOut = false;
            break;
        case GHOSTTY_SGR_ATTR_FG_8:
            mForeGroundColor = *normal[v.fg_8 & 7];
            mForeGroundColorLight = *light[v.fg_8 & 7];
            mIsDefaultColor = false;
            break;
        case GHOSTTY_SGR_ATTR_BRIGHT_FG_8:
            mForeGroundColor = *light[v.bright_fg_8 & 7];
            mForeGroundColorLight = mForeGroundColor;
            mIsDefaultColor = false;
            break;
        case GHOSTTY_SGR_ATTR_BG_8:
            mBackGroundColor = *normal[v.bg_8 & 7];
            break;
        case GHOSTTY_SGR_ATTR_BRIGHT_BG_8:
            mBackGroundColor = *light[v.bright_bg_8 & 7];
            break;
        case GHOSTTY_SGR_ATTR_FG_256: {
            const SgrParameters indexed{u"38", u"5", indexText(v.fg_256)};
            decodeSGR38(indexed, false);
        } break;
        case GHOSTTY_SGR_ATTR_BG_256: {
            const SgrParameters indexed{u"48", u"5", indexText(v.bg_256)};
            decodeSGR48(indexed, false);
        } break;
        case GHOSTTY_SGR_ATTR_DIRECT_COLOR_FG:
            mForeGroundColor = QColor(v.direct_color_fg.r, v.direct_color_fg.g, v.direct_color_fg.b);
            mForeGroundColorLight = mForeGroundColor;
            break;
        case GHOSTTY_SGR_ATTR_DIRECT_COLOR_BG:
            mBackGroundColor = QColor(v.direct_color_bg.r, v.direct_color_bg.g, v.direct_color_bg.b);
            break;
        case GHOSTTY_SGR_ATTR_RESET_FG:
            mIsDefaultColor = true;
            mForeGroundColor = hostForegroundColor;
            mForeGroundColorLight = hostForegroundColor;
            break;
        case GHOSTTY_SGR_ATTR_RESET_BG:
            mBackGroundColor = hostBackgroundColor;
            break;
        // Parsed so their arguments are not read as codes of their own, but Mudlet draws no underline colour
        case GHOSTTY_SGR_ATTR_UNDERLINE_COLOR:
        case GHOSTTY_SGR_ATTR_UNDERLINE_COLOR_256:
        case GHOSTTY_SGR_ATTR_RESET_UNDERLINE_COLOR:
            break;
        case GHOSTTY_SGR_ATTR_UNKNOWN:
            if (v.unknown.partial_ptr && v.unknown.partial_len && v.unknown.full_ptr) {
                const auto index = static_cast<qsizetype>(v.unknown.partial_ptr - v.unknown.full_ptr);
                const bool colonGroup = index >= 0 && index < separators.size() && separators.at(index) == ':';
                applyGhosttyUnknownSgr(sequence, v.unknown.partial_ptr, v.unknown.partial_len, colonGroup);
            }
            if (Q_UNLIKELY(mSpeculationStopped)) {
                return true;
            }
            break;
        default:
            break;
        }
    }
    return true;
}

// What libghostty does not model but Mudlet does: alternate fonts and the italic sub-parameter. Without a colon
// the parameters run to the end of the list, with one they are that colon group.
void TBuffer::applyGhosttyUnknownSgr(const QStringView sequence, const uint16_t* parameters, const size_t count, const bool colonGroup)
{
    const uint16_t code = parameters[0];
    if (!colonGroup && code >= 10 && code <= 19) {
        mAltFont = code - 10;
        return;
    }
    if (colonGroup && code == 3 && count >= 2) {
        switch (parameters[1]) {
        case 0:
            mItalics = false;
            return;
        case 1:
        case 2:
            mItalics = true;
            return;
        default:
            mItalics = false;
            return;
        }
    }
    if (speculationStops()) {
        return;
    }
    qDebug().noquote().nospace() << "TBuffer::decodeSGRWithGhostty(\"" << sequence << "\") INFO - unhandled SGR code " << code << ", Mudlet will ignore it.";
}
