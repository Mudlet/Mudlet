//
// Created by gustavo on 21/04/2020.
//

#ifndef MUDLET__TMXPVARTAGHANDLER_H
#define MUDLET__TMXPVARTAGHANDLER_H

//<VAR>     <V>
//<VAR Name [DESC=description] [PRIVATE] [PUBLISH] [DELETE] [ADD] [REMOVE]>Value</VAR>
#include "TMxpTagHandler.h"
class TMxpVarTagHandler : public TMxpTagHandler {
    MxpStartTag mCurrentStartTag;
    QString mCurrentVarContent;
public:
    TMxpVarTagHandler() : mCurrentStartTag(MxpStartTag("VAR")) {}
    bool supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override;
    TMxpTagHandlerResult handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag) override;
    TMxpTagHandlerResult handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag) override;

    void handleContent(char ch) override;
};

#endif//MUDLET__TMXPVARTAGHANDLER_H
