/* rand 家族的序列與巨集行為。輸出 RAND.TXT，與 expected-rand.txt 比對。 */
#include <stdio.h>
#include <stdlib.h>

static void seq6(FILE *f, const char *label, int do_srand, unsigned seed)
{
    int i, v[6];
    if (do_srand)
        srand(seed);
    for (i = 0; i < 6; i++)
        v[i] = rand();
    fprintf(f, "%s", label);
    for (i = 0; i < 6; i++)
        fprintf(f, " %d", v[i]);
    fprintf(f, "\n");
}

int main(void)
{
    FILE *f = fopen("RAND.TXT", "w");
    int i, r10[10], r100[10], again[6];
    unsigned a1, a2;

    seq6(f, "default(Seed=1):", 0, 0);
    seq6(f, "srand(0):", 1, 0);
    seq6(f, "srand(1):", 1, 1);
    seq6(f, "srand(2):", 1, 2);
    seq6(f, "srand(32767):", 1, 32767);
    seq6(f, "srand(65535):", 1, 65535);
    fprintf(f, "RAND_MAX=%d\n", (int)RAND_MAX);

    srand(1);
    for (i = 0; i < 10; i++)
        r10[i] = random(10);
    fprintf(f, "random(10):");
    for (i = 0; i < 10; i++)
        fprintf(f, " %d", r10[i]);
    fprintf(f, "\n");

    srand(1);
    for (i = 0; i < 10; i++)
        r100[i] = random(100);
    fprintf(f, "random(100):");
    for (i = 0; i < 10; i++)
        fprintf(f, " %d", r100[i]);
    fprintf(f, "\n");

    srand(1);
    a1 = (unsigned)rand();
    srand(1);
    for (i = 0; i < 6; i++)
        again[i] = rand();
    fprintf(f, "reseed-gives-same-first=%d\n", a1 == (unsigned)again[0]);

    srand(123);
    a1 = (unsigned)rand();
    srand(123);
    a2 = (unsigned)rand();
    fprintf(f, "srand(123)-repeatable=%d\n", a1 == a2);

    fclose(f);
    return 0;
}
