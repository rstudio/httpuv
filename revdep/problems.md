# autota

<details>

* Version: 0.1.3
* Source code: https://github.com/cran/autota
* Date/Publication: 2020-03-22 07:10:09 UTC
* Number of recursive dependencies: 42

Run `revdep_details(,"autota")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘memoise’
      All declared Imports should be used.
    ```

# bea.R

<details>

* Version: 1.0.6
* Source code: https://github.com/cran/bea.R
* URL: https://github.com/us-bea/bea.R
* Date/Publication: 2018-02-23 19:30:19 UTC
* Number of recursive dependencies: 74

Run `revdep_details(,"bea.R")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘Rcpp’ ‘chron’ ‘colorspace’ ‘gtable’ ‘htmltools’ ‘htmlwidgets’
      ‘httpuv’ ‘magrittr’ ‘munsell’ ‘plyr’ ‘scales’ ‘stringi’ ‘xtable’
      ‘yaml’
      All declared Imports should be used.
    ```

# curl

<details>

* Version: 4.3
* Source code: https://github.com/cran/curl
* URL: https://jeroen.cran.dev/curl (docs) https://github.com/jeroen/curl#readme (devel) https://curl.haxx.se/libcurl/ (upstream)
* BugReports: https://github.com/jeroen/curl/issues
* Date/Publication: 2019-12-02 14:00:03 UTC
* Number of recursive dependencies: 47

Run `revdep_details(,"curl")` for more info

</details>

## In both

*   checking compiled code ... NOTE
    ```
    File ‘curl/libs/curl.so’:
      Found non-API call to R: ‘R_new_custom_connection’
    
    Compiled code should not call non-API entry points in R.
    
    See ‘Writing portable packages’ in the ‘Writing R Extensions’ manual.
    ```

# elementR

<details>

* Version: 1.3.6
* Source code: https://github.com/cran/elementR
* URL: https://github.com/charlottesirot/elementR
* BugReports: https://github.com/charlottesirot/elementR/issues
* Date/Publication: 2018-05-03 14:08:24 UTC
* Number of recursive dependencies: 120

Run `revdep_details(,"elementR")` for more info

</details>

## In both

*   checking examples ... ERROR
    ```
    Running examples in ‘elementR-Ex.R’ failed
    The error most likely occurred in:
    
    > ### Name: elementR_project
    > ### Title: Object elementR_project
    > ### Aliases: elementR_project
    > 
    > ### ** Examples
    > 
    > ## create a new elementR_repStandard object based on the "filePath" 
    > ## from a folder containing sample replicate
    > 
    > filePath <- system.file("Example_Session", package="elementR")
    > 
    > exampleProject <- elementR_project$new(filePath)
    Error in structure(.External(.C_dotTclObjv, objv), class = "tclObj") : 
      [tcl] invalid command name "toplevel".
    Calls: <Anonymous> ... tktoplevel -> tkwidget -> tcl -> .Tcl.objv -> structure
    Execution halted
    ```

# igvR

<details>

* Version: 1.8.2
* Source code: https://github.com/cran/igvR
* URL: https://paul-shannon.github.io/igvR/
* Date/Publication: 2020-05-12
* Number of recursive dependencies: 99

Run `revdep_details(,"igvR")` for more info

</details>

## In both

*   checking Rd \usage sections ... WARNING
    ```
    Undocumented arguments in documentation object 'BedpeInteractionsTrack-class'
      ‘table’ ‘displayMode’
    
    Functions with \usage entries need to have the appropriate \alias
    entries, and all their arguments documented.
    The \usage entries must correspond to syntactically valid R code.
    See chapter ‘Writing R documentation files’ in the ‘Writing R
    Extensions’ manual.
    ```

*   checking package vignettes in ‘inst/doc’ ... WARNING
    ```
      Found ‘inst/doc/makefile’: should be ‘Makefile’ and will be ignored
    ```

*   checking installed package size ... NOTE
    ```
      installed size is  5.9Mb
      sub-directories of 1Mb or more:
        doc       2.6Mb
        extdata   1.9Mb
    ```

# MetaIntegrator

<details>

* Version: 2.1.3
* Source code: https://github.com/cran/MetaIntegrator
* URL: http://biorxiv.org/content/early/2016/08/25/071514
* Date/Publication: 2020-02-26 13:00:11 UTC
* Number of recursive dependencies: 171

Run `revdep_details(,"MetaIntegrator")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘BiocManager’ ‘DT’ ‘GEOmetadb’ ‘RMySQL’ ‘RSQLite’ ‘gplots’ ‘pheatmap’
      ‘readr’
      All declared Imports should be used.
    ```

# mlflow

<details>

* Version: 1.8.0
* Source code: https://github.com/cran/mlflow
* URL: https://github.com/mlflow/mlflow
* BugReports: https://github.com/mlflow/mlflow/issues
* Date/Publication: 2020-04-22 05:00:02 UTC
* Number of recursive dependencies: 79

