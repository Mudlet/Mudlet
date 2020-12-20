#include "AltFocusMenuBarDisable.h"

#include <QProxyStyle>

int AltFocusMenuBarDisable::styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
{
    if (stylehint == QStyle::SH_MenuBar_AltKeyNavigation)
        return 0;

    return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
}
