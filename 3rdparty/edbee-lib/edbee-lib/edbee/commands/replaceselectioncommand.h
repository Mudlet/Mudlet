/**
 * Copyright 2011-2012 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/texteditorcommand.h"

namespace edbee {

/// This command can be used to replace the current selection with a given text
class ReplaceSelectionCommand : public TextEditorCommand
{
public:
    ReplaceSelectionCommand( const QString& text, int caolesceId );

    virtual int commandId() { return coalesceId_; }

    virtual void execute( TextEditorController* controller );
    virtual QString toString();

private:
    QString text_;          ///< The text to 'replace'
    int coalesceId_;        ///< The coalesce id to use for this command

};

} // edbee