Run `revdep_details(,"mlflow")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘xml2’
      All declared Imports should be used.
    ```

# opencpu

<details>

* Version: 2.1.7
* Source code: https://github.com/cran/opencpu
* URL: https://www.opencpu.org (website) https://github.com/opencpu/opencpu#readme (devel)
* BugReports: https://github.com/opencpu/opencpu/issues
* Date/Publication: 2020-05-06 06:30:14 UTC
* Number of recursive dependencies: 59

Run `revdep_details(,"opencpu")` for more info

</details>

## In both

*   checking data for non-ASCII characters ... NOTE
    ```
      Note: found 4 marked UTF-8 strings
    ```

# phantasus

<details>

* Version: 1.8.0
* Source code: https://github.com/cran/phantasus
* URL: https://genome.ifmo.ru/phantasus, https://artyomovlab.wustl.edu/phantasus
* BugReports: https://github.com/ctlab/phantasus/issues
* Date/Publication: 2020-04-27
* Number of recursive dependencies: 141

Run `revdep_details(,"phantasus")` for more info

</details>

## In both

*   checking Rd \usage sections ... WARNING
    ```
    Documented arguments not in \usage in documentation object 'write.gct':
      ‘...’
    
    Functions with \usage entries need to have the appropriate \alias
    entries, and all their arguments documented.
    The \usage entries must correspond to syntactically valid R code.
    See chapter ‘Writing R documentation files’ in the ‘Writing R
    Extensions’ manual.
    ```

*   checking installed package size ... NOTE
    ```
      installed size is 21.6Mb
      sub-directories of 1Mb or more:
        doc        3.0Mb
        testdata   4.3Mb
        www       14.0Mb
    ```

*   checking dependencies in R code ... NOTE
    ```
    Unexported objects imported by ':::' calls:
      'GEOquery:::getDirListing' 'opencpu:::rookhandler'
      'opencpu:::tmp_root' 'opencpu:::win_or_mac'
      See the note in ?`:::` about the use of this operator.
    ```

*   checking R code for possible problems ... NOTE
    ```
    loadSession: no visible binding for global variable 'es'
    reproduceInR: no visible global function definition for 'object.size'
    safeDownload: no visible binding for global variable 'tempDestFile'
    Undefined global functions or variables:
      es object.size tempDestFile
    Consider adding
      importFrom("utils", "object.size")
    to your NAMESPACE file.
    ```

*   checking Rd files ... NOTE
    ```
    prepare_Rd: convertByAnnotationDB.Rd:27-32: Dropping empty section \examples
    ```

# RCyjs

<details>

* Version: 2.10.0
* Source code: https://github.com/cran/RCyjs
* Date/Publication: 2020-04-27
* Number of recursive dependencies: 81

Run `revdep_details(,"RCyjs")` for more info

</details>

## In both

*   checking Rd \usage sections ... WARNING
    ```
    ...
    Undocumented arguments in documentation object 'hideNodes,RCyjs-method'
      ‘nodeIDs’
    
    Undocumented arguments in documentation object 'readAndStandardizeJSONNetworkFile'
      ‘filename’
    Documented arguments not in \usage in documentation object 'readAndStandardizeJSONNetworkFile':
      ‘file’
    
    Undocumented arguments in documentation object 'readAndStandardizeJSONStyleFile'
      ‘filename’
    Documented arguments not in \usage in documentation object 'readAndStandardizeJSONStyleFile':
      ‘file’
    
    Undocumented arguments in documentation object 'showNodes,RCyjs-method'
      ‘nodeIDs’
    
    Functions with \usage entries need to have the appropriate \alias
    entries, and all their arguments documented.
    The \usage entries must correspond to syntactically valid R code.
    See chapter ‘Writing R documentation files’ in the ‘Writing R
    Extensions’ manual.
    ```

# rfigshare

<details>

* Version: 0.3.7
* Source code: https://github.com/cran/rfigshare
* URL: https://github.com/ropensci/rfigshare
* BugReports: https://github.com/ropensci/rfigshare/issues
* Date/Publication: 2015-06-15 07:59:06
* Number of recursive dependencies: 64

Run `revdep_details(,"rfigshare")` for more info

</details>

## In both

*   checking R code for possible problems ... NOTE
    ```
    fs_author_ids : <anonymous>: no visible global function definition for
      ‘select.list’
    fs_download : <anonymous>: no visible global function definition for
      ‘download.file’
    Undefined global functions or variables:
      download.file select.list
    Consider adding
      importFrom("utils", "download.file", "select.list")
    to your NAMESPACE file.
    ```

# routr

<details>

* Version: 0.4.0
* Source code: https://github.com/cran/routr
* URL: https://routr.data-imaginist.com, https://github.com/thomasp85/routr#routr
* BugReports: https://github.com/thomasp85/routr/issues
* Date/Publication: 2019-10-03 07:20:02 UTC
* Number of recursive dependencies: 53

Run `revdep_details(,"routr")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘httpuv’ ‘utils’
      All declared Imports should be used.
    ```

