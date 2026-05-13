#include "staticpath.h"
#include "constants.h"
#include "optional.h"
#include "thread.h"
#include "utils.h"

// ============================================================================
// StaticPathOptions
// ============================================================================

StaticPathOptions::StaticPathOptions(const list &options)
    : indexhtml(std::experimental::nullopt),
      fallthrough(std::experimental::nullopt),
      html_charset(std::experimental::nullopt),
      headers(std::experimental::nullopt),
      validation(std::experimental::nullopt),
      exclude(std::experimental::nullopt) {
  ASSERT_MAIN_THREAD()

  std::string obj_class = std::string(strings(SEXP(options.attr("class")))[0]);
  if (obj_class != "staticPathOptions") {
    stop("staticPath options object must have class 'staticPathOptions'.");
  }

  SEXP temp;

  temp = options.attr("normalized");
  std::experimental::optional<bool> normalized = optional_as<bool>(temp);
  if (!normalized || !*normalized) {
    stop("staticPathOptions object must be normalized.");
  }

  // There's probably a more concise way to do this assignment than by using
  // temp.
  temp = options["indexhtml"];
  indexhtml = optional_as<bool>(temp);
  temp = options["fallthrough"];
  fallthrough = optional_as<bool>(temp);
  temp = options["html_charset"];
  html_charset = optional_as<std::string>(temp);
  temp = options["headers"];
  headers = optional_as<ResponseHeaders>(temp);
  temp = options["validation"];
  validation = optional_as<std::vector<std::string>>(temp);
  temp = options["exclude"];
  exclude = optional_as<bool>(temp);
}

void StaticPathOptions::setOptions(const list &options) {
  ASSERT_MAIN_THREAD()
  SEXP temp;
  if (options.contains("indexhtml")) {
    temp = options["indexhtml"];
    if (!Rf_isNull(temp)) {
      indexhtml = optional_as<bool>(temp);
    }
  }
  if (options.contains("fallthrough")) {
    temp = options["fallthrough"];
    if (!Rf_isNull(temp)) {
      fallthrough = optional_as<bool>(temp);
    }
  }
  if (options.contains("html_charset")) {
    temp = options["html_charset"];
    if (!Rf_isNull(temp)) {
      html_charset = optional_as<std::string>(temp);
    }
  }
  if (options.contains("headers")) {
    temp = options["headers"];
    if (!Rf_isNull(temp)) {
      headers = optional_as<ResponseHeaders>(temp);
    }
  }
  if (options.contains("validation")) {
    temp = options["validation"];
    if (!Rf_isNull(temp)) {
      validation = optional_as<std::vector<std::string>>(temp);
    }
  }
  if (options.contains("exclude")) {
    temp = options["exclude"];
    if (!Rf_isNull(temp)) {
      exclude = optional_as<bool>(temp);
    }
  }
}

list StaticPathOptions::asRObject() const {
  ASSERT_MAIN_THREAD()
  writable::list obj = {"indexhtml"_nm = optional_wrap(indexhtml),
                        "fallthrough"_nm = optional_wrap(fallthrough),
                        "html_charset"_nm = optional_wrap(html_charset),
                        "headers"_nm = optional_wrap(headers),
                        "validation"_nm = optional_wrap(validation),
                        "exclude"_nm = optional_wrap(exclude)};
  obj.attr("class") = "staticPathOptions";
  return obj;
}

// Merge StaticPathOptions object `a` with `b`. Values in `a` take precedence.
StaticPathOptions StaticPathOptions::merge(const StaticPathOptions &a,
                                           const StaticPathOptions &b) {
  StaticPathOptions new_sp = a;
  if (new_sp.indexhtml == std::experimental::nullopt)
    new_sp.indexhtml = b.indexhtml;
  if (new_sp.fallthrough == std::experimental::nullopt)
    new_sp.fallthrough = b.fallthrough;
  if (new_sp.html_charset == std::experimental::nullopt)
    new_sp.html_charset = b.html_charset;
  if (new_sp.headers == std::experimental::nullopt)
    new_sp.headers = b.headers;
  if (new_sp.validation == std::experimental::nullopt)
    new_sp.validation = b.validation;
  if (new_sp.exclude == std::experimental::nullopt)
    new_sp.exclude = b.exclude;
  return new_sp;
}

