#include "nbt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
// [SECTION] MEMORY MANAGEMENT
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
static void *gMemUserData = cNBT_NULLPTR;

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
// [SECTION] NBT PARSER
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
// FIXME: Incorrect value endians.
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

  if (length)
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

  memset((void *)item, 0, sizeof(cNBT));
  item->prev = item->next = cNBT_NULLPTR;

  while (length > 0) {
    item->key = cNBT_NULLPTR;

    cNBT_ParseX(reader, item, type);

    length--;
    if (length) {
      // Create next node.
      cNBT *next = gMemAllocFn(sizeof(cNBT), gMemUserData);
      memset((void *)next, 0, sizeof(cNBT));
      item->next = next;
      next->prev = item;
      next->next = cNBT_NULLPTR;
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

  item->next = item->prev = cNBT_NULLPTR;

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
      next->next = cNBT_NULLPTR;
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
  item->listElementType = cNBT_END;
  item->type = type;
  item->child = cNBT_NULLPTR;
  memset((void *)&item->value, 0, sizeof(cNBTPayload));

  switch (type) {
    // Basic types.
    case cNBT_I08:
      item->value.valueI08 = cNBT_ParseI08(reader);
      break;
    case cNBT_I16:
      item->value.valueI16 = cNBT_ParseI16(reader);
      break;
    case cNBT_I32:
      item->value.valueI32 = cNBT_ParseI32(reader);
      break;
    case cNBT_I64:
      item->value.valueI64 = cNBT_ParseI64(reader);
      break;
    case cNBT_F32:
      item->value.valueF32 = cNBT_ParseF32(reader);
      break;
    case cNBT_F64:
      item->value.valueF64 = cNBT_ParseF64(reader);
      break;

    // Array of 8-bit integers.
    case cNBT_A08:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int8_t));
      break;

    // String.
    case cNBT_STR:
      item->value.lengthString = cNBT_ParseStr(reader, &item->value.valueString);
      break;

    // List.
    case cNBT_LST:
      item->listElementType = cNBT_ParseLst(reader, &item->child);
      break;

    // Object.
    case cNBT_OBJ:
      item->child = cNBT_ParseObj(reader);
      break;

    // Array of 32-bit integers.
    case cNBT_A32:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int32_t));
      break;

    // Array of 64-bit integers.
    case cNBT_A64:
      item->value.lengthArray = cNBT_ParseArr(reader, &item->value.valueArray, sizeof(int64_t));
      break;
  }
}

//-----------------------------------------------------------------------------
// [SECTION] VALUE OPERATIONS
//-----------------------------------------------------------------------------

uint8_t cNBT_IsType(
  cNBT *nbt,
  uint8_t type
) {
  if (!nbt)
    // Invalid object.
    return 0;
  if (type > cNBT_A64)
    // Invalid type byte.
    return 0;
  if (nbt->type != type)
    return 0;
  return 1;
}

cNBT *cNBT_GetNodeByKey(
  cNBT *nbt,
  const char *key
) {
  if (!nbt || !key || nbt->type != cNBT_OBJ)
    return cNBT_NULLPTR;

  cNBT *item;
  cNBT_ForEach(nbt, item) {
    if (item->key && !strcmp(key, item->key))
      return item;
  }

  return cNBT_NULLPTR;
}

cNBT *cNBT_GetNodeByKeyType(
  cNBT *nbt,
  const char *key,
  uint8_t type
) {
  if (!nbt || !key || nbt->type != cNBT_OBJ)
    return cNBT_NULLPTR;

  cNBT *item;
  cNBT_ForEach(nbt, item) {
    if (item->key && !strcmp(key, item->key) && item->type == type)
      return item;
  }

  return cNBT_NULLPTR;
}

uint8_t cNBT_GetNodeType(
  cNBT *nbt
) {
  if (!nbt)
    return cNBT_END;
  return nbt->type;
}

const char *cNBT_GetNodeKey(
  cNBT *nbt
) {
  if (!nbt)
    return cNBT_NULLPTR;
  return nbt->key;
}

uint16_t cNBT_GetValueStringLength(
  cNBT *nbt
) {
  if (!nbt || nbt->type != cNBT_STR || !nbt->value.valueString)
    return 0;
  return nbt->value.lengthString;
}

const char *cNBT_GetValueString(
  cNBT *nbt
) {
  if (!nbt || nbt->type != cNBT_STR  || !nbt->value.valueString)
    return cNBT_NULLPTR;
  return nbt->value.valueString;
}

