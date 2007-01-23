/** @file
 * IPFIX protocol constants.
 *
 * This function provides constants and functions necessary or useful for interpretation of IPFIX messages.
 *
 */

/* wrapper into ipfixlolib - double data makes no sense */

#include <string.h>
#include "ipfixlolib/ipfixlolib.h"
#include "exp_ipfix.h"


int Expressstring2typeid(const char *s)
{
	return ipfix_name_lookup(s);
}

char* Expresstypeid2string(int i)
{
        const struct ipfix_identifier *ix;
	ix=ipfix_id_lookup(i);

	if(!ix) {
		return NULL;
	}

	return ix->name;
}

int Expressstring2typelength(const char *s)
{
	const struct ipfix_identifier *ix;

	ix=ipfix_id_lookup(ipfix_name_lookup(s));

	if(!ix) {
		return 0;
	}

	return ix->length;
}