# rtweet

<details>

* Version: 0.7.0
* Source code: https://github.com/cran/rtweet
* URL: https://CRAN.R-project.org/package=rtweet
* BugReports: https://github.com/ropensci/rtweet/issues
* Date/Publication: 2020-01-08 23:00:10 UTC
* Number of recursive dependencies: 78

Run `revdep_details(,"rtweet")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘Rcpp’
      All declared Imports should be used.
    ```

*   checking data for non-ASCII characters ... NOTE
    ```
      Note: found 113868 marked UTF-8 strings
    ```

# shiny

<details>

* Version: 1.4.0.2
* Source code: https://github.com/cran/shiny
* URL: http://shiny.rstudio.com
* BugReports: https://github.com/rstudio/shiny/issues
* Date/Publication: 2020-03-13 10:00:02 UTC
* Number of recursive dependencies: 68

Run `revdep_details(,"shiny")` for more info

</details>

## In both

*   checking installed package size ... NOTE
    ```
      installed size is 11.2Mb
      sub-directories of 1Mb or more:
        R     2.0Mb
        www   8.2Mb
    ```

# shinyloadtest

<details>

* Version: 1.0.1
* Source code: https://github.com/cran/shinyloadtest
* URL: https://rstudio.github.io/shinyloadtest/, https://github.com/rstudio/shinyloadtest
* BugReports: https://github.com/rstudio/shinyloadtest/issues
* Date/Publication: 2020-01-09 10:20:02 UTC
* Number of recursive dependencies: 92

Run `revdep_details(,"shinyloadtest")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘R6’ ‘getPass’ ‘svglite’ ‘websocket’
      All declared Imports should be used.
    ```

# Ularcirc

<details>

* Version: 1.6.0
* Source code: https://github.com/cran/Ularcirc
* Date/Publication: 2020-04-27
* Number of recursive dependencies: 144

Run `revdep_details(,"Ularcirc")` for more info

</details>

## In both

*   checking package dependencies ... ERROR
    ```
    Package required but not available: ‘mirbase.db’
    
    Packages suggested but not available for checking:
      'BSgenome.Hsapiens.UCSC.hg38', 'org.Hs.eg.db',
      'TxDb.Hsapiens.UCSC.hg38.knownGene'
    
    See section ‘The DESCRIPTION file’ in the ‘Writing R Extensions’
    manual.
    ```

# vkR

<details>

* Version: 0.1
* Source code: https://github.com/cran/vkR
* URL: https://github.com/Dementiy/vkR
* BugReports: https://github.com/Dementiy/vkR/issues
* Date/Publication: 2016-12-02 10:46:29
* Number of recursive dependencies: 48

Run `revdep_details(,"vkR")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Missing or unexported object: ‘jsonlite::rbind.pages’
    ```

# webglobe

<details>

* Version: 1.0.2
* Source code: https://github.com/cran/webglobe
* URL: https://github.com/r-barnes/webglobe/
* BugReports: https://github.com/r-barnes/webglobe/
* Date/Publication: 2017-06-02 17:55:43 UTC
* Number of recursive dependencies: 67

Run `revdep_details(,"webglobe")` for more info

</details>

## In both

*   checking installed package size ... NOTE
    ```
      installed size is 10.5Mb
      sub-directories of 1Mb or more:
        client   9.4Mb
        doc      1.0Mb
    ```

# websocket

<details>

* Version: 1.1.0
* Source code: https://github.com/cran/websocket
* Date/Publication: 2019-08-08 21:20:02 UTC
* Number of recursive dependencies: 41

Run `revdep_details(,"websocket")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘R6’ ‘later’
      All declared Imports should be used.
    ```

*   checking for GNU extensions in Makefiles ... NOTE
    ```
    GNU make is a SystemRequirements.
    ```

# webutils

<details>

* Version: 1.1
* Source code: https://github.com/cran/webutils
* URL: https://github.com/jeroen/webutils
* BugReports: https://github.com/jeroen/webutils/issues
* Date/Publication: 2020-04-28 21:00:02 UTC
* Number of recursive dependencies: 31

Run `revdep_details(,"webutils")` for more info

</details>

## In both

*   checking tests ...
    ```
     ERROR
    Running the tests in ‘tests/testthat.R’ failed.
    Last 13 lines of output:
      createTcpServer: address already in use
      ── 1. Error: Echo a big file (@test-echo.R#55)  ────────────────────────────────
      Failed to create server
      Backtrace:
       1. curl::curl_echo(h)
       2. httpuv::startServer("0.0.0.0", port, list(call = echo_handler))
       3. WebServer$new(host, port, app, quiet)
       4. .subset2(public_bind_env, "initialize")(...)
      
      ══ testthat results  ═══════════════════════════════════════════════════════════
      [ OK: 26 | SKIPPED: 0 | WARNINGS: 0 | FAILED: 1 ]
      1. Error: Echo a big file (@test-echo.R#55) 
      
      Error: testthat unit tests failed
      Execution halted
    ```

