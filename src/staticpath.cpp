#include "staticpath.h"
#include "thread.h"
#include "utils.h"
#include "constants.h"
#include "optional.h"

// ============================================================================
// StaticPathOptions
// ============================================================================

StaticPathOptions::StaticPathOptions(SEXP options) :
  indexhtml(std::experimental::nullopt),
  fallthrough(std::experimental::nullopt),
  html_charset(std::experimental::nullopt),
  headers(std::experimental::nullopt),
  validation(std::experimental::nullopt),
  exclude(std::experimental::nullopt)
{
  ASSERT_MAIN_THREAD()

  SEXP class_attr = Rf_getAttrib(options, R_ClassSymbol);
  std::string obj_class = "";
  if (class_attr != R_NilValue) {
    obj_class = CHAR(STRING_ELT(class_attr, 0));
  }
  if (obj_class != "staticPathOptions") {
    throw std::runtime_error("staticPath options object must have class 'staticPathOptions'.");
  }

  // This seems to be a necessary intermediary for passing objects to
  // `optional_as()`.
  SEXP temp;

  temp = Rf_getAttrib(options, Rf_install("normalized"));
  std::experimental::optional<bool> normalized = optional_as<bool>(temp);
  if (!normalized || !*normalized) {
    throw std::runtime_error("staticPathOptions object must be normalized.");
  }

  // There's probably a more concise way to do this assignment than by using temp.
  temp = VECTOR_ELT(options, 0);  // indexhtml
  indexhtml = optional_as<bool>(temp);
  temp = VECTOR_ELT(options, 1);  // fallthrough
  fallthrough = optional_as<bool>(temp);
  temp = VECTOR_ELT(options, 2);  // html_charset
  html_charset = optional_as<std::string>(temp);
  temp = VECTOR_ELT(options, 3);  // headers
  if (temp != R_NilValue) {
    headers = response_headers_from_sexp(temp);
  }
  temp = VECTOR_ELT(options, 4);  // validation
  if (temp != R_NilValue) {
    validation = as_cpp<std::vector<std::string>>(temp);
  }
  temp = VECTOR_ELT(options, 5);  // exclude
  exclude = optional_as<bool>(temp);
}

void StaticPathOptions::setOptions(SEXP options) {
  ASSERT_MAIN_THREAD()
  SEXP temp;
  SEXP opts = options;
  if (list_contains_element(opts, "indexhtml")) {
    temp = get_list_element(opts, "indexhtml");
    if (temp != R_NilValue) {
      indexhtml = optional_as<bool>(temp);
    }
  }
  if (list_contains_element(opts, "fallthrough")) {
    temp = get_list_element(opts, "fallthrough");
    if (temp != R_NilValue) {
      fallthrough = optional_as<bool>(temp);
    }
  }
  if (list_contains_element(opts, "html_charset")) {
    temp = get_list_element(opts, "html_charset");
    if (temp != R_NilValue) {
      html_charset = optional_as<std::string>(temp);
    }
  }
  if (list_contains_element(opts, "headers")) {
    temp = get_list_element(opts, "headers");
    if (temp != R_NilValue) {
      headers = response_headers_from_sexp(temp);
    }
  }
  if (list_contains_element(opts, "validation")) {
    temp = get_list_element(opts, "validation");
    if (temp != R_NilValue) {
      validation = as_cpp<std::vector<std::string>>(temp);
    }
  }
  if (list_contains_element(opts, "exclude")) {
    temp = get_list_element(opts, "exclude");
    if (temp != R_NilValue) {
      exclude = optional_as<bool>(temp);
    }
  }
}

