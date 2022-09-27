#include "types.h"
#include "user.h"
#include "stat.h"

#pragma GCC push_options
#pragma GCC optimize("O0")

int
factorial(int n)
{
    if(n < 0)
        return 0;
    else if(n == 0)
        return 1;
    else
        return n * factorial(n - 1);
}

int
main(int argc, char *argv[])
{
    if(argc != 2)
        exit();
    int n;
    n = atoi(argv[1]);
    factorial(n);
    exit();
}