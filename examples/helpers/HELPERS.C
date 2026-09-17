/* 讓 BCC 2.0 產生各種編譯器 helper 呼叫，並把邊界值的結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/compiler-helpers.md。 */
#include <stdio.h>

struct big { char b[37]; };

long mul32(long a, long b)          { return a * b; }
long div32(long a, long b)          { return a / b; }
long mod32(long a, long b)          { return a % b; }
unsigned long udiv32(unsigned long a, unsigned long b) { return a / b; }
unsigned long umod32(unsigned long a, unsigned long b) { return a % b; }
long shl32(long a, int n)           { return a << n; }
long shr32(long a, int n)           { return a >> n; }
unsigned long ushr32(unsigned long a, int n) { return a >> n; }
long ftol32(double d)               { return (long)d; }
struct big copy_big(struct big *p)  { struct big t; t = *p; return t; }
int take_big(struct big s)          { return s.b[36]; }
char huge *hadd(char huge *p, long n) { return p + n; }
long hsub(char huge *p, char huge *q) { return p - q; }
int hlt(char huge *p, char huge *q) { return p < q; }
void hinc(char huge **pp)           { (*pp)++; }

int main(void)
{
    FILE *f = fopen("OUT.TXT", "w");
    struct big b;
    int i;
    char huge *p = (char huge *)0x12340008L;
    for (i = 0; i < 37; i++) b.b[i] = (char)i;
    fprintf(f, "mul %ld %ld\n", mul32(123456L, -789L), mul32(0x7FFFFFFFL, 3L));
    fprintf(f, "div %ld %ld %ld\n", div32(-7L, 2L), div32(7L, -2L), div32(0x80000000L, -1L));
    fprintf(f, "mod %ld %ld %ld\n", mod32(-7L, 2L), mod32(7L, -2L), mod32(0x80000000L, -1L));
    fprintf(f, "udiv %lu %lu\n", udiv32(0xFFFFFFFFUL, 7UL), umod32(0xFFFFFFFFUL, 0x10001UL));
    fprintf(f, "shl %ld %ld %ld %ld\n", shl32(1L, 31), shl32(0x12345678L, 32), shl32(0x12345678L, 40), shl32(0x12345678L, 48));
    fprintf(f, "shr %ld %ld %ld\n", shr32(-256L, 4), shr32(-1L, 40), shr32(0x12345678L, 48));
    fprintf(f, "ushr %lu %lu\n", ushr32(0x80000000UL, 31), ushr32(0x12345678UL, 48));
    fprintf(f, "ftol %ld %ld %ld %ld\n", ftol32(-1.5), ftol32(2.999), ftol32(3.0e9), ftol32(-3.0e9));
    fprintf(f, "big %d %d\n", copy_big(&b).b[36], take_big(b));
    fprintf(f, "huge %Fp %ld %d\n", hadd(p, 0x10000L), hsub(hadd(p, 0x10000L), p), hlt(p, hadd(p, 1L)));
    hinc(&p);
    fprintf(f, "hinc %Fp\n", p);
    fclose(f);
    return 0;
}
