#ifndef __NBT_H__
#define __NBT_H__

#include <stdint.h>
#include "nbtconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32)
// Windows.

// You can override cNBT_API and cNBT_ATTR in nbtconfig.h.
#ifndef cNBT_API
#define cNBT_API __stdcall
#endif

#ifndef cNBT_ATTR
#define cNBT_ATTR
#endif

#else
// Non-windows.

#ifndef cNBT_API
#define cNBT_API
#endif

#ifndef cNBT_ATTR
#define cNBT_ATTR
#endif

#endif

#define cNBT_END 0x00
#define cNBT_I08 0x01
#define cNBT_I16 0x02
#define cNBT_I32 0x03
#define cNBT_I64 0x04
#define cNBT_F32 0x05
#define cNBT_F64 0x06
#define cNBT_A08 0x07
#define cNBT_STR 0x08
#define cNBT_LST 0x09
#define cNBT_OBJ 0x0A
#define cNBT_A32 0x0B
#define cNBT_A64 0x0C

#ifdef NULL
#define cNBT_NULLPTR NULL
#else
#define cNBT_NULLPTR ((void *)0)
#endif

typedef union {
  // Basic types.
  int8_t valueI08;
  int16_t valueI16;
  int32_t valueI32;
  int64_t valueI64;
  float valueF32;
  double valueF64;

  // String.
  struct {
    uint16_t lengthString;
    char *valueString;
  };

  // Array.
  struct {
    int32_t lengthArray;
    void *valueArray;
  };
} cNBTPayload;

struct cNBT_t;
typedef struct cNBT_t {
  // The chain of the items in the list/object.
  struct cNBT_t *next;
  // For the first element of the list, this pointer points to the last element
  // of the list. For a list only have 1 element, it points to itself.
  struct cNBT_t *prev;

  // A list or object item will have a child pointer pointing to another chain
  // of the items in the list/object.
  struct cNBT_t *child;

  // The name of the item. This will be NBT_NULLPTR if the item is in a list.
  // When the key is empty (length == 0), it's recorded as a pointer points 
  // to "\0".
  char *key;

  // The type of the payload of this item.
  uint8_t type;
  // The element is considered as a list if this field is set. Note that we
  // won't record the length of a list.
  uint8_t listElementType;

  // Stored data.
  cNBTPayload value;
} cNBT;

//-----------------------------------------------------------------------------
// [SECTION] MEMORY MANAGEMENT
//-----------------------------------------------------------------------------

typedef void *(cNBT_API *cNBTMemAllocFn)(
  size_t, void *);
typedef void (cNBT_API *cNBTMemFreeFn)(
  void *, void *);

// Get current memory allocator functions.
cNBT_ATTR void cNBT_API cNBT_GetAllocators(
  cNBTMemAllocFn *allocFn, cNBTMemFreeFn *freeFn, void **userData);

// Set current memory allocator functions. you can implement your own
// allocator with this function.
cNBT_ATTR void cNBT_API cNBT_SetAllocators(
  cNBTMemAllocFn allocFn, cNBTMemFreeFn freeFn, void *userData);

//-----------------------------------------------------------------------------
// [SECTION] VALUE OPERATIONS
//-----------------------------------------------------------------------------

// Traverse all items of an object or list.
#define cNBT_ForEach(object, item) \
  for(item = (object) ? (object)->child : cNBT_NULLPTR; item; item = item->next)

// Check the type of the given item.
cNBT_ATTR uint8_t cNBT_API cNBT_IsType(
  const cNBT *const nbt, uint8_t type);

// Find item matching the given key name.
cNBT_ATTR cNBT *cNBT_API cNBT_GetNodeByKey(
  const cNBT *const nbt, const char *key);

// Find item matching the given key name and the given type.
cNBT_ATTR cNBT *cNBT_API cNBT_GetNodeByKeyTyped(
  const cNBT *const nbt, const char *key, uint8_t type);

// Find item in an "array" with given "index".
cNBT_ATTR cNBT *cNBT_API cNBT_GetNodeByIndex(
  const cNBT *const nbt, int32_t index);

// Get the type of an item.
cNBT_ATTR uint8_t cNBT_API cNBT_GetNodeType(
  const cNBT *const nbt);

// Get the key name of an item.
cNBT_ATTR const char *cNBT_API cNBT_GetNodeKey(
  const cNBT *const nbt);

// Obtain the string length carried by the string node.
// Avaliable only for string nodes.
cNBT_ATTR uint16_t cNBT_API cNBT_GetValueStringLength(
  const cNBT *const nbt);

// Obtain the pointer to the string carried by the string node.
// Avaliable only for string nodes.
cNBT_ATTR const char *cNBT_API cNBT_GetValueString(
  const cNBT *const nbt);

// Create an NBT item.
cNBT_ATTR cNBT *cNBT_API cNBT_CreateNode(
  uint8_t type);

// Add a node to the object. The node must be an independent node.
cNBT_ATTR cNBT *cNBT_API cNBT_AddNode(
  cNBT *nbt, cNBT *item, const char *key);

// Set the type of the elements in a list. The function fails when the type of
// the given list has already been set.
cNBT_ATTR cNBT *cNBT_API cNBT_SetListElementType(
  cNBT *nbt, uint8_t type);

// Set the value of a node. The node's type must match the function, or the
// function fails.
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueI08(
  cNBT *nbt,
  int8_t data);
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueI16(
  cNBT *nbt,
  int16_t data);
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueI32(
  cNBT *nbt,
  int32_t data);
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueI64(
  cNBT *nbt,
  int64_t data);
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueF32(
  cNBT *nbt,
  float data);
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueF64(
  cNBT *nbt,
  double data);

// Set the value of a string node. The function will calculate the length of
// the given string automatically when `maxLen` == 0.
//
// The function will not access characters greater than `maxLen`.
// 
// The function fails when the length of the given string is bigger than 65535
// and the `maxLen` is not specified.
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueString(
  cNBT *nbt,
  const char *string,
  uint16_t maxLen);

// Set the value of an array node. The function fails when `length` < 0.
cNBT_ATTR cNBT *cNBT_API cNBT_SetValueArray(
  cNBT *nbt,
  const void *data,
  int32_t length);

// Remove a node from an object.
cNBT_ATTR cNBT *cNBT_API cNBT_RemoveNode(
  cNBT *nbt, cNBT *item);

// Remove all child nodes of an object.
cNBT_ATTR cNBT *cNBT_API cNBT_Clear(
  cNBT *nbt);

//-----------------------------------------------------------------------------
// [SECTION] GENERAL OPERATIONS
//-----------------------------------------------------------------------------

// Allocate memory with cNBT allocator function.
cNBT_ATTR void *cNBT_API cNBT_Alloc(
  size_t size);

// Free a created pointer by cNBT, e.g. the return value of cNBT_Write().
// If you want to free an NBT object, use cNBT_Delete() instead.
cNBT_ATTR void cNBT_API cNBT_Free(
  const void *ptr);

// Free the whole NBT object recursively. DO NOT access deleted NBT objects.
cNBT_ATTR void cNBT_API cNBT_Delete(
  cNBT *nbt);

// Parse a binary NBT data.
cNBT_ATTR cNBT *cNBT_API cNBT_Parse(
  const void *data, size_t size, uint8_t bigEndian);

// Serialize a NBT object to binary data.
cNBT_ATTR const void *cNBT_API cNBT_Write(
  cNBT *nbt, size_t initialCapacity, uint8_t bigEndian, size_t *length);

#ifdef __cplusplus
}
#endif

#endif
