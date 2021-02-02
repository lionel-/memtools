#include <stdio.h>
#include <rlang.h>
#include "size.h"

#define DICT_INIT_SIZE 1024

#define STACK_INIT_SIZE 1024
#define STACK_GROWTH_FACTOR 2

#define NODES_INIT_SIZE 65536
#define NODES_GROWTH_FACTOR 2

#define ARROWS_INIT_SIZE 2
#define ARROWS_GROWTH_FACTOR 2


static
const char* snapshot_df_names_c_strings[] = {
  "id",
  "type",
  "parents",
  "self_size",
  "retained_size",
  "retained_count"
};
static
const enum r_type snapshot_df_types[] = {
  r_type_character,
  r_type_character,
  r_type_list,
  r_type_double,
  r_type_double,
  r_type_integer
};
enum snapshot_df_locs {
  SNAPSHOT_DF_LOCS_id = 0,
  SNAPSHOT_DF_LOCS_type,
  SNAPSHOT_DF_LOCS_parents,
  SNAPSHOT_DF_LOCS_self_size,
  SNAPSHOT_DF_LOCS_retained_size,
  SNAPSHOT_DF_LOCS_retained_count
};

#define SNAPSHOT_DF_SIZE R_ARR_SIZEOF(snapshot_df_types)

static
sexp* snapshot_df_names = NULL;


struct snapshot_data {
  r_ssize retained_size;
  r_ssize retained_count;
};
struct snapshot_data_stack {
  sexp* shelter;
  r_ssize size;
  r_ssize n;
  struct snapshot_data v_data[];
};

struct snapshot_node {
  sexp* id;
  enum r_type type;
  r_ssize self_size;
  sexp* arrow_list;
  int arrow_list_n;
  r_ssize retained_count;
  r_ssize retained_size;
};
struct snapshot_node_stack {
  sexp* shelter;
  r_ssize size;
  r_ssize n;
  struct snapshot_node v_nodes[];
};
enum shelter_node {
  SHELTER_NODE_location = 0,
  SHELTER_NODE_arrow_list
};


struct snapshot_state {
  sexp* shelter;
  struct r_dict* p_dict;
  struct snapshot_node_stack* p_node_stack;
  struct snapshot_data_stack* p_data_stack;
};

#include "decl/snapshot-decl.h"


// [[ register() ]]
sexp* snapshot(sexp* x) {
  struct snapshot_state* p_state = new_snapshot_state();
  KEEP(p_state->shelter);

  sexp_iterate(x, &snapshot_iterator, p_state);

  struct snapshot_node_stack* p_node_stack = p_state->p_node_stack;

  // Transform to data frame
  r_ssize n_rows = p_node_stack->n;
  sexp* df = KEEP(r_alloc_df_list(n_rows,
                                  snapshot_df_names,
                                  snapshot_df_types,
                                  SNAPSHOT_DF_SIZE));
  r_init_tibble(df, n_rows);

  sexp* id = r_list_get(df, SNAPSHOT_DF_LOCS_id);
  sexp* type = r_list_get(df, SNAPSHOT_DF_LOCS_type);
  sexp* parents = r_list_get(df, SNAPSHOT_DF_LOCS_parents);
  sexp* self_size = r_list_get(df, SNAPSHOT_DF_LOCS_self_size);
  sexp* retained_size = r_list_get(df, SNAPSHOT_DF_LOCS_retained_size);
  sexp* retained_count = r_list_get(df, SNAPSHOT_DF_LOCS_retained_count);

  double* v_self_size = r_dbl_deref(self_size);
  double* v_retained_size = r_dbl_deref(retained_size);
  int* v_retained_count = r_int_deref(retained_count);

  struct snapshot_node* v_nodes = p_node_stack->v_nodes;

  for (r_ssize i = 0; i < n_rows; ++i) {
    struct snapshot_node node = v_nodes[i];
    r_chr_poke(id, i, node.id);
    r_chr_poke(type, i, r_type_as_string(node.type));
    r_list_poke(parents, i, arrow_list_compact(node.arrow_list));
    v_self_size[i] = r_ssize_as_double(node.self_size);
    v_retained_size[i] = r_ssize_as_double(node.retained_size);
    v_retained_count[i] = r_ssize_as_integer(node.retained_count);
  }

  FREE(2);
  return df;
}

