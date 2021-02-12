#ifndef RLANG_CND_H
#define RLANG_CND_H

#include <stdbool.h>
#include "rlang.h"


void r_inform(const char* fmt, ...);
void r_warn(const char* fmt, ...);
void r_abort(const char* fmt, ...) __attribute__((noreturn));
void r_interrupt();

__attribute__((noreturn))
void r_stop_internal(const char* fn, const char* fmt, ...);

static inline
__attribute__((noreturn))
void r_stop_unreached(const char* fn) {
  r_stop_internal(fn, "Reached the unreachable.");
}

static inline
__attribute__((noreturn))
void r_stop_unimplemented_type(const char* fn, enum r_type type) {
  r_stop_internal(fn, "Unimplemented type `%s`.", Rf_type2char(type));
}
static inline
__attribute__((noreturn))
void r_stop_unexpected_type(const char* fn, enum r_type type) {
  r_stop_internal(fn, "Unexpected type `%s`.", Rf_type2char(type));
}


static inline
bool r_is_condition(sexp* x) {
  return r_typeof(x) == r_type_list && r_inherits(x, "condition");
}
sexp* r_new_condition(sexp* type, sexp* msg, sexp* data);

void r_cnd_signal(sexp* cnd);
void r_cnd_inform(sexp* cnd, bool mufflable);
void r_cnd_warn(sexp* cnd, bool mufflable);
void r_cnd_abort(sexp* cnd, bool mufflable);

enum r_condition_type {
  r_cnd_type_condition = 0,
  r_cnd_type_message = 1,
  r_cnd_type_warning = 2,
  r_cnd_type_error = 3,
  r_cnd_type_interrupt = 4
};

enum r_condition_type r_cnd_type(sexp* cnd);


#endif