SEXP StaticPathOptions::asRObject() const {
  ASSERT_MAIN_THREAD()

  SEXP obj   = PROTECT(Rf_allocVector(VECSXP, 6));
  SEXP names = PROTECT(Rf_allocVector(STRSXP, 6));

  SET_STRING_ELT(names, 0, Rf_mkChar("indexhtml"));
  SET_STRING_ELT(names, 1, Rf_mkChar("fallthrough"));
  SET_STRING_ELT(names, 2, Rf_mkChar("html_charset"));
  SET_STRING_ELT(names, 3, Rf_mkChar("headers"));
  SET_STRING_ELT(names, 4, Rf_mkChar("validation"));
  SET_STRING_ELT(names, 5, Rf_mkChar("exclude"));

  SET_VECTOR_ELT(obj, 0, optional_wrap(indexhtml));
  SET_VECTOR_ELT(obj, 1, optional_wrap(fallthrough));
  SET_VECTOR_ELT(obj, 2, optional_wrap(html_charset));
  if (headers.has_value()) {
    SET_VECTOR_ELT(obj, 3, response_headers_to_sexp(*headers));
  } else {
    SET_VECTOR_ELT(obj, 3, R_NilValue);
  }
  SET_VECTOR_ELT(obj, 4, optional_wrap(validation));
  SET_VECTOR_ELT(obj, 5, optional_wrap(exclude));

  Rf_setAttrib(obj, R_NamesSymbol, names);
  Rf_setAttrib(obj, R_ClassSymbol, Rf_mkString("staticPathOptions"));

  UNPROTECT(2);
  return obj;
}

// Merge StaticPathOptions object `a` with `b`. Values in `a` take precedence.
StaticPathOptions StaticPathOptions::merge(
  const StaticPathOptions& a,
  const StaticPathOptions& b)
{
  StaticPathOptions new_sp = a;
  if (new_sp.indexhtml    == std::experimental::nullopt) new_sp.indexhtml    = b.indexhtml;
  if (new_sp.fallthrough  == std::experimental::nullopt) new_sp.fallthrough  = b.fallthrough;
  if (new_sp.html_charset == std::experimental::nullopt) new_sp.html_charset = b.html_charset;
  if (new_sp.headers      == std::experimental::nullopt) new_sp.headers      = b.headers;
  if (new_sp.validation   == std::experimental::nullopt) new_sp.validation   = b.validation;
  if (new_sp.exclude      == std::experimental::nullopt) new_sp.exclude      = b.exclude;
  return new_sp;
}

// Check if a set of request headers satisfies the condition specified by
// `validation`.
bool StaticPathOptions::validateRequestHeaders(const RequestHeaders& headers) const {
  if (validation == std::experimental::nullopt) {
    throw std::runtime_error("Cannot validate request headers because validation pattern is not set.");
  }

  // Should have the format {"==", "aaa", "bbb"}, or {} if there's no
  // validation pattern.
  const std::vector<std::string>& pattern = *validation;

  if (pattern.size() == 0) {
    return true;
  }

  if (pattern[0] != "==") {
    throw std::runtime_error("Validation only knows the == operator.");
  }

  RequestHeaders::const_iterator it = headers.find(pattern[1]);
  if (it != headers.end() && constant_time_compare(it->second, pattern[2])) {
    return true;
  }

  return false;
}


// ============================================================================
// StaticPath
// ============================================================================

StaticPath::StaticPath(SEXP sp) {
  ASSERT_MAIN_THREAD()
  path = as_cpp<std::string>(get_list_element(sp, "path"));

  SEXP options_list = get_list_element(sp, "options");
  options = StaticPathOptions(options_list);

  if (path.length() == 0) {
    if (!*options.exclude) {
      throw std::runtime_error("Static path must not be empty.");
      // Note that empty paths are OK for excluded paths, but we don't have to
      // mention it in the exception.
    }
  } else if (path.at(path.length() - 1) == '/') {
    throw std::runtime_error("Static path must not have trailing slash.");
  }
}

SEXP StaticPath::asRObject() const {
  ASSERT_MAIN_THREAD()

  SEXP obj   = PROTECT(Rf_allocVector(VECSXP, 2));
  SEXP names = PROTECT(Rf_allocVector(STRSXP, 2));

  SET_STRING_ELT(names, 0, Rf_mkChar("path"));
  SET_STRING_ELT(names, 1, Rf_mkChar("options"));

  SET_VECTOR_ELT(obj, 0, Rf_mkString(path.c_str()));
  SET_VECTOR_ELT(obj, 1, options.asRObject());

  Rf_setAttrib(obj, R_NamesSymbol, names);
  Rf_setAttrib(obj, R_ClassSymbol, Rf_mkString("staticPath"));

  UNPROTECT(2);
  return obj;
}


