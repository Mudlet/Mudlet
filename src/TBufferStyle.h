//
// Created by gustavo on 06/05/2020.
//

#ifndef MUDLET_SRC_TBUFFERSTYLE_H
#define MUDLET_SRC_TBUFFERSTYLE_H

#include "TChar.h"
#include "TColorSettings.h"
class TBufferStyle : public TColorSettings
{
public:
    explicit TBufferStyle(const TColorSettings& colorSettings);

    bool mBold;
    bool mItalics;
    bool mOverline;
    bool mReverse;
    bool mStrikeOut;
    bool mUnderline;
    bool mItalicBeforeBlink;

    bool mIsDefaultColor;

    TChar::AttributeFlags getTCharFlags() const;
    TChar createChar() const;
    TChar createTransparent() const;

    void updateColorSettings(const TColorSettings &colors);
    void decodeSGR38(const QStringList& parameters, bool isColonSeparated);
    void decodeSGR48(const QStringList& parameters, bool isColonSeparated);
    void decodeSGR(const QString& sequence, bool haveColorSpaceId, const TColorSettings& colors);



private:
    void resetFontStyle();
};


#endif //MUDLET_SRC_TBUFFERSTYLE_H
