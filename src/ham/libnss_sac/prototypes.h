#ifndef _PROTOTYPES_H_
#define _PROTOTYPES_H_

extern "C" {

enum nss_status _nss_sac_getpwnam_r(const char    * name,
                                    struct passwd * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop);
}

#endif
