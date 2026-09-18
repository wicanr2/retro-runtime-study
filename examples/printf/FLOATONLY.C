/* 有浮點運算、用 %f，但完全不碰 scanf 家族。
   用來看備用模組 CVTFAK 是不是被 scanf 那條路拉進來的。 */
#include <stdio.h>

int main(void)
{
    double d = 1.0 / 3.0;
    char buf[64];
    FILE *f;

    sprintf(buf, "%f %.2e", d, d * 1000.0);
    f = fopen("OUT.TXT", "wb");
    fprintf(f, "floatonly |%s|\n", buf);
    fclose(f);
    return 0;
}
