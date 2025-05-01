#pragma once

#include "pch.h"
#include "MyBuf.h"
#include "HelperFunctions.h"
#include "ProducerConsumer.h"

struct SearchThreadContext {
    ProducerConsumer* pcFull;  // Producer-consumer queue for full slots
    ProducerConsumer* pcEmpty; // Producer-consumer queue for empty slots
    LONG64* bytesProcessed;    // Counter for bytes processed
    LONG64* invalidWords;      // Counter for invalid words
    LONG64* totalCount;        // Counter for total words
};

DWORD WINAPI SearchThread(LPVOID lpParam) {
    SearchThreadContext* ctx = (SearchThreadContext*)lpParam;
    MyBuf mb; //LOOK INTO THIS
    
    int off = 0;
    int wordStart = 0;
    int wordEnd;
    uint64_t hashKey = 0;

    while (true) {
        LONG64 localInvalidWords = 0;
        LONG64 localTotalWords = 0; 
        if(ctx->pcFull->Pop(&mb) == QUIT) { //AND THIS
            break;
        }

        if (!mb.first) {
            if (HelperFunctions::FindThisWordEnd(&mb, 0, &wordEnd) == 0) { //dont have hash key yet
                off = wordEnd + 1;
            }
            else {
                off = 1;
            }
        } else {
            off = 0;
        }

        while (off < mb.lastStart) {
            if (HelperFunctions::FindNextWordStart(&mb, off, &wordStart) == EOB) {
                break;
            }

            if (HelperFunctions::FindThisWordEnd(&mb, wordStart, &wordEnd) == EOB) { //dont have hash key yet
                localInvalidWords++;
                localTotalWords++;
                break;
            }

            int wordLen = wordEnd - wordStart;

            if (HelperFunctions::WordIsEligible(&mb, wordStart, wordEnd, wordLen)) {
                //
            }
            else {
                localInvalidWords++;
            }

            localTotalWords++;
            off = wordEnd + 1;
        }

        if (ctx->invalidWords) {
            InterlockedAdd64(ctx->invalidWords, localInvalidWords);
        }
        if (ctx->totalCount) {
            InterlockedAdd64(ctx->totalCount, localTotalWords);
        }
        if (ctx->bytesProcessed) {
            InterlockedAdd64(ctx->bytesProcessed, mb.lastStart);
        }

        ctx->pcEmpty->Push(&(mb.slotID));

    }

    return 0;
}