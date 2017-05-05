/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/texteditorcommand.h"

namespace edbee {


/// A debug command, for simply suppling/dumping some editor state information to the console
class DebugCommand : public TextEditorCommand
{
public:

    enum DebugCommandType {
        DumpScopes,
        RebuildScopes,
        DumpUndoStack
    };

    DebugCommand( DebugCommandType command );

    virtual void execute( TextEditorController* controller );
    virtual QString toString();

protected:

    void dumpScopes( TextEditorController* controller );
    void rebuildScopes( TextEditorController* controller );

    void dumpUndoStack( TextEditorController* controller );

private:

    DebugCommandType command_;      ///< the command to execute

};

} // edbee
