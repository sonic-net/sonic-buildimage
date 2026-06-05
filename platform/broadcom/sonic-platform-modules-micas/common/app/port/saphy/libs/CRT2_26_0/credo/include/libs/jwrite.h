//
// jWrite.h
//
// A *really* simple JSON writer in C  (C89)
// - a collection of functions to generate JSON semi-automatically
//
// The idea is to simplify writing native C values into a JSON string and
// to provide some error trapping to ensure that the result is valid JSON.
//
// Example:
//		jwOpen( buffer, buflen, JW_OBJECT, JW_PRETTY );		// open root node as object
//		jwObj_string( "key", "value" );
//		jwObj_int( "int", 1 );
//		jwObj_array( "anArray");
//			jwArr_int( 0 );
//			jwArr_int( 1 );
//			jwArr_int( 2 );
//		jwEnd();
//		err= jwClose();								// close root object
//
// results in:
//
//		{
//		    "key": "value",
//		    "int": 1,
//		    "anArray": [
//		        0,
//		        1,
//		        2
//		    ]
//		}
//
// Note that jWrite handles string quoting and getting commas in the right place.
// If the sequence of calls is incorrect
// e.g.
//		jwOpen( buffer, buflen, JW_OBJECT, 1 );
//		jwObj_string( "key", "value" );
//			jwArr_int( 0 );
//      ...
//
// then the error code returned from jwClose() would indicate that you attempted to
// put an array element into an object (instead of a key:value pair)
// To locate the error, the supplied buffer has the JSON created upto the error point
// and a call to jwErrorPos() would return the function call at which the error occurred
// - in this case 3, the 3rd function call "jwArr_int(0)" is not correct at this point.
//
// The root JSON type can be JW_OBJECT or JW_ARRAY.
//
// For more information on each function, see the prototypes below.
//
//
// GLOBAL vs. Application-Supplied Control Structure
// -------------------------------------------------
// jWrite requires a jWriteControl structure to save the internal state.
// For many applications it is much simpler for this to be a global variable as
// used by the above examples.
//
// To use multiple instances of jWrite, an application has to supply unique instances
// of jWriteControl structures.
//
// This feature is enabled by commenting out the definition of JW_GLOBAL_CONTROL_STRUCT
//
// All the jWrite functions then take an additional parameter: a ptr to the structure
// e.g.
//		struct jWriteControl jwc;
//
//		jwOpen( &jwc, buffer, buflen, JW_OBJECT, 1 );
//		jwObj_string( &jwc, "key", "value" );
//		jwObj_int( &jwc, "int", 1 );
//		jwObj_array( &jwc, "anArray");
//			jwArr_int( &jwc, 0 );
//			jwArr_int( &jwc, 1 );
//			jwArr_int( &jwc, 2 );
//		jwEnd( &jwc );
//		err= jwClose( &jwc );
//
// - which is more flexible, but a pain to type in !
//
// TonyWilk, Mar 2015
//
//

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// format checker for safety
#if defined(__clang__) || defined(__GNUC__)
#define JW_PRINTF_ATTRIBUTE_FORMAT(string_index, first_to_check) \
    __attribute__((format(printf, string_index, first_to_check)))
#else
#define JW_PRINTF_ATTRIBUTE_FORMAT(string_index, first_to_check)
#endif /* defined(FT_CLANG_COMPILER) || defined(FT_GCC_COMPILER) */

#define JWRITE_STACK_DEPTH 32  // max nesting depth of objects/arrays

#define JW_COMPACT 0  // output string control for jwOpen()
#define JW_PRETTY  1  // pretty adds \n and indentation

enum jwNodeType { JW_OBJECT = 1, JW_ARRAY };

struct jwNodeStack {
    enum jwNodeType nodeType;
    int elementNo;
};

typedef struct jWriteControl {
    char *buffer;                                      // pointer to application's buffer
    size_t buflen;                                     // length of buffer
    char *bufp;                                        // current write position in buffer
    int error;                                         // error code
    int callNo;                                        // API call on which error occurred
    struct jwNodeStack nodeStack[JWRITE_STACK_DEPTH];  // stack of array/object nodes
    int stackpos;
    int isPretty;  // 1= pretty output (inserts \n and spaces)
    bool inStr;
    unsigned inOpt;
} jw_t;

