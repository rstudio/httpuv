#!/usr/bin/env Rscript

# This script can be run from the command line or sourced from an R session.
library(rprojroot)

version <- "1.23.1"

# The git tag for the https://github.com/libuv/libuv repo.
tag <- paste0("v", version)

dest_file <- file.path(tempdir(), paste0("libuv-", version, ".tar.gz"))

url <- paste0("https://github.com/libuv/libuv/archive/", tag, ".tar.gz")
message("Downloading ", url)
download.file(url, dest_file)

untar(dest_file, exdir = rprojroot::find_package_root_file("src"))

# Remove old libuv
unlink("src/libuv", recursive = TRUE)

# Rename new libuv
file.rename(paste0("src/libuv-", version), "src/libuv")

unlink(dest_file)

message("Make sure to fix up the libuv sources in src/libuv/ as described in src/README.md.")
