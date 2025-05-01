#include "pch.h"
#include "HelperFunctions.h"

namespace HelperFunctions {
    unsigned char isalphaLUT[256];
    unsigned char isdelimiterLUT[256];

    void InitLUTs() {
        memset(isalphaLUT, 0, sizeof(isalphaLUT));
        for (int i = 'a'; i <= 'z'; i++) isalphaLUT[i] = 1;
        for (int i = 'A'; i <= 'Z'; i++) isalphaLUT[i] = 1;

        memset(isdelimiterLUT, 0, sizeof(isdelimiterLUT));
        isdelimiterLUT[0] = 1;
        isdelimiterLUT[' '] = 1;
        isdelimiterLUT[','] = 1;
        isdelimiterLUT['\n'] = 1;
        isdelimiterLUT['\r'] = 1;
        isdelimiterLUT['.'] = 1;
        isdelimiterLUT['\''] = 1;
        isdelimiterLUT['"'] = 1;
        isdelimiterLUT['?'] = 1;
        isdelimiterLUT['-'] = 1;
        isdelimiterLUT[':'] = 1;
        isdelimiterLUT[';'] = 1;
        isdelimiterLUT['*'] = 1;
        isdelimiterLUT['!'] = 1;
        isdelimiterLUT['\t'] = 1;
    }

    int FindNextWordStart(MyBuf* mb, int startPos, int* wordStart) {
        if (startPos >= mb->lastStart) {
            return EOB;
        }

        char* ptr = mb->ptr + startPos;
        int pos = startPos;

        while (pos < mb->lastStart) {
            if (isalphaLUT[(unsigned char)*ptr]) {
                *wordStart = pos;
                return 0;
            }
            ptr++;
            pos++;
        }

        return EOB;
    }

    int FindThisWordEnd(MyBuf* mb, int wordStart, int* wordEnd) {
        if (wordStart >= mb->size) {
            return EOB;
        }

        char* ptr = mb->ptr + wordStart;
        int pos = wordStart;

        while (pos < mb->size) {
            if (!isalphaLUT[(unsigned char)*ptr]) {
                *wordEnd = pos;
                return 0;
            }
            ptr++;
            pos++;
        }

        return EOB;
    }

    bool WordIsEligible(MyBuf* mb, int wordStart, int wordEnd, int wordLen) {
        if (wordLen < MIN_WORD_LEN || wordLen > MAX_WORD_LEN) {
            return false;
        }
    
        if (wordStart > 0) {
            if (!isdelimiterLUT[(unsigned char)mb->ptr[wordStart - 1]]) {
                return false;
            }
        } else if (!mb->first) {
            if (!isdelimiterLUT[(unsigned char)mb->ptr[-1]]) {
                return false;
            }
        }

        if (wordEnd < mb->size) {
            if (!isdelimiterLUT[(unsigned char)mb->ptr[wordEnd]]) {
                return false;
            }
        } else {
            return false;
        }
    
        return true;
    }
}