/* 讀鐘的那一側：getdate/gettime/time/clock/ftime 與檔案時間戳。
   時間會隨執行推進（dosgolem 的決定性時鐘），所以輸出用 PASS 形式：
   固定值直接印，會動的值印界內檢查。輸出 DAY.TXT。 */
#include <stdio.h>
#include <time.h>
#include <dos.h>
#include <io.h>
#include <sys\timeb.h>

static FILE *f;

static int in_window(long t, long base)
{
    return t >= base && t <= base + 3600L;
}

int main(void)
{
    struct date d, d2;
    struct time t1, t2;
    struct timeb tb;
    struct ftime ft, ft2;
    time_t K, t0;
    clock_t c1, c2;
    volatile long spin;
    int i, fd, ok;
    long a, b;

    f = fopen("DAY.TXT", "w");

    d.da_year = 1993; d.da_mon = 1; d.da_day = 1;
    t1.ti_hour = 0; t1.ti_min = 0; t1.ti_sec = 0; t1.ti_hund = 0;
    K = dostounix(&d, &t1);            /* dosgolem tick=0 的理論 time() */

    getdate(&d);
    fprintf(f, "getdate: %d-%d-%d\n", d.da_year, d.da_mon, d.da_day);

    t0 = time(NULL);
    fprintf(f, "time-in-window: %d\n", in_window((long)t0, (long)K));

    gettime(&t1);
    for (i = 0; i < 3; i++)
        for (spin = 0; spin < 20000L; spin++)
            ;
    gettime(&t2);
    a = ((long)t1.ti_hour * 3600 + t1.ti_min * 60 + t1.ti_sec) * 100 + t1.ti_hund;
    b = ((long)t2.ti_hour * 3600 + t2.ti_min * 60 + t2.ti_sec) * 100 + t2.ti_hund;
    fprintf(f, "gettime-monotonic: %d\n", b >= a && b - a < 60000L);

    c1 = clock();
    for (i = 0; i < 5; i++)
        for (spin = 0; spin < 20000L; spin++)
            ;
    c2 = clock();
    fprintf(f, "clock: %d CLK_TCK10=%d\n",
            c1 >= 0 && c2 >= c1 && c2 <= 1000000L,
            (int)(CLK_TCK * 10.0 + 0.5));

    ftime(&tb);
    fprintf(f, "ftime: tz=%d dst=%d millitm<1000=%d time-in-window=%d\n",
            (int)tb.timezone, (int)tb.dstflag, tb.millitm < 1000,
            in_window((long)tb.time, (long)K));

    fd = creat("STAMP.TMP", 0);
    ft.ft_year = 13; ft.ft_month = 4; ft.ft_day = 1;      /* 1993-04-01 */
    ft.ft_hour = 12; ft.ft_min = 34; ft.ft_tsec = 28;     /* 12:34:56 */
    /* dosgolem 的 AH=57h「設定」不落地（素材唯讀）、「讀取」回宿主檔案的
       mtime，所以 roundtrip 只驗兩個服務都成功、欄位落在合法範圍。 */
    ok = setftime(fd, &ft) == 0 && getftime(fd, &ft2) == 0 &&
         ft2.ft_year <= 127 && ft2.ft_month >= 1 && ft2.ft_month <= 12 &&
         ft2.ft_day >= 1 && ft2.ft_day <= 31 && ft2.ft_hour <= 23 &&
         ft2.ft_min <= 59 && ft2.ft_tsec <= 29;
    fprintf(f, "setgetftime-ok-valid: %d\n", ok);
    close(fd);

    d.da_year = 1999; d.da_mon = 12; d.da_day = 31;
    t1.ti_hour = 23; t1.ti_min = 59; t1.ti_sec = 59; t1.ti_hund = 99;
    setdate(&d);
    settime(&t1);
    getdate(&d2);
    fprintf(f, "setdate-readback: %d-%d-%d\n", d2.da_year, d2.da_mon, d2.da_day);

    fclose(f);
    return 0;
}
