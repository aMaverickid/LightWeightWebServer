
#ifndef DEF_H
#define DEF_H
#include <stdio.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define DEEPGREEN "\033[36m"
#define CLEAR "\033[0m"

#define Log(format, ...) \
    printf("\33[1;32m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Err(format, ...) {                              \
    printf("\33[1;31m[%s,%d,%s] " format "\33[0m\n",    \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__);  \
    while(1);                                           \
}        

#define SOCKPORT 2682
#define MAX_CONNENCTION 100
#define MAX_BUFFER 4096
#define BUFFER_SIZE 1024
#endif