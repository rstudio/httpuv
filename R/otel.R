# Make promise domain for active span
otel_create_active_span_promise_domain <- function(
  active_span
  # , tracer = otel::get_tracer()
  # span_ctx = tracer$get_current_span_context()
) {
  force(active_span)

  promises::new_promise_domain(
    wrapOnFulfilled = function(onFulfilled) {
      # During binding ("then()")
      force(onFulfilled)

      function(value) {
        # During runtime ("resolve()")
        otel::with_active_span(active_span, {
          onFulfilled(value)
        })
      }
    },
    wrapOnRejected = function(onRejected) {
      force(onRejected)

      function(reason) {
        otel::with_active_span(active_span, {
          onRejected(reason)
        })
      }
    }
  )
}

# with_promise_domain <- function(domain, expr, replace = FALSE) {
#   oldval <- current_promise_domain()
#   if (replace) {
#     globals$domain <- domain
#   } else {
#     globals$domain <- compose_domains(oldval, domain)
#   }
#   on.exit(globals$domain <- oldval)

#   if (!is.null(domain)) domain$wrapSync(expr) else force(expr)
# }

# Make a promise domain for a local scope
local_promise_domain <- function(
  domain,
  ...,
  replace = FALSE,
  local_envir = parent.frame()
) {
  stopifnot(length(list(...)) == 0L)

  oldval <- promises:::current_promise_domain()
  if (replace) {
    promises:::globals$domain <- domain
  } else {
    promises:::globals$domain <- promises:::compose_domains(
      promises:::globals$domain,
      domain
    )
  }
  withr::defer(
    {
      promises:::globals$domain <- oldval
    },
    envir = local_envir
  )

  if (!is.null(domain)) {
    domain$wrapSync(expr)
    promises::with_promise_domain(
      createGraphicsDevicePromiseDomain(device_id),
      {
        .next(...)
      }
    )
  }
}

# Set promise domain to local scope with active span
local_active_span_promise_domain <- function(
  active_span,
  ...,
  replace = FALSE,
  .envir = parent.frame()
) {
  stopifnot(length(list(...)) == 0L)

  act_span_pd <- otel_create_active_span_promise_domain(active_span)

  local_promise_domain(
    act_span_pd,
    replace = replace,
    local_envir = .envir
  )

  invisible()
}


otel_start_active_span <- function(
  name,
  ...,
  attributes = list(),
  active_frame = parent.frame()
) {
  otel::start_span(
    name = name,
    scope = NULL,
    active_frame = active_frame,
    attributes = otel::as_attributes(attributes)
  )
}


