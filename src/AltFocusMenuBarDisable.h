#include <QProxyStyle>

class AltFocusMenuBarDisable : public QProxyStyle
{
public:
    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const;

};
