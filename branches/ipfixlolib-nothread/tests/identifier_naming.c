#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ipfix_names.h"

static int test(int start, int end)
{
	struct ipfix_identifier *x;
        int hash_id;
	int i=start;

	while((x = ipfix_id_lookup(i))) {

		if(x->id != i) {
			printf("Failure in mapping tab!\n");
			printf("Position: %d and Tab has: %d (%s)\n", i, x->id, x->name);
			return -1;
		}

		hash_id=ipfix_name_lookup(x->name);
                assert((hash_id >= 0));

		/* I love short circuit */
		if((strcasecmp(x->name, "RESERVED") != 0) && (x->id != hash_id)) {
			printf("Failure in mapping hash!\n");
			return -1;
		}

		printf("%d -> %s at %d\n", i, x->name, x->id);
		i++;
	}

        i--;

	if(i != end) {
                printf("Possibly not all IDs have been iterated!\n");
		return -1;
	}
	printf("Check completed; %d IDs (%d:%d)\n", end-start, start, end);
        return 0;
}


int main()
{
        int hash_id;

	test(IPFIX_ID_MIN, IPFIX_ID_MAX);
        test(PSAMP_ID_MIN, PSAMP_ID_MAX);

        if((hash_id=ipfix_name_lookup("UNKNOWN_THING") != -1)) {
		printf("Failure in mapping hash: unknown key does not return -1\n");
                return -1;
        }

	printf("Unknow key check passed\n");

        return 0;
}