static
struct snapshot_node* get_cached_node(struct snapshot_state* p_state, sexp* cached) {
  int node_i = r_int_get(r_list_get(cached, SHELTER_NODE_location), 0);
  return &p_state->p_node_stack->v_nodes[node_i];
}

static
enum r_sexp_iterate snapshot_iterator(void* payload,
                                      sexp* x,
                                      enum r_type type,
                                      int depth,
                                      sexp* parent,
                                      enum r_node_relation rel,
                                      r_ssize i,
                                      enum r_node_direction dir) {
  struct snapshot_state* p_state = (struct snapshot_state*) payload;

  if (type == r_type_null) {
    return R_SEXP_ITERATE_next;
  }

  sexp* cached = r_dict_get0(p_state->p_dict, x);

  struct snapshot_data_stack* p_data_stack = p_state->p_data_stack;
  struct snapshot_data* p_data = &p_data_stack->v_data[p_data_stack->n - 1];

  if (!cached && dir == R_NODE_DIRECTION_incoming) {
    // Push node
    data_stack_push(&p_state->p_data_stack);
  }

  if (dir == R_NODE_DIRECTION_outgoing) {
    // Commit
    struct snapshot_node* p_node = get_cached_node(p_state, cached);
    p_node->retained_count = p_data->retained_count;
    p_node->retained_size = p_data->retained_size;

    // Collect
    p_data->retained_count += 1;
    p_data->retained_size += p_node->self_size;

    // Pop
    --p_data_stack->n;

    // Carry
    r_ssize i = p_data_stack->n - 1;
    p_data_stack->v_data[i].retained_size += p_data->retained_size;
    p_data_stack->v_data[i].retained_count += p_data->retained_count;

    return R_SEXP_ITERATE_next;
  }


  sexp* id = KEEP(r_sexp_address(x));
  sexp* arrow = KEEP(new_arrow(id, depth, parent, rel, i));

  if (cached) {
    struct snapshot_node* p_node = get_cached_node(p_state, cached);
    node_push_arrow(p_node, arrow, cached);

    FREE(2);
    return R_SEXP_ITERATE_skip;
  }

  // Shelter node objects in the dictionary
  sexp* node_shelter = KEEP(r_new_list(2));

  // Store node location in the stack so we can update the list of
  // parents when the node is reached again
  sexp* node_location = r_int(p_state->p_node_stack->n);
  r_list_poke(node_shelter, SHELTER_NODE_location, node_location);

  sexp* arrow_list = new_arrow_list(x);
  r_list_poke(node_shelter, SHELTER_NODE_arrow_list, arrow_list);

  struct snapshot_node node = {
    .id = id,
    .type = type,
    .self_size = sexp_self_size(x, type),
    .arrow_list = arrow_list
  };

  node_push_arrow(&node, arrow, node_shelter);
  node_stack_push(p_state, node);

  r_dict_put(p_state->p_dict, x, node_shelter);
  FREE(3);

  // Collect leaf
  if (dir == R_NODE_DIRECTION_leaf) {
    // FIXME: What if root is a leaf?
    p_data->retained_count += 1;
    p_data->retained_size += node.self_size;
  }


  // Skip bindings of the global environment as they will contain
  // objects from the debugging session, including memory snapshots.
  // TODO: Traverse global env manually to collect hidden symbols
  // starting with a dot.
  if (parent == r_global_env && rel != R_NODE_RELATION_environment_enclos) {
    return R_SEXP_ITERATE_skip;
  }

  return R_SEXP_ITERATE_next;
}


// Snapshot data ----------------------------------------------------------