// Error Codes
// -----------
#define JWRITE_OK          0
#define JWRITE_OUT_OF_MEM  1  // output buffer full
#define JWRITE_NOT_ARRAY   2  // tried to write Array value into Object
#define JWRITE_NOT_OBJECT  3  // tried to write Object key/value into Array
#define JWRITE_STACK_FULL  4  // array/object nesting > JWRITE_STACK_DEPTH
#define JWRITE_STACK_EMPTY 5  // stack underflow error (too many 'end's)
#define JWRITE_NEST_ERROR  6  // nesting error, not all objects closed when jwClose() called
#define JWRITE_BAD_FORMAT  7  // bad formatting

// API functions
// -------------

// Returns '\0'-termianted string describing the error (as returned by jwClose())
//
const char *jwErrorToString(int err);

// user is always given a printable string, must check jwClose for validation
static inline const char *jwStr(jw_t *jwc) {
    return (jwc->buffer != NULL) ? jwc->buffer : "";
}

#define JW_OPEN(start_size, rootType, isPretty) jwOpen(&(jw_t){0}, start_size, rootType, isPretty)

#define JWOBJ_NUMARRAY(jwc, array_name, array_data, size, format) \
    do {                                                          \
        jwObj_array(jwc, array_name);                             \
        for (size_t i = 0; i < (size); i++) {                     \
            jwArr_number(jwc, format, (array_data)[i]);           \
        }                                                         \
        jwEnd(jwc);                                               \
    } while (0)

#define JWOBJ_STRARRAY(jwc, array_name, array_data, size) \
    do {                                                  \
        jwObj_array(jwc, array_name);                     \
        for (size_t i = 0; i < (size); i++) {             \
            jwArr_string(jwc, (array_data)[i]);           \
        }                                                 \
        jwEnd(jwc);                                       \
    } while (0)

jw_t *jwOpen(jw_t *jwc, size_t buflen, enum jwNodeType rootType, int isPretty);
int jwClose(jw_t *jwc);
static inline void jwObj_opt(jw_t *jwc, bool condition) {
    jwc->inOpt += (condition || jwc->inOpt > 0) ? 1 : 0;
}
static inline void jwObj_optend(jw_t *jwc) {
    jwc->inOpt -= (jwc->inOpt > 0) ? 1 : 0;
}
#define JWOBJ_OPT(jwc, condtion) jwObj_opt(jwc, condtion)
#define JWOBJ_OPTEND(jwc)        jwObj_optend(jwc)

int jwErrorPos(jw_t *jwc);
void jwObj_stringf(jw_t *jwc, const char *key, const char *fmt, ...) JW_PRINTF_ATTRIBUTE_FORMAT(3, 4);
void jwObj_string(jw_t *jwc, const char *key, const char *value);
void jwObj_number(jw_t *jwc, const char *key, const char *fmt, ...) JW_PRINTF_ATTRIBUTE_FORMAT(3, 4);
void jwObj_bool(jw_t *jwc, const char *key, bool oneOrZero);
void jwObj_null(jw_t *jwc, const char *key);
void jwObj_object(jw_t *jwc, const char *key);
void jwObj_array(jw_t *jwc, const char *key);
void jwArr_stringf(jw_t *jwc, const char *fmt, ...) JW_PRINTF_ATTRIBUTE_FORMAT(2, 3);
void jwArr_string(jw_t *jwc, const char *value);
void jwArr_number(jw_t *jwc, const char *fmt, ...) JW_PRINTF_ATTRIBUTE_FORMAT(2, 3);
void jwArr_bool(jw_t *jwc, int oneOrZero);
void jwArr_null(jw_t *jwc);
void jwArr_object(jw_t *jwc);
void jwArr_array(jw_t *jwc);
int jwEnd(jw_t *jwc);
void jwObj_raw(jw_t *jwc, const char *key, const char *rawtext);
void jwArr_raw(jw_t *jwc, const char *rawtext);
void jwFree(jw_t *jwc);

/* end of jWrite.h */
