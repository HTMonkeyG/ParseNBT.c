#ifndef __NBTCONFIG_H__
#define __NBTCONFIG_H__

//-----------------------------------------------------------------------------
// cNBT preprocessor options.
//-----------------------------------------------------------------------------

// Define attributes of all API symbols declarations, e.g. for DLL under Windows.
//#define cNBT_ATTR __declspec(dllexport)
//#define cNBT_ATTR __declspec(dllimport)

// Remove the calling convension.
//#define cNBT_API

// Don't implement default allocators calling malloc()/free() to avoid
// linking them. cNBT_SetAllocators() need to be called to set allocators.
//#define cNBT_DISABLE_DEFAULT_ALLOCATORS

#endif
