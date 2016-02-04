#include <Windows.h>
#include <Winnt.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
//#include <conio.h>

#define BUF_SIZE 255

typedef struct aThreadObject{
	DWORD nBuffers;
	DWORD workerID;
	DWORD sleepTime;
	DWORD nReadErrors;
	HANDLE * memoryHandles;
	HANDLE * semaphoreHandles;
} workerStruct;



static DWORD WINAPI theThreadFunction(LPVOID pArgs);
static DWORD WINAPI theSemaphoreThreadFunction(LPVOID pArgs);
void sortRandomNumbers(DWORD *randomNumbers, DWORD nWorkers);
void createSortChildProcess(HANDLE *sortChildInRead, HANDLE *sortChildOutWrite);
BOOL isPrime(DWORD pNumber);

int main(DWORD argc, LPTSTR argv[])
{
	DWORD nBuffers;
	DWORD nWorkers;
	CHAR * forErrorNB, *forErrorNW;
	DWORD randSeed = 1;
	CHAR * forErrorRS;
	DWORD sleepMin;
	DWORD sleepMax;
	CHAR *forErrorSMin, *forErrorSMax;
	BOOL useSemaphore = FALSE;

	/*Get arguments from command line*/
	if (argc >= 5 && argc <= 7)
	{
		nBuffers = strtol(argv[1], &forErrorNB, 10);
		printf_s("nBuffers: %d\n", nBuffers);
		nWorkers = strtol(argv[2], &forErrorNW, 10);
		sleepMin = strtol(argv[3], &forErrorSMin, 10);
		sleepMax = strtol(argv[4], &forErrorSMax, 10);
		if (nBuffers < 2 || nBuffers > 32 || !isPrime(nBuffers))
		{
			printf_s("nBuffers must be a prime number between 2 and 32\n");
			getchar();
			exit(1);
		}
		if (nWorkers >= nBuffers || nWorkers <= 0)
		{
			printf_s("nWorkers must be less than nBuffers and greater than 0");
			getchar();
			exit(1);
		}
		if (sleepMin > sleepMax || sleepMin < 0 || sleepMax < 0)
		{
			printf_s("sleepMin must be less than sleepMax and both real numbers must be greater than zero\n");
			getchar();
			exit(1);
		}
		if (argc == 6)
		{
			randSeed = strtol(argv[5], &forErrorRS, 10);
			if (strlen(forErrorRS) > 0)
			{
				if (strcmp("-lock", forErrorRS) == 0)
					useSemaphore = TRUE;
				else if (strcmp("-nlock", forErrorRS) == 0)
					useSemaphore = FALSE;
				else
				{
					printf_s("Last argument may be \"-lock\" or \"-nlock\", you have typed an invalid entry.\n");
					getchar();
					exit(1);
				}
			}
			else if (randSeed == 0)
				randSeed = 1;
		//	printf_s("Test: %s\n", forErrorRS);
		//	getchar();
		}
		if (argc == 7)
		{
			randSeed = strtol(argv[5], &forErrorRS, 10);
			if (randSeed == 0)
				randSeed = 1;

			if (strcmp("-lock", argv[6]) == 0)
				useSemaphore = TRUE;
			else if (strcmp("-nlock", argv[6]) == 0)
				useSemaphore = FALSE;
			else
			{
				printf_s("Last argument may be \"-lock\" or \"-nolock\", you have typed an invalid entry.\n");
				getchar();
				exit(1);
			}
		}

	}
	else
	{
		printf_s("Enter execution of raceTest with arguments as shown below\n");
		printf_s("raceTest nBuffers nWorkers sleepMin sleepMax [ randseed ] [ -lock | -nolock ] \n");
		exit(1);
	}
	/*  Create lpSemName values if the program is to use semaphores*/
	LPTSTR *semaphoreNames = (LPCTSTR *)malloc(nBuffers * sizeof(LPCTSTR));
	DWORD i, j = 0;
	if (useSemaphore == FALSE)
	{
		DWORD i;
		for (i = 0;i < nBuffers;i++)
		{
			semaphoreNames[i] = NULL;
		}
	}
	else
	{
		DWORD i, j = 0;
		for (i = 0;i < nBuffers;i++)
		{
			CHAR *temp = (CHAR*)malloc(3 * sizeof(CHAR));
			if (i > 0 && i % 26 == 0) j++;
			temp[0] = 65 + j;
			temp[1] = 65 + i % 26;
			temp[2] = '\0';
			semaphoreNames[i] = temp;
			printf_s("%s\n", semaphoreNames[i]);
		}
	}

	/*Create lpMapName values*/
	LPTSTR * memoryNames = (LPTSTR *)malloc(nBuffers * sizeof(LPTSTR));
	for (i = 0;i<nBuffers;i++)
	{
		CHAR *temp = (CHAR*)malloc(3 * sizeof(CHAR));
		if (i>0 && i % 26 == 0) j++;
		temp[0] = 97 + j;
		temp[1] = 97 + i % 26;
		temp[2] = '\0';
		memoryNames[i] = temp;
		printf_s("%s\n", memoryNames[i]);
	}
	/*Create nWorkers random numbers*/
	DWORD *randomNumbers = (DWORD *)malloc(nWorkers * sizeof(DWORD));
	srand(randSeed);
	for (i = 0;i<nWorkers;i++)
	{
		randomNumbers[i] = (double)rand() / (RAND_MAX + 1) * (sleepMax - sleepMin)
			+ sleepMin;
	}
	printf_s("Random Numbers Unsorted:\n");
	for (i = 0;i < nWorkers;i++)
		printf_s("%d: %d\n", i, randomNumbers[i]);
	sortRandomNumbers(randomNumbers, nWorkers);
	printf_s("Random Numbers Sorted:\n");
	for (i = 0;i < nWorkers; i++)
		printf_s("%d: %d\n", i, randomNumbers[i]);
	getchar();
	/*Create semaphores if is to be used in the program*/
	HANDLE *semaphores = (HANDLE*)malloc(nBuffers * sizeof(HANDLE));
	if (useSemaphore == FALSE)
	{	
		DWORD i;
		for (i = 0;i < nBuffers;i++)
			semaphores[i] = NULL;
	}
	else
	{
		DWORD i;
		LPCTSTR tempSemaphoreName;
		for (i = 0;i < nBuffers;i++)
		{
			tempSemaphoreName = semaphoreNames[i];
			semaphores[i] = CreateSemaphore(NULL, 1, 1, tempSemaphoreName);
			if (semaphores[i] == NULL)
			{
				printf("CreateSemaphore error: %d\n", GetLastError());
				return 1;
			}
		}
	}
	

	/*Create shared memory array*/
	HANDLE *hMapFile = (HANDLE*)malloc(nBuffers * sizeof(HANDLE));
	LPDWORD hMapOut;
	DWORD bufSize = 256;
	DWORD n=0;
	LPCTSTR temp;
	for (i = 0;i<nBuffers;i++)
	{
		temp = memoryNames[i];
		hMapFile[i] = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufSize, temp);
		if (hMapFile[i] == NULL)
		{
			_tprintf(TEXT("Could not create file mapping object (%d).\n"), GetLastError());
			getchar();
			exit(2);
		}
		hMapOut = (LPDWORD)MapViewOfFile(hMapFile[i], FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (hMapOut == NULL)
		{
			_tprintf(TEXT("Could not map view of file (%d).\n"),
				GetLastError());
			CloseHandle(hMapFile[i]);
			return 1;
		}
		CopyMemory((PVOID)hMapOut, &n, (1 * sizeof(DWORD)));
		UnmapViewOfFile(hMapOut);
	}


	/*Create worker structs and threads*/
	workerStruct *workerArray = (workerStruct *)malloc(nWorkers * sizeof(workerStruct));	
	HANDLE *threadHandles = (HANDLE*)malloc(nWorkers*sizeof(HANDLE));
	DWORD * threadIDs = (DWORD *)malloc(nWorkers * sizeof(DWORD));

	for(i=0;i<nWorkers;i++)
	{
	workerArray[i].nBuffers = nBuffers;
	workerArray[i].workerID = i+1;
	workerArray[i].sleepTime = randomNumbers[i];
	workerArray[i].nReadErrors = 0;
	workerArray[i].memoryHandles = hMapFile;
	if (useSemaphore == TRUE)
		workerArray[i].semaphoreHandles = semaphores;
	else
		workerArray[i].semaphoreHandles = NULL;
	if(useSemaphore == TRUE)
		threadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, theSemaphoreThreadFunction, &workerArray[i], 0, &threadIDs[i]);
	else
		threadHandles[i] = (HANDLE)_beginthreadex(NULL,0,theThreadFunction,&workerArray[i],0,&threadIDs[i]);
	}

	WaitForMultipleObjects(nWorkers,threadHandles,TRUE,INFINITE);

	for (i = 0;i < nBuffers;i++)
	{
		hMapOut = (LPDWORD)MapViewOfFile(hMapFile[i], FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (hMapOut == NULL)
		{
			_tprintf(TEXT("Could not map view of file (%d).\n"),
				GetLastError());
			CloseHandle(hMapFile[i]);
			return 1;
		}
		_tprintf(TEXT("The value in Buffer %d is: %d\n"), i+1,*hMapOut);
		UnmapViewOfFile(hMapOut);
	}
	getchar();

	for(i=0;i<nWorkers;i++)
		CloseHandle(threadHandles[i]);

	for (i = 0;i < nBuffers;i++)
	{
		CloseHandle(hMapFile[i]);
		CloseHandle(semaphores[i]);
	}
	free(hMapFile);
	free(semaphores);
	free(workerArray);
	getchar();
	for (i = 0;i<nBuffers;i++)
	{
		free(semaphoreNames[i]);
		free(memoryNames[i]);
	}
	free(semaphoreNames);
	free(memoryNames);
	free(randomNumbers);


}

