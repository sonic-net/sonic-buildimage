#ifndef NEXTHOPGROUP_CAPI_H
#define NEXTHOPGROUP_CAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Library version */
#define LIBNEXTHOPGROUP_VERSION "1.0.0"

/* Returns the runtime version string of the loaded library */
const char* nexthopgroup_version(void);

typedef struct NextHopGroupFull NextHopGroupFull;

#ifdef __cplusplus
namespace fib { class NextHopGroupFull; }
// C++ APIs
char* nexthopgroupfull_json_from_c_nhg_multi(const struct C_NextHopGroupFull* c_nhg, 
                                              uint32_t nh_grp_full_count, 
                                              uint32_t depends_count, 
                                              uint32_t dependents_count,
                                              bool is_recurisve);
char* nexthopgroupfull_json_from_c_nhg_singleton(const struct C_NextHopGroupFull* c_nhg, 
                                                  uint32_t depends_count, 
                                                  uint32_t dependents_count);
void nexthopgroup_free(fib::NextHopGroupFull* obj);
char* nexthopgroup_to_json(fib::NextHopGroupFull* obj);
#else
/* C APIs */
char* nexthopgroupfull_json_from_c_nhg_multi(const struct C_NextHopGroupFull* c_nhg, 
                                              uint32_t nh_grp_full_count, 
                                              uint32_t depends_count, 
                                              uint32_t dependents_count,
                                              bool is_recurisve);
char* nexthopgroupfull_json_from_c_nhg_singleton(const struct C_NextHopGroupFull* c_nhg, 
                                                  uint32_t depends_count, 
                                                  uint32_t dependents_count);
void nexthopgroup_free(NextHopGroupFull* obj);
char* nexthopgroup_to_json(NextHopGroupFull* obj);
#endif

/* C callback signature matching FRR's needs */
typedef void (*fib_frr_log_fn)(int level,
                                const char *file,
                                int line,
                                const char *func,
                                const char *fmt,
                                va_list args);

/* Register FRR-compatible callback from C code */
void fib_frr_register_callback(fib_frr_log_fn cb);
void fib_frr_set_log_level(int level);
int fib_frr_get_log_level();

#ifdef __cplusplus
}
#endif

#endif // NEXTHOPGROUP_CAPI_H