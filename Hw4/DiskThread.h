#pragma once

#include "pch.h"
#include "BufferManager.h"
#include "ProducerConsumer.h"
#include "HelperFunctions.h"
#include "MyBuf.h"

struct DiskThreadContext {
    const char* filename;
    ProducerConsumer* pcEmpty;
    ProducerConsumer* pcFull;
    BufferManager* bufferManager;
    HANDLE eventQuit;
    BOOL nonBufferedIO;
    int maxKeywordLength;
    UINT64 fileSize;
};

DWORD WINAPI DiskReadThread(LPVOID lpParam) {
    DiskThreadContext* ctx = (DiskThreadContext*)lpParam;

    // Open the file
    HANDLE hFile = CreateFileA(
        ctx->filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        ctx->nonBufferedIO ? FILE_FLAG_NO_BUFFERING : FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file: %lu\n", GetLastError());
        SetEvent(ctx->eventQuit);
        return 1;
    }

    int shadowSize = ctx->bufferManager->GetShadowSize();
    int dataSize = ctx->bufferManager->GetDataSize();
    int shadowBufferSize = MAX_WORD_LEN + 1;
    char* shadowBuffer = new char[shadowBufferSize];
    bool first = true;
    UINT64 bytesRead = 0;

    while (true) {
        int slotID;
        if (ctx->pcEmpty->Pop(&slotID) == QUIT) {
            break;
        }

        char* buffer = ctx->bufferManager->GetSlot(slotID);

        if (!first) {
            char* dest = ctx->bufferManager->GetShadowBuffer(slotID);
            memcpy(dest, shadowBuffer, shadowBufferSize);
        }

        DWORD readBytes;
        if (ReadFile(hFile, buffer, dataSize, &readBytes, NULL) == FALSE) {
            printf("Error reading file: %lu\n", GetLastError());
            ctx->pcEmpty->Push(&slotID);
            break;
        }

        bytesRead += readBytes;
        buffer[readBytes] = '\0';

        if (readBytes >= shadowBufferSize) {
            memcpy(shadowBuffer, buffer + readBytes - shadowBufferSize, shadowBufferSize);
        }
        else if (readBytes > 0) {
            memcpy(shadowBuffer, buffer, readBytes);
        }

        MyBuf mb;
        mb.slotID = slotID;

        if (first) {
            // First buffer
            mb.ptr = buffer;
            mb.lastStart = dataSize - shadowBufferSize;
            mb.size = readBytes;
            mb.first = true;
            first = false;
        }
        else if (readBytes == 0) {
            ctx->pcEmpty->Push(&slotID);
            break;
        }
        else if (bytesRead == ctx->fileSize) {
            // Last buffer
            mb.ptr = buffer - shadowBufferSize;
            mb.lastStart = readBytes + shadowBufferSize;
            mb.size = readBytes + shadowBufferSize;
            mb.first = false;
        }
        else {
            // Middle buffers
            mb.ptr = buffer - shadowBufferSize; 
            mb.lastStart = dataSize;
            mb.size = readBytes + shadowBufferSize;
            mb.first = false;
        }

        ctx->pcFull->Push(&mb);

        if (readBytes == 0) {
            break;  // End of file
        }
    }

    delete[] shadowBuffer;
    CloseHandle(hFile);
    SetEvent(ctx->eventQuit);
    return 0;
}