library(rprojroot)

dest_file <- rprojroot::find_package_root_file("src", "mime.cpp")

mimemap <- mime::mimemap

mime_map_text <- mapply(
  function(ext, mime_type) {
    sprintf('  {"%s", "%s"}', ext, mime_type)
  },
  names(mimemap),
  mimemap,
  SIMPLIFY = TRUE,
  USE.NAMES = FALSE
)
mime_map_text <- paste(mime_map_text, collapse = ",\n")


output <- glue::glue(
  .open = "[",
  .close = "]",
  '
// Do not edit.
// Generated by ../tools/update_mime.R using R package mime [packageVersion("mime")]

#include <map>
#include <string>

const std::map<std::string, std::string> mime_map = {
[mime_map_text]
};

std::string find_mime_type(const std::string& ext) {
  std::map<std::string, std::string>::const_iterator it = mime_map.find(ext);
  if (it == mime_map.end()) {
    return "";
  }
  
  return it->second;
}

'
)

cat(output, file = dest_file)
