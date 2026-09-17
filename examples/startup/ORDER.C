/* 記錄 #pragma startup、#pragma exit、atexit 與 main 的執行順序，寫到 ORDER.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/startup-and-exit.md。 */
#include <stdio.h>
#include <stdlib.h>

static char log[64];
static int n;
static void mark(char c) { log[n++] = c; log[n] = 0; }

void s100a(void) { mark('a'); }
void s100b(void) { mark('b'); }
void s70(void)   { mark('c'); }
void e100a(void) { mark('A'); }
void e100b(void) { mark('B'); }
void e70(void)   { mark('C'); }
void ax1(void)   { mark('1'); }
void ax2(void)   { mark('2'); }
void dump(void)
{
    FILE *f = fopen("ORDER.TXT", "w");
    fprintf(f, "%s\n", log);
    fclose(f);
}

#pragma startup s100a 100
#pragma startup s100b 100
#pragma startup s70 70
#pragma exit e100a 100
#pragma exit e100b 100
#pragma exit e70 70
#pragma exit dump 64

int main(void)
{
    mark('M');
    atexit(ax1);
    atexit(ax2);
    return 0;
}
