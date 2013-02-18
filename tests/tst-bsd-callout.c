#include <stdio.h>
#include <unistd.h>
#include <bsd/porting/callout.h>

struct callout c1, c2;
int ctr;

void aaa(void *unused)
{
    ctr++;
    printf("TICK %d\n", ctr);
    callout_reset(&c1, hz, aaa, NULL);
}

void bbb(void *unused)
{
    // Stop aaa
    printf("SHUT-UP\n");
    _callout_stop_safe(&c1, 1);
}

void test1(void)
{
    printf("BSD Callout Test\n");

    ctr = 0;

    callout_init(&c1, 1);
    callout_reset(&c1, hz, aaa, NULL);

    callout_init(&c2, 1);
    callout_reset(&c2, 10.1*hz, bbb, NULL);

    sleep(11);
    printf("BSD Callout Test Done\n");
}

int main(int argc, char **argv)
{
    test1();
    return 0;
}
