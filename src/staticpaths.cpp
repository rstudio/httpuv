#include "staticpaths.h"
#include "thread.h"
#include "utils.h"
#include <boost/optional.hpp>

// ============================================================================
// StaticPath
// ============================================================================

StaticPath::StaticPath(const Rcpp::List& sp) {
  ASSERT_MAIN_THREAD()
  path        = Rcpp::as<std::string>(sp["path"]);
  indexhtml   = Rcpp::as<bool>       (sp["indexhtml"]);
  fallthrough = Rcpp::as<bool>       (sp["fallthrough"]);

  if (path.at(path.length() - 1) == '/') {
    throw std::runtime_error("Static path must not have trailing slash.");
  }
}

Rcpp::List StaticPath::asRObject() const {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  List obj = List::create(
    _["path"]        = path,
    _["indexhtml"]   = indexhtml,
    _["fallthrough"] = fallthrough
  );
  
  obj.attr("class") = "staticPath";

  return obj;
}


// ============================================================================
// StaticPaths
// ============================================================================
StaticPaths::StaticPaths() {
  uv_mutex_init(&mutex);
}

StaticPaths::StaticPaths(const Rcpp::List& source) {
  ASSERT_MAIN_THREAD()
  uv_mutex_init(&mutex);

  if (source.size() == 0) {
    return;
  }

  Rcpp::CharacterVector names = source.names();
  if (names.isNULL()) {
    throw Rcpp::exception("Error processing static paths.");
  }

  for (int i=0; i<source.size(); i++) {
    std::string name = Rcpp::as<std::string>(names[i]);
    if (name == "") {
      throw Rcpp::exception("Error processing static paths.");
    }

    Rcpp::List sp(source[i]);
    StaticPath staticpath(sp);

    path_map.insert(
      std::pair<std::string, StaticPath>(name, staticpath)
    );
  }
}


boost::optional<const StaticPath&> StaticPaths::get(const std::string& path) const {
  guard guard(mutex);
  std::map<std::string, StaticPath>::const_iterator it = path_map.find(path);
  if (it == path_map.end()) {
    return boost::none;
  }
  return it->second;
}

boost::optional<const StaticPath&> StaticPaths::get(const Rcpp::CharacterVector& path) const {
  ASSERT_MAIN_THREAD()
  if (path.size() != 1) {
    throw Rcpp::exception("Can only get a single StaticPath object.");
  }
  return get(Rcpp::as<std::string>(path));
}


void StaticPaths::set(const std::string& path, const StaticPath& sp) {
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

void StaticPaths::set(const std::map<std::string, StaticPath>& pmap) {
  std::map<std::string, StaticPath>::const_iterator it;
  for (it = pmap.begin(); it != pmap.end(); it++) {
    set(it->first, it->second);
  }
}

void StaticPaths::set(const Rcpp::List& pmap) {
  ASSERT_MAIN_THREAD()
  std::map<std::string, StaticPath> pmap2 = toMap<StaticPath, Rcpp::List>(pmap);
  set(pmap2);
}


void StaticPaths::remove(const std::string& path) {
  guard guard(mutex);
  std::map<std::string, StaticPath>::const_iterator it = path_map.find(path);
  if (it != path_map.end()) {
    path_map.erase(it);
  }
}

void StaticPaths::remove(const std::vector<std::string>& paths) {
  std::vector<std::string>::const_iterator it;
  for (it = paths.begin(); it != paths.end(); it++) {
    remove(*it);
  }
}

void StaticPaths::remove(const Rcpp::CharacterVector& paths) {
  ASSERT_MAIN_THREAD()
  std::vector<std::string> paths_vec = Rcpp::as<std::vector<std::string>>(paths);
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
// If no matching static path is found, then it returns boost::none.
//
boost::optional<std::pair<const StaticPath&, std::string>> StaticPaths::matchStaticPath(
  const std::string& url_path) const
{

  if (url_path.empty()) {
    return boost::none;
  }

  std::string path = url_path;
  size_t found_idx = std::string::npos;

  std::string pre_slash;
  std::string post_slash;

  // Strip off a trailing slash. A path like "/foo/bar/" => "/foo/bar".
  // One exception: don't alter it if the path is just "/".
  if (path.length() > 1 && path.at(path.length() - 1) == '/') {
    path = path.substr(0, path.length() - 1);
  }

  pre_slash  = path;
  post_slash = "";

  // This loop searches for a match in path_map of pre_slash, the part before
  // the last split-on '/'. If found, it returns a pair with the part before
  // the slash, and the part after the slash. If not found, it splits on the
  // previous '/' and searches again, and so on, until there are no more to
  // split on.
  while (true) {
    // Check if the part before the split-on '/' is in the staticPaths.
    boost::optional<const StaticPath&> sp = this->get(pre_slash);
    if (sp) {
      return std::pair<const StaticPath&, std::string>(*sp, post_slash);
    }

    if (found_idx == 0) {
      // We get here after checking the leading '/'.
      return boost::none;
    }

    // Split the string on '/'
    found_idx = path.find_last_of('/', found_idx - 1);

    if (found_idx == std::string::npos) {
      // This is an extra check that could only be hit if the first character
      // of the URL is not a slash. Shouldn't be possible to get here because
      // the http parser will throw an "invalid URL" error when it encounters
      // such a URL, but we'll check just in case.
      return boost::none;
    }

    pre_slash = path.substr(0, found_idx);
    if (pre_slash == "") {
      // Special case if we've hit the leading slash.
      pre_slash = "/";
    }
    post_slash = path.substr(found_idx + 1);
  }
}


Rcpp::List StaticPaths::asRObject() const {
  ASSERT_MAIN_THREAD()
  guard guard(mutex);
  Rcpp::List obj;

  std::map<std::string, StaticPath>::const_iterator it;
  for (it = path_map.begin(); it != path_map.end(); it++) {
    obj[it->first] = it->second.asRObject();
  }
  
  return obj;
}

