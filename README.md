# ParseNBT.c (cNBT)
An NBT parser and serializer written in C.

## Example

```c
#include <stdio.h>
#include "nbt.h"

int main() {
  cNBT *root = cNBT_CreateNode(cNBT_OBJ)
    , format_version = cNBT_CreateNode(cNBT_I32);

  static const int a = 1;
  cNBT_SetValue(format_version, &a, 0);

  cNBT_AddNode(root, format_version, "format_version");

  size_t v;
  const char *w = cNBT_Write(u, 5, 0, &v);
  for (size_t i = 0; i < v; i++)
    printf("%02x ", w[i]);

  cNBT_Free(w);
  cNBT_Delete(root);

  return 0;
}
```
