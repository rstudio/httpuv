## Comments

#### 2022-09-08

Releasing a patch to `{httpuv}` which has documentation by the latest version of `{roxygen2}`.

This patch also includes a small update to the configure script of libuv.

Best,
Winston

#### 2022-08-19

....
R 4.2.0 switched to use HTML5 for documentation pages.  Now validation
using HTML Tidy finds problems in the HTML generated from your Rd
files.

To fix, in most cases it suffices to re-generate the Rd files using the
current CRAN version of roxygen2.
....

Best,
-k


## Test environments

* local macOS, R 4.1.3
* GitHub Actions
  * macOS
    * 4.2
  * windows
    * 4.2
  * ubuntu18
    * devel, 4.2, 4.1, 4.0, 3.6, 3.5
* devtools::
  * check_win_devel()

#### R CMD check results

There are 2 NOTEs, all of which are expected:

* checking installed package size ... NOTE
  installed size is 10.7Mb
  sub-directories of 1Mb or more:
    libs  10.3Mb


* checking for GNU extensions in Makefiles ... NOTE
GNU make is a SystemRequirements.

0 errors | 0 warnings | 2 notes

## revdepcheck results

We checked 71 reverse dependencies (64 from CRAN + 7 from Bioconductor), comparing R CMD check results across CRAN and dev versions of this package.

 * We saw 0 new problems
 * We failed to check 0 packages
