/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QObject>
#include <QHash>

#include "edbee/models/textdocument.h"


namespace edbee {

class TextCodec;
class TextDocumentFilter;
class TextDocumentScopes;
class TextEditorConfig;
class TextLineDataManager;
class TextBufferChange;

/// A plain textdocument. A document with simple character-buffer implementation
class CharTextDocument : public TextDocument
{
Q_OBJECT

public:
    CharTextDocument( QObject* object=0 );
    virtual ~CharTextDocument();


    /// This method should return the active textbuffer
    virtual TextBuffer* buffer() const;

    /// this method can be used to give a 'custom' line data item to a given line
    virtual TextLineDataManager* lineDataManager() { return textLineDataManager_; }


    /// Should return the document-scopes of this document
    virtual TextDocumentScopes* scopes() { return textScopes_; }

    /// This method returns the current encoding
    virtual TextCodec* encoding() { return textCodecRef_; }

    /// Sets the encoding
    virtual void setEncoding( TextCodec* codec ) {
        Q_ASSERT(codec);
        textCodecRef_ = codec;
    }

    /// This method should return the current line ending
    virtual const edbee::LineEnding* lineEnding() { return lineEndingRef_; }

    /// Set the used line ending
    virtual void setLineEnding( const edbee::LineEnding* lineEnding ) {
        Q_ASSERT(lineEnding);
        lineEndingRef_ = lineEnding;
    }


    ///  Should return the current document lexer
    virtual TextLexer* textLexer() { return textLexer_; }

    /// This method should return the current language grammar
    virtual TextGrammar* languageGrammar();
    virtual void setLanguageGrammar( TextGrammar* grammar );

    /// returns the text undo stack
    virtual TextUndoStack* textUndoStack() { return textUndoStack_; }

    virtual TextEditorConfig* config() const;

    virtual Change* giveChangeWithoutFilter(Change* change, int coalesceId );


protected slots:
//    virtual void textReplaced( int offset, int length, const QChar* data, int dataLength );
//    virtual void linesReplaced( int line, int lineCount, int newLineCount );
    virtual void textBufferChanged( const edbee::TextBufferChange& change );

private:
    TextEditorConfig* config_;                          ///< The text editor configuration
    TextBuffer* textBuffer_;                            ///< The textbuffers

    TextLineDataManager* textLineDataManager_;          ///< A class for managing text line data items    
    TextDocumentScopes* textScopes_;                    ///< The text document scopes
    TextLexer* textLexer_;                              ///< The lexer used for finding the scopes

    TextCodec* textCodecRef_;                           ///< The used encoding
    const edbee::LineEnding* lineEndingRef_;            ///< The used line-ending

    TextUndoStack* textUndoStack_;                      ///< The text undo stack


};

} // edbee