static DWORD WINAPI theThreadFunction(LPVOID pArgs)
{
	workerStruct *object;
	object = (workerStruct *)pArgs;
	HANDLE hStdout;
	DWORD i,j;
	DWORD buffer = object->workerID - 1;
	DWORD temp,tempWrite;
	LPDWORD hMap;

	TCHAR msgBuf[BUF_SIZE];
	size_t cchStringSize;
	DWORD dwChars;

	for (i = 0;i < object->nBuffers;i++)
	{
		for (j = 0;j < 3;j++)
		{
			if (j % 3 != 2)
			{
				hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
				temp = *hMap;
				UnmapViewOfFile(hMap);
				Sleep(object->sleepTime);
				hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
				if (temp != *hMap)
				{

					hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
					if (hStdout == INVALID_HANDLE_VALUE)
						return 1;

					StringCchPrintf(msgBuf, BUF_SIZE, TEXT("For Worker %d, Buffer %d value was changed from %d to %d while asleep\n"),
						object->workerID, buffer+1, temp, *hMap);
					StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
					WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
				}
				UnmapViewOfFile(hMap);
				buffer = (buffer + object->workerID) % object->nBuffers;
			}
			else
			{
				hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
				temp = *hMap;
				Sleep(object->sleepTime);
				temp = temp + (1 << (object->workerID - 1));
				tempWrite = temp;
				CopyMemory((PVOID)hMap, &tempWrite, (1 * sizeof(DWORD)));
				UnmapViewOfFile(hMap);
				buffer = (buffer + object->workerID) % object->nBuffers;
			}
		}

	}
	return 0;
}	


