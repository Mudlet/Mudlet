/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "tmlanguageparser.h"

#include <QDir>
#include <QList>
#include <QHash>
#include <QVariant>
#include <QXmlStreamReader>

#include "edbee/models/textgrammar.h"
#include "edbee/edbee.h"

#include "edbee/debug.h"

namespace edbee {

/// Parses a textmate language
TmLanguageParser::TmLanguageParser()
{
}


/// reads the content of a single file
/// @param device the device to read from. The device NEEDS to be open!!
/// @return the language grammar or 0 on error
TextGrammar* TmLanguageParser::parse(QIODevice* device)
{
    TextGrammar* result=0;

    if( beginParsing(device) ) {
        QVariant plist = readNextPlistType( );
        result = createLanguage( plist );
    }

    if( !endParsing() ) {
        delete result;
        result = 0;
    }

    // returns the language
    return result;
}


/// parses the given grammar file
/// @param file the file to read
/// @return the language grammar or 0 on error
TextGrammar* TmLanguageParser::parse(const QString& fileName)
{
    QFile file(fileName);
    if( file.open( QIODevice::ReadOnly ) ) {
        TextGrammar* result = parse( &file );
        file.close();
        return result;
    } else {
        setLastErrorMessage( file.errorString()  );
        return 0;
    }
}


/// sets the captures
void TmLanguageParser::addCapturesToGrammarRule(TextGrammarRule* rule, QHash<QString, QVariant> captures, bool endCapture)
{
    if( captures.isEmpty() ){ return; }
    QHashIterator<QString,QVariant> itr(captures);
    while( itr.hasNext() ) {
        itr.next();
        QHash<QString,QVariant> fields = itr.value().toHash();
        int keyIndex = itr.key().toInt();
        QString name = fields.value("name").toString();
        if( endCapture ) {
            rule->setEndCapture(keyIndex,name);
        } else {
            rule->setCapture(keyIndex,name);
        }
    }
}


/// Adds all patterns to the grammar rules
void TmLanguageParser::addPatternsToGrammarRule(TextGrammarRule* rule, QList<QVariant> patterns)
{
    foreach( QVariant pattern, patterns ) {
        TextGrammarRule* childRule = createGrammarRule( rule->grammar(), pattern );
        if( childRule ) { rule->giveRule( childRule ); }
    }
}


/// creates a grammar rue
TextGrammarRule* TmLanguageParser::createGrammarRule( TextGrammar* grammar, const QVariant& data)
{
    QHash<QString,QVariant> map = data.toHash();
    QString match = map.value("match").toString();
    QString include = map.value("include").toString();
    QString begin = map.value("begin").toString();
    QString name = map.value("name").toString();

    // match filled?
    if( !match.isEmpty() ) {
        TextGrammarRule* rule = TextGrammarRule::createSingleLineRegExp( grammar, name, match );
        QHash<QString,QVariant> captures = map.value("captures").toHash();
        addCapturesToGrammarRule( rule, captures );
        return rule;

    } else if( !include.isEmpty() ) {
        return TextGrammarRule::createIncludeRule( grammar, include );

    } else if( !begin.isEmpty() ) {
        QString end = map.value("end").toString();

        // TODO: contentScopeName
        QString contentScope = name;
        TextGrammarRule* rule = TextGrammarRule::createMultiLineRegExp( grammar, name, contentScope, begin, end  );

        // add the patterns
        QList<QVariant> patterns = map.value("patterns").toList();
        addPatternsToGrammarRule( rule, patterns );

        if( map.contains("captures")) {
            QHash<QString,QVariant> captures = map.value("captures").toHash();
            addCapturesToGrammarRule( rule, captures );
            addCapturesToGrammarRule( rule, captures, true );
        }
        if( map.contains("beginCaptures")) {
            QHash<QString,QVariant> captures = map.value("beginCaptures").toHash();
            addCapturesToGrammarRule( rule, captures );
        }
        if( map.contains("endCaptures")) {
            QHash<QString,QVariant> endCaptures = map.value("endCaptures").toHash();
            addCapturesToGrammarRule( rule, endCaptures, true );
        }
        return rule;


    } else {
        TextGrammarRule* rule = TextGrammarRule::createRuleList(grammar);

        // add the patterns
        QList<QVariant> patterns = map.value("patterns").toList();
        addPatternsToGrammarRule( rule, patterns );

        return rule;
    }


//    <key>angle_brackets</key>
//    <dict>
//        <key>begin</key>
//        <string>&lt;</string>
//        <key>end</key>
//        <string>&gt;</string>
//        <key>name</key>
//        <string>meta.angle-brackets.c++</string>
//        <key>patterns</key>
//        <array>
//            <dict>
//                <key>include</key>
//                <string>#angle_brackets</string>
//            </dict>
//            <dict>
//                <key>include</key>
//                <string>$base</string>
//            </dict>
//        </array>
//    </dict>
}


// parses the given language
TextGrammar* TmLanguageParser::createLanguage(QVariant& data)
{
    QHash<QString,QVariant> hashMap = data.toHash();
    QString name      = hashMap.value("name").toString();
    QString scopeName = hashMap.value("scopeName").toString();
    QString uuid      = hashMap.value("uuid").toString();

    if( name.isEmpty() || scopeName.isEmpty() ) {
        raiseError("Name or scope is empty. Cannot parse language!");
        return 0;
    }

    // construct the grammar
    TextGrammar* grammar = new TextGrammar( scopeName, name );

    // and get the main patterns
    // construct the main rule
    TextGrammarRule* mainRule = TextGrammarRule::createMainRule( grammar, scopeName );
    grammar->giveMainRule(mainRule);

    QList<QVariant> patterns = hashMap.value("patterns").toList();
    addPatternsToGrammarRule( mainRule, patterns );


    // get the repos
    QHash<QString,QVariant> repos  = hashMap.value("repository").toHash();
    QHashIterator<QString,QVariant> itr(repos);
    while( itr.hasNext() ) {
        itr.next();
        TextGrammarRule* rule = createGrammarRule( grammar, itr.value() );
        if( rule ) {
            grammar->giveToRepos( itr.key(), rule  );
        } else {
            qlog_warn() << "Error create grammar rule!";
        }
    }

    // add the file types
    QStringList fileTypes = hashMap.value("fileTypes").toStringList();
    foreach( QString fileType, fileTypes ) {
        grammar->addFileExtension( fileType );
    }


    return grammar;
}



} // edbee
