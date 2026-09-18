/* 整支程式不出現 double 或 float 這兩個型別：8 個位元組用兩個 long 推上堆疊，
   格式字串也在執行時才組出來，編譯器沒有理由留下任何浮點修正符號。
   用來看「連一個浮點轉換模組都沒連進來」時會發生什麼事。 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    char fmt[8];
    char buf[64];
    long lo, hi;
    FILE *f;

    /* 1.5 的 IEEE 雙精度位元樣式，拆成兩個 long 手動擺好 */
    lo = 0L;
    hi = 0x3FF80000L;

    strcpy(fmt, "%");
    strcat(fmt, "f");
    sprintf(buf, fmt, lo, hi);

    f = fopen("OUT.TXT", "wb");
    fprintf(f, "nocvt |%s|\n", buf);
    fclose(f);
    return 0;
}
