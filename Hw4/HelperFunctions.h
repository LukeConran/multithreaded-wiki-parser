#pragma once

#include "pch.h"
#include "MyBuf.h"

#define MIN_WORD_LEN 3
#define MAX_WORD_LEN 31
#define EOB -1

namespace HelperFunctions {
    extern unsigned char isalphaLUT[256];
    extern unsigned char isdelimiterLUT[256];

    void InitLUTs();
    int FindNextWordStart(MyBuf* mb, int startPos, int* wordStart);
    int FindThisWordEnd(MyBuf* mb, int wordStart, int* wordEnd);
    bool WordIsEligible(MyBuf* mb, int wordStart, int wordEnd, int wordLen);
}