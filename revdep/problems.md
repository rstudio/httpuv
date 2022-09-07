# ambiorix

<details>

* Version: 2.1.0
* GitHub: https://github.com/devOpifex/ambiorix
* Source code: https://github.com/cran/ambiorix
* Date/Publication: 2022-04-06 18:42:29 UTC
* Number of recursive dependencies: 73

Run `revdep_details(, "ambiorix")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘promises’ ‘websocket’
      All declared Imports should be used.
    ```

# autota

<details>

* Version: 0.1.3
* GitHub: NA
* Source code: https://github.com/cran/autota
* Date/Publication: 2020-03-22 07:10:09 UTC
* Number of recursive dependencies: 50

Run `revdep_details(, "autota")` for more info

</details>

## In both

*   checking tests ...
    ```
      Running ‘testthat.R’
     ERROR
    Running the tests in ‘tests/testthat.R’ failed.
    Last 13 lines of output:
      Backtrace:
          ▆
       1. ├─testthat::test_check("autota")
       2. │ └─testthat::test_dir(...)
       3. │   └─testthat:::test_files(...)
       4. │     └─testthat:::test_files(...)
       5. │       └─testthat:::test_files_check(...)
       6. │         └─base::stop("Test failures", call. = FALSE)
       7. └─autota (local) `<fn>`()
      Auto TA failed while trying to handle your error. Try re-installing the package to see if that fixes your issue. Otherwise, click Addins > Disable Auto TA for now.
      To help us improve the Auto TA, please take a screenshot and file an issue on our GitHub:
        https://github.com/willcrichton/r-autota
      The specific error was:
        Error in viewer(full_url): could not find function "viewer"
      Execution halted
    ```

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘memoise’
      All declared Imports should be used.
    ```

*   checking LazyData ... NOTE
    ```
      'LazyData' is specified without a 'data' directory
    ```

# bea.R

<details>

* Version: 1.0.6
* GitHub: https://github.com/us-bea/bea.R
* Source code: https://github.com/cran/bea.R
* Date/Publication: 2018-02-23 19:30:19 UTC
* Number of recursive dependencies: 69

Run `revdep_details(, "bea.R")` for more info

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

# bs4Dash

<details>

* Version: 2.1.0
* GitHub: https://github.com/RinteRface/bs4Dash
* Source code: https://github.com/cran/bs4Dash
* Date/Publication: 2022-05-05 10:50:02 UTC
* Number of recursive dependencies: 106

Run `revdep_details(, "bs4Dash")` for more info

</details>

## In both

*   checking installed package size ... NOTE
    ```
      installed size is  5.6Mb
      sub-directories of 1Mb or more:
        doc    2.1Mb
        help   1.3Mb
    ```

# curl

<details>

* Version: 4.3.2
* GitHub: https://github.com/jeroen/curl
* Source code: https://github.com/cran/curl
* Date/Publication: 2021-06-23 07:00:06 UTC
* Number of recursive dependencies: 57

Run `revdep_details(, "curl")` for more info

</details>

## Newly fixed

*   checking tests ...
    ```
      Running ‘engine.R’
      Comparing ‘engine.Rout’ to ‘engine.Rout.save’ ...4d3
    < Using libcurl 7.79.1 with LibreSSL/3.3.6
      Running ‘spelling.R’
      Running ‘testthat.R’
     ERROR
    Running the tests in ‘tests/testthat.R’ failed.
    Last 13 lines of output:
      • libcurl does not have libidn (1)
      
    ...
      Backtrace:
          ▆
       1. └─curl::curl_echo(handle = handle) at test-echo.R:10:2
       2.   └─httpuv::startServer("0.0.0.0", port, list(call = echo_handler))
       3.     └─WebServer$new(host, port, app, quiet)
       4.       └─httpuv (local) initialize(...)
      
      [ FAIL 1 | WARN 0 | SKIP 1 | PASS 212 ]
      Error: Test failures
      Execution halted
    ```

## In both

*   checking compiled code ... NOTE
    ```
    File ‘curl/libs/curl.so’:
      Found non-API call to R: ‘R_new_custom_connection’
    
    Compiled code should not call non-API entry points in R.
    
    See ‘Writing portable packages’ in the ‘Writing R Extensions’ manual.
    ```

# epitweetr

<details>

* Version: 2.0.3
* GitHub: https://github.com/EU-ECDC/epitweetr
* Source code: https://github.com/cran/epitweetr
* Date/Publication: 2022-01-05 10:00:08 UTC
* Number of recursive dependencies: 144

Run `revdep_details(, "epitweetr")` for more info

</details>

## In both

*   checking package dependencies ... NOTE
    ```
    Package suggested but not available for checking: ‘taskscheduleR’
    ```

*   checking installed package size ... NOTE
    ```
      installed size is  5.2Mb
      sub-directories of 1Mb or more:
        doc    3.0Mb
        java   1.3Mb
    ```

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘httpuv’ ‘knitr’ ‘plyr’ ‘tidyverse’ ‘tokenizers’ ‘xml2’
      All declared Imports should be used.
    ```