// ============================================================================
// StaticPathManager
// ============================================================================
StaticPathManager::StaticPathManager() {
  uv_mutex_init(&mutex);
}

StaticPathManager::StaticPathManager(SEXP path_list, SEXP options_list) {
  ASSERT_MAIN_THREAD()
  uv_mutex_init(&mutex);

  this->options = StaticPathOptions(options_list);

  if (Rf_xlength(path_list) == 0) {
    return;
  }

  SEXP names_sexp = Rf_getAttrib(path_list, R_NamesSymbol);
  if (names_sexp == R_NilValue) {
    throw std::runtime_error("Error processing static paths: all static paths must be named.");
  }

  for (R_xlen_t i = 0; i < Rf_xlength(path_list); i++) {
    std::string name = std::string(CHAR(STRING_ELT(names_sexp, i)));
    if (name == "") {
      throw std::runtime_error("Error processing static paths.");
    }

    StaticPath staticpath(VECTOR_ELT(path_list, i));

    this->path_map.insert(
      std::pair<std::string, StaticPath>(name, staticpath)
    );
  }
}


// Returns a StaticPath object, which has its options merged with the overall ones.
std::experimental::optional<StaticPath> StaticPathManager::get(const std::string& path) const {
  guard guard(mutex);
  std::map<std::string, StaticPath>::const_iterator it = path_map.find(path);
  if (it == path_map.end()) {
    return std::experimental::nullopt;
  }

  // Get a copy of the StaticPath object; we'll modify the options in the copy
  // by merging it with the overall options.
  StaticPath sp = it->second;
  sp.options = StaticPathOptions::merge(sp.options, this->options);
  return sp;
}

std::experimental::optional<StaticPath> StaticPathManager::get(SEXP path) const {
  ASSERT_MAIN_THREAD()
  if (Rf_xlength(path) != 1) {
    throw std::runtime_error("Can only get a single StaticPath object.");
  }
  return get(std::string(CHAR(STRING_ELT(path, 0))));
}


void StaticPathManager::set(const std::string& path, const StaticPath& sp) {
  guard guard(mutex);
  // If the key already exists, replace the value.
  std::map<std::string, StaticPath>::iterator it = path_map.find(path);
  if (it != path_map.end()) {
    it->second = sp;
  }

  // Otherwise, insert the pair.
  path_map.insert(
    std::pair<std::string, StaticPath>(path, sp)
  );
}

void StaticPathManager::set(const std::map<std::string, StaticPath>& pmap) {
  std::map<std::string, StaticPath>::const_iterator it;
  for (it = pmap.begin(); it != pmap.end(); it++) {
    set(it->first, it->second);
  }
}

void StaticPathManager::set(SEXP pmap) {
  ASSERT_MAIN_THREAD()
  SEXP names_sexp = Rf_getAttrib(pmap, R_NamesSymbol);
  for (R_xlen_t i = 0; i < Rf_xlength(pmap); i++) {
    std::string name = std::string(CHAR(STRING_ELT(names_sexp, i)));
    StaticPath sp(VECTOR_ELT(pmap, i));
    set(name, sp);
  }
}


void StaticPathManager::remove(const std::string& path) {
  guard guard(mutex);
  std::map<std::string, StaticPath>::iterator it = path_map.find(path);
  if (it != path_map.end()) {
    path_map.erase(it);
  }
}

void StaticPathManager::remove(const std::vector<std::string>& paths) {
  std::vector<std::string>::const_iterator it;
  for (it = paths.begin(); it != paths.end(); it++) {
    remove(*it);
  }
}

void StaticPathManager::remove(SEXP paths) {
  ASSERT_MAIN_THREAD()
  std::vector<std::string> paths_vec = as_cpp<std::vector<std::string>>(paths);
  remove(paths_vec);
}


