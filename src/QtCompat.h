#pragma once

// This is a minimal QtCompat.h that just helps with Qt version differences

// Make sure Qt macros aren't redefined
#ifndef QT_COMPAT_DEFINES
#define QT_COMPAT_DEFINES

// We'll use Qt's own definitions for these keywords
#ifndef QT_NO_KEYWORDS
// Don't define anything here - let Qt handle it
#endif

#endif // QT_COMPAT_DEFINES

// Include common Qt headers
#include <QtGlobal>