# ganalytics

<details>

* Version: 0.10.7
* GitHub: https://github.com/jdeboer/ganalytics
* Source code: https://github.com/cran/ganalytics
* Date/Publication: 2019-03-02 09:20:03 UTC
* Number of recursive dependencies: 129

Run `revdep_details(, "ganalytics")` for more info

</details>

## In both

*   checking LazyData ... NOTE
    ```
      'LazyData' is specified without a 'data' directory
    ```

# igvR

<details>

* Version: 1.14.0
* GitHub: NA
* Source code: https://github.com/cran/igvR
* Date/Publication: 2021-10-26
* Number of recursive dependencies: 114

Run `revdep_details(, "igvR")` for more info

</details>

## In both

*   checking whether the namespace can be unloaded cleanly ... WARNING
    ```
    ---- unloading
    Error in .mergeMethodsTable(generic, mtable, tt, attach) : 
      trying to get slot "defined" from an object of a basic class ("environment") with no slots
    Calls: unloadNamespace ... <Anonymous> -> .updateMethodsInTable -> .mergeMethodsTable
    Execution halted
    ```

*   checking package vignettes in ‘inst/doc’ ... WARNING
    ```
      Found ‘inst/doc/makefile’: should be ‘Makefile’ and will be ignored
    ```

*   checking installed package size ... NOTE
    ```
      installed size is  6.7Mb
      sub-directories of 1Mb or more:
        doc       3.2Mb
        extdata   2.0Mb
    ```

# JBrowseR

<details>

* Version: 0.9.1
* GitHub: https://github.com/GMOD/JBrowseR
* Source code: https://github.com/cran/JBrowseR
* Date/Publication: 2022-07-19 22:30:02 UTC
* Number of recursive dependencies: 74

Run `revdep_details(, "JBrowseR")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘ids’
      All declared Imports should be used.
    ```

# MetaIntegrator

<details>

* Version: 2.1.3
* GitHub: NA
* Source code: https://github.com/cran/MetaIntegrator
* Date/Publication: 2020-02-26 13:00:11 UTC
* Number of recursive dependencies: 179

Run `revdep_details(, "MetaIntegrator")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘BiocManager’ ‘DT’ ‘GEOmetadb’ ‘RMySQL’ ‘RSQLite’ ‘gplots’ ‘pheatmap’
      ‘readr’
      All declared Imports should be used.
    ```

# opencpu

<details>

* Version: 2.2.8
* GitHub: https://github.com/opencpu/opencpu
* Source code: https://github.com/cran/opencpu
* Date/Publication: 2022-05-18 08:10:02 UTC
* Number of recursive dependencies: 64

Run `revdep_details(, "opencpu")` for more info

</details>

## In both

*   checking data for non-ASCII characters ... NOTE
    ```
      Note: found 4 marked UTF-8 strings
    ```

# phantasus

<details>

* Version: 1.14.0
* GitHub: https://github.com/ctlab/phantasus
* Source code: https://github.com/cran/phantasus
* Date/Publication: 2021-10-28
* Number of recursive dependencies: 159

Run `revdep_details(, "phantasus")` for more info

</details>

## In both

*   checking examples ... ERROR
    ```
    Running examples in ‘phantasus-Ex.R’ failed
    The error most likely occurred in:
    
    > ### Name: generatePreloadedSession
    > ### Title: Generate files for preloaded session from a session link.
    > ### Aliases: generatePreloadedSession
    > 
    > ### ** Examples
    > 
    > sessionURL <- "https://ctlab.itmo.ru/phantasus/?session=x063c1b365b9211" # link from 'Get dataset link...' tool in phantasus
    > newName <- "my_session" # user defined name
    > preloadedDir <- "./preloaded" # directory where files will be stored. In order too get access through phantasus web-app should be preloadedDir
    > dir.create(preloadedDir, showWarnings = FALSE)
    > generatePreloadedSession(sessionURL= sessionURL,
    +                          preloadedName = newName,
    +                          preloadedDir = preloadedDir)
    Error in generatePreloadedSession(sessionURL = sessionURL, preloadedName = newName,  : 
      Invalid session
    Execution halted
    ```

