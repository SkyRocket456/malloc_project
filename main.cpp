//
// Created by skyrocket on 5/20/26.
//

#include <cstdio>

#include "my_malloc.h"

int main () {
    void* p = my_malloc(1);
    void* g = my_malloc(1);
    return 0;
}