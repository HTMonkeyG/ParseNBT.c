#include "nbt.h"
#include <stdlib.h>
#include <string.h>

// We do not use memcpy() because we do not know what endian the target machine
// and raw data used.
#define cNBT_GetByteLE(curs, offs) (((uint32_t)*((curs) + (offs))) << ((offs) * 8))
#define cNBT_GetByteBE(curs, offs, size) (((uint32_t)*((curs) + (offs))) << ((size) - (offs) * 8 - 8))

typedef struct {
  const void *data;
  size_t offset;
  size_t length;
  uint8_t bigEndian;
  uint32_t errorFlag;
} cNBTReader;

//-----------------------------------------------------------------------------
// [SECTION] Memory management functions.
//-----------------------------------------------------------------------------

#ifndef cNBT_DISABLE_DEFAULT_ALLOCATORS
static void *cNBT_MallocWrapper(size_t size, void *userData) { (void)userData; return malloc(size); }
static void cNBT_FreeWrapper(void *ptr, void *userData) { (void)userData; free(ptr); }
#else
static void *cNBT_MallocWrapper(size_t size, void *userData) { (void)userData; (void)size; return NBT_NULLPTR; }
static void cNBT_FreeWrapper(void *ptr, void *userData) { (void)userData; (void)ptr; }
#endif

static cNBTMemAllocFn gMemAllocFn = cNBT_MallocWrapper;
static cNBTMemFreeFn gMemFreeFn = cNBT_FreeWrapper;
static void *gMemUserData = NBT_NULLPTR;

void cNBT_GetAllocators(
  cNBTMemAllocFn *allocFn,
  cNBTMemFreeFn *freeFn,
  void **userData
) {
  *allocFn = gMemAllocFn;
  *freeFn = gMemFreeFn;
  *userData = gMemUserData;
}

void cNBT_SetAllocators(
  cNBTMemAllocFn allocFn,
  cNBTMemFreeFn freeFn,
  void *userData
) {
  gMemAllocFn = allocFn;
  gMemFreeFn = freeFn;
  gMemUserData = userData;
}

//-----------------------------------------------------------------------------
// [SECTION] NBT PARSER.
//-----------------------------------------------------------------------------

// Declaration of the dispatcher function.
static void cNBT_ParseX(
  cNBTReader *reader,
  cNBT *item,
  uint8_t type);

static int8_t cNBT_ParseI08(
  cNBTReader *reader
) {
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  reader->offset += 1;
  return (int8_t)cNBT_GetByteLE(cursor, 0);
}

static int16_t cNBT_ParseI16(
  cNBTReader *reader
) {
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  uint16_t result;

  reader->offset += 2;

  if (reader->bigEndian) {
    result = cNBT_GetByteBE(cursor, 0, 16)
           | cNBT_GetByteBE(cursor, 1, 16);
  } else {
    result = cNBT_GetByteLE(cursor, 0)
           | cNBT_GetByteLE(cursor, 1);
  }

  return (int16_t)result;
}

static int32_t cNBT_ParseI32(
  cNBTReader *reader
) {
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  uint32_t result;

  reader->offset += 4;

  if (reader->bigEndian) {
    result = cNBT_GetByteBE(cursor, 0, 32)
           | cNBT_GetByteBE(cursor, 1, 32)
           | cNBT_GetByteBE(cursor, 2, 32)
           | cNBT_GetByteBE(cursor, 3, 32);
  } else {
    result = cNBT_GetByteLE(cursor, 0)
           | cNBT_GetByteLE(cursor, 1)
           | cNBT_GetByteLE(cursor, 2)
           | cNBT_GetByteLE(cursor, 3);
  }

  return (int32_t)result;
}

static int64_t cNBT_ParseI64(
  cNBTReader *reader
) {
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  uint32_t resultHi, resultLo;
  uint64_t result;

  reader->offset += 8;

  if (reader->bigEndian) {
    resultHi = cNBT_GetByteBE(cursor, 0, 32)
             | cNBT_GetByteBE(cursor, 1, 32)
             | cNBT_GetByteBE(cursor, 2, 32)
             | cNBT_GetByteBE(cursor, 3, 32);
    resultLo = cNBT_GetByteBE(cursor + 4, 0, 32)
             | cNBT_GetByteBE(cursor + 4, 1, 32)
             | cNBT_GetByteBE(cursor + 4, 2, 32)
             | cNBT_GetByteBE(cursor + 4, 3, 32);
  } else {
    resultLo = cNBT_GetByteLE(cursor, 0)
             | cNBT_GetByteLE(cursor, 1)
             | cNBT_GetByteLE(cursor, 2)
             | cNBT_GetByteLE(cursor, 3);
    resultHi = cNBT_GetByteLE(cursor + 4, 0)
             | cNBT_GetByteLE(cursor + 4, 1)
             | cNBT_GetByteLE(cursor + 4, 2)
             | cNBT_GetByteLE(cursor + 4, 3);
  }

  result = (uint64_t)resultHi << 32 | (uint64_t)resultLo;

  return (int64_t)result;
}

static float cNBT_ParseF32(
  cNBTReader *reader
) {
  float result;
  int32_t tmp = cNBT_ParseI32(reader);
  memcpy((void *)&result, (void *)&tmp, sizeof(float));
  return result;
}

static double cNBT_ParseF64(
  cNBTReader *reader
) {
  double result;
  int64_t tmp = cNBT_ParseI64(reader);
  memcpy((void *)&result, (void *)&tmp, sizeof(double));
  return result;
}

