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
  index       = Rcpp::as<bool>       (sp["index"]);
  fallthrough = Rcpp::as<bool>       (sp["fallthrough"]);
}

Rcpp::List StaticPath::asRObject() const {
  ASSERT_MAIN_THREAD()
  using namespace Rcpp;

  List obj = List::create(
    _["path"]        = path,
    _["index"]       = index,
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

