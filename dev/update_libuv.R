#!/usr/bin/env Rscript

# This script can be run from the command line or sourced from an R session.

version <- "1.52.1"

# The git tag for the https://github.com/libuv/libuv repo.
tag <- paste0("v", version)

dest_file <- file.path(tempdir(), paste0("libuv-", version, ".tar.gz"))

url <- paste0("https://github.com/libuv/libuv/archive/", tag, ".tar.gz")
message("Downloading ", url)
download.file(url, dest_file)

libuv_dir <- "src/libuv"

untar(dest_file, exdir = "src/")

# Remove old libuv
unlink(libuv_dir, recursive = TRUE)

# Rename new libuv
file.rename(paste0(libuv_dir, "-", version), libuv_dir)

unlink(dest_file)

# Copy over Makefile for mingw
file.copy(
  file.path("dev", "Makefile-libuv.mingw"),
  libuv_dir
)

message(
  "Make sure to fix up the libuv sources in src/libuv/ as described in dev/build-notes.md."
)
