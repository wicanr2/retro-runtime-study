/* mktime 的閏年 2 月 29：預期無限迴圈（RTL 原始碼的迴圈對 day==28 不動狀態）。
   對照組先印出來並關檔，然後才踩雷。run.sh 用步數上限偵測「沒有結束」。 */
#include <stdio.h>
#include <time.h>

int main(void)
{
    FILE *f = fopen("FEB.TXT", "w");
    struct tm t;

    t.tm_year = 93; t.tm_mon = 1; t.tm_mday = 29;   /* 1993 非閏年：正規化成 3/1 */
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    t.tm_isdst = 0; t.tm_wday = 0; t.tm_yday = 0;
    fprintf(f, "mk-1993-02-29-nonleap: %ld\n", mktime(&t));

    t.tm_year = 92; t.tm_mon = 1; t.tm_mday = 30;   /* 1992 閏年 2/30：正規化成 3/1 */
    fprintf(f, "mk-1992-02-30-leap: %ld\n", mktime(&t));

    t.tm_year = 92; t.tm_mon = 1; t.tm_mday = 28;   /* 1992-02-28：正常 */
    fprintf(f, "mk-1992-02-28: %ld\n", mktime(&t));

    fprintf(f, "calling mktime(1992-02-29)...\n");
    fclose(f);

    t.tm_year = 92; t.tm_mon = 1; t.tm_mday = 29;   /* 合法日期，預期不回來 */
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    t.tm_isdst = 0; t.tm_wday = 0; t.tm_yday = 0;
    mktime(&t);

    f = fopen("FEB.TXT", "a");                       /* 走到這裡代表沒有掛 */
    fprintf(f, "returned?! %ld\n", mktime(&t));
    fclose(f);
    return 0;
}
