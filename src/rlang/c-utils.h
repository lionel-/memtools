#ifndef RLANG_C_UTILS_H
#define RLANG_C_UTILS_H

#include <float.h>
#include "cnd.h"

#define R_ARR_SIZEOF(X) sizeof(X) / sizeof(X[0])

void* r_shelter_deref(sexp* x);


// Adapted from CERT C coding standards
static inline
intmax_t r__intmax_add(intmax_t x, intmax_t y) {
  if ((y > 0 && x > (INTMAX_MAX - y)) ||
      (y < 0 && x < (INTMAX_MIN - y))) {
    r_stop_internal("intmax_add", "Values too large to be added.");
  }

  return x + y;
}
static inline
intmax_t r__intmax_subtract(intmax_t x, intmax_t y) {
  if ((y > 0 && x < (INTMAX_MIN + y)) ||
      (y < 0 && x < (INTMAX_MAX + y))) {
    r_stop_internal("intmax_subtract", "Subtraction resulted in overflow or underflow.");
  }

  return x - y;
}

static inline
r_ssize r_ssize_add(r_ssize x, r_ssize y) {
  intmax_t out = r__intmax_add(x, y);

  if (out > R_SSIZE_MAX) {
    r_stop_internal("r_ssize_safe_add", "Result too large for an `r_ssize`.");
  }

  return (r_ssize) out;
}

static inline
r_ssize r_ssize_mult(r_ssize x, r_ssize y) {
  if (x > 0) {
    if (y > 0) {
      if (x > (R_SSIZE_MAX / y)) {
        goto error;
      }
    } else {
      if (y < (R_SSIZE_MIN / x)) {
        goto error;
      }
    }
  } else {
    if (y > 0) {
      if (x < (R_SSIZE_MIN / y)) {
        goto error;
      }
    } else {
      if ( (x != 0) && (y < (R_SSIZE_MAX / x))) {
        goto error;
      }
    }
  }

  return x * y;

 error:
  r_stop_internal("r_ssize_mult", "Result too large for an `r_ssize`.");
}

static inline
r_ssize r_ssize_min(r_ssize x, r_ssize y) {
  return (y < x) ? y : x;
}

static inline
r_ssize r_ssize_max(r_ssize x, r_ssize y) {
  return (y < x) ? x : y;
}

static inline
int r_ssize_as_integer(r_ssize x) {
  if (x > INT_MAX || x < INT_MIN) {
    r_stop_internal("r_ssize_as_integer", "Result can't be represented as `int`.");
  }

  return (int) x;
}
static inline
double r_ssize_as_double(r_ssize x) {
  if (x > DBL_MAX || x < -DBL_MAX) {
    r_stop_internal("r_ssize_as_double", "Result can't be represented as `double`.");
  }

  return (double) x;
}

static inline
r_ssize r_double_as_ssize(double x) {
  if (x > R_SSIZE_MAX || x < R_SSIZE_MIN) {
    r_stop_internal("r_ssize_as_double", "Result can't be represented as `r_ssize`.");
  }

  return (r_ssize) x;
}

static inline
double r_double_mult(double x, double y) {
  double out = x * y;

  if (!isfinite(out)) {
    r_stop_internal("r_double_mult", "Can't multiply double values.");
  }

  return out;
}

#endif