enum shelter_snapshot {
  SHELTER_SNAPSHOT_data,
  SHELTER_SNAPSHOT_stack,
  SHELTER_SNAPSHOT_nodes,
  SHELTER_SNAPSHOT_dict,
  SHELTER_SNAPSHOT_total_size
};

static
struct snapshot_state* new_snapshot_state() {
  sexp* shelter = KEEP(r_new_vector(r_type_list, SHELTER_SNAPSHOT_total_size));


  sexp* state_shelter = r_new_vector(r_type_raw, sizeof(struct snapshot_state));
  r_list_poke(shelter, SHELTER_SNAPSHOT_data, state_shelter);

  sexp* data_stack_shelter = r_pairlist(r_new_vector(r_type_raw, data_stack_byte_size(STACK_INIT_SIZE)));
  r_list_poke(shelter, SHELTER_SNAPSHOT_stack, data_stack_shelter);

  sexp* node_stack_shelter = r_pairlist(r_new_vector(r_type_raw, node_stack_byte_size(NODES_INIT_SIZE)));
  r_list_poke(shelter, SHELTER_SNAPSHOT_nodes, node_stack_shelter);


  struct r_dict* p_dict = r_new_dict(DICT_INIT_SIZE);
  r_list_poke(shelter, SHELTER_SNAPSHOT_dict, p_dict->shelter);


  struct snapshot_data_stack* p_data_stack = (struct snapshot_data_stack*) r_raw_deref(r_node_car(data_stack_shelter));
  p_data_stack->shelter = data_stack_shelter;
  p_data_stack->size = STACK_INIT_SIZE;
  p_data_stack->n = 0;

  struct snapshot_node_stack* p_node_stack = (struct snapshot_node_stack*) r_raw_deref(r_node_car(node_stack_shelter));
  p_node_stack->shelter = node_stack_shelter;
  p_node_stack->size = NODES_INIT_SIZE;
  p_node_stack->n = 0;

  struct snapshot_state* state = (struct snapshot_state*) r_raw_deref(state_shelter);
  state->shelter = shelter;
  state->p_node_stack = p_node_stack;
  state->p_data_stack = p_data_stack;
  state->p_dict = p_dict;

  FREE(1);
  return state;
}

static
size_t data_stack_byte_size(size_t n) {
  return
    sizeof(struct snapshot_data_stack) +
    sizeof(struct snapshot_data) * n;
}
static
size_t node_stack_byte_size(size_t n) {
  return
    sizeof(struct snapshot_node_stack) +
    sizeof(struct snapshot_node) * n;
}

static
void data_stack_grow(struct snapshot_data_stack** pp_x, r_ssize i) {
  struct snapshot_data_stack* p_x = *pp_x;

  r_ssize size = p_x->size;
  if (i <= size) {
    return;
  }

  r_ssize new_size = r_ssize_mult(size, STACK_GROWTH_FACTOR);

  sexp* shelter = r_raw_resize(r_node_car(p_x->shelter),
                               data_stack_byte_size(new_size));
  p_x = (struct snapshot_data_stack*) r_raw_deref(shelter);
  r_node_poke_car(p_x->shelter, shelter);

  p_x->size = new_size;
  *pp_x = p_x;
}

static
void node_stack_grow(struct snapshot_state* p_state, r_ssize i) {
  r_ssize size = p_state->p_node_stack->size;
  if (i < size) {
    return;
  }

  r_ssize new_size = r_ssize_mult(size, NODES_GROWTH_FACTOR);

  sexp* node_stack_shelter = r_raw_resize(r_node_car(p_state->p_node_stack->shelter),
                                          node_stack_byte_size(new_size));
  struct snapshot_node_stack* p_node_stack = r_raw_deref(node_stack_shelter);
  r_node_poke_car(p_node_stack->shelter, node_stack_shelter);

  p_node_stack->size = new_size;
  p_state->p_node_stack = p_node_stack;
}

