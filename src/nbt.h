#ifndef __NBT_H__
#define __NBT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NBT_API
#define NBT_API __stdcall
#endif

#ifndef NBT_ATTR
#define NBT_ATTR
#endif

#define NBT_END 0x00
#define NBT_I08 0x01
#define NBT_I16 0x02
#define NBT_I32 0x03
#define NBT_I64 0x04
#define NBT_F32 0x05
#define NBT_F64 0x06
#define NBT_A08 0x07
#define NBT_STR 0x08
#define NBT_LST 0x09
#define NBT_OBJ 0x0A
#define NBT_A32 0x0B
#define NBT_A64 0x0C

#ifdef NULL
#define NBT_NULLPTR NULL
#else
#define NBT_NULLPTR ((void *)0)
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

typedef void *(__stdcall *cNBTMemAllocFn)(
  size_t, void *);
typedef void (__stdcall *cNBTMemFreeFn)(
  void *, void *);

// Get current memory allocator functions.
NBT_ATTR void NBT_API cNBT_GetAllocators(
  cNBTMemAllocFn *allocFn,
  cNBTMemFreeFn *freeFn,
  void **userData);

// Set current memory allocator functions. you can implement your own
// allocator with this function.
NBT_ATTR void NBT_API cNBT_SetAllocators(
  cNBTMemAllocFn allocFn,
  cNBTMemFreeFn freeFn,
  void *userData);

// Free the whole NBT object recursively. DO NOT access deleted NBT objects.
NBT_ATTR void NBT_API cNBT_Delete(
  cNBT *nbt);

// Parse a binary NBT data.
NBT_ATTR cNBT NBT_API *cNBT_Parse(
  const void *data,
  size_t size,
  uint8_t bigEndian);

#ifdef __cplusplus
}
#endif

#endif
