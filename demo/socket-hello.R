#!/usr/bin/env Rscript

library(httpuv)

app <- list(
  call = function(req) {
    list(
      status = 200L,
      headers = list(
        'Content-Type' = 'text/html'
      ),
      body = paste(
        sep = "\r\n",
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        '<style type="text/css">',
        'body { font-family: Helvetica; }',
        'pre { margin: 0 }',
        '</style>',
        "</head>",
        "<body>",
        '<h3>Hello World</h3>',
        "</body>",
        "</html>"
      )
    )
  }
)

runPipeServer("/tmp/httpuv-hello-demo", strtoi("022", 8), app, 250)
# echo -e "GET / HTTP/1.1\r\n" | socat unix-connect:/tmp/httpuv-hello-demo STDIO

