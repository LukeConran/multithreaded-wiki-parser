// Luke Conran
// Class of 2027
// Spring 2025


#pragma once

#include "pch.h"

enum PCStatus {
    SUCCESS = 0,
    QUIT = 1
};

class ProducerConsumer {
private:
    HANDLE semaFullSlots;
    HANDLE semaEmptySlots;
    CRITICAL_SECTION critSection;
    HANDLE eventQuit;

    char* buffer;
    int head;
    int tail;
    int capacity;              
    int itemSize;              
    int count;                 

public:
    ProducerConsumer(HANDLE quitEvent, int maxItems, int sizeOfItem) {
        eventQuit = quitEvent;

        InitializeCriticalSection(&critSection);
        semaFullSlots = CreateSemaphore(NULL, 0, maxItems, NULL);
        semaEmptySlots = CreateSemaphore(NULL, maxItems, maxItems, NULL);

        capacity = maxItems;
        itemSize = sizeOfItem;
        head = 0;
        tail = 0;
        count = 0;

        buffer = (char*)VirtualAlloc(NULL, capacity * itemSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

    ~ProducerConsumer() {
        DeleteCriticalSection(&critSection);
        CloseHandle(semaFullSlots);
        CloseHandle(semaEmptySlots);

        if (buffer) {
            VirtualFree(buffer, 0, MEM_RELEASE);
        }
    }

    void Push(const void* item) {
        WaitForSingleObject(semaEmptySlots, INFINITE);
        EnterCriticalSection(&critSection);

        char* dest = buffer + (tail * itemSize);
        memcpy(dest, item, itemSize);

        tail = (tail + 1) % capacity;
        count++;

        LeaveCriticalSection(&critSection);
        ReleaseSemaphore(semaFullSlots, 1, NULL);
    }

    PCStatus Pop(void* item) {
        HANDLE waitHandles[2] = { semaFullSlots, eventQuit };
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0 + 1) {
            return QUIT;
        }

        EnterCriticalSection(&critSection);

        char* src = buffer + (head * itemSize);
        memcpy(item, src, itemSize);

        head = (head + 1) % capacity;
        count--;

        LeaveCriticalSection(&critSection);
        ReleaseSemaphore(semaEmptySlots, 1, NULL);

        return SUCCESS;
    }
};