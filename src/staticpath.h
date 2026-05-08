#ifndef STATICPATH_HPP
#define STATICPATH_HPP

#include <string>
#include <map>
#include <Rinternals.h>
#ifdef length
# undef length
#endif
#include "optional.h"
#include "thread.h"
#include "constants.h"

class StaticPathOptions {
public:
  std::experimental::optional<bool> indexhtml;
  std::experimental::optional<bool> fallthrough;
  std::experimental::optional<std::string> html_charset;
  std::experimental::optional<ResponseHeaders> headers;
  std::experimental::optional<std::vector<std::string> > validation;
  std::experimental::optional<bool> exclude;
  StaticPathOptions() :
    indexhtml(std::experimental::nullopt),
    fallthrough(std::experimental::nullopt),
    html_charset(std::experimental::nullopt),
    headers(std::experimental::nullopt),
    validation(std::experimental::nullopt),
    exclude(std::experimental::nullopt)
  { };
  StaticPathOptions(SEXP options);

  void setOptions(SEXP options);

  SEXP asRObject() const;

  static StaticPathOptions merge(const StaticPathOptions& a, const StaticPathOptions& b);

  bool validateRequestHeaders(const RequestHeaders& headers) const;
};


class StaticPath {
public:
  std::string path;
  StaticPathOptions options;

  StaticPath(SEXP sp);

  SEXP asRObject() const;
};


class StaticPathManager {
  std::map<std::string, StaticPath> path_map;
  // Mutex is used whenever path_map is accessed.
  mutable uv_mutex_t mutex;

  StaticPathOptions options;

public:
  StaticPathManager();
  StaticPathManager(SEXP path_list, SEXP options_list);

  std::experimental::optional<StaticPath> get(const std::string& path) const;
  std::experimental::optional<StaticPath> get(SEXP path) const;

  void set(const std::string& path, const StaticPath& sp);
  void set(const std::map<std::string, StaticPath>& pmap);
  void set(SEXP pmap);

  void remove(const std::string& path);
  void remove(const std::vector<std::string>& paths);
  void remove(SEXP paths);

  std::experimental::optional<std::pair<StaticPath, std::string> > matchStaticPath(
    const std::string& url_path) const;


  const StaticPathOptions& getOptions() const;
  void setOptions(SEXP opts);

  SEXP pathsAsRObject() const;
};

#endif
