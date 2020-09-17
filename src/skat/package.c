#include "skat/package.h"
#include <string.h>

#undef PACKAGE_C_HDR
#define PACKAGE_HDR_TO_STRING

#include "skat/package.h"

void
package_clean(package *p) {
  memset(p, '\0', sizeof(package));
  p->type = PACKAGE_INVALID;
  p->payload_size = 0;
  p->payload.v = NULL;
}

void
package_free(package *p) {
  if (p->payload.v)
	free(p->payload.v);
  package_clean(p);
}