cNBT *cNBT_CreateNode(
  uint8_t type
) {
  if (type > cNBT_A64)
    return cNBT_NULLPTR;

  cNBT *result = gMemAllocFn(sizeof(cNBT), gMemUserData);
  memset((void *)result, 0, sizeof(cNBT));

  result->type = type;

  return result;
}

cNBT *cNBT_AddNode(
  cNBT *nbt,
  cNBT *item,
  const char *key
) {
  if (!nbt || !item)
    return cNBT_NULLPTR;
  if (nbt->type != cNBT_LST && nbt->type != cNBT_OBJ)
    return cNBT_NULLPTR;

  if (nbt->type == cNBT_OBJ && cNBT_GetNodeByKey(nbt, key))
    return cNBT_NULLPTR;
  if (nbt->type == cNBT_LST && item->type != nbt->listElementType)
    return cNBT_NULLPTR;

  if (item->key || item->next || item->prev)
    // We don't know where the key from, so we just return.
    return cNBT_NULLPTR;

  // Copy the key.
  if (nbt->type != cNBT_LST) {
    size_t length = strlen(key);
    item->key = gMemAllocFn(length + 1, gMemUserData);
    if (length)
      memcpy((void *)item->key, key, length);
    item->key[length] = '\0';
  } else {
    item->key = cNBT_NULLPTR;
  }

  // Set as a child of given object.
  if (!nbt->child) {
    nbt->child = item;
    return nbt;
  }

  cNBT *node;
  cNBT_ForEach(nbt, node) {
    if (!node->next) {
      node->next = item;

      item->next = cNBT_NULLPTR;
      item->prev = node;

      break;
    }
  }

  return nbt;
}

cNBT *cNBT_SetValue(
  cNBT *nbt,
  const void *data,
  size_t length
) {
  if (!nbt || !data)
    return cNBT_NULLPTR;

  switch (nbt->type) {
    // Basic types. The length will be ignored.
    case cNBT_I08:
      memcpy(&nbt->value.valueI08, data, sizeof(int8_t));
      break;
    case cNBT_I16:
      memcpy(&nbt->value.valueI16, data, sizeof(int16_t));
      break;
    case cNBT_I32:
      memcpy(&nbt->value.valueI32, data, sizeof(int32_t));
      break;
    case cNBT_I64:
      memcpy(&nbt->value.valueI64, data, sizeof(int64_t));
      break;
    case cNBT_F32:
      memcpy(&nbt->value.valueF32, data, sizeof(float));
      break;
    case cNBT_F64:
      memcpy(&nbt->value.valueF64, data, sizeof(double));
      break;

    case cNBT_A08:
      if (nbt->value.valueArray)
        gMemFreeFn(nbt->value.valueArray, gMemUserData);
      nbt->value.lengthArray = length;
      if (length) {
        nbt->value.valueArray = gMemAllocFn(length * sizeof(int8_t), gMemUserData);
        memcpy(nbt->value.valueArray, data, length * sizeof(int8_t));
      } else {
        nbt->value.valueArray = cNBT_NULLPTR;
      }
      break;

    case cNBT_STR:
      if (nbt->value.valueString)
        gMemFreeFn(nbt->value.valueString, gMemUserData);
      if (!length)
        length = strlen((const char *)data);
      nbt->value.lengthString = length;
      if (length) {
        nbt->value.valueString = gMemAllocFn((length + 1) * sizeof(char), gMemUserData);
        memcpy(nbt->value.valueString, data, length * sizeof(char));
        nbt->value.valueString[length] = '\0';
      } else {
        nbt->value.valueString = cNBT_NULLPTR;
      }
      break;

    default:
      return cNBT_NULLPTR;
  }

  return nbt;
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
      item->type == cNBT_A08
      || item->type == cNBT_A32
      || item->type == cNBT_A64
    )
      gMemFreeFn(item->value.valueArray, gMemUserData);
    if (item->type == cNBT_STR)
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
    return cNBT_NULLPTR;

  cNBTReader reader = {
    .bigEndian = bigEndian,
    .data = data,
    .length = size,
    .offset = 0,
    .errorFlag = 0
  };

  cNBT *result = gMemAllocFn(sizeof(cNBT), gMemUserData);
  uint8_t type = cNBT_ParseI08(&reader);

  result->next = result->prev = cNBT_NULLPTR;

  // Parse the key of the element.
  cNBT_ParseStr(&reader, &result->key);
  cNBT_ParseX(&reader, result, type);

  return result;
}

const void *cNBT_Write() {

}
