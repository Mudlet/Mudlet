/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QString>

#include "edbee/texteditorcommand.h"

namespace edbee {

class TextEditorController;

/// This command is used for commenting / decommenting a line
/// We cannot implement this truely at the moment, we first need support for making
/// scope-based 'environment-variables' so we can creating something like TM_COMMENT_START
class CommentCommand : public TextEditorCommand
{  
public:
    CommentCommand( bool block );
    virtual void execute( TextEditorController* controller );
    virtual QString toString(); 

private:

    bool block_;                ///< When this flag is set it uses a block comment (if possible)

};


} // edbee
