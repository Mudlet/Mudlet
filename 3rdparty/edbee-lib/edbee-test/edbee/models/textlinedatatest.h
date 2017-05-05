/**
 * Copyright 2011-2012 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/util/test.h"

namespace edbee {

class TextLineDataTest : public edbee::test::TestCase
{
    Q_OBJECT

private slots:
    
    void testLineDataManager();


};

} // edbee

DECLARE_TEST(edbee::TextLineDataTest);
