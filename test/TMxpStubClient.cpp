#include "TMxpStubClient.h"


// Handle 'stacks' of attribute settings:
void TMxpStubClient::setBold(bool bold)
{
    if (bold) {
        boldCtr++;
    } else if (boldCtr > 0){
        boldCtr--;
    }
}

void TMxpStubClient::setItalic(bool italic)
{
    if (italic) {
        italicCtr++;
    } else if (italicCtr > 0){
        italicCtr--;
    }
}

void TMxpStubClient::setUnderline(bool underline)
{
    if (underline) {
        underlineCtr++;
    } else if (underlineCtr > 0){
        underlineCtr--;
    }
}

void TMxpStubClient::setStrikeOut(bool strikeOut)
{
    if (strikeOut) {
        strikeOutCtr++;
    } else if (strikeOutCtr > 0){
        strikeOutCtr--;
    }
}
