/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QList>
#include <QMap>
#include <QStack>
#include <QVariant>

#include "baseplistparser.h"

class QFile;
class QIODevice;
class QXmlStreamReader;

namespace edbee {

class TextGrammar;
class TextGrammarManager;
class TextGrammarRule;

/// For parsing a Textmate Language
class TmLanguageParser : public BasePListParser
{
public:
    TmLanguageParser();
    TextGrammar* parse( QIODevice* device );
    TextGrammar* parse(const QFile& file );
    TextGrammar* parse( const QString& fileName );

protected:

    void addCapturesToGrammarRule( TextGrammarRule* rule, QHash<QString,QVariant> captures, bool endCapture=false );
    void addPatternsToGrammarRule( TextGrammarRule* rule, QList<QVariant> patterns );

    TextGrammarRule* createGrammarRule(TextGrammar *grammar, const QVariant &data );
    TextGrammar* createLanguage(QVariant& data );


};

} // edbee
