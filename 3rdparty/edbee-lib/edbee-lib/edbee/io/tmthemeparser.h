/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QHash>
#include <QString>
#include <QVariant>

#include "baseplistparser.h"

namespace edbee {

class TextTheme;

class TmThemeParser : public BasePListParser
{
public:
    TmThemeParser();

    TextTheme* readContent(QIODevice* device);

protected:

    void fillRuleSettings(TextTheme* theme, const QHash<QString, QVariant> &settings );
    void parseRules(TextTheme* theme, const QList<QVariant> &settings );
    TextTheme* createTheme( QVariant& data );

};

} // edbee
