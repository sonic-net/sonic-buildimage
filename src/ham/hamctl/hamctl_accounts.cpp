// Host Account Management
#ifndef _GNU_SOURCE
#   define _GNU_SOURCE                  // crypt.h
#endif
#include <getopt.h>                     // getopt_long(), no_argument, struct option
#include <crypt.h>                      // crypt_r()
#include <termios.h>                    // tcgetattr(), tcsetattr(), struct termios
#include <unistd.h>                     // STDIN_FILENO
#include <stdlib.h>                     // rand(), srand()
#include <vector>                       // std::vector
#include <string>                       // std::string
#include <ostream>                      // std::endl
#include <iostream>                     // std::cin, std::cout

#include "hamctl.h"                     // get_dbusconn()
#include "hamctl_subsys.h"
#include "../shared/dbus-address.h"     // DBUS_BUS_NAME_BASE
#include "../shared/utils.h"            // split_any()

#define FMT_RED         "\x1b[0;31m"
#define FMT_NORMAL      "\x1b[0m"
#define DEL             0x7F
#define ESC             0x1B

typedef enum { normal, escaped, square, getpars } esc_state_t;

#define ESCAPE_NPAR     8
struct esc_data_c
{
    esc_state_t  state = normal;
    unsigned     npar  = 0;
};

/**
 * @brief Eliminate escape sequences from the input. This may happen, for
 *  example, when the user presses the up/down/left/right arrows.
 *
 *  Here's a list of the sequences supported by this algorithm.
 *
 *    Esc 7                 Save Cursor Position
 *    Esc 8                 Restore Cursor Position
 *    Esc [ Pn ; Pn ; .. m  Set attributes
 *    Esc [ Pn ; Pn H       Cursor Position
 *    Esc [ Pn ; Pn f       Cursor Position
 *    Esc [ Pn A            Cursor Up
 *    Esc [ Pn B            Cursor Down
 *    Esc [ Pn C            Cursor Forward
 *    Esc [ Pn D            Cursor Backward
 *    Esc [ Pn G            Cursor Horizontal Absolute
 *    Esc [ Pn X            Erase Characters
 *    Esc [ Ps J            Erase in Display
 *    Esc [ Ps K            Erase in Line
 *
 *  Pn is a string of zero or more decimal digits.
 *  Ps is a selective parameter.
 */
static void escape_sequence(esc_data_c & esc, char c)
{
    if (esc.state == normal)
    {
        if (c == ESC)
            esc.state = escaped; /* Starting new escape sequence. */
        return;
    }

    if (esc.state == escaped)
    {
        esc.state = c == '[' ? square : normal;
        return;
    }

    if (esc.state == square)
    {
        esc.state = getpars;
        esc.npar = 0;
        if (c == '?')
            return;
    }

    if (esc.state == getpars)
    {
        if (c == ';' && esc.npar < (ESCAPE_NPAR - 1))
        {
            esc.npar++;
            return;
        }
        if (c >= '0' && c <= '9')
            return;
    }

    esc.state = normal;
}

/**
 * @brief Read password from stdin. The password gets obscured with '*'
 *        characters as it is being typed.
 *
 * @param[OUT] pw Where password will be saved.
 */
void read_pw(char *pw)
{
    char        c;
    unsigned    i = 0;
    esc_data_c  esc_data;

    // Disable STDIN echo to obscure password while it is being typed
    struct termios oflags, nflags;

    tcgetattr(STDIN_FILENO, &oflags);
    nflags = oflags;

    nflags.c_lflag &= ~(ICANON | ECHO);
    nflags.c_cc[VTIME] = 0;
    nflags.c_cc[VMIN] = 1;

    (void)tcsetattr(STDIN_FILENO, TCSANOW/*TCSAFLUSH*/, &nflags);

    while (std::cin.get(c) && (c != '\n'))
    {
        if (esc_data.state != normal)
        {
            escape_sequence(esc_data, c);
            continue;
        }

        switch (c)
        {
        case '\b':
        case DEL:  // backspace
            if (i > 0)
            {
                pw[--i] = '\0';
                std::cout << "\b \b";
            }
            continue;

        case ESC:
            escape_sequence(esc_data, c);
            break;

        default:
            if (isprint(c) && !isspace(c))
            {
                pw[i++] = c;
                std::cout << '*';
            }
        }
    }
    std::cout << std::endl;

    // Restore STDIN config (i.e. echo)
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &oflags);

    pw[i] = '\0';
}

/**
 * @brief Generate a random SHA-512 salt to be used when invoking crypt_r()
 *
 * @return The salt string in the form "$6$random-salt-string$"
 */
static std::string get_salt()
{
    srand(time(NULL)); // Seed the randomizer

    static const char   valid_salts[]  = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
    static const size_t valid_salts_sz = sizeof(valid_salts) - 1;

    std::string  salt;
    unsigned     salt_len = 0;         // Salt can be at most 16 chars long

    while (salt_len < 4)               // Make sure salt is at least 4 chars long
        salt_len = 1 + (rand() & 0xF); // Yields a value in the range 1..16

    while (salt.length() < salt_len)
        salt.push_back(valid_salts[rand() % valid_salts_sz]);

    salt.insert(0, "$6$");
    salt.push_back('$');

    return salt;
}

