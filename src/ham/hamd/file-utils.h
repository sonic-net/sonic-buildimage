#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include <string>
#include <vector>
#include <map>

// Portability check for "std::filesystem". Depending on the version
// of C++ that's available, "std::filesystem" is included as follows:
//   - 2017 onwards:  #include <filesystem>                /* provides std::filesystem */
//   - 2011-2016:     #include <experimental/filesystem>   /* provides std::experimental::filesystem */
//   - Prior to 2011: Not available
#if __cplusplus >= 201703L    /* 2017 onwards */
#   include <filesystem>

#elif __cplusplus >= 201103L  /* 2011 to 2016 */
#   include <experimental/filesystem>

// Let's alias "std::experimental::filesystem" as "std::filesystem"
namespace std {
    namespace filesystem = experimental::filesystem;
}

#else                         /* Prior to 2011 */
#   error Missing C++ header files: "<filesystem>" or "<experimental/filesystem>"
#endif

typedef std::filesystem::path  path_t;
extern  std::vector<std::string/*filename*/> sorted_filelist(const path_t & dir_path_r = "");
extern  std::map<std::string/*key*/, std::vector<std::string>/*values*/> read_key_csv(const path_t & fname_r);
extern  std::string get_file_contents(const path_t & fname_r, bool verbose=false);

#endif // __FILE_UTILS_H__








