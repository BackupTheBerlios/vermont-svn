#include <stdio.h>
#include <string.h>
#include "ipfix_names.h"

int main()
{
	struct ipfix_identifier *x;
        int hash_id;
	int i=1;

	while((x = ipfix_id_lookup(i))) {
		if(x->id != i) {
			printf("Failure in mapping tab!\n");
			printf("Position: %d and Tab has: %d (%s)\n", i, x->id, x->name);
			return -1;
		}

		hash_id=ipfix_name_lookup(x->name);

                /* I love short circuit */
                if((strcmp(x->name, "RESERVED") != 0) && (x->id != hash_id)) {
			printf("Failure in mapping hash!\n");
                        return -1;
		}

                printf("%d -> %s at %d\n", i, x->name, x->id);
		i++;
	}

	printf("Check completed over %d ids\n", i-1);

        if((hash_id=ipfix_name_lookup("UNKNOWN_THING") != -1)) {
		printf("Failure in mapping hash: unknown key does not return -1\n");
                return -1;
        }

	printf("Unknow key check passed\n");

        return 0;
}
