## Comments

#### 2021-08-18

Bug fixes.

Thank you,
Winston


## Test environments and R CMD check results

* GitHub Actions - https://github.com/rstudio/httpuv/pull/309/checks
  * macOS
    * devel, release
  * windows
    * release, 3.6
  * ubuntu20
    * devel, release, oldrel/1, oldrel/2, oldrel/3, oldrel/4
  * ubuntu18
    * devel, release, oldrel/1, oldrel/2, oldrel/3, oldrel/4
* devtools::
  * check_win_devel()
  * check_win_release()
  * check_win_oldrelease()

#### R CMD check results

There are 2 NOTEs, all of which are expected:

* checking installed package size ... NOTE
  installed size is 17.6Mb
  sub-directories of 1Mb or more:
    libs  17.2Mb


* checking for GNU extensions in Makefiles ... NOTE
GNU make is a SystemRequirements.

0 errors | 0 warnings | 2 notes

## revdepcheck results

We checked 63 reverse dependencies (56 from CRAN + 7 from BioConductor), comparing R CMD check results across CRAN and dev versions of this package.

 * We saw 0 new problems
 * We failed to check 0 packages
