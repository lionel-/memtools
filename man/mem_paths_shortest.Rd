% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/snapshot.R
\name{mem_paths_shortest}
\alias{mem_paths_shortest}
\alias{mem_paths_simple}
\title{Find all shortest or simple paths}
\usage{
mem_paths_shortest(snapshot, to, from = NULL)

mem_paths_simple(snapshot, to, from = NULL)
}
\arguments{
\item{snapshot}{A memory snapshot data frame created by
\code{\link[=mem_snapshot]{mem_snapshot()}}.}

\item{to, from}{Nodes from the \code{node} column of \code{snapshot}.  If
\code{from} is not supplied, the paths from the root are taken by
default. Can also be indices into \code{snapshot$node}.}
}
\description{
Wrappers around \code{\link[igraph:distances]{igraph::all_shortest_paths()}} and
\code{\link[igraph:all_simple_paths]{igraph::all_simple_paths()}}.
}