*   checking installed package size ... NOTE
    ```
      installed size is 21.7Mb
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
    generatePreloadedSession: no visible binding for global variable 'es'
    generatePreloadedSession: no visible binding for global variable
      'heatmapJson'
    loadSession: no visible binding for global variable 'es'
    reproduceInR: no visible global function definition for 'object.size'
    safeDownload: no visible binding for global variable 'tempDestFile'
    Undefined global functions or variables:
      es heatmapJson object.size tempDestFile
    Consider adding
      importFrom("utils", "object.size")
    to your NAMESPACE file.
    ```

*   checking Rd files ... NOTE
    ```
    prepare_Rd: convertByAnnotationDB.Rd:27-32: Dropping empty section \examples
    ```

# plumbertableau

<details>

* Version: 0.1.0
* GitHub: https://github.com/rstudio/plumbertableau
* Source code: https://github.com/cran/plumbertableau
* Date/Publication: 2021-08-06 08:00:02 UTC
* Number of recursive dependencies: 69

Run `revdep_details(, "plumbertableau")` for more info

</details>

## Newly fixed

*   checking examples ... ERROR
    ```
    Running examples in ‘plumbertableau-Ex.R’ failed
    The error most likely occurred in:
    
    > ### Name: tableau_invoke
    > ### Title: Programatically invoke a Tableau extension function
    > ### Aliases: tableau_invoke
    > 
    > ### ** Examples
    > 
    > pr_path <- system.file("plumber/stringutils/plumber.R",
    ...
    +   package = "plumbertableau")
    > 
    > tableau_invoke(pr_path, "/lowercase", LETTERS[1:5])
    Verbose logging is off. To enable it please set the environment variable `DEBUGME` to include `plumbertableau`.
    
    
    createTcpServer: address already in use
    Error in initialize(...) : Failed to create server
    Calls: tableau_invoke ... <Anonymous> -> startServer -> <Anonymous> -> initialize
    Execution halted
    ```

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘glue’
      All declared Imports should be used.
    ```

# rchie

<details>

* Version: 1.0.2
* GitHub: https://github.com/noamross/rchie
* Source code: https://github.com/cran/rchie
* Date/Publication: 2019-05-07 22:11:19 UTC
* Number of recursive dependencies: 52

Run `revdep_details(, "rchie")` for more info

</details>

## In both

*   checking LazyData ... NOTE
    ```
      'LazyData' is specified without a 'data' directory
    ```

# RCyjs

<details>

* Version: 2.16.0
* GitHub: NA
* Source code: https://github.com/cran/RCyjs
* Date/Publication: 2021-10-26
* Number of recursive dependencies: 36

Run `revdep_details(, "RCyjs")` for more info

</details>

## In both

*   checking Rd \usage sections ... WARNING
    ```
    Undocumented arguments in documentation object 'hideNodes,RCyjs-method'
      ‘nodeIDs’
    
    Undocumented arguments in documentation object 'readAndStandardizeJSONNetworkFile'
      ‘filename’
    Documented arguments not in \usage in documentation object 'readAndStandardizeJSONNetworkFile':
      ‘file’
    
    Undocumented arguments in documentation object 'readAndStandardizeJSONStyleFile'
      ‘filename’
    ...
      ‘file’
    
    Undocumented arguments in documentation object 'showNodes,RCyjs-method'
      ‘nodeIDs’
    
    Functions with \usage entries need to have the appropriate \alias
    entries, and all their arguments documented.
    The \usage entries must correspond to syntactically valid R code.
    See chapter ‘Writing R documentation files’ in the ‘Writing R
    Extensions’ manual.
    ```

# Rlinkedin

<details>

* Version: 0.2
* GitHub: https://github.com/mpiccirilli/Rlinkedin
* Source code: https://github.com/cran/Rlinkedin
* Date/Publication: 2016-10-30 08:58:23
* Number of recursive dependencies: 15

Run `revdep_details(, "Rlinkedin")` for more info

</details>

## In both

*   checking LazyData ... NOTE
    ```
      'LazyData' is specified without a 'data' directory
    ```

# rStrava

<details>

* Version: 1.1.4
* GitHub: https://github.com/fawda123/rStrava
* Source code: https://github.com/cran/rStrava
* Date/Publication: 2021-10-27 11:40:05 UTC
* Number of recursive dependencies: 88

Run `revdep_details(, "rStrava")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘bitops’ ‘httpuv’
      All declared Imports should be used.
    ```

# shiny

<details>

* Version: 1.7.2
* GitHub: https://github.com/rstudio/shiny
* Source code: https://github.com/cran/shiny
* Date/Publication: 2022-07-19 03:30:02 UTC
* Number of recursive dependencies: 91

Run `revdep_details(, "shiny")` for more info

</details>

## In both

*   checking installed package size ... NOTE
    ```
      installed size is 10.4Mb
      sub-directories of 1Mb or more:
        R     2.0Mb
        www   7.1Mb
    ```

# shinyloadtest

<details>

* Version: 1.1.0
* GitHub: https://github.com/rstudio/shinyloadtest
* Source code: https://github.com/cran/shinyloadtest
* Date/Publication: 2021-02-11 14:50:02 UTC
* Number of recursive dependencies: 91

Run `revdep_details(, "shinyloadtest")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘R6’ ‘scales’ ‘websocket’
      All declared Imports should be used.
    ```

# spotGUI

<details>

* Version: 0.2.3
* GitHub: NA
* Source code: https://github.com/cran/spotGUI
* Date/Publication: 2021-03-30 17:50:02 UTC
* Number of recursive dependencies: 172

Run `revdep_details(, "spotGUI")` for more info

</details>

## In both

*   checking dependencies in R code ... NOTE
    ```
    Namespace in Imports field not imported from: ‘batchtools’
      All declared Imports should be used.
    ```

# tfdeploy

<details>

* Version: 0.6.1
* GitHub: NA
* Source code: https://github.com/cran/tfdeploy
* Date/Publication: 2019-06-14 16:30:03 UTC
* Number of recursive dependencies: 82

Run `revdep_details(, "tfdeploy")` for more info

</details>

## In both

*   checking LazyData ... NOTE
    ```
      'LazyData' is specified without a 'data' directory
    ```

# Ularcirc

<details>

* Version: 1.12.0
* GitHub: NA
* Source code: https://github.com/cran/Ularcirc
* Date/Publication: 2021-10-26
* Number of recursive dependencies: 147

Run `revdep_details(, "Ularcirc")` for more info

</details>

## In both

*   checking examples ... ERROR
    ```
    Running examples in ‘Ularcirc-Ex.R’ failed
    The error most likely occurred in:
    
    > ### Name: bsj_to_circRNA_sequence
    > ### Title: bsj_to_circRNA_sequence
    > ### Aliases: bsj_to_circRNA_sequence
    > 
    > ### ** Examples
    > 
    > 
    > library('Ularcirc')
    > library('BSgenome.Hsapiens.UCSC.hg38')
    Error in library("BSgenome.Hsapiens.UCSC.hg38") : 
      there is no package called ‘BSgenome.Hsapiens.UCSC.hg38’
    Execution halted
    ```

*   checking package dependencies ... NOTE
    ```
    Package suggested but not available for checking: ‘BSgenome.Hsapiens.UCSC.hg38’
    ```

*   checking installed package size ... NOTE
    ```
      installed size is  5.8Mb
      sub-directories of 1Mb or more:
        doc       1.1Mb
        extdata   4.3Mb
    ```

*   checking dependencies in R code ... NOTE
    ```
    Namespaces in Imports field not imported from:
      ‘DT’ ‘GenomeInfoDb’ ‘GenomeInfoDbData’ ‘Organism.dplyr’ ‘Sushi’
      ‘ggplot2’ ‘ggrepel’ ‘gsubfn’ ‘mirbase.db’ ‘moments’ ‘shinyFiles’
      ‘shinydashboard’ ‘shinyjs’ ‘yaml’
      All declared Imports should be used.
    ```

# webglobe

<details>

* Version: 1.0.3
* GitHub: https://github.com/r-barnes/webglobe
* Source code: https://github.com/cran/webglobe
* Date/Publication: 2020-09-15 22:20:03 UTC
* Number of recursive dependencies: 90

Run `revdep_details(, "webglobe")` for more info

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

* Version: 1.4.1
* GitHub: https://github.com/rstudio/websocket
* Source code: https://github.com/cran/websocket
* Date/Publication: 2021-08-18 20:30:02 UTC
* Number of recursive dependencies: 54

Run `revdep_details(, "websocket")` for more info

</details>

## In both

*   checking for GNU extensions in Makefiles ... NOTE
    ```
    GNU make is a SystemRequirements.
    ```

