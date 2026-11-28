/* Resolve the delivered PAM module without making an authentication request. */
#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    void *module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!module) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    const char *symbols[] = {"pam_sm_authenticate", "GenerateNTResponse", "pam_peap_authenticate", "_pam_radius_incr", "_pam_radius_trace"};
    for (unsigned i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
        if (!dlsym(module, symbols[i])) { fprintf(stderr, "%s\n", dlerror()); dlclose(module); return 1; }
    }
    return dlclose(module) != 0;
}
