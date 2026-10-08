/* 把 BC++ 2.0 的 strlen / _fstrlen 邊界行為跑出來，結果寫到 OUT.TXT。
   自寫測試程式（非原廠碼）。說明見 docs/20-msvc-crt/strlen-anatomy.md。
   MSVC 那邊的對應行為（far 飽和回 65535）在此工具鏈跑不出來，
   見文章的證據節；這裡驗 Borland 側：一般、空、未對齊、
   far NULL 衛兵、64K 無結束符（R83 的 allocmem 手法）。 */
#include <stdio.h>
#include <string.h>
#include <dos.h>

/* 照行為規格重寫的參照實作（自寫，非原廠碼） */
static unsigned ref_len(const char far *s)
{
    unsigned n = 0;
    while (s[n] != 0)
        n++;
    return n;
}

int main(void)
{
    FILE *out = fopen("OUT.TXT", "w");
    static char buf[512];
    char far *big;
    unsigned seg;
    unsigned long i;
    int k;

    for (k = 0; k < 300; k++)
        buf[k] = 'A' + (k % 26);
    buf[300] = 0;

    fprintf(out, "empty %u\n", strlen(""));
    fprintf(out, "short %u\n", strlen("hello"));
    fprintf(out, "long %u\n", strlen(buf));
    fprintf(out, "unalign %u\n", strlen(buf + 1));
    fprintf(out, "refmatch %d\n",
        ref_len(buf) == strlen(buf) &&
        ref_len(buf + 1) == strlen(buf + 1));

    /* far：一般、NULL 衛兵 */
    fprintf(out, "fshort %u\n", _fstrlen("hello"));
    fprintf(out, "fnull %u\n", _fstrlen((char far *)0));

    /* far 飽和：直配整段 64K 全填無 NUL，靜讀預測回 65534 */
    if (allocmem(0x1000, &seg) != -1) {  /* Borland 成功回 -1，非 0 */
        fprintf(out, "sat alloc-fail\n");
    } else {
        big = (char far *)((unsigned long)seg << 16);
        for (i = 0; i < 65536UL; i++)
            big[i] = 'A';
        fprintf(out, "sat %u\n", _fstrlen(big));
        big[65534] = 0;
        fprintf(out, "edge %u\n", _fstrlen(big));
        freemem(seg);
    }

    fclose(out);
    return 0;
}
