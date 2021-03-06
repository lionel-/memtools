% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/node.R
\name{mem_node_parent}
\alias{mem_node_parent}
\alias{mem_node_bindings_parent}
\alias{mem_node_parents}
\title{Climb parents}
\usage{
mem_node_parent(node, i = NULL)

mem_node_bindings_parent(node)

mem_node_parents(node)
}
\arguments{
\item{node}{A memtools node.}

\item{i}{Which parent to return. If \code{NULL}, \code{node} must have a
single parent otherwise this is an error.}
}
\description{
\itemize{
\item \code{mem_node_parent()} returns a direct parent of \code{node}. It is
equivalent to \code{node$parents[[i]]$from}.
}
}
\details{
\itemize{
\item \code{mem_node_parents()} returns all the parents. It is equivalent to
\code{map(node$parents, "from")}.
\item \code{mem_node_bindings_parent()} takes a pairlist as input and
returns the environment which holds onto that pairlist, either
through its frame or its hash table. If the pairlist is not part
of a frame or a hash table, \code{NULL} is returned.
}
}
