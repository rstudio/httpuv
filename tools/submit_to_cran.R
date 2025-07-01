# Call with
# source("tools/submit_to_cran.R")

# Remove all unknown files
cli::cli_alert_info("Cleaning")
system("git clean -xdi src")

pkg <- devtools::as.package(".")
built_path <- devtools:::build_cran(pkg, args = NULL)

devtools::check_built(built_path)

if (
  devtools:::yesno(
    "Ready to submit ",
    pkg$package,
    " (",
    pkg$version,
    ") to CRAN?"
  )
) {
  return(invisible())
}

devtools:::upload_cran(pkg, built_path)
usethis::with_project(pkg$path, devtools:::flag_release(pkg))