static DWORD WINAPI theSemaphoreThreadFunction(LPVOID pArgs)
{
	workerStruct *object;
	object = (workerStruct *)pArgs;
	HANDLE hStdout;
	DWORD i, j;
	DWORD buffer = object->workerID - 1;
	DWORD temp, tempWrite;
	LPDWORD hMap;

	TCHAR msgBuf[BUF_SIZE];
	size_t cchStringSize;
	DWORD dwChars;

	for (i = 0;i < object->nBuffers;i++)
	{
		for (j = 0;j < 3;j++)
		{	
			DWORD dwWaitResult;
			BOOL bContinue = TRUE;
			while (bContinue)
			{
				dwWaitResult = WaitForSingleObject(object->semaphoreHandles[buffer], 0L);
				switch (dwWaitResult)
				{
				case WAIT_OBJECT_0:
					if (j % 3 != 2)
					{
						hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
						temp = *hMap;
						UnmapViewOfFile(hMap);
						Sleep(object->sleepTime);
						hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
						if (temp != *hMap)
						{

							hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
							if (hStdout == INVALID_HANDLE_VALUE)
								return 1;

							StringCchPrintf(msgBuf, BUF_SIZE, TEXT("For Worker %d, Buffer %d value was changed from %d to %d while asleep\n"),
								object->workerID, buffer + 1, temp, *hMap);
							StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
							WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
						}
						UnmapViewOfFile(hMap);
					}
					else
					{
						hMap = (LPDWORD)MapViewOfFile(object->memoryHandles[buffer], FILE_MAP_ALL_ACCESS, 0, 0, 0);
						temp = *hMap;
						Sleep(object->sleepTime);
						temp = temp + (1 << (object->workerID - 1));
						tempWrite = temp;
						CopyMemory((PVOID)hMap, &tempWrite, (1 * sizeof(DWORD)));
						UnmapViewOfFile(hMap);
					}
					if (!ReleaseSemaphore(object->semaphoreHandles[buffer],1,NULL))
						printf("ReleaseSemaphore error: %d\n", GetLastError());
					buffer = (buffer + object->workerID) % object->nBuffers;
					bContinue = FALSE;
					break;
				case WAIT_TIMEOUT:
					Sleep(10);
					break;
				}
			}
		}

	}
	return 0;
}



