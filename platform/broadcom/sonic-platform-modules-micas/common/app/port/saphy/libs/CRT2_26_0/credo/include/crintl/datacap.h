#ifndef CRI_DATACAP_H
#define CRI_DATACAP_H

#include "credo.h"

// utility for documentation generation
void* cri_data_get_commands(CredoSlice_t* slice, size_t* command_count);

CREDOAPI void cri_data_free_buffer(char** output);

#endif
