#ifndef _HW_TEST_H
#define _HW_TEST_H

struct hw_applet {
    const char *name;
    const char *help;
    int (*main) (int argc, char **argv);
};

#define HWTEST_APPLET(a, b) {#a, b, a##_main}

extern int hw_help_main(int argc, char **argv);

#endif /* _HW_TEST_H */