otel_start_active_call_span <- function(
  req,
  ...,
  active_frame = parent.frame()
) {
  str(as.list(req))

  otel_start_active_span(
    paste0(
      # "httpuv ",
      req$METHOD,
      " ",
      req$PATH
    ),

    active_frame = active_frame,

    # https://opentelemetry.io/docs/specs/semconv/http/http-spans/#http-server
    attributes = list(
      # HTTP request method.
      # Ex: `"GET"`, `"POST"`, `"PUT"`, etc.
      'http.request.method' = req$METHOD,

      # The URI path component
      # Ex: `"/users/123"`
      'url.path' = req$PATH,

      # The URI scheme component identifying the used protocol.
      # Ex: `"http"`, `"https"`
      'url.scheme' = req$HTTP_VERSION,

      # Describes a class of error the operation ended with.
      # https://opentelemetry.io/docs/specs/semconv/registry/attributes/error/
      # 'error.type' = NULL,

      # Original HTTP method sent by the client in the request line
      # Ex: `"GET"`, `"POST"`, `"PUT"`, etc.
      'http.request.method_original' = req$METHOD,

      # HTTP response status code
      # Ex: `200`
      'http.response.status_code' = NULL,

      # # The matched route, that is, the path template in the format used by the respective server framework.
      # # Ex: `"/users/:userID?"`
      # ## Maybe something for plumber2?
      # 'http.route' = NULL,

      # OSI application layer or non-OSI equivalent.
      # Ex: `"http"`, `"spdy"`
      'network.protocol.name' = "http",

      # Port of the local HTTP server that received the request.
      # Ex: `80`, `8080`, `443`
      'server.port' = if (is.null(req$SERVER_PORT)) {
        NULL
      } else {
        as.integer(req$SERVER_PORT)
      },

      'url.scheme' = req$HTTP_SCHEME,

      # The URI query component.
      # Ex: `"q=OpenTelemetry"`
      'url.query' = req$QUERY_STRING,

      # Client address
      # Ex: `"83.164.160.102"`
      # TODO: Implement `client.address`
      'client.address' = req$REMOTE_ADDR,

      # # Peer address of the network connection - IP address or Unix domain socket name.
      # # Ex: `"10.1.2.80"`, `"/tmp/my.sock"`
      # # TODO: Implement `network.peer.address`
      # 'network.peer.address' = req$REMOTE_ADDR,

      # # Peer port number of the network connection.
      # # Ex: `65123`
      # # TODO: Implement `network.peer.port`
      # 'network.peer.port' = if (is.null(req$REMOTE_PORT)) NULL else as.integer(req$REMOTE_PORT),

      # # The actual version of the protocol used for network communication.
      # # Ex: `"1.0"`, `"1.1"`, `"2"`, `"3"`
      # # TODO: Implement `network.protocol.version`
      # 'network.protocol.version' = req$HTTP_VERSION,

      # Name of the local HTTP server that received the request.
      # Ex: `"example.com"`, `"10.1.2.80"`, `"/tmp/my.sock"`
      'server.address' = req$SERVER_NAME,

      # Value of the HTTP User-Agent header sent by the client.
      # Ex: `"CERN-LineMode/2.15 libwww/2.17b3"`, `"Mozilla/5.0 (iPhone; CPU iPhone OS 14_7_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.2 Mobile/15E148 Safari/604.1"`
      'user_agent.original' = req$HTTP_USER_AGENT,

      # The port of whichever client was captured in client.address.
      # Ex: `65123`
      # TODO: Implement `client.port`
      'client.port' = if (is.null(req$REMOTE_PORT)) {
        NULL
      } else {
        as.integer(req$REMOTE_PORT)
      },

      # The size of the request payload body in bytes.
      # This is the number of bytes transferred excluding headers and is often, but not always, present as the Content-Length header.
      # For requests using transport encoding, this should be the compressed size.
      # TODO: Implement `http.request.body.size`
      'http.request.body.size' = if (is.null(req$HTTP_CONTENT_LENGTH)) {
        NULL
      } else {
        as.integer(req$HTTP_CONTENT_LENGTH)
      },

      # HTTP request headers, <key> being the normalized HTTP Header name (lowercase), the value being the header values.
      # Ex: `["application/json"]`, `["1.2.3.4", "1.2.3.5"]`
      # TODO: Implement `http.request.header.<key>`
      # TODO: Expand the list! rlang::list2()? !!! the result
      'http.request.header' = lapply(req$HEADERS, function(x) {
        if (is.null(x)) {
          return(NULL)
        }
        # Are these formats needed?

        if (is.character(x)) {
          return(as.character(x))
        } else if (is.raw(x)) {
          return(rawToChar(x))
        } else {
          return(as.character(x))
        }
      }),

      # # The total size of the request in bytes.
      # # This should be the total number of bytes sent over the wire, including the request line (HTTP/1.1), framing (HTTP/2 and HTTP/3), headers, and request body if any.
      # # Ex: `1437`
      # TODO: Implement `http.request.size`
      # 'http.request.size' = if (is.null(req$HTTP_CONTENT_LENGTH)) NULL else as.integer(req$HTTP_CONTENT_LENGTH) + nchar(req$PATH) + nchar(req$HTTP_VERSION) + sum(nchar(names(req$HEADERS))) + sum(nchar(unlist(req$HEADERS))),

      # # The size of the response payload body in bytes.
      # # This is the number of bytes transferred excluding headers and is often, but not always, present as the Content-Length header.
      # # For requests using transport encoding, this should be the compressed size.
      # # Ex: `3495`
      # TODO: Implement `http.response.body.size`
      # 'http.response.body.size' = if (is.null(req$HTTP_CONTENT_LENGTH)) NULL else as.integer(req$HTTP_CONTENT_LENGTH),

      # HTTP response headers, <key> being the normalized HTTP Header name (lowercase), the value being the header values.
      # Ex: `["application/json"]`, `["abc", "def"]`
      # TODO: Implement headers
      'http.response.header' = lapply(res$HEADERS, function(x) {
        if (is.null(x)) {
          return(NULL)
        }
        # Are these formats needed?

        if (is.character(x)) {
          return(as.character(x))
        } else if (is.raw(x)) {
          return(rawToChar(x))
        } else {
          return(as.character(x))
        }
      }),

      # # The total size of the response in bytes.
      # # This should be the total number of bytes sent over the wire, including the status line (HTTP/1.1), framing (HTTP/2 and HTTP/3), headers, and response body and trailers if any.
      # # Ex: `1437`
      # # TODO: Implement `http.response.size`
      # 'http.response.size' = if (is.null(req$HTTP_CONTENT_LENGTH)) NULL else as.integer(req$HTTP_CONTENT_LENGTH) + nchar(req$PATH) + nchar(req$HTTP_VERSION) + sum(nchar(names(req$HEADERS))) + sum(nchar(unlist(req$HEADERS))),

      # # Local socket address. Useful in case of a multi-IP host.
      # # Ex: `"10.1.2.80"`, `"/tmp/my.sock"`
      # # TODO: Implement `network.local.address`
      # 'network.local.address' = req$SERVER_NAME,

      # # Local socket port. Useful in case of a multi-port host.
      # # Ex: `65123`
      # # TODO: Implement `network.local.port`; get from host?
      # 'network.local.port' = if (is.null(req$SERVER_PORT)) NULL else as.integer(req$SERVER_PORT),

      # # OSI transport layer or inter-process communication method.
      # # Ex: `"tcp"`, `"udp"`
      # # TODO: Implement `network.transport`
      # 'network.transport' = if (is.null(req$HTTP_TRANSPORT)) "tcp" else req$HTTP_TRANSPORT,

      # # Specifies the category of synthetic traffic, such as tests or bots.
      # # Ex: `bot`, `test`
      # # TODO: Implement `user_agent.synthetic.type`
      # 'user_agent.synthetic.type' = if (is.null(req$HTTP_USER_AGENT_SYNTHETIC_TYPE)) NULL else req$HTTP_USER_AGENT_SYNTHETIC_TYPE,
    )
  )
}
