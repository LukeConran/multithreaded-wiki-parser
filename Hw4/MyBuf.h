#pragma once

#include "pch.h"

struct MyBuf {
    char* ptr;          // Pointer to start of the search area
    int size;           // Size of the area to search
    int slotID;         // ID of the slot to return to the pool after searching
    int lastStart;      // Position of the last possible word start in the buffer
    bool first;         // Whether this is the first buffer (from disk.cpp)

    MyBuf() : ptr(nullptr), size(0), slotID(0), lastStart(0), first(false) {}
};