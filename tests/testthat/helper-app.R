library(curl)
library(promises)


random_open_port <- function(min = 3000, max = 9000, n = 20) {
  # Unsafe port list from shiny::runApp()
  valid_ports <- setdiff(min:max, c(3659, 4045, 6000, 6665:6669, 6697))

  # Try up to n ports
  for (port in sample(valid_ports, n)) {
    handle <- NULL

    # Check if port is open
    tryCatch(
      handle <- httpuv::startServer("127.0.0.1", port, list()),
      error = function(e) { }
    )
    if (!is.null(handle)) {
      httpuv::stopServer(handle)
      return(port)
    }
  }

  stop("Cannot find an available port")
}


curl_fetch_async <- function(url, pool = NULL, data = NULL, handle = new_handle()) {
  p <- promises::promise(function(resolve, reject) {
    curl_fetch_multi(url, done = resolve, fail = reject, pool = pool, data = data, handle = handle)
  })
  
  finished <- FALSE
  poll <- function() {
    if (!finished) {
      multi_run(timeout = 0, poll = TRUE, pool = pool)
      later::later(poll, 0.01)
    }
  }
  poll()
  
  p %>% finally(function() {
    finished <<- TRUE
  })
}


# A way of sending an HTTP request using a socketConnection. This isn't as
# reliable as using curl, so we'll use it only when curl can't do what we want.
http_request_con_async <- function(request, host, port) {
  resolve_fun <- NULL
  reject_fun  <- NULL
  con         <- NULL

  p <- promises::promise(function(resolve, reject) {
    resolve_fun <<- resolve
    reject_fun  <<- reject
    con <<- socketConnection(host, port)
    writeLines(c(request, ""), con)
  })
  
  result   <- NULL
  # finished <- FALSE
  poll <- function() {
    result <<- readLines(con)
    if (length(result) > 0) {
      resolve_fun(result)
    } else {
      later::later(poll, 0.01)
    }
  }
  poll()
  
  p %>% finally(function() {
    close(con)
  })
}


wait_for_it <- function() {
  while (!later::loop_empty()) {
    later::run_now()
  }
}


# Block until the promise is resolved/rejected. If resolved, return the value.
# If rejected, throw (yes throw, not return) the error.
extract <- function(promise) {
  promise_value <- NULL
  error <- NULL
  promise %...>%
    (function(value) promise_value <<- value) %...!%
    (function(reason) error <<- reason)

  wait_for_it()
  if (!is.null(error))
    stop(error)
  else
    promise_value
}


# Make an HTTP request using curl.
fetch <- function(url, handle = new_handle()) {
  p <- curl_fetch_async(url, handle = handle)
  extract(p)
}

# Make an HTTP request using a socketConnection. Not as robust as fetch(), so
# we'll use this only when necessary.
http_request_con <- function(request, host, port) {
  p <- http_request_con_async(request, host, port)
  extract(p)
}


local_url <- function(path, port) {
  stopifnot(grepl("^/", path))
  paste0("http://127.0.0.1:", port, path)
}

parse_http_date <- function(x) {
  strptime(x, format = "%a, %d %b %Y %H:%M:%S GMT", tz = "GMT")  
}

raw_file_content <- function(filename) {
  size <- file.info(filename)$size
  readBin(filename, "raw", n = size)
}
