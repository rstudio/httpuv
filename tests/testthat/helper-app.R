library(curl)
library(promises)


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

# Given a POSIXct object, return a date string in the format required for a
# HTTP Date header. For example: "Wed, 21 Oct 2015 07:28:00 GMT"
http_date_string <- function(time) {
  weekday_names <- c("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat")
  weekday_num <- as.integer(strftime(time, format = "%w", tz = "GMT"))
  weekday_name <- weekday_names[weekday_num + 1]

  month_names <- c("Jan", "Feb", "Mar", "Apr", "May", "Jun",
                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec")
  month_num   <- as.integer(strftime(time, format = "%m", tz = "GMT"))
  month_name <- month_names[month_num]

  strftime(time,
    paste0(weekday_name, ", %d ", month_name, " %Y %H:%M:%S GMT"),
    tz = "GMT"
  )
}
