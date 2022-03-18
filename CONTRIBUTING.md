We welcome contributions to the **httpuv** package. To submit a contribution:

1. [Fork](https://github.com/rstudio/httpuv/fork) the repository and make your changes.

2. Submit a [pull request](https://help.github.com/articles/using-pull-requests).

3. You will be prompted to read and agree to the "RStudio Corporate and Individual Contributor Agreement."

We generally do not merge pull requests that update included web libraries (such as Bootstrap or jQuery) because it is difficult for us to verify that the update is done correctly; we prefer to update these libraries ourselves.


## How to make changes

Before you submit a pull request, please do the following:

* Add an entry to NEWS.md concisely describing what you changed.

* If appropriate, add unit tests in the tests/ directory.

* Run Build->Check Package in the RStudio IDE, or `devtools::check()`, to make sure your change did not add any messages, warnings, or errors.

Doing these things will make it easier for the httpuv development team to evaluate your pull request. Even so, we may still decide to modify your code or even not merge it at all. Factors that may prevent us from merging the pull request include:

* breaking backward compatibility
* adding a feature that we do not consider relevant for httpuv
* is hard to understand
* is hard to maintain in the future
* is computationally expensive
* is not intuitive for people to use

We will try to be responsive and provide feedback in case we decide not to merge your pull request.


## Filing issues

If you find a bug in httpuv, you can also [file an issue](https://github.com/rstudio/httpuv/issues/new). Please provide as much relevant information as you can, and include a minimal reproducible example if possible.
