/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/texteditorcommand.h"


namespace edbee {

/// The Duplicate command.
/// Duplicates the selected line or text
///
class DuplicateCommand : public TextEditorCommand
{
public:
    virtual void execute( TextEditorController* controller );
    virtual QString toString();
};

} // edbee