// Given a URL path, this returns a pair where the first element is a matching
// StaticPath object, and the second element is the portion of the url_path that
// comes after the match for the static path.
//
// For example, if:
// - The input url_path is "/foo/bar/page.html"
// - There is a StaticPath object (call it `s`) for which s.path == "/foo"
// Then:
// - This function returns a pair consisting of <s, "bar/page.html">
//
// If there are multiple potential static path matches, for example "/foo" and
// "/foo/bar", then this will match the most specific (longest) path.
//
// If url_path has a trailing "/", it is stripped off. If the url_path matches
// a static path in its entirety (e.g., the url_path is "/foo" or "/foo/" and
// there is a static path "/foo"), then the returned pair consists of the
// matching StaticPath object and an empty string "".
//
// If no matching static path is found, then it returns std::experimental::nullopt.
//
std::experimental::optional<std::pair<StaticPath, std::string> > StaticPathManager::matchStaticPath(
  const std::string& url_path) const
{

  if (url_path.empty()) {
    return std::experimental::nullopt;
  }

  if (url_path.find('\\') != std::string::npos) {
    return std::experimental::nullopt;
  }

  std::string path = url_path;

  std::string pre_slash;
  std::string post_slash;

  // Strip off a trailing slash. A path like "/foo/bar/" => "/foo/bar".
  // One exception: don't alter it if the path is just "/".
  if (path.length() > 1 && path.at(path.length() - 1) == '/') {
    path = path.substr(0, path.length() - 1);
  }

  pre_slash  = path;
  post_slash = "";

  size_t found_idx = path.length() + 1;

  // This loop searches for a match in path_map of pre_slash, the part before
  // the last split-on '/'. If found, it returns a pair with the part before
  // the slash, and the part after the slash. If not found, it splits on the
  // previous '/' and searches again, and so on, until there are no more to
  // split on.
  while (true) {
    // Check if the part before the split-on '/' is a staticPath.
    std::experimental::optional<StaticPath> sp = this->get(pre_slash);

    if (sp) {
      return std::pair<StaticPath, std::string>(*sp, post_slash);
    }

    if (found_idx == 0) {
      // We get here after checking the leading '/'.
      return std::experimental::nullopt;
    }

    // Split the string on '/'
    found_idx = path.find_last_of('/', found_idx - 1);

    if (found_idx == std::string::npos) {
      // This is an extra check that could only be hit if the first character
      // of the URL is not a slash. Shouldn't be possible to get here because
      // the http parser will throw an "invalid URL" error when it encounters
      // such a URL, but we'll check just in case.
      return std::experimental::nullopt;
    }

    pre_slash = path.substr(0, found_idx);
    if (pre_slash == "") {
      // Special case if we've hit the leading slash.
      pre_slash = "/";
    }
    post_slash = path.substr(found_idx + 1);
  }
}

const StaticPathOptions& StaticPathManager::getOptions() const {
  return options;
}

void StaticPathManager::setOptions(SEXP opts) {
  options.setOptions(opts);
}

// Returns a list of R objects that reflect the StaticPaths, without merging
// the overall options.
SEXP StaticPathManager::pathsAsRObject() const {
  ASSERT_MAIN_THREAD()
  guard guard(mutex);

  R_xlen_t n = (R_xlen_t)path_map.size();
  SEXP obj   = PROTECT(Rf_allocVector(VECSXP, n));
  SEXP names = PROTECT(Rf_allocVector(STRSXP, n));

  R_xlen_t i = 0;
  std::map<std::string, StaticPath>::const_iterator it;
  for (it = path_map.begin(); it != path_map.end(); it++, i++) {
    SET_STRING_ELT(names, i, Rf_mkChar(it->first.c_str()));
    SET_VECTOR_ELT(obj, i, it->second.asRObject());
  }

  Rf_setAttrib(obj, R_NamesSymbol, names);

  UNPROTECT(2);
  return obj;
}
