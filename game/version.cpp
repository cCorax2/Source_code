#include <stdio.h>

void WriteVersion()
{
#ifndef __WIN32__
	FILE* fp = fopen("VERSION.txt", "w");

	if (fp)
	{
		fprintf(fp, "game revision: 40250\n");
		//fprintf(fp, "%s@%s:%s\n", __USER__, __HOSTNAME__, __PWD__);
		fclose(fp);
	}
#endif
}

