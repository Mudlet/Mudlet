/**
 * Copyright 2011-2014 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

namespace edbee {

class TextEditorKeyMap;

/**
 * This class can fill the texteditor keymap with the factory defaults
 */
class FactoryKeyMap
{
public:
    void fill(TextEditorKeyMap* km );
};


} // edbee
