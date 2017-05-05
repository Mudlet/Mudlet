/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "debugcommand.h"

#include "edbee/models/textdocument.h"
#include "edbee/models/textdocumentscopes.h"
#include "edbee/models/textlexer.h"
#include "edbee/models/textundostack.h"
#include "edbee/texteditorcontroller.h"

#include "edbee/debug.h"

namespace edbee {

DebugCommand::DebugCommand(DebugCommandType command)
    : command_(command)
{
}

void DebugCommand::execute(TextEditorController* controller)
{
    switch( command_ ) {
        case DumpScopes: dumpScopes( controller ); break;
        case RebuildScopes: rebuildScopes( controller ); break;
        case DumpUndoStack: dumpUndoStack( controller ); break;
    }

}

QString DebugCommand::toString()
{
    return "DebugCommand";
}

/// This method dumps the scopes
void DebugCommand::dumpScopes( TextEditorController* controller )
{
    TextDocumentScopes* scopes = controller->textDocument()->scopes();
    QString dump = "\nMultiLineScopes:---\n";
    dump.append( QString(scopes->toString()).replace("|","\n") );

    dump.append("\nPer Line:---\n");
    for( int i=0; i<scopes->scopedLineCount(); ++i ) {
        ScopedTextRangeList* range = scopes->scopedRangesAtLine(i);
        dump.append( QString("%1: %2\n").arg(i).arg( range->toString() ) );
    }

    qlog_info() << dump;
}

/// rebuilds all scopes
void DebugCommand::rebuildScopes(TextEditorController* controller)
{
    TextDocument* document = controller->textDocument();
    TextDocumentScopes* scopes = document->scopes();
    TextLexer* lexer = document->textLexer();

    /// lex the complete document
    scopes->removeScopesAfterOffset(0);
    lexer->lexRange( 0, document->length() );
}

/// dumps the undostack
void DebugCommand::dumpUndoStack(TextEditorController* controller)
{
    qlog_info() << controller->textDocument()->textUndoStack()->dumpStack();
}

} // edbee
