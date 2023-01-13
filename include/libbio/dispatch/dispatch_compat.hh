/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_DISPATCH_DISPATCH_COMPAT_HH
#define LIBBIO_DISPATCH_DISPATCH_COMPAT_HH

// <dispatch/dispatch.h> uses Clang's __has_feature and __has_extension
// without checking, so we need a workaround.

#ifndef __has_feature
#	define __has_feature(x) 0
#endif

#ifndef __has_extension
#	define __has_extension(x) 0
#endif

#include <dispatch/dispatch.h>
#endif
