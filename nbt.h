#ifndef __NBT_H__
#define __NBT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef cNBT_API
#define cNBT_API __stdcall
#endif

#ifndef cNBT_ATTR
#define cNBT_ATTR
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
  struct cNBT_t *prev;

  // A list or object item will have a child pointer pointing to a chain of
  // the items in the list/object.
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

typedef void *(__stdcall *cNBTMemAllocFn)(
  size_t, void *);
typedef void (__stdcall *cNBTMemFreeFn)(
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
  cNBT *nbt, uint8_t type);

// Find items with matching the given key name.
cNBT_ATTR cNBT *cNBT_API cNBT_GetNodeByKey(
  cNBT *nbt, const char *key);

// Find items with matching the given key name and the given type.
cNBT_ATTR cNBT *cNBT_API cNBT_GetNodeByKeyType(
  cNBT *nbt, const char *key, uint8_t type);

// Get the type of an item.
cNBT_ATTR uint8_t cNBT_API cNBT_GetNodeType(
  cNBT *nbt);

// Get the key name of an item.
cNBT_ATTR const char *cNBT_API cNBT_GetNodeKey(
  cNBT *nbt);

// Obtain the string length carried by the string node.
// Avaliable only for string nodes.
cNBT_ATTR uint16_t cNBT_API cNBT_GetValueStringLength(
  cNBT *nbt);

// Obtain the pointer to the string carried by the string node.
// Avaliable only for string nodes.
cNBT_ATTR const char *cNBT_API cNBT_GetValueString(
  cNBT *nbt);

// Create an NBT item.
cNBT_ATTR cNBT *cNBT_API cNBT_CreateNode(
  uint8_t type);

// Add a node to the object.
cNBT_ATTR cNBT *cNBT_API cNBT_AddNode(
  cNBT *nbt, cNBT *item, const char *key);

// Set the value of a node.
cNBT_ATTR cNBT *cNBT_API cNBT_SetValue(
  cNBT *nbt,
  const void *data,
  size_t length);

cNBT_ATTR cNBT *cNBT_API cNBT_SetValueString(
  cNBT *nbt,
  const char *string,
  size_t length);

//-----------------------------------------------------------------------------
// [SECTION] GENERAL OPERATIONS
//-----------------------------------------------------------------------------

// Free a created pointer by cNBT, like the return value of cNBT_Write().
cNBT_ATTR void cNBT_API cNBT_Free(
  const void *p);

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