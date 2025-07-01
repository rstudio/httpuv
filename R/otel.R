otel_create_span_promise_domain <- function(
  active_span
  # , tracer = otel::get_tracer()
  # span_ctx = tracer$get_current_span_context()
) {
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


if (FALSE) {
  promises::with_promise_domain(
    createGraphicsDevicePromiseDomain(device_id),
    {
      .next(...)
    }
  )
}
