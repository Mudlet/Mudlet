/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/texteditorcommand.h"

namespace edbee {

/// The selection command is used to move the caret and anchors to
/// make selections and move the carets around
class SelectionCommand : public TextEditorCommand
{
public:
    enum SelectionType {

      // movement and selection
        MoveCaretByCharacter,           ///< Moves the caret(s) by the given amount of characters
        MoveCaretsOrDeselect,           ///< Moves the caret(s) by the given amount of characters or deselects the current selection
        // SubWord, // TODO, implement subword selecting
        MoveCaretByWord,                ///< moves the caret(s) by the given amount of words
        MoveCaretByLine,                ///< moves the caret(s) by the given amount of lines
        MoveCaretByPage,                ///< moves the caret(s) by the given amount of pages
        MoveCaretToLineBoundary,        ///< moves the caret to a line-boundary (<0 begin of line, >0 end of line)
        MoveCaretToDocumentBegin,       ///< moves the caret to the document start
        MoveCaretToDocumentEnd,         ///< moves the caret to the document end
        MoveCaretToExactOffset,         ///< moves the caret to the given offset (given in  amount)

      // selection only
        SelectAll,                      ///< selects the complete document
        SelectWord,                     ///< select a full word
        SelectFullLine,                 ///< select a full line
        SelectWordAt,                   ///< select 'toggles' a word. Double click on a word to select a word or deselect a word
        ToggleWordSelectionAt,          ///< Toggles the selection and caret at the given location

      // add an extra caret
        AddCaretAtOffset,               ///< adds a extra caret at the given offset (amount is the caret offset)
        AddCaretByLine,                 ///< adds a caret at the given line amount is the amount of lines and the direction to add
        ResetSelection
    };

public:
    explicit SelectionCommand( SelectionType unit, int amount=0, bool keepSelection=false );
    virtual ~SelectionCommand();

    virtual int commandId();


    virtual void execute( TextEditorController* controller );
    virtual QString toString();

    SelectionType unit() { return unit_; }
    int amount() { return amount_; }
    bool keepSelection() { return keepSelection_; }

private:

    SelectionType unit_;
    int amount_;
    bool keepSelection_;

    
};


} // edbee
