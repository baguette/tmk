#include <stdio.h>

int main()
{
	if (sizeof(unsigned long) == 4) {
		printf("unsigned long\n");
	} else if (sizeof(unsigned int) == 4) {
		printf("unsigned int\n");
	} else if (sizeof(unsigned short) == 4) {
		printf("unsigned short\n");
	} else if (sizeof(unsigned char) == 4) {
		printf("unsigned char\n");
	} else {
		return 1;
	}

	return 0;
}

