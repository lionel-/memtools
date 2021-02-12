#include "rlang.h"

static sexp* quote_prim = NULL;


bool r_is_call(sexp* x, const char* name) {
  if (r_typeof(x) != LANGSXP) {
    return false;
  } else {
    return name == NULL || r_is_symbol(r_node_car(x), name);
  }
}

bool r_is_call_any(sexp* x, const char** names, int n) {
  if (r_typeof(x) != LANGSXP) {
    return false;
  } else {
    return r_is_symbol_any(r_node_car(x), names, n);
  }
}

sexp* r_expr_protect(sexp* x) {
  switch (r_typeof(x)) {
  case r_type_symbol:
  case r_type_call:
  case r_type_promise:
    return r_call2(quote_prim, x);
  default:
    return x;
  }
}


void r_init_library_call() {
  quote_prim = r_base_ns_get("quote");
}
