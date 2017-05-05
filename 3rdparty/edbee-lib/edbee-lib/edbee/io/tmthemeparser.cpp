/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */


#include "edbee/views/texttheme.h"
#include "tmthemeparser.h"

#include "edbee/debug.h"

namespace edbee {

TmThemeParser::TmThemeParser()
{
}



/// reads the content of a single file
/// @param device the device to read from. The device NEEDS to be open!!
/// @return the language grammar or 0 on error
TextTheme* TmThemeParser::readContent(QIODevice *device)
{
    TextTheme* result=0;

    if( beginParsing(device) ) {
        QVariant plist = readNextPlistType( );
        result = createTheme( plist );
    }

    if( !endParsing() ) {
        delete result;
        result = 0;
    }

    // returns the language
    return result;
}

/// fetches the settings from the hashmap and puts them in the theme file
void TmThemeParser::fillRuleSettings(TextTheme* theme, const QHash<QString, QVariant> &settings)
{
    theme->setBackgroundColor( QColor( settings.value("background").toString() ) );
    theme->setForegroundColor( QColor( settings.value("foreground").toString() ) );
    theme->setCaretColor( QColor( settings.value("caret").toString() ) );
    theme->setInvisiblesColor( QColor( settings.value("invisibles").toString() ) );
    theme->setLineHighlightColor( QColor( settings.value("lineHighlight").toString() ) );
    theme->setSelectionColor( QColor( settings.value("selection").toString() ) );
    theme->setFindHighlightForegroundColor( QColor( settings.value("findHighlightForeground").toString() ) );
    theme->setFindHighlightBackgroundColor( QColor( settings.value("findHighlight").toString() ) );
    theme->setSelectionBorderColor( QColor( settings.value("selectionBorder").toString() ) );
    theme->setActiveGuideColor( QColor( settings.value("activeGuide").toString() ) );
    theme->setBracketForegroundColor( QColor( settings.value("bracketsForeground").toString() ) );
    theme->setBracketOptions( settings.value("bracketsOptions").toString() );
    theme->setBracketContentsForegroundColor( QColor( settings.value("bracketContentsForeground").toString() ) );
    theme->setBracketContentsOptions( settings.value("bracketContentsOptions").toString() );
    theme->setTagsOptions( settings.value("tagsOptions").toString() );
}

/// parses all rules
void TmThemeParser::parseRules(TextTheme *theme, const QList<QVariant> &rules)
{
    foreach( const QVariant& rule, rules ) {
        QHash<QString,QVariant> hashMap = rule.toHash();
        QString name = hashMap.value("name").toString();
        QString scope = hashMap.value("scope").toString();
        QString comment = hashMap.value("comment").toString();
        QHash<QString, QVariant> settings = hashMap.value("settings").toHash();

        // no name or scope then it's the settings rule
        if( name.isEmpty() && scope.isEmpty() ) {
            fillRuleSettings( theme, settings);      // yes it's true settings/settings !
        } else {
            QColor foreground( settings.value("foreground").toString());
            QColor background( settings.value("background").toString());
            QString fontStyle( settings.value("fontStyle").toString());
            bool bold = fontStyle.contains("bold");
            bool italic = fontStyle.contains("italic");
            bool underline = fontStyle.contains("underline");

            TextThemeRule* rule = new TextThemeRule(name,scope,foreground,background,bold,italic,underline);
            theme->giveThemeRule( rule );
        }

    }
}

/// Reads and parsers the theme
TextTheme *TmThemeParser::createTheme(QVariant& data)
{
    QHash<QString,QVariant> hashMap = data.toHash();

    QString name = hashMap.value("name").toString();
    QString uuid = hashMap.value("uuid").toString();

    if( name.isEmpty()  ) {
        raiseError("Name is empty. Cannot parse theme!");
        return 0;
    }

    TextTheme* theme = new TextTheme();

    // fill the settings
    QList<QVariant> rules = hashMap.value("settings").toList();
    parseRules(theme,rules);


    return theme;
}


} // edbee
