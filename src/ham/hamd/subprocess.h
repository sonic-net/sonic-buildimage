#ifndef   SUBPROCESS_H
#define   SUBPROCESS_H
#include <string>   // std::string
#include <tuple>    // std::tuple

std::tuple<int/*rc*/, std::string/*stdout*/, std::string/*stderr*/> run(const std::string & cmd_r);

#endif // SUBPROCESS_H
