# Check whether an address is IPv4 or IPv6

Given an IP address, this checks whether it is an IPv4 or IPv6 address.

## Usage

``` r
ipFamily(ip)
```

## Arguments

- ip:

  A single string representing an IP address.

## Value

For IPv4 addresses, `4`; for IPv6 addresses, `6`. If the address is
neither, `-1`.

## Examples

``` r
ipFamily("127.0.0.1")   # 4
#> [1] 4
ipFamily("500.0.0.500") # -1
#> [1] -1
ipFamily("500.0.0.500") # -1
#> [1] -1

ipFamily("::")          # 6
#> [1] 6
ipFamily("::1")         # 6
#> [1] 6
ipFamily("fe80::1ff:fe23:4567:890a") # 6
#> [1] 6
```
