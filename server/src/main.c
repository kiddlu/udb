#include <unistd.h>

#include "net_debugd.h"
#include "rlog.h"

int main(int argc, void **argv)
{
    rlog_init();

    printf("HI, Build time: " __DATE__ " " __TIME__ "\n");

    net_debugd_entry();

    while (1)
    {
        sleep(1);
    }
}