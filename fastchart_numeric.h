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

#ifndef FASTCHART_NUMERIC_H
#define FASTCHART_NUMERIC_H

#include <math.h>

typedef struct {
	double sum;
	double correction;
} fastchart_sum;

static inline void fastchart_sum_add(fastchart_sum *acc, double value)
{
	double total = acc->sum + value;
	if (fabs(acc->sum) >= fabs(value)) {
		acc->correction += (acc->sum - total) + value;
	} else {
		acc->correction += (value - total) + acc->sum;
	}
	acc->sum = total;
}

static inline void fastchart_sum_product(
	fastchart_sum *acc, double left, double right)
{
	double product = left * right;
	fastchart_sum_add(acc, product);
	fastchart_sum_add(acc, fma(left, right, -product));
}

static inline double fastchart_sum_value(const fastchart_sum *acc)
{
	return acc->sum + acc->correction;
}

static inline double fastchart_binary_scale(double magnitude)
{
	int exponent;
	if (magnitude == 0.0) return 1.0;
	frexp(magnitude, &exponent);
	/* A power of two keeps scaling exact except at the subnormal floor. */
	return scalbn(1.0, exponent - 1);
}

#endif
