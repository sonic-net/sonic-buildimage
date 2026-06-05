#pragma once
#ifdef _WIN32
#ifdef PWRCIF_EXPORTS
#define HWL_API __declspec(dllexport)
#else
#define HWL_API __declspec(dllimport)
#endif
#else
#define HWL_API
#endif  // _WIN32
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
        // <<<< Required APIs >>>
        // Initialize C internal library: detect and connect the evluation board with FTDI USB dongle
        // @param pszMbName, motherboard name, e.g., pwr_mb
        // @param pszDbName, daughter board name or additional informatoin for the motherboard type board. e.g. owl400
        // @return: true if board detected and connected; false: error
HWL_API bool HWL_Initialize(const char* pszMbName, const char* pszDbName);
HWL_API bool HWL_Disconnect(void);  // Free the board for other application
HWL_API bool HWL_Reconnect(void);   // Reconnect the board

// <<<< Optional APIs >>>
// @param, pszCommandLine: command name and arguments
HWL_API bool HWL_RunCommand(const char* pszCommandLine);
// Get number of outputs of the run command
HWL_API uint16_t HWL_GetRunCommandOutputCount(void);
// Get number of items in each output of the run command
HWL_API uint16_t HWL_GetRunCommandOutputSubCount(uint16_t nOutputIndex);
// Get an output item, returns NULL if either index is invalid
HWL_API const char* HWL_GetRunCommandOutputItem(uint16_t nOutputIndex, uint16_t nItemIndex);

// Serdes register Access - single register
HWL_API bool HWL_ReadRegister(uint16_t nRegAddr, uint16_t* pnRegVal);
HWL_API bool HWL_WriteRegister(uint16_t nRegAddr, uint16_t nRegVal);

// <<<< Optional APIs >>>
// Serdes register Access - multiple registers
HWL_API bool HWL_BurstReadRegister(const uint16_t* pnRegAddr, uint16_t* pnRegVal, size_t nCount);
HWL_API bool HWL_BurstWriteRegister(const uint16_t* pnRegAddr, const uint16_t* pnRegVal, size_t nCount);

// Select current device for register access
HWL_API void HWL_SelectDevice(uint8_t nDeviceId);
HWL_API uint8_t HWL_GetCurrentDevice(void);

// Select current slice in the current selected device for register access
HWL_API void HWL_SelectSlice(uint8_t nSliceId);
HWL_API uint8_t HWL_GetCurrentSlice(void);

// <<<< APIs for use with C-SDK >>>>
typedef struct SliceContext {
    // you must define this
    uint8_t m_nChipId;   // Select a chip on the board
    uint8_t m_nSliceId;  // Select a slice of the selected chip
    uint8_t m_nSysId;    // Select a system
} SliceContext_t;

HWL_API int DRV_ReadRegister(void* slice_context, unsigned reg_addr, unsigned* val);
HWL_API int DRV_WriteRegister(void* slice_context, unsigned reg_addr, unsigned val);
HWL_API int DRV_ReadRegisterBurst(void* slice_context, unsigned first_addr, unsigned val[], unsigned count);
HWL_API int DRV_WriteRegisterBurst(void* slice_context, unsigned first_addr, const unsigned val[], unsigned count);

#ifdef __cplusplus
}
#endif  // __cplusplus
