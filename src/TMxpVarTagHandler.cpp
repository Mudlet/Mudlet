//
// Created by gustavo on 21/04/2020.
//

#include "TMxpVarTagHandler.h"
#include "TMxpClient.h"
bool TMxpVarTagHandler::supports(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
{
    return tag->isNamed("VAR") || tag->isNamed("V");
}


TMxpTagHandlerResult TMxpVarTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    mCurrentStartTag = *tag;
    mCurrentVarContent.clear();
    return MXP_TAG_HANDLED;
}

//<VAR>     <V>
//<VAR Name [DESC=description] [PRIVATE] [PUBLISH] [DELETE] [ADD] [REMOVE]>Value</VAR>
TMxpTagHandlerResult TMxpVarTagHandler::handleEndTag(TMxpContext& ctx, TMxpClient& client, MxpEndTag* tag)
{
    const QString& name = mCurrentStartTag.getAttrName(0);
    const QString& value = mCurrentVarContent;

    if (mCurrentStartTag.hasAttr("PUBLISH") || !mCurrentStartTag.hasAttr("DELETE"))
        client.setVariable(name, value);

    return MXP_TAG_HANDLED;
}
void TMxpVarTagHandler::handleContent(char ch)
{
    mCurrentVarContent.append(ch);
}