void sortRandomNumbers(DWORD *randomNumbers, DWORD nWorkers)
{
	/*HANDLE *sortChildInRead = (HANDLE*)malloc(1*sizeof(HANDLE));
	HANDLE *sortChildInWrite = (HANDLE*)malloc(1*sizeof(HANDLE));
	HANDLE *sortChildOutRead = (HANDLE*)malloc(1*sizeof(HANDLE));
	HANDLE *sortChildOutWrite = (HANDLE*)malloc(1*sizeof(HANDLE));*/

	HANDLE sortChildInRead = NULL;
	HANDLE sortChildInWrite = NULL;
	HANDLE sortChildOutRead = NULL;
	HANDLE sortChildOutWrite = NULL;

	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 

	if (!CreatePipe(&sortChildOutRead, &sortChildOutWrite, &saAttr, 0))
	{
		printf_s("StdoutRd CreatePipe\n");
		exit(2);
	}
	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(sortChildOutRead, HANDLE_FLAG_INHERIT, 0))
	{
		printf_s("Stdout SetHandleInformation");
		exit(3);
	}

	// Create a pipe for the child process's STDIN. 

	if (!CreatePipe(&sortChildInRead, &sortChildInWrite, &saAttr, 0))
	{
		printf_s("Stdin CreatePipe");
		exit(4);
	}

	// Ensure the write handle to the pipe for STDIN is not inherited. 

	if (!SetHandleInformation(sortChildInWrite, HANDLE_FLAG_INHERIT, 0))
	{
		printf_s("Stdin SetHandleInformation");
		exit(5);
	}

	//createSortChildProcess(sortChildInRead, sortChildOutWrite);
	////////////////////////////////////////////////////////////////////
	CHAR *temp = (CHAR*)malloc(5 * sizeof(CHAR));
	temp[0] = 'S';
	temp[1] = 'O';
	temp[2] = 'R';
	temp[3] = 'T';
	temp[4] = '\0';
	LPTSTR cmd = temp;
	DWORD dwCreationFlags = 0;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startUpSearch;

	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&startUpSearch, sizeof(STARTUPINFO));
	startUpSearch.cb = sizeof(STARTUPINFO);
	startUpSearch.hStdError = sortChildOutWrite;
	startUpSearch.hStdOutput = sortChildOutWrite;
	startUpSearch.hStdInput = sortChildInRead;
	startUpSearch.dwFlags = STARTF_USESTDHANDLES;
	getchar();
	BOOL success = FALSE;
	success = CreateProcess(
		NULL,
		cmd,
		NULL,
		NULL,
		TRUE,
		dwCreationFlags,
		NULL,
		NULL,
		&startUpSearch,
		&processInfo);

	// If an error occurs, exit the application. 
	if (!success)
	{
		printf_s("Create Process failed\n");
		exit(7);
	}
	else
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	////////////////////////////////////////////////////////////////////
	//Write unsorted random numbers to Sort Child Process
	DWORD  numberOfBytesWritten=0, i=0;    
	BOOL bSuccess = FALSE;
	/////////////////////  TESTING
