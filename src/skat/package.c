#include "skat/package.h"

#undef PACKAGE_C_HDR
#define PACKAGE_HDR_TO_STRING

#include "skat/package.h"

void
package_clean(package *p) {
  p->type = PACKAGE_INVALID;
  p->payload_size = 0;
  p->payload = NULL;
}

void
package_free(package *p) {
  if (p->payload)
	free(p->payload);
  package_clean(p);
}
