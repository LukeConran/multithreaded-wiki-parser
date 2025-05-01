#include "pch.h"
#include "BufferManager.h"
#include "DiskThread.h"
#include "ProducerConsumer.h"
#include "MyBuf.h"
#include "HelperFunctions.h"
#include "SearchThread.h"
#include <time.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <buffer size power> <wiki text>\n", argv[0]);
        return 1;
    }

    int powerOfTwo = atoi(argv[1]);
    const char* wikipediaFilename = argv[2];
    int numSlots = 24; // Number of buffer slots to use (can be adjusted)
    int maxKeywordLength = MAX_WORD_LEN;
    BOOL nonBufferedIO = FALSE; // Use buffered I/O by default

    HelperFunctions::InitLUTs();

    HANDLE hFile = CreateFileA(
        wikipediaFilename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    UINT64 fileSize = 0;
    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSizeLarge;
        if (GetFileSizeEx(hFile, &fileSizeLarge)) {
            fileSize = fileSizeLarge.QuadPart;
        }
        CloseHandle(hFile);
    }
    else {
        printf("Error: Cannot open file %s\n", wikipediaFilename);
        return 1;
    }

    printf("File size: %llu bytes\n", fileSize);
    printf("Buffer size: %d bytes (2^%d)\n", 1 << powerOfTwo, powerOfTwo);

    BufferManager bufferManager(powerOfTwo, numSlots, nonBufferedIO, maxKeywordLength);
    if (!bufferManager.GetSlot(0)) {
        printf("Error: Failed to allocate buffer memory\n");
        return 1;
    }

    HANDLE eventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
    ProducerConsumer pcEmpty(eventQuit, numSlots, sizeof(int));
    ProducerConsumer pcFull(eventQuit, numSlots, sizeof(MyBuf));
    for (int i = 0; i < numSlots; i++) {
        pcEmpty.Push(&i);
    }

    LONG64 bytesProcessed = 0;
    LONG64 invalidWords = 0;
    LONG64 totalWords = 0;

    DiskThreadContext diskCtx = {
        wikipediaFilename,
        &pcEmpty,
        &pcFull,
        &bufferManager,
        eventQuit,
        nonBufferedIO,
        maxKeywordLength,
        fileSize
    };

    HANDLE hDiskThread = CreateThread(NULL, 0, DiskReadThread, &diskCtx, 0, NULL);
    if (!hDiskThread) {
        printf("Error creating disk thread\n");
        CloseHandle(eventQuit);
        return 1;
    }

    LARGE_INTEGER startTime, endTime, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);

    const int MAX_SEARCH_THREADS = 1;
    HANDLE searchThreads[MAX_SEARCH_THREADS];
    int numSearchThreads = 0;
    BOOL done = FALSE;

    SearchThreadContext searchCtx = {
        &pcFull,
        &pcEmpty,
        &bytesProcessed,
        &invalidWords, 
        &totalWords 
    };

    searchThreads[numSearchThreads] = CreateThread(NULL, 0, SearchThread, &searchCtx, 0, NULL);
    if (!searchThreads[numSearchThreads]) {
        printf("Error creating search thread\n");
    } else {
        numSearchThreads++;  // Increment counter after successful creation
    }


    printf("Waiting for %d search threads to complete...\n", numSearchThreads);
    WaitForMultipleObjects(numSearchThreads, searchThreads, TRUE, INFINITE);
    for (int i = 0; i < numSearchThreads; i++) {
        CloseHandle(searchThreads[i]);
    }

    WaitForSingleObject(hDiskThread, INFINITE);
    CloseHandle(hDiskThread);
    CloseHandle(eventQuit);

    QueryPerformanceCounter(&endTime);
    double executionTime = (double)(endTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;

    printf("\n");
    printf("File size: %llu bytes\n", fileSize);
    printf("Bytes processed: %lld\n", bytesProcessed);
    printf("\n");
    printf("Invalid words: %lld \n", invalidWords);
    printf("Total words found: %lld\n", totalWords);
    printf("\n");
    printf("Execution time: %.2f seconds\n", executionTime);
    printf("Processing speed: %.2f MB/sec\n", fileSize / (1024.0 * 1024.0) / executionTime);
    printf("------------------------------------\n");

    return 0;
}