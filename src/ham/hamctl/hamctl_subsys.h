// Host Account Management
#ifndef SUBSYS_H
#define SUBSYS_H

typedef  int (* cmd_pt)(int argc, char *argv[]);

class subsys_c
{
public:
    subsys_c(const char * name_p, const char * help_p, cmd_pt cmd_p, bool hidden=false) :
        name_pm(name_p),
        help_pm(help_p),
        cmd_pm(cmd_p),
        hidden_m(hidden)
    {
    }

    const char * name_pm;
    const char * help_pm;
    cmd_pt       cmd_pm;
    bool         hidden_m;
};

extern const subsys_c subsys_accounts;
extern const subsys_c subsys_debug;
extern const subsys_c subsys_sac;

#endif /* SUBSYS_H */
