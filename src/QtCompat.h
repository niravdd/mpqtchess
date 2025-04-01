#pragma once

// IMPORTANT: This header must be included BEFORE any other Qt headers
// in all source files to ensure proper namespace handling

// Undefine any problematic macros that might be set by system headers
#ifdef signals
#undef signals
#endif

#ifdef slots
#undef slots
#endif

// Define macro for qualified usage of Qt types
#define QTCOMPAT_USE_NAMESPACE_QT

// Predefine QT namespace macros with proper qualification
#define signals public QT_PREPEND_NAMESPACE(QPrivateSignal):
#define slots Q_SLOTS
#define emit Q_EMIT

// Now include core Qt headers with proper namespace handling
#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QMap>

// Include standard C++ headers we commonly use
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>