/**
 * @brief Interactive prompt asking for password. As the password is
 *        entered it is obscured with '*' characters. The password will be
 *        requested twice to make sure there are no typos.
 *
 * @return The hashed password that can be used for the useradd/usermod
 *         --password option.
 *
 */
static std::string get_hashed_pw()
{
    std::string hashed_pw = "";
    char        clear_pw1[200];
    char        clear_pw2[200];

    printf("Enter new UNIX password: ");
    read_pw(clear_pw1);

    printf("Retype new UNIX password: ");
    read_pw(clear_pw2);

    if (streq(clear_pw1, clear_pw2))
    {
        struct crypt_data  data;
        data.initialized = 0;

        char * hash_p = crypt_r(clear_pw1, get_salt().c_str(), &data);
        if (hash_p != nullptr)
        {
            hashed_pw = hash_p;
        }
        else
        {
            fprintf(stderr, FMT_RED "Error! Unable to hash password" FMT_NORMAL "\n");
        }

    }
    else
    {
        fprintf(stderr, FMT_RED "Error! Passwords do not match" FMT_NORMAL "\n");
    }

    memset(clear_pw1, 0, sizeof clear_pw1);
    memset(clear_pw2, 0, sizeof clear_pw2);

    return hashed_pw;
}

/**
 * @brief Interactive prompt asking for the list of roles.
 *
 * @return List of roles as a std::vector.
 */
static std::vector< std::string > get_roles()
{
    char roles[1024];

    printf("Enter comma-separated roles: ");
    std::cin.getline(roles, sizeof roles);

    return split_any(roles, ", \t");
}

/**
 * @brief hamctl's accounts command
 *
 * @param argc
 * @param argv
 *
 * @return int
 */
static int accounts(int argc, char *argv[])
{
    static const struct option options[] =
    {
        { "help",  no_argument,       NULL, 'h' },
        { "help",  no_argument,       NULL, '?' },
        {}
    };

    static const char * usage_p =
        CTL_NAME " accounts [OPTIONS] [COMMANDS]\n"
        "\n"
        "This is used to manage user accounts\n"
        "\n"
        "OPTIONS:\n"
        " -?,-h,--help      print this message\n"
        "\n"
        "COMMANDS:\n"
        " useradd   [LOGIN] Add a user account.\n"
        " userdel   [LOGIN] Delete a user account.\n"
        " passwd    [LOGIN] Change a user's password.\n"
        " set_roles [LOGIN] Change a user's roles.\n"
        "\n"
        "ARGUMENTS:\n"
        " [LOGIN]   User login name\n";

    int  c;
    int  rc = 0;

    while ((c = getopt_long(argc, argv, "h?", options, NULL)) >= 0)
    {
        switch (c)
        {
        case '?':
        case 'h': printf("%s\n", usage_p); return 0;
        default:  return 1;
        }
    }

    const char * command_p = argv[optind];

    if (command_p == NULL)
    {
        rc = 1;
        fprintf(stderr, FMT_RED "Error! Missing command" FMT_NORMAL "\n");
    }

    if (rc == 0)
    {
        try
        {
            accounts_proxy_c  acct(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

            if (0 == strcmp("useradd", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char   * login_p   = argv[2];
                std::string    hashed_pw = get_hashed_pw();

                rc = hashed_pw.length() == 0 ? 1 : 0;
                if (rc == 0)
                {
                    std::vector< std::string > roles = get_roles();

                    ::DBus::Struct< bool, std::string > rv = acct.useradd(login_p, roles, hashed_pw);
                    if (!rv._1)
                    {
                        rc = 1;
                        std::cout << rv._2 << '\n';
                    }
                }
            }
            else if (0 == strcmp("userdel", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char * login_p  = argv[2];

                ::DBus::Struct< bool, std::string > rv = acct.userdel(login_p);
                if (!rv._1)
                {
                    rc = 1;
                    std::cout << rv._2 << '\n';
                }
            }
            else if (0 == strcmp("passwd", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char   * login_p   = argv[2];
                std::string    hashed_pw = get_hashed_pw();

                rc = hashed_pw.length() == 0 ? 1 : 0;
                if (rc == 0)
                {
                    ::DBus::Struct< bool, std::string > rv = acct.passwd(login_p, hashed_pw);
                    if (!rv._1)
                    {
                        rc = 1;
                        std::cout << rv._2 << '\n';
                    }
                }
            }
            else if (0 == strcmp("set_roles", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char                  * login_p = argv[2];
                std::vector< std::string >    roles   = get_roles();

                ::DBus::Struct< bool, std::string > rv = acct.set_roles(login_p, roles);
                if (!rv._1)
                {
                    rc = 1;
                    std::cout << rv._2 << '\n';
                }
            }
            else
            {
                rc = 1;
                fprintf(stderr, FMT_RED "Error! Unknown command \"%s\"" FMT_NORMAL "\n", command_p);
            }
        }
        catch (DBus::Error &ex)
        {
            rc = 1;
            fprintf(stderr, "hamctl accounts - %s\n", ex.what());
        }
    }

    return rc;
}

const subsys_c subsys_accounts("accounts", "Accounts management commands", accounts, false);


