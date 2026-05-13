#ifndef STATICPATH_HPP
#define STATICPATH_HPP

#include "constants.h"
#include "optional.h"
#include "thread.h"
#include <map>
#include <string>

#include "cpp11.hpp"

using namespace cpp11;

class StaticPathOptions {
public:
  std::experimental::optional<bool> indexhtml;
  std::experimental::optional<bool> fallthrough;
  std::experimental::optional<std::string> html_charset;
  std::experimental::optional<ResponseHeaders> headers;
  std::experimental::optional<std::vector<std::string>> validation;
  std::experimental::optional<bool> exclude;
  StaticPathOptions()
      : indexhtml(std::experimental::nullopt),
        fallthrough(std::experimental::nullopt),
        html_charset(std::experimental::nullopt),
        headers(std::experimental::nullopt),
        validation(std::experimental::nullopt),
        exclude(std::experimental::nullopt) {};
  StaticPathOptions(const list &options);

  void setOptions(const list &options);

  list asRObject() const;

  static StaticPathOptions merge(const StaticPathOptions &a,
                                 const StaticPathOptions &b);

  bool validateRequestHeaders(const RequestHeaders &headers) const;
};

class StaticPath {
public:
  std::string path;
  StaticPathOptions options;

  StaticPath(const list &sp);

  list asRObject() const;
};

class StaticPathManager {
  std::map<std::string, StaticPath> path_map;
  // Mutex is used whenever path_map is accessed.
  mutable uv_mutex_t mutex;

  StaticPathOptions options;

public:
  StaticPathManager();
  StaticPathManager(const list &path_list, const list &options_list);

  std::experimental::optional<StaticPath> get(const std::string &path) const;
  std::experimental::optional<StaticPath> get(const strings &path) const;

  void set(const std::string &path, const StaticPath &sp);
  void set(const std::map<std::string, StaticPath> &pmap);
  void set(const list &pmap);

  void remove(const std::string &path);
  void remove(const std::vector<std::string> &paths);
  void remove(const strings &paths);

  std::experimental::optional<std::pair<StaticPath, std::string>>
  matchStaticPath(const std::string &url_path) const;

  const StaticPathOptions &getOptions() const;
  void setOptions(const list &opts);

  list pathsAsRObject() const;
};

#endif
