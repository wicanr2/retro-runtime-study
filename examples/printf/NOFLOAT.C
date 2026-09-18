/* 沒有任何浮點運算，卻用 %f 印一個從檔案讀進來的 double。
   用來看「浮點格式沒有連結」的錯誤訊息什麼時候出現。 */
#include <stdio.h>

int main(void)
{
    double d;
    FILE *f = fopen("VAL.BIN", "rb");
    char buf[64];

    if (!f) return 1;
    fread(&d, sizeof d, 1, f);
    fclose(f);
    sprintf(buf, "%f", d);
    f = fopen("OUT.TXT", "wb");
    fprintf(f, "noflt |%s|\n", buf);
    fclose(f);
    return 0;
}
