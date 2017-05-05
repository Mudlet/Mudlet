/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include "edbee/util/logging.h"

#ifdef QT_DEBUG
    #include "edbee/util/mem/debug_new.h"
    /// This assert requires the inclusion of QApplication an QThread
    #define Q_ASSERT_GUI_THREAD Q_ASSERT(  qApp->thread() == QThread::currentThread( ) )
    #define Q_ASSERT_NOT_GUI_THREAD Q_ASSERT(  qApp->thread() != QThread::currentThread( ) )
#else
    #define Q_ASSERT_GUI_THREAD
    #define Q_ASSERT_NOT_GUI_THREAD
    #define pause_memleak_detection(value)
#endif


