// redundancy_test.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <conio.h>
#include "fool_redundancy.h"

#ifdef HOST
#define IP "192.168.101.122"
#else
#define IP "192.168.101.70"
#endif

int main(int argc, char* argv[])
{
	int rc = 0;
	Redundancy rddc;
	HANDLE quit = 0;

	quit = CreateEvent(NULL, FALSE, FALSE, NULL);

	rc = rddc.init(1, IP, "255.255.255.0", 6868, 6869, quit);
	if (rc != 0) return -1;

	while (!_kbhit()) {
		if (WAIT_OBJECT_0+1-1 == 
			WaitForMultipleObjects(1, &quit, TRUE, 1000))
			break;
	}

	rddc.uninit();

	CloseHandle(quit);
	return 0;
}