// Check if a set of request headers satisfies the condition specified by
// `validation`.
bool StaticPathOptions::validateRequestHeaders(
    const RequestHeaders &headers) const {
  if (validation == std::experimental::nullopt) {
    throw std::runtime_error("Cannot validate request headers because "
                             "validation pattern is not set.");
  }

  // Should have the format {"==", "aaa", "bbb"}, or {} if there's no
  // validation pattern.
  const std::vector<std::string> &pattern = *validation;

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

StaticPath::StaticPath(const list &sp) {
  ASSERT_MAIN_THREAD()
  path = as_cpp<std::string>(sp["path"]);

  list options_list(sp["options"]);
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

list StaticPath::asRObject() const {
  ASSERT_MAIN_THREAD()
  writable::list obj = {"path"_nm = path, "options"_nm = options.asRObject()};
  obj.attr("class") = "staticPath";
  return obj;
}

// ============================================================================
// StaticPathManager
// ============================================================================
StaticPathManager::StaticPathManager() { uv_mutex_init(&mutex); }

StaticPathManager::StaticPathManager(const list &path_list,
                                     const list &options_list) {
  ASSERT_MAIN_THREAD()
  uv_mutex_init(&mutex);

  this->options = StaticPathOptions(options_list);

  if (path_list.size() == 0) {
    return;
  }

  strings names = path_list.names();
  if (names.size() == 0) {
    stop("Error processing static paths: all static paths must be named.");
  }

  for (R_xlen_t i = 0; i < path_list.size(); i++) {
    std::string name = std::string(names[i]);
    if (name == "") {
      stop("Error processing static paths.");
    }

    list sp(path_list[i]);
    StaticPath staticpath(sp);

    this->path_map.insert(std::pair<std::string, StaticPath>(name, staticpath));
  }
}

// Returns a StaticPath object, which has its options merged with the overall
// ones.
std::experimental::optional<StaticPath>
StaticPathManager::get(const std::string &path) const {
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

std::experimental::optional<StaticPath>
StaticPathManager::get(const strings &path) const {
  ASSERT_MAIN_THREAD()
  if (path.size() != 1) {
    stop("Can only get a single StaticPath object.");
  }
  return get(std::string(path[0]));
}

void StaticPathManager::set(const std::string &path, const StaticPath &sp) {
  guard guard(mutex);
  // If the key already exists, replace the value.
  std::map<std::string, StaticPath>::iterator it = path_map.find(path);
  if (it != path_map.end()) {
    it->second = sp;
  }

  // Otherwise, insert the pair.
  path_map.insert(std::pair<std::string, StaticPath>(path, sp));
}

void StaticPathManager::set(const std::map<std::string, StaticPath> &pmap) {
  std::map<std::string, StaticPath>::const_iterator it;
  for (it = pmap.begin(); it != pmap.end(); it++) {
    set(it->first, it->second);
  }
}

void StaticPathManager::set(const list &pmap) {
  ASSERT_MAIN_THREAD()
  std::map<std::string, StaticPath> pmap2 = toMap<StaticPath, list>(pmap);
  set(pmap2);
}

void StaticPathManager::remove(const std::string &path) {
  guard guard(mutex);
  std::map<std::string, StaticPath>::iterator it = path_map.find(path);
  if (it != path_map.end()) {
    path_map.erase(it);
  }
}

void StaticPathManager::remove(const std::vector<std::string> &paths) {
  std::vector<std::string>::const_iterator it;
  for (it = paths.begin(); it != paths.end(); it++) {
    remove(*it);
  }
}

void StaticPathManager::remove(const strings &paths) {
  ASSERT_MAIN_THREAD()
  for (R_xlen_t i = 0; i < paths.size(); i++) {
    remove(std::string(paths[i]));
  }
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
// If no matching static path is found, then it returns
// std::experimental::nullopt.
//
std::experimental::optional<std::pair<StaticPath, std::string>>
StaticPathManager::matchStaticPath(const std::string &url_path) const {

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

  pre_slash = path;
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

const StaticPathOptions &StaticPathManager::getOptions() const {
  return options;
}

void StaticPathManager::setOptions(const list &opts) {
  options.setOptions(opts);
}

// Returns a list of R objects that reflect the StaticPaths, without merging
// the overall options.
list StaticPathManager::pathsAsRObject() const {
  ASSERT_MAIN_THREAD()
  guard guard(mutex);

  R_xlen_t n = static_cast<R_xlen_t>(path_map.size());
  writable::list obj(n);
  writable::strings nms(n);

  R_xlen_t i = 0;
  std::map<std::string, StaticPath>::const_iterator it;
  for (it = path_map.begin(); it != path_map.end(); ++it, ++i) {
    nms[i] = it->first;
    obj[i] = it->second.asRObject();
  }
  obj.attr("names") = nms;

  return obj;
}
