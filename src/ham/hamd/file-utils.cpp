#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>             // opendir(), readdir()
#include <unistd.h>             // stat()
#include <glib.h>               // g_file_test(), etc...
#include <syslog.h>             // syslog(), LOG_WARNING
#include <limits.h>             // LINE_MAX

#include <algorithm>            // std::sort()
#include <vector>               // std::vector
#include <string>               // std::string
#include <map>                  // std::map

#include "file-utils.h"         // is_regular_file(), sorted_filelist()
#include "../shared/utils.h"    // startswith(), streq(), split_any()

/**
 * @brief Return an ASCII-sorted vector of filename entries in a given
 *        directory. If no path is specified, the current working directory
 *        is used.
 *
 * @param dir_path_p Oath to the directory
 *
 * @return Sorted vector containing the list of files. Always check the
 *         value of the global @errno variable after using this function to
 *         see if anything went wrong. (It will be zero if all is well.)
 */
std::vector<std::string> sorted_filelist(const path_t & dir_path_r)
{
    std::vector<std::string> result;
    DIR    * dp;
    errno = 0;
    dp = opendir(dir_path_r.empty() ? "." : dir_path_r.c_str());
    if (dp)
    {
        dirent * de;
        while (true)
        {
            errno = 0;
            de = readdir(dp);
            if (de == NULL) break;

            path_t fullname = dir_path_r / de->d_name;

            if (g_file_test(fullname.c_str(), G_FILE_TEST_IS_REGULAR)) // Only keep regular files
                result.push_back(fullname);
        }
        closedir(dp);
        std::sort(result.begin(), result.end());
    }
    return result;
}

/**
 * @brief Read text file containing a list of key:csv pairs (csv stands for
 *        comma-separated-values).
 *
 *        This is used to read, for example, the group mapping file or the
 *        roles DB.
 *
 *        The group mapping file contains a mapping of roles and/or
 *        privilege-levels to Linux groups. Here are a few examples:
 *
 *          ​sysadmin:sudo,adm,sys
 *          15:sudo,adm,sys
 *          secadmin:adm
 *          netoperator:operator
 *          0:operator​
 *
 *        The roles DB file contains the list of users and their roles.
 *        Here are a few an examples:
 *
 *          bob:sysadmin
 *          doug:netoperator
 *          jane:secadmin,netadmin
 *
 * @param fname_r Name of the file to read from.
 *
 * @return The output is a std::map. Note that the csv gets expanded. That
 *         is, the csv value is split at the commas (commas are
 *         removed), and the values are saved to a std::vector.
 */
std::map<std::string/*key*/, std::vector<std::string>/*values*/> read_key_csv(const path_t & fname_r)
{
    std::map<std::string/*key*/, std::vector<std::string>/*values*/> output;

    FILE * file = fopen(fname_r.c_str(), "re");
    if (file)
    {
        #define WHITESPACE " \t\n\r"
        char    line[LINE_MAX];
        size_t  lineno = 0;
        char  * p;
        char  * s;
        char  * key;
        char  * csv; // comma-separated-value
        while (nullptr != (p = fgets(line, sizeof line, file)))
        {
            lineno++;

            p += strspn(p, WHITESPACE);            // Remove leading newline and spaces
            if (*p == '#' || *p == '\0') continue; // Skip comments and empty lines

            // Delete trailing comments, spaces, tabs, and newline chars.
            s = &p[strcspn(p, "#\n\r")];
            *s-- = '\0';
            while ((s >= p) && ((*s == ' ') || (*s == '\t')))
            {
                *s-- = '\0';
            }

            if (*p == '\0') continue; // Check that there is still something left in the string

            csv = p;
            key = strsep(&csv, ":");
            if (csv == nullptr)
            {
                syslog(LOG_WARNING, "read_key_csv() - Invalid syntax at line %lu in file %s", lineno, fname_r.c_str());
                continue;
            }

            csv += strspn(csv, WHITESPACE);       // Remove leading newline and spaces
            key[strcspn(key, WHITESPACE)] = '\0'; // Remove trailing spaces

            output[key] = split_any(csv, ", \t");
        }

        fclose(file);
    }

    return output;
}

/**
 * @brief Read the whole contents of a file into a std::string
 *
 * @param fname File to read
 * @param verbose For debugging purposes
 *
 * @return A std::string containing the whole file's content.
 */
std::string get_file_contents(const path_t & fname_r, bool verbose)
{
    LOG_CONDITIONAL(verbose, LOG_DEBUG, "get_file_contents() - fname_r=%s", fname_r.c_str());

    std::string   contents;
    std::FILE   * fp = std::fopen(fname_r.c_str(), "re");

    if (fp != nullptr)
    {
        std::fseek(fp, 0, SEEK_END);
        contents.resize(std::ftell(fp));
        std::rewind(fp);
        LOG_CONDITIONAL(verbose, LOG_DEBUG, "get_file_contents() - Reading %lu chars from %s", contents.size(), fname_r.c_str());
        if (std::fread(&contents[0], contents.size(), 1, fp) != 1)
            syslog(LOG_ERR, "get_file_contents() - failed to read %lu bytes from %s", contents.size(), fname_r.c_str());

        std::fclose(fp);
    }
    else
    {
        syslog(LOG_ERR, "get_file_contents() - failed to open %s", fname_r.c_str());
    }

    return contents;
}
