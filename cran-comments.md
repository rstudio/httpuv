## Comments

#### 2021-01-11

Bug fixes.

Thank you,
Winston


## Test environments and R CMD check results

* local macOS install 10.15.7
  * R 4.0
* GitHub Actions - https://github.com/rstudio/httpuv/pull/294/checks
  * macOS - R devel
  * macOS, windows, ubuntu 16 - R 4.0
  * windows, ubuntu 16 - R 3.6
  * ubuntu 16 - R 3.5
  * ubuntu 16 - R 3.4
  * ubuntu 16 - R 3.3

* win-builder
  * devel

#### R CMD check results

There are 2 NOTEs, all of which are expected:

* checking installed package size ... NOTE
  installed size is 11.4Mb
  sub-directories of 1Mb or more:
    libs  11.0Mb

* checking for GNU extensions in Makefiles ... NOTE
GNU make is a SystemRequirements.

0 errors | 0 warnings | 2 notes

## revdepcheck results

We checked 57 reverse dependencies (50 from CRAN + 7 from BioConductor), comparing R CMD check results across CRAN and dev versions of this package.

 * We saw 0 new problems
 * We failed to check 0 packages
