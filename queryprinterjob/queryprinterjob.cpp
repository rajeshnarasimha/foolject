// queryprinterjob.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Winspool.h>

int main(int argc, char* argv[])
{
	HANDLE hPrinter = NULL;
	if (OpenPrinter(argc<1?NULL:argv[1], &hPrinter, NULL))
	{
		JOB_INFO_1 jobs [16] = {0};
		DWORD cbNeeded = 0;
		DWORD cReturned = 0;
		EnumJobs(hPrinter, 0, 10, 1, (LPBYTE)&jobs, sizeof(jobs), &cbNeeded,  &cReturned);
		ClosePrinter(hPrinter);
		printf("%d jobs in this printer queue.\n", cReturned);
		return cReturned;
	}
	printf("open printer failed.\n");
	return -1;
}