static
void data_stack_push(struct snapshot_data_stack** pp_stack) {
  r_ssize i = (*pp_stack)->n;
  data_stack_grow(pp_stack, i);

  static const struct snapshot_data empty_data = { 0 };
  (*pp_stack)->v_data[i] = empty_data;

  ++((*pp_stack)->n);
}

static
void node_stack_push(struct snapshot_state* p_state,
                     struct snapshot_node node) {
  r_ssize n = p_state->p_node_stack->n++;
  node_stack_grow(p_state, n);

  p_state->p_node_stack->v_nodes[n] = node;
}


// Nodes and arrows -------------------------------------------------------

static
sexp* new_arrow_list(sexp* x) {
  // Make space for a few arrows per node. The arrow lists are
  // compacted later on.
  r_ssize n = ARROWS_INIT_SIZE;

  switch (r_typeof(x)) {
  default:
    break;
  case r_type_character:
  case r_type_expression:
  case r_type_list:
    n += r_length(x);
    break;
  }

  return r_new_list(n);
}

static
const char* v_arrow_names_c_strs[] = {
  "parent",
  "child",
  "depth",
  "rel",
  "i",
  "name"
};
#define ARROW_SIZE R_ARR_SIZEOF(v_arrow_names_c_strs)

enum arrow_locs {
  ARROW_LOCS_parent = 0,
  ARROW_LOCS_child,
  ARROW_LOCS_depth,
  ARROW_LOCS_rel,
  ARROW_LOCS_i,
  ARROW_LOCS_name
};

static
sexp* arrow_names = NULL;

static
sexp* new_arrow(sexp* id,
                int depth,
                sexp* parent,
                enum r_node_relation rel,
                r_ssize i) {
  sexp* arrow = KEEP(r_new_vector(r_type_list, ARROW_SIZE));
  sexp* addr_parent = KEEP(r_sexp_address(parent));

  r_list_poke(arrow, ARROW_LOCS_parent, r_str_as_character(addr_parent));
  r_list_poke(arrow, ARROW_LOCS_child, r_str_as_character(id));
  r_list_poke(arrow, ARROW_LOCS_depth, r_int(depth));
  r_list_poke(arrow, ARROW_LOCS_rel, r_chr(r_node_relation_as_c_string(rel)));
  r_list_poke(arrow, ARROW_LOCS_i, r_len(i));
  r_list_poke(arrow, ARROW_LOCS_name, r_null); // TODO: Fetch name

  r_attrib_poke_names(arrow, arrow_names);

  FREE(2);
  return arrow;
}

static
void node_push_arrow(struct snapshot_node* node,
                     sexp* arrow,
                     sexp* shelter) {
  sexp* arrow_list = node->arrow_list;
  r_ssize n = r_length(arrow_list);

  if (node->arrow_list_n == n) {
    r_ssize new_n = r_ssize_mult(n, ARROWS_GROWTH_FACTOR);
    arrow_list = r_list_resize(arrow_list, new_n);

    r_list_poke(shelter, SHELTER_NODE_arrow_list, arrow_list);
    node->arrow_list = arrow_list;
  }

  r_list_poke(arrow_list, node->arrow_list_n, arrow);
  ++node->arrow_list_n;
}

static
sexp* arrow_list_compact(sexp* x) {
  sexp* const * v_x = r_list_deref_const(x);

  r_ssize i = 0;
  r_ssize n = r_length(x);

  for (; i < n; ++i) {
    if (v_x[i] == r_null) {
      break;
    }
  }

  if (i == n) {
    return x;
  } else {
    return r_list_resize(x, i);
  }
}


void init_snapshot() {
  size_t df_names_size = R_ARR_SIZEOF(snapshot_df_names_c_strings);
  size_t df_types_size = R_ARR_SIZEOF(snapshot_df_types);
  RLANG_ASSERT(df_names_size == df_types_size);

  snapshot_df_names = r_chr_n(snapshot_df_names_c_strings, df_names_size);
  r_preserve_global(snapshot_df_names);

  arrow_names = r_chr_n(v_arrow_names_c_strs, ARROW_SIZE);
  r_preserve_global(arrow_names);
}
