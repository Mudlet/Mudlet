/**
 * Copyright 2011-2012 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/texteditorcommand.h"

namespace edbee {

class TextEditorController;


/// Performs a cut command. The cut command removes the selected content
/// and places onto the clipboard.
///
/// When pressing cut without selection, the full line is cut
class CutCommand : public TextEditorCommand
{
public:
    virtual void execute( TextEditorController* controller );
    virtual QString toString();
};


} // edbee
