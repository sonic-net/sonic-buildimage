
#include "bh_functions.h"
#include "bh_option.h"

#include "common/options.h"

#include "utility.h"
#include "sdk.h"

#include <string.h>

const OptionHandler_t option_port_list[] = {
    OPTION_DEF("test", "Test option", NULL, NULL),
};

const int option_port_count = COUNT_OF(option_port_list);
