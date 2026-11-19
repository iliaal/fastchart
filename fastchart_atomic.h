/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2025-2026, Ilia Alshanetsky                            |
  | Copyright (c) 2025-2026, Advanced Internet Designs Inc.              |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD 3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifndef FASTCHART_ATOMIC_H
#define FASTCHART_ATOMIC_H

#include "php.h"
#if PHP_VERSION_ID >= 80200 && PHP_VERSION_ID < 80700
# include "zend_atomic.h"
#endif

/* EG(timed_out) is a plain bool on 8.1, zend_atomic_bool on 8.2-8.6
 * (php/php-src#8327) and a C11 atomic_bool from 8.7. */
static zend_always_inline bool fastchart_timed_out(void)
{
#if PHP_VERSION_ID >= 80700
	return atomic_load(&EG(timed_out));
#elif PHP_VERSION_ID >= 80200
	return zend_atomic_bool_load_ex(&EG(timed_out));
#else
	return EG(timed_out);
#endif
}

#endif
