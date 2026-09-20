/* time 家族：TZ 剖析、換日、mktime 邊界與 strftime。輸出 TIME.TXT。
   全部用固定輸入，輸出與記憶體模型無關。閏年 2 月 29 的 case 在 MKFEB29.C（它會掛）。 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dos.h>

static struct tm *tmk, tcopy;

static void show_tz(FILE *f, const char *label)
{
    fprintf(f, "%s: timezone=%ld daylight=%d tz=[%s][%s]\n",
            label, timezone, daylight, tzname[0], tzname[1]);
}

static void show_d2u(FILE *f, const char *label, int y, int mo, int d,
                     int h, int mi, int s)
{
    struct date dd;
    struct time tt;
    dd.da_year = (short)y; dd.da_mon = (char)mo; dd.da_day = (char)d;
    tt.ti_hour = (unsigned char)h; tt.ti_min = (unsigned char)mi;
    tt.ti_sec = (unsigned char)s; tt.ti_hund = 0;
    fprintf(f, "%s: %ld\n", label, dostounix(&dd, &tt));
}

static long mk(FILE *f, const char *label, int yr, int mo, int mday,
               int h, int mi, int s)
{
    struct tm t;
    long r;
    t.tm_year = yr; t.tm_mon = mo; t.tm_mday = mday;
    t.tm_hour = h; t.tm_min = mi; t.tm_sec = s;
    t.tm_isdst = 0; t.tm_wday = 0; t.tm_yday = 0;
    r = mktime(&t);
    if (r == -1)
        fprintf(f, "%s: -1\n", label);
    else
        fprintf(f, "%s: %ld (norm %04d-%02d-%02d %02d:%02d:%02d wday=%d yday=%d dst=%d)\n",
                label, r, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                t.tm_hour, t.tm_min, t.tm_sec, t.tm_wday, t.tm_yday, t.tm_isdst);
    return r;
}

int main(void)
{
    FILE *f = fopen("TIME.TXT", "w");
    time_t K, J, t;
    struct date dd;
    struct time tt;

    show_d2u(f, "d2u-1980-01-01", 1980, 1, 1, 0, 0, 0);
    show_d2u(f, "d2u-1980-02-29", 1980, 2, 29, 0, 0, 0);
    show_d2u(f, "d2u-1993-01-01", 1993, 1, 1, 0, 0, 0);
    show_d2u(f, "d2u-1993-07-01-12:00", 1993, 7, 1, 12, 0, 0);
    K = 725864400L;   /* d2u-1993-01-01 的值，下面重印一次以便對照 */
    J = 741542400L;   /* d2u-1993-07-01 12:00 的值 */

    show_tz(f, "tz-default");

    putenv("TZ=EST5EDT");  tzset(); show_tz(f, "tz-EST5EDT");
    putenv("TZ=GMT0");     tzset(); show_tz(f, "tz-GMT0");
    putenv("TZ=CCT-8");    tzset(); show_tz(f, "tz-CCT-8");
    putenv("TZ=XYZ5DST");  tzset(); show_tz(f, "tz-XYZ5DST");
    putenv("TZ=AB");       tzset(); show_tz(f, "tz-AB-bogus");
    putenv("TZ=ABCD5");    tzset(); show_tz(f, "tz-ABCD5-bogus");
    putenv("TZ=EST5EDT");  tzset();   /* 還原，以下轉換都用預設時區 */

    fprintf(f, "ctime(K)=%s", ctime(&K));
    fprintf(f, "ctime(J)=%s", ctime(&J));
    tmk = gmtime(&K);
    fprintf(f, "gmtime(K)=%04d-%02d-%02d %02d:%02d:%02d wday=%d\n",
            tmk->tm_year + 1900, tmk->tm_mon + 1, tmk->tm_mday,
            tmk->tm_hour, tmk->tm_min, tmk->tm_sec, tmk->tm_wday);
    tmk = localtime(&J);
    tcopy = *tmk;      /* localtime 的 tm 是共用的，先抄走 */
    fprintf(f, "localtime(J)=%04d-%02d-%02d %02d:%02d:%02d wday=%d yday=%d dst=%d\n",
            tcopy.tm_year + 1900, tcopy.tm_mon + 1, tcopy.tm_mday,
            tcopy.tm_hour, tcopy.tm_min, tcopy.tm_sec,
            tcopy.tm_wday, tcopy.tm_yday, tcopy.tm_isdst);

    {
        char buf[80];
        strftime(buf, sizeof buf, "%a %A %b %B", &tcopy);
        fprintf(f, "strftime1=[%s]\n", buf);
        strftime(buf, sizeof buf, "%c", &tcopy);
        fprintf(f, "strftime-c=[%s]\n", buf);
        strftime(buf, sizeof buf, "%d %H %I %j %m %M %p %S", &tcopy);
        fprintf(f, "strftime2=[%s]\n", buf);
        strftime(buf, sizeof buf, "%U %w %W", &tcopy);
        fprintf(f, "strftime3=[%s]\n", buf);
        strftime(buf, sizeof buf, "%x|%X|%y|%Y|%Z|%%", &tcopy);
        fprintf(f, "strftime4=[%s]\n", buf);
    }

    mk(f, "mk-1993-01-01", 93, 0, 1, 0, 0, 0);
    mk(f, "mk-1993-02-28", 93, 1, 28, 23, 59, 59);
    mk(f, "mk-1993-02-29-nonleap", 93, 1, 29, 0, 0, 0);
    mk(f, "mk-1993-03-01-mday0", 93, 2, 0, 0, 0, 0);
    mk(f, "mk-1993-06-30-sec75", 93, 5, 30, 12, 0, 75);
    mk(f, "mk-1993-06-30-hour25", 93, 5, 30, 25, 0, 0);
    mk(f, "mk-1993-01-32", 93, 0, 32, 0, 0, 0);
    mk(f, "mk-1993-07-01-12:00", 93, 6, 1, 12, 0, 0);
    mk(f, "mk-year69", 69, 0, 1, 0, 0, 0);
    mk(f, "mk-year139", 139, 0, 1, 0, 0, 0);
    mk(f, "mk-2037-12-31", 137, 11, 31, 23, 59, 59);
    mk(f, "mk-2038-01-18-overflow", 138, 0, 18, 23, 59, 59);
    mk(f, "mk-1970-01-01-est", 70, 0, 1, 0, 0, 0);
    putenv("TZ=GMT0"); tzset();
    mk(f, "mk-1970-01-01-gmt0", 70, 0, 1, 0, 0, 0);
    putenv("TZ=EST5EDT"); tzset();   /* 還原 */

    dd.da_year = 1993; dd.da_mon = 7; dd.da_day = 1;
    tt.ti_hour = 12; tt.ti_min = 0; tt.ti_sec = 0; tt.ti_hund = 0;
    unixtodos(J, &dd, &tt);
    fprintf(f, "unixtodos(J)=%d-%d-%d %02d:%02d:%02d.%02d\n",
            dd.da_year, dd.da_mon, dd.da_day, tt.ti_hour, tt.ti_min,
            tt.ti_sec, tt.ti_hund);

    fclose(f);
    return 0;
}
