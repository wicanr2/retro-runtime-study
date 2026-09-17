/* 讓 BCC 2.0 對常見語法構造各產生一段程式碼，並把計算結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/60-re-fingerprints/bcc20-codegen.md。
   每個 cg_ 函式只示範一種構造，名稱即構造。 */
#include <stdio.h>

struct pt { int x, y; char tag[6]; };

int g_int = 5;                 /* 有初值的全域變數 */
int g_arr[4];                  /* 沒有初值的全域變數 */
char far g_far[8];             /* 明寫 far 的全域陣列 */

void cg_empty(void)            { }
int cg_params(int a, int b)    { return a + b; }
int cg_locals(int a)           { int x, y; x = a * 3; y = x + 1; return x * y; }
int cg_bigframe(int a)         { char buf[300]; buf[299] = (char)a; return buf[299]; }
int cg_regloop(int n)          { int i, s = 0; for (i = 0; i < n; i++) s += i; return s; }
int cg_regptr(char *p)         { int n = 0; while (*p++) n++; return n; }
int cg_dowhile(int n)          { int s = 0; do { s += n; } while (--n > 0); return s; }

int cg_sw_dense(int k)
{
    switch (k) {
    case 0: return 10; case 1: return 11; case 2: return 12;
    case 3: return 13; case 4: return 14; case 5: return 15;
    default: return -1;
    }
}

int cg_sw_sparse(int k)
{
    switch (k) {
    case -7: return 5; case 1: return 1; case 100: return 2;
    case 1000: return 3; case 5000: return 4;
    default: return 0;
    }
}

int cg_sw_two(int k)
{
    switch (k) { case 3: return 1; case 9: return 2; default: return 0; }
}

int cg_sw_long(long k)
{
    switch (k) {
    case 1L: return 1; case 2L: return 2; case 3L: return 3;
    case 70000L: return 4;
    default: return 0;
    }
}

int cg_many(int a, int b, int c, int d, int e) { return a - b + c - d + e; }
int cg_call2(void)             { return cg_params(1, 2) + cg_params(3, 4); }
int cg_call5(void)             { return cg_many(1, 2, 3, 4, 5); }
int pascal cg_pascal(int a, int b) { return a - b; }
int cg_callpas(void)           { return cg_pascal(9, 4); }
int far cg_farfunc(int a)      { return a + 1; }
int cg_callfar(void)           { return cg_farfunc(41); }
long cg_long_inline(long a, long b) { return ((a + b) - (a & b)) ^ (a | b); }
int cg_long_cmp(long a, long b) { return a < b; }
int cg_globals(void)           { g_arr[g_int & 3] = g_int; return g_arr[1]; }
int cg_fardata(int i)          { g_far[i & 7] = (char)i; return g_far[i & 7]; }
int cg_member(struct pt *p)    { return p->x + p->y + p->tag[5]; }
int cg_farptr(char far *p, int i) { return p[i]; }
int cg_constmath(int a)        { return a * 10 + a / 4 + a % 8; }
unsigned cg_umath(unsigned a)  { return a / 16 + (a >> 3); }
int cg_chars(char c, unsigned char u) { return c + u; }
int cg_cond(int a, int b)      { return a > b ? a : b; }
int cg_logic(int a, int b)     { return (a && b) || !a; }
int cg_immops(int a)           { int r = a & 12; r |= 3; r ^= 5; r += 7; r -= 2; if (r > 9) r = 9; return r; }
int cg_immmem(int a)           { g_int &= 12; g_int |= 3; g_int ^= 5; g_int += 7; g_int -= a; return g_int < 300; }
double cg_float(double a, int b) { return a * b + 0.5; }
char *cg_string(void)          { return "codegen"; }

int main(void)
{
    FILE *f = fopen("OUT.TXT", "w");
    struct pt p;
    char s[8];
    char far *fp = s;
    int i;
    p.x = 3; p.y = 4; p.tag[5] = 5;
    for (i = 0; i < 8; i++) s[i] = (char)('a' + i);
    s[7] = 0;
    cg_empty();
    fprintf(f, "frame %d %d %d\n", cg_params(2, 3), cg_locals(4), cg_bigframe(77));
    fprintf(f, "loop %d %d %d\n", cg_regloop(10), cg_regptr(s), cg_dowhile(4));
    fprintf(f, "dense %d %d %d\n", cg_sw_dense(0), cg_sw_dense(5), cg_sw_dense(6));
    fprintf(f, "sparse %d %d %d %d\n", cg_sw_sparse(-7), cg_sw_sparse(1000), cg_sw_sparse(5000), cg_sw_sparse(2));
    fprintf(f, "two %d %d %d\n", cg_sw_two(3), cg_sw_two(9), cg_sw_two(4));
    fprintf(f, "swlong %d %d %d\n", cg_sw_long(3L), cg_sw_long(70000L), cg_sw_long(65537L));
    fprintf(f, "call %d %d %d %d\n", cg_call2(), cg_call5(), cg_callpas(), cg_callfar());
    fprintf(f, "long %ld %d %d\n", cg_long_inline(0x12345L, 0x0F0F0L), cg_long_cmp(-1L, 1L), cg_long_cmp(0x10000L, 0xFFFFL));
    fprintf(f, "data %d %d %d\n", cg_globals(), cg_fardata(13), cg_member(&p));
    fprintf(f, "ptr %d\n", cg_farptr(fp, 2));
    fprintf(f, "math %d %d %u %d\n", cg_constmath(-37), cg_constmath(37), cg_umath(1000), cg_chars(-2, 200));
    fprintf(f, "cond %d %d %d %d\n", cg_cond(3, 8), cg_cond(-3, -8), cg_logic(1, 0), cg_logic(0, 5));
    fprintf(f, "imm %d %d %d\n", cg_immops(13), cg_immops(-1), cg_immmem(2));
    fprintf(f, "float %ld\n", (long)(cg_float(1.25, 4) * 10));
    fprintf(f, "string %s\n", cg_string());
    fclose(f);
    return 0;
}
