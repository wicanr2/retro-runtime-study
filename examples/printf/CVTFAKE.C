/* 用了 scanf 家族（把備用模組拉進來），但整支程式不出現 double 或 float，
   所以真正的浮點轉換模組沒有被連結。這時候 %f 走到的就是備用模組填的樁。 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    char fmt[8];
    char buf[64];
    long lo, hi;
    int n, v;
    FILE *f;

    v = 0;
    n = sscanf("42", "%d", &v);

    f = fopen("OUT.TXT", "wb");
    fprintf(f, "scan %d %d\n", n, v);
    fclose(f);

    lo = 0L;
    hi = 0x3FF80000L;
    strcpy(fmt, "%");
    strcat(fmt, "f");
    sprintf(buf, fmt, lo, hi);   /* 這一行預期走到樁 */

    f = fopen("OUT.TXT", "ab");
    fprintf(f, "after |%s|\n", buf);
    fclose(f);
    return 0;
}
