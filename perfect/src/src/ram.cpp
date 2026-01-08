// credits go to
// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

#include <iostream>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

size_t parseLine(char*);
size_t parseLine(char* line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    size_t i = std::strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

size_t getCurrentRAM();
size_t getCurrentRAM() { //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    size_t result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

