#include "revert_string.h"
#include <string.h>
#include <stdio.h>

void RevertString(char *str)
{
	char buf, *end;

    end = str + strlen(str) - 1;
	for (char *i = str;i < end; i++) {
		buf = *i;
        *i = *end;
		*end = buf;
		end--;
	}
}

