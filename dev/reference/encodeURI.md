# URI encoding/decoding

Encodes/decodes strings using URI encoding/decoding in the same way that
web browsers do. The precise behaviors of these functions can be found
at developer.mozilla.org:
[encodeURI](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI),
[encodeURIComponent](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent),
[decodeURI](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURI),
[decodeURIComponent](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/decodeURIComponent)

## Usage

``` r
encodeURI(value)

encodeURIComponent(value)

decodeURI(value)

decodeURIComponent(value)
```

## Arguments

- value:

  Character vector to be encoded or decoded.

## Value

Encoded or decoded character vector of the same length as the input
value. `decodeURI` and `decodeURIComponent` will return strings that are
UTF-8 encoded.

## Details

Intended as a faster replacement for
[`utils::URLencode()`](https://rdrr.io/r/utils/URLencode.html) and
[`utils::URLdecode()`](https://rdrr.io/r/utils/URLencode.html).

encodeURI differs from encodeURIComponent in that the former will not
encode reserved characters: `;,/?:@&=+$`

decodeURI differs from decodeURIComponent in that it will refuse to
decode encoded sequences that decode to a reserved character. (If in
doubt, use decodeURIComponent.)

For `encodeURI` and `encodeURIComponent`, input strings will be
converted to UTF-8 before URL-encoding.
