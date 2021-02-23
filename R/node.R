mem_node_size <- function(x) {
  .Call(c_ptr_mem_node_size, x)
}

#' @export
mem_node_undominated_parents <- function(x) {
  check_memtools_node(x)

  dominated <- mem_node_dominated_ids(x)
  parents_ids <- purrr::map_chr(x$parents, purrr::pluck, "from", "id")

  x$parents[!parents_ids %in% dominated]
}

#' @export
mem_node_dominated_ids <- function(x) {
  check_memtools_node(x)

  ids <- chr()
  for (node in x$dominated) {
    ids <- c(ids, node$id, mem_node_dominated_ids(node))
  }

  ids
}


is_memtools_node <- function(x) {
  inherits(x, "memtools_node")
}
check_memtools_node <- function(x, arg = substitute(x)) {
  if (!is_memtools_node(x)) {
    msg <- sprintf("`%s` must be a `memtools_node`.", as_string(arg))
    abort(msg)
  }
}

#' @export
print.memtools_node <- function(x, ...) {
  writeLines(sprintf("<memtools/node>"))
  writeLines(sprintf("id: \"%s\"", x$id))
  writeLines(sprintf("type: \"%s\"", x$type))
  writeLines(sprintf("self_size: %d", x$self_size))
  writeLines(sprintf("parents: %s", pillar::obj_sum(list(x$parents))))
  writeLines(sprintf("children: %s", pillar::obj_sum(list(x$children))))
  writeLines(sprintf("dominator: <%s>", pillar::obj_sum(x$dominator)))
  writeLines(sprintf("dominated: %s", pillar::obj_sum(x$dominated)))
}