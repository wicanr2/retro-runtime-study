/* 讓 BC++ 2.0 的 printf／scanf 家族把格式化的邊界行為跑出來，結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/printf-engine.md。
   每一行是「標籤 |格式化結果| 回傳值」，兩個直線之間就是輸出，方便看空白與長度。 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static FILE *rep;
static char buf[256];

/* 用 sprintf 產生一次輸出，把結果與回傳值記下來。 */
static void show(char *tag, int n)
{
    fprintf(rep, "%s |%s| %d\n", tag, buf, n);
}

static void integers(void)
{
    show("d", sprintf(buf, "%d %d %d", 0, -1, 32767));
    show("d.width", sprintf(buf, "[%5d][%-5d][%05d]", 42, 42, 42));
    show("d.prec", sprintf(buf, "[%.3d][%8.3d][%.0d]", 5, 5, 0));
    show("d.sign", sprintf(buf, "[%+d][% d][%+d]", 7, 7, -7));
    show("u.x.o", sprintf(buf, "%u %x %X %o", 65535U, 48879U, 48879U, 8U));
    show("alt", sprintf(buf, "[%#x][%#X][%#o][%#x]", 255, 255, 8, 0));
    show("long", sprintf(buf, "%ld %lu %lx", -2147483647L - 1L, 4294967295UL, 305419896L));
    show("star", sprintf(buf, "[%*d][%-*d][%.*d]", 6, 42, 6, 42, 4, 42));
    show("negstar", sprintf(buf, "[%*d]", -6, 42));
    show("i", sprintf(buf, "%i %hd", 123, (short)-5));
}

static void strings_chars(void)
{
    char *nullp = NULL;

    show("s", sprintf(buf, "[%s][%8s][%-8s]", "abc", "abc", "abc"));
    show("s.prec", sprintf(buf, "[%.2s][%8.2s][%.0s]", "abcdef", "abcdef", "abcdef"));
    show("s.null", sprintf(buf, "[%s]", nullp));
    show("c", sprintf(buf, "[%c][%3c][%-3c]", 'A', 'A', 'A'));
    show("c.zero", sprintf(buf, "[%c]x", 0));
    show("pct", sprintf(buf, "[%%][%5%]", 1));
}

static void pointers(void)
{
    char near *np = buf;
    char far *fp = (char far *)0x12345678L;
    int n;

    n = sprintf(buf, "%Fp", fp);
    show("p.far", n);
    n = sprintf(buf, "%Np", (void near *)0x1234);
    show("p.near", n);
    n = sprintf(buf, "%p", (void *)np);
    fprintf(rep, "p.default %d\n", n);   /* 值隨載入位址而變，只看長度 */
}

static void badformats(void)
{
    show("bad.q", sprintf(buf, "a%qb"));
    show("bad.end", sprintf(buf, "a%"));
    show("bad.width", sprintf(buf, "a%5"));
}

static void counted(void)
{
    int n1 = -1, n2 = -1;
    int r = sprintf(buf, "abc%ndef%n", &n1, &n2);
    fprintf(rep, "n %d %d %d\n", r, n1, n2);
}

static void longoutput(void)
{
    int n = sprintf(buf, "%200d", 7);          /* 超過內部 80 bytes 緩衝區 */
    fprintf(rep, "wide200 %d %d %c %c\n", n, (int)strlen(buf),
            buf[0], buf[199]);
    n = sprintf(buf, "%-120s|", "x");
    fprintf(rep, "wide120 %d %d %c\n", n, (int)strlen(buf), buf[119]);
}

static void floats(void)
{
    double v = 1.0 / 3.0;

    show("f", sprintf(buf, "%f", 1.5));
    show("f.prec", sprintf(buf, "[%.0f][%.1f][%8.3f][%-8.3f]", 2.5, 2.45, v, v));
    show("e", sprintf(buf, "[%e][%.2e][%E]", 1234.5678, 1234.5678, 0.00012));
    show("g", sprintf(buf, "[%g][%.3g][%G]", 1234.5678, 1234.5678, 0.00012));
    show("round", sprintf(buf, "[%.0f][%.0f][%.0f][%.0f]", 0.5, 1.5, 2.5, 3.5));
    show("zero", sprintf(buf, "[%f][%g][%e]", 0.0, 0.0, 0.0));
    show("neg", sprintf(buf, "[%.2f][%+.2f]", -0.125, 0.125));
    show("big", sprintf(buf, "[%.3e]", 3.0e9));
}

static void scanning(void)
{
    int a, b, n;
    long l;
    unsigned u;
    char s1[16], s2[16], c;
    double d;

    a = b = -1;
    n = sscanf("12 34", "%d %d", &a, &b);
    fprintf(rep, "scan.d %d %d %d\n", n, a, b);

    a = -1;
    n = sscanf("  \t 7abc", "%d%s", &a, s1);
    fprintf(rep, "scan.skip %d %d |%s|\n", n, a, s1);

    a = -1;
    n = sscanf("12345", "%2d%3d", &a, &b);
    fprintf(rep, "scan.width %d %d %d\n", n, a, b);

    n = sscanf("abc", "%d", &a);
    fprintf(rep, "scan.fail %d\n", n);

    n = sscanf("", "%d", &a);
    fprintf(rep, "scan.empty %d\n", n);

    n = sscanf("0x1F 017 99", "%x %o %u", &u, &b, &a);
    fprintf(rep, "scan.radix %d %u %d %d\n", n, u, b, a);

    n = sscanf("2147483647", "%ld", &l);
    fprintf(rep, "scan.long %d %ld\n", n, l);

    s1[0] = s2[0] = 0;
    n = sscanf("hello world", "%5s %s", s1, s2);
    fprintf(rep, "scan.s %d |%s| |%s|\n", n, s1, s2);

    c = '?';
    n = sscanf("xy", "%c%c", &c, &s1[0]);
    fprintf(rep, "scan.c %d %c %c\n", n, c, s1[0]);

    s1[0] = 0;
    n = sscanf("abc123", "%[abc]%s", s1, s2);
    fprintf(rep, "scan.set %d |%s| |%s|\n", n, s1, s2);

    a = -1;
    n = sscanf("42 7", "%*d %d", &a);
    fprintf(rep, "scan.star %d %d\n", n, a);

    a = -1;
    n = sscanf("abc42", "abc%d", &a);
    fprintf(rep, "scan.lit %d %d\n", n, a);

    a = -1;
    n = sscanf("zz42", "abc%d", &a);
    fprintf(rep, "scan.litfail %d %d\n", n, a);

    d = -1.0;
    n = sscanf("3.5e2", "%lf", &d);
    fprintf(rep, "scan.f %d %.1f\n", n, d);

    a = -1;
    n = sscanf("12ab", "%d%n", &a, &b);
    fprintf(rep, "scan.n %d %d %d\n", n, a, b);
}

int main(void)
{
    rep = fopen("OUT.TXT", "wb");
    integers();
    strings_chars();
    pointers();
    badformats();
    counted();
    longoutput();
    floats();
    scanning();
    fclose(rep);
    return 0;
}
