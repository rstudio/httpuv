#ifndef STATICPATH_HPP
#define STATICPATH_HPP

#include <string>
#include <map>
#include <Rcpp.h>
#include <boost/optional.hpp>
#include "thread.h"
#include "constants.h"

class StaticPathOptions {
public:
  boost::optional<bool> indexhtml;
  boost::optional<bool> fallthrough;
  boost::optional<std::string> html_charset;
  boost::optional<ResponseHeaders> headers;
  boost::optional<std::vector<std::string> > validation;
  boost::optional<bool> exclude;
  StaticPathOptions() :
    indexhtml(boost::none),
    fallthrough(boost::none),
    html_charset(boost::none),
    headers(boost::none),
    validation(boost::none),
    exclude(boost::none)
  { };
  StaticPathOptions(const Rcpp::List& options);

  void setOptions(const Rcpp::List& options);

  Rcpp::List asRObject() const;

  static StaticPathOptions merge(const StaticPathOptions& a, const StaticPathOptions& b);

  bool validateRequestHeaders(const RequestHeaders& headers) const;
};


class StaticPath {
public:
  std::string path;
  StaticPathOptions options;

  StaticPath(const Rcpp::List& sp);

  Rcpp::List asRObject() const;
};


class StaticPathManager {
  std::map<std::string, StaticPath> path_map;
  // Mutex is used whenever path_map is accessed.
  mutable uv_mutex_t mutex;

  StaticPathOptions options;

public:
  StaticPathManager();
  StaticPathManager(const Rcpp::List& path_list, const Rcpp::List& options_list);

  boost::optional<StaticPath> get(const std::string& path) const;
  boost::optional<StaticPath> get(const Rcpp::CharacterVector& path) const;

  void set(const std::string& path, const StaticPath& sp);
  void set(const std::map<std::string, StaticPath>& pmap);
  void set(const Rcpp::List& pmap);

  void remove(const std::string& path);
  void remove(const std::vector<std::string>& paths);
  void remove(const Rcpp::CharacterVector& paths);

  boost::optional<std::pair<StaticPath, std::string> > matchStaticPath(
    const std::string& url_path) const;


  const StaticPathOptions& getOptions() const;
  void setOptions(const Rcpp::List& opts);

  Rcpp::List pathsAsRObject() const;
};

#endif
