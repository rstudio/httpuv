#include "utils.h"
#include <iostream>
#include <Rcpp.h>

void trace(const std::string& msg) {
#ifdef DEBUG_TRACE
  std::cerr << msg << std::endl;
#endif
}


std::map<std::string, std::string> toStringMap(Rcpp::CharacterVector x) {
  ASSERT_MAIN_THREAD()

  std::map<std::string, std::string> strmap;

  if (x.size() == 0) {
    return strmap;
  }

  Rcpp::CharacterVector names = x.names();
  if (names.isNULL()) {
    throw Rcpp::exception("Error converting CharacterVector to map<string, string>: vector does not have names.");
  }


  for (int i=0; i<x.size(); i++) {
    std::string name  = Rcpp::as<std::string>(names[i]);
    std::string value = Rcpp::as<std::string>(x[i]);
    if (name == "") {
      throw Rcpp::exception("Error converting CharacterVector to map<string, string>: element has empty name.");
    }

    strmap.insert(
      std::pair<std::string, std::string>(name, value)
    );
  }

  return strmap;
}


Rcpp::CharacterVector toCharacterVector(const std::map<std::string, std::string>& strmap) {
  ASSERT_MAIN_THREAD()

  Rcpp::CharacterVector values(strmap.size());
  Rcpp::CharacterVector names(strmap.size());

  std::map<std::string, std::string>::const_iterator it = strmap.begin();
  for (int i=0; it != strmap.end(); i++, it++) {
    values[i] = it->second;
    names[i]  = it->first;
  }

  values.attr("names") = names;
  return values;
}