// Read an array.
static int32_t cNBT_ParseArr(
  cNBTReader *reader,
  void **result,
  size_t perElement
) {
  int32_t length = cNBT_ParseI32(reader);
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  void *valueArr = gMemAllocFn(length * perElement, gMemUserData);

  memcpy(valueArr, (void *)cursor, length * perElement);
  *result = valueArr;

  return length;
}

// Read a string.
static uint16_t cNBT_ParseStr(
  cNBTReader *reader,
  char **result
) {
  uint16_t length = (uint16_t)cNBT_ParseI16(reader);
  const uint8_t *cursor = ((uint8_t *)reader->data + reader->offset);
  char *valueString = gMemAllocFn(length + 1, gMemUserData);

  memcpy((void *)valueString, (void *)cursor, length);
  valueString[length] = '\0';
  reader->offset += length;

  *result = valueString;

  return length;
}

// Read a list.
static uint8_t cNBT_ParseLst(
  cNBTReader *reader,
  cNBT **result
) {
  cNBT *first = gMemAllocFn(sizeof(cNBT), gMemUserData)
    , *item = first;
  uint8_t type = cNBT_ParseI08(reader);
  int32_t length = cNBT_ParseI32(reader);

  item->prev = item->next = NBT_NULLPTR;

  while (length > 0) {
    item->key = NBT_NULLPTR;

    cNBT_ParseX(reader, item, type);

    length--;
    if (length) {
      // Create next node.
      cNBT *next = gMemAllocFn(sizeof(cNBT), gMemUserData);
      item->next = next;
      next->prev = item;
      next->next = NBT_NULLPTR;
      item = next;
    }
  }

  *result = first;

  return type;
}

// Read an object.
static cNBT *cNBT_ParseObj(
  cNBTReader *reader
) {
  cNBT *result = gMemAllocFn(sizeof(cNBT), gMemUserData)
    , *item = result;
  uint8_t type = cNBT_ParseI08(reader);
  char *key;

  item->next = item->prev = NBT_NULLPTR;

  while (type) {
    // Parse the key of the element.
    cNBT_ParseStr(reader, &key);
    item->key = key;

    cNBT_ParseX(reader, item, type);

    type = cNBT_ParseI08(reader);
    if (type) {
      // Create next node.
      cNBT *next = gMemAllocFn(sizeof(cNBT), gMemUserData);
      item->next = next;
      next->prev = item;
      next->next = NBT_NULLPTR;
      item = next;
    }
  }

  return result;
}

// Parse an item of the specified type.
static void cNBT_ParseX(
  cNBTReader *reader,
  cNBT *item,
  uint8_t type
) {
  item->listElementType = NBT_END;
  item->type = type;
  item->child = NBT_NULLPTR;
  memset((void *)&item->value, 0, sizeof(cNBTPayload));

  switch (type) {
    // Basic types.
    case NBT_I08:
      item->value.valueI08 = cNBT_ParseI08(reader);
      break;
    case NBT_I16:
      item->value.valueI16 = cNBT_ParseI16(reader);
      break;
    case NBT_I32:
      item->value.valueI32 = cNBT_ParseI32(reader);
      break;
    case NBT_I64:
      item->value.valueI64 = cNBT_ParseI64(reader);
      break;
    case NBT_F32:
      item->value.valueF32 = cNBT_ParseF32(reader);
      break;
    case NBT_F64:
      item->value.valueF64 = cNBT_ParseF64(reader);
      break;

    // Array of 8-bit integers.
    case NBT_A08:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int8_t));
      break;

    // String.
    case NBT_STR:
      item->value.lengthString = cNBT_ParseStr(reader, &item->value.valueString);
      break;

    // List.
    case NBT_LST:
      item->listElementType = cNBT_ParseLst(reader, &item->child);
      break;

    // Object.
    case NBT_OBJ:
      item->child = cNBT_ParseObj(reader);
      break;

    // Array of 32-bit integers.
    case NBT_A32:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int32_t));
      break;

    // Array of 64-bit integers.
    case NBT_A64:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int64_t));
      break;
  }
}

//-----------------------------------------------------------------------------
// [SECTION] GENERAL OPERATIONS
//-----------------------------------------------------------------------------

void cNBT_Delete(
  cNBT *nbt
) {
  for (cNBT *item = nbt, *next, *child; item; item = next) {
    // Save the next and child nodes to avoid access freed items.
    next = item->next;
    child = item->child;
    if (
      item->type == NBT_A08
      || item->type == NBT_A32
      || item->type == NBT_A64
    )
      gMemFreeFn(item->value.valueArray, gMemUserData);
    if (item->type == NBT_STR)
      gMemFreeFn(item->value.valueString, gMemUserData);
    if (item->key)
      gMemFreeFn(item->key, gMemUserData);

    if (child)
      // Free child nodes recursively.
      cNBT_Delete(child);

    gMemFreeFn(item, gMemUserData);
  }
}

cNBT *cNBT_Parse(
  const void *data,
  size_t size,
  uint8_t bigEndian
) {
  if (!data)
    return NBT_NULLPTR;

  cNBTReader reader = {
    .bigEndian = bigEndian,
    .data = data,
    .length = size,
    .offset = 0,
    .errorFlag = 0
  };

  return cNBT_ParseObj(&reader);
}

const void *cNBT_Write() {

}
