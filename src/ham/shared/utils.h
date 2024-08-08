// Host Account Management
#ifndef __UTILS_H
#define __UTILS_H

#include <string.h>     /* strcmp(), strncmp() */
#include <syslog.h>     /* syslog() */

#define LOG_CONDITIONAL(condition, args...) do { if (condition) {syslog(args);} } while(0)

#define streq(a,b)    (strcmp((a),(b)) == 0)
#define strneq(a,b,n) (strncmp((a),(b),(n)) == 0)

/**
 * @brief Checks that a string starts with a given prefix.
 *
 * @param s The string to check
 * @param prefix A string that s could be starting with
 *
 * @return If s starts with prefix then return a pointer inside s right
 *         after the end of prefix.
 *         NULL otherwise
 */
static inline char * startswith(const char *s, const char *prefix)
{
    size_t l = strlen(prefix);
    if (strncmp(s, prefix, l) == 0) return (char *)s + l;

    return NULL;
}

/**
 * Copy string to buffer
 *
 * @param dest Where to copy srce to
 * @param srce String to be copied
 * @param len  Number of characters to copy.
 *
 * @return a pointer to the location in dest after the NUL terminating
 *         character of the string that was copied.
 */
static inline char * cpy2buf(char * dest, const char * srce, size_t len)
{
    memcpy(dest, srce, len);
    return dest + len;
}




#ifdef __cplusplus
#   include <string>                /* std::string */
#   include <sstream>               /* std::ostringstream, std::istringstream */
#   include <vector>                /* std::vector */
#   include <memory>                /* std::unique_ptr */

    inline const char * true_false(bool x, const char * pos_p = "true", const char * neg_p = "false")   { return (x) ? pos_p : neg_p; }

    /**
     * This is an equivalent to Python's ''.join().
     *
     * @example
     *
     *      static std::vector<std::string> v = {"a", "b", "c"};
     *      std::string s = join(v.begin(), v.end(), ", ", ".");
     *      // Result: "a, b, c."
     *
     * @return std::string
     */
    template<typename InputIt>
    std::string join(InputIt begin,
                     InputIt end,
                     const std::string & separator = ", ")
    {
        std::ostringstream ss;

        if (begin != end)
            ss << *begin++;

        while (begin != end)
        {
            ss << separator;
            ss << *begin++;
        }

        return ss.str();
    }

    /**
     * @brief Return a list (vector) of the sub-strings of %s that are
     *        delimited by any of the bytes in %any.
     *
     * @param s - The string to split
     * @param any - A set of characters that will be used as delimiters.
     * @param min_vector_size - The minimum size of the returned vector. This
     *                          can be used to append empty strings at the end
     *                          of the returned vector to increase its length
     *                          to @min_vector_size.
     *
     * @return std::vector<std::string>
     */
    static inline std::vector<std::string> split_any(const std::string& s, const char * any=";", size_t min_vector_size=0)
    {
        std::vector<std::string> tokens;

        char   s_copy[s.length() + 1];
        memcpy(s_copy, s.data(), sizeof s_copy); // Copy whole string including nul char at the end.

        char  * next  = &s_copy[0];
        char  * token = strsep(&next, any);
        while (token != nullptr)
        {
            tokens.push_back(token);
            if (next != nullptr)
                next += strspn(next, any); // Skip all delimiters
            token = strsep(&next, any);
        }

        if (tokens.size() < min_vector_size)
            tokens.resize(min_vector_size, "");

        return tokens;
    }

    /**
     * Returns a list (vector) of the sub-strings of @s that are delimited by
     * the %exact string.
     *
     * @param s - The string to split
     * @param exact - The exact delimiter.
     * @param min_vector_size - The minimum size of the returned vector. This
     *                          can be used to append empty strings at the end
     *                          of the returned vector to increase its length
     *                          to @min_vector_size.
     *
     * @return std::vector<std::string>
     */
    static inline std::vector<std::string> split_exact(const std::string& s, const char * exact=";", size_t min_vector_size=0)
    {
        std::vector<std::string> tokens;

        size_t s_len = s.length();
        char   s_copy[s_len + 1];
        memcpy(s_copy, s.data(), s_len + 1); // Copy whole string including nul char at the end.

        size_t   delim_len = strlen(exact);
        char   * next      = &s_copy[0];
        char   * end       = &s_copy[s_len + 1]; // Point at the terminating nul
        char   * delim;

        while ((next < end) && (nullptr != (delim = strstr(next, exact))))
        {
            delim[0] = '\0';
            tokens.push_back(next);
            next = delim + delim_len;
        }
        if (next < end)
            tokens.push_back(next);

        if (tokens.size() < min_vector_size)
            tokens.resize(min_vector_size, "");

        return tokens;
    }

    /**
     * @brief Remove leading and trailing characters
     *
     * @param str
     * @param whitespace
     *
     * @return The trimmed string
     */
    static inline std::string trim(const std::string & str,
                                   const std::string & whitespace = " \t")
    {
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
            return ""; // no content

        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    template<typename ... VArgs>
    std::string strfmt(const std::string &format, VArgs&& ... vargs)
    {
        size_t buf_size = std::snprintf(nullptr, 0, format.c_str(), std::forward<VArgs>(vargs)...) + 1;
        std::unique_ptr<char[]> buffer(new char[buf_size]);
        std::snprintf(&buffer[0], buf_size, format.c_str(), vargs ...);
        return std::string(buffer.get(), buffer.get() + buf_size - 1);
    }

#endif // __cplusplus

#endif /* __UTILS_H */
