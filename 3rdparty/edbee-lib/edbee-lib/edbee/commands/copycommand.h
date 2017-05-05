/**
 * Copyright 2011-2012 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QString>

#include "edbee/texteditorcommand.h"

namespace edbee {

class TextEditorController;


/// This command is used for copying data to the clipboard
class CopyCommand : public TextEditorCommand
{
public:
    static const QString EDBEE_TEXT_TYPE;

public:
    virtual void execute( TextEditorController* controller );
    virtual QString toString();
};


} // edbee
