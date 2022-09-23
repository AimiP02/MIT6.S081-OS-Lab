#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
    char buf[3000];

    printf(1, "sys_call wolfie, return: %d\n", wolfie((void*)buf, 3000));
    printf(1,"%s", buf);

    exit();
}