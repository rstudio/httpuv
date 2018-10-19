#ifndef STATICPATHS_HPP
#define STATICPATHS_HPP

#include <string>
#include <map>
#include <Rcpp.h>
#include <boost/optional.hpp>
#include "thread.h"

class StaticPath {
public:
  std::string path;
  bool indexhtml;
  bool fallthrough;

  StaticPath(std::string _path, bool _indexhtml, bool _fallthrough) :
    path(_path), indexhtml(_indexhtml), fallthrough(_fallthrough)
  {
    if (path.at(path.length() - 1) == '/') {
      throw std::runtime_error("Static path must not have trailing slash.");
    }
  }

  StaticPath(const Rcpp::List& sp);

  Rcpp::List asRObject() const;
};


class StaticPaths {
  std::map<std::string, StaticPath> path_map;
  mutable uv_mutex_t mutex;

public:
  StaticPaths();  
  StaticPaths(const Rcpp::List& source);

  boost::optional<const StaticPath&> get(const std::string& path) const;
  boost::optional<const StaticPath&> get(const Rcpp::CharacterVector& path) const;

  void set(const std::string& path, const StaticPath& sp);
  void set(const std::map<std::string, StaticPath>& pmap);
  void set(const Rcpp::List& pmap);

  void remove(const std::string& path);
  void remove(const std::vector<std::string>& paths);
  void remove(const Rcpp::CharacterVector& paths);

  boost::optional<std::pair<const StaticPath&, std::string>> matchStaticPath(
    const std::string& url_path) const;

  Rcpp::List asRObject() const;
};

#endif
