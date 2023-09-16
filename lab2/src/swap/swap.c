#include "swap.h"
#include <stdio.h>

void Swap(char *left, char *right)
{
	char buf = *left;

	*left = *right;
	*right = buf;
}