//	CHAR * numbers = (CHAR*) malloc(4*sizeof(CHAR));
	CHAR numbers[] = "THis is some data to write to the file";
	DWORD numberOfBytesToWrite = (DWORD) strlen(numbers);
	//printf_s("Unsorted: %c, %c, %c\n",numbers[0],numbers[1],numbers[2]);
	for(;;)
	{
		bSuccess = WriteFile(sortChildInWrite, numbers,numberOfBytesToWrite,
				&numberOfBytesWritten,NULL);
		//if(!bSuccess) break;
		i++;
		break;
	}
	printf_s("i: %d\n",i);
	//	DWORD j = 0;
	//for(j;j<3;j++)
	//	printf_s("numbers[%d]: %c\n",j,numbers[j]);

	////////////////////// TESTING
	/*while(i < nWorkers)
	{
		bSuccess = WriteFile(sortChildInWrite, randomNumbers[i], numberOfBytesToWrite,
			&numberOfBytesWritten, NULL);
		if (!bSuccess)
			break;
			i++;
	}*/

	//Read sorted random numbers from Sort Child Process
	i = 0;
	bSuccess = FALSE;
	DWORD numberOfBytesToRead=(DWORD)strlen(numbers), numberOfBytesRead=0;
       	CHAR data;
	/*while (i < nWorkers)
	{
		bSuccess = ReadFile(sortChildOutRead, data, numberOfBytesToRead, &numberOfBytesRead, NULL);
		if (!bSuccess || numberOfBytesRead == 0) 
			break;
		randomNumbers[i] = data;
		i++;
	}*/

	/////////////////////////////  TESTING
	CHAR buffer[80];
	for(;;)
	{
		bSuccess = ReadFile(sortChildOutRead, buffer, numberOfBytesToRead, &numberOfBytesRead, NULL);
		//if(!bSuccess) break;
		i++;
		printf_s("i: %d\n",i);
		break;
	}
	printf_s("i: %d\n",i);
	//for(j=0;j<3;j++)
		printf_s("Are they  sorted: %s\n",buffer);
	getchar();
	exit(10);
	/////////////////////////////  TESTING

	CloseHandle(sortChildInRead);
	CloseHandle(sortChildInWrite);
	CloseHandle(sortChildOutRead);
	CloseHandle(sortChildOutWrite);
	/*printf_s("Hopefully sorted numbers\n");
	for(i=0;i<nWorkers;i++)
		printf_s("%d\n",randomNumbers[i]);*/
	/*free(sortChildInRead);
	free(sortChildInWrite);
	free(sortChildOutRead);
	free(sortChildOutWrite);*/

	return;
}

/*void createSortChildProcess(HANDLE *sortChildInRead,HANDLE *sortChildOutWrite)
{
	LPTSTR executable = "SORT";
	DWORD dwCreationFlags = 0;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startUpSearch;

	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&startUpSearch, sizeof(STARTUPINFO));
	startUpSearch.cb = sizeof(STARTUPINFO);
	startUpSearch.hStdError = *sortChildOutWrite;
	startUpSearch.hStdOutput = *sortChildOutWrite;
	startUpSearch.hStdInput = *sortChildInRead;
	startUpSearch.dwFlags |= STARTF_USESTDHANDLES;
	getchar();
	BOOL bSuccess = CreateProcess(
		NULL,
		"sort",
		NULL,
		NULL,
		TRUE,
		dwCreationFlags,
		NULL,
		NULL,
		&startUpSearch,
		&processInfo);

	// If an error occurs, exit the application. 
	if (!bSuccess)
	{
		printf_s("Create Process failed\n");
		exit(7);
	}
	else
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	return;
}*/

BOOL isPrime(DWORD pNumber)
{
	DWORD flag = 0, i;
	for (i = 2;i <= pNumber / 2;i++)
	{
		if (pNumber % i == 0)
		{
			flag = 1;
			break;
		}
	}
	if (flag == 0)
		return TRUE;
	else
		return FALSE;
}




/*if (buffer == object->nBuffers - 1)
{
	StringCchPrintf(msgBuf, BUF_SIZE, TEXT("For Worker %d, Buffer %d temp: %d   tempWrite: %d\n"),
		object->workerID, buffer + 1, temp, tempWrite);
	StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
	WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
}*/
