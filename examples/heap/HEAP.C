/* 讓 BC++ 2.0 的 heap 把配置策略跑出來，結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/heap.md。
   位址本身隨載入位置而變，所以只印「差值、是否相等、回傳值」這類跨執行穩定的東西。 */
#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <dos.h>
#include <string.h>
#include <errno.h>

static FILE *rep;

static void note3(char *tag, long a, long b, long c)
{
    fprintf(rep, "%s %ld %ld %ld\n", tag, a, b, c);
}

/* 兩個區塊的實際距離；區塊頭大小就從這裡看出來。
   far 資料模型不能直接相減——far 指標的減法只算位移，跨段就得出 0，
   所以自己把段與位移攤成線性位址。 */
static long gap(void *lo, void *hi)
{
#if defined(__SMALL__) || defined(__MEDIUM__)
    return (long)((char *)hi - (char *)lo);
#else
    return ((long)FP_SEG(hi) * 16L + (long)FP_OFF(hi))
         - ((long)FP_SEG(lo) * 16L + (long)FP_OFF(lo));
#endif
}

static void block_layout(void)
{
    void *p[6];
    int i;

    /* 連續配置同樣大小，相鄰兩塊的距離＝資料區＋區塊頭 */
    for (i = 0; i < 6; i++)
        p[i] = malloc(8);
    note3("layout.8", gap(p[0], p[1]), gap(p[1], p[2]), gap(p[2], p[3]));
    for (i = 0; i < 6; i++)
        free(p[i]);

    for (i = 0; i < 4; i++)
        p[i] = malloc(1);
    note3("layout.1", gap(p[0], p[1]), gap(p[1], p[2]), gap(p[2], p[3]));
    for (i = 0; i < 4; i++)
        free(p[i]);

    /* 奇數與偶數請求各配一輪，看進位規則 */
    for (i = 0; i < 4; i++)
        p[i] = malloc(9);
    note3("layout.9", gap(p[0], p[1]), gap(p[1], p[2]), gap(p[2], p[3]));
    for (i = 0; i < 4; i++)
        free(p[i]);

    for (i = 0; i < 4; i++)
        p[i] = malloc(10);
    note3("layout.10", gap(p[0], p[1]), gap(p[1], p[2]), gap(p[2], p[3]));
    for (i = 0; i < 4; i++)
        free(p[i]);

    /* malloc(0) 給不給東西 */
    note3("zero", malloc(0) == NULL ? 1 : 0, 0, 0);

    /* free(NULL) 應該靜默略過 */
    free(NULL);
    note3("free.null", heapcheck(), 0, 0);
}

static void reuse_and_split(void)
{
    char *a, *b, *c, *d;

    a = malloc(64);
    b = malloc(64);
    c = malloc(64);
    /* 放掉中間那塊，再要一塊小的：看新區塊落在原空洞的哪一端 */
    free(b);
    d = malloc(16);
    note3("split", gap(a, b), gap(b, d), d == b ? 1 : 0);
    free(d);
    free(a);
    free(c);

    /* 放掉整塊再要同樣大小，應該拿回同一個位址 */
    a = malloc(100);
    free(a);
    b = malloc(100);
    note3("exact", a == b ? 1 : 0, 0, 0);
    free(b);

    /* 相鄰兩塊都放掉，合併之後要得下一塊大的 */
    a = malloc(64);
    b = malloc(64);
    c = malloc(64);
    free(a);
    free(b);
    /* a 與 b 實體相鄰，合併後的空間放得下 120；但 first fit 是從 rover 起算的，
       所以還要看它實際落在哪 */
    d = malloc(120);
    note3("coalesce", d == a ? 1 : 0, gap(a, c), gap(a, d));
    free(d);
    free(c);
}

static void realloc_behaviour(void)
{
    char *a, *b;
    int same, kept;

    a = malloc(32);
    memset(a, 'A', 32);
    b = realloc(a, 200);
    same = (b == a);
    kept = (memcmp(b, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 32) == 0);
    note3("realloc.grow", same, kept, 0);
    free(b);

    a = malloc(200);
    memset(a, 'B', 200);
    b = realloc(a, 32);
    same = (b == a);
    kept = (memcmp(b, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 32) == 0);
    note3("realloc.shrink", same, kept, 0);
    free(b);

    /* 縮小之後放出來的空間能不能再用 */
    a = malloc(200);
    b = realloc(a, 16);
    note3("realloc.tail", gap(b, malloc(16)) > 0 ? 1 : 0, 0, 0);

    a = malloc(16);
    b = realloc(a, 0);
    note3("realloc.zero", b == NULL ? 1 : 0, 0, 0);

    b = realloc(NULL, 24);
    note3("realloc.null", b != NULL ? 1 : 0, 0, 0);
    free(b);
}

static void calloc_and_errors(void)
{
    char *a;
    int zeroed = 1, i;
    void *big;

    a = calloc(40, 2);
    for (i = 0; i < 80; i++)
        if (a[i] != 0)
            zeroed = 0;
    note3("calloc", a != NULL ? 1 : 0, zeroed, 0);
    free(a);

    /* 一定要不到的量：near heap 連 DS 都放不下 */
    errno = 0;
    big = malloc(65000U);
    note3("toobig", big == NULL ? 1 : 0, errno, 0);
    if (big)
        free(big);

    errno = 0;
    big = calloc(30000U, 4);
    note3("calloc.overflow", big == NULL ? 1 : 0, errno, 0);
    if (big)
        free(big);
}

static void walk_and_check(void)
{
    struct heapinfo hi;
    char *a, *b;
    int used = 0, freeblocks = 0, r;

    a = malloc(48);
    b = malloc(48);
    free(a);

    note3("heapcheck", heapcheck(), heapchecknode(b), heapchecknode(a));

    hi.ptr = NULL;
    while ((r = heapwalk(&hi)) == _HEAPOK) {
        if (hi.in_use)
            used++;
        else
            freeblocks++;
    }
    note3("heapwalk", used, freeblocks, r);
    free(b);
}

/* 在任何配置發生之前，用全新的 far heap 量一次：配一塊、放掉，
   看 farcoreleft 有沒有回到原點、heap 是不是真的空了。 */
static void far_clean(void)
{
    struct farheapinfo fhi;
    unsigned long before, mid, after;
    char far *f1;
    int n1 = 0, n2 = 0, r;

    before = farcoreleft();
    f1 = (char far *)farmalloc(1000L);
    mid = farcoreleft();

    fhi.ptr = NULL;
    while ((r = farheapwalk(&fhi)) == _HEAPOK && n1 < 32)
        n1++;

    farfree(f1);
    after = farcoreleft();

    fhi.ptr = NULL;
    while ((r = farheapwalk(&fhi)) == _HEAPOK && n2 < 32)
        n2++;

    note3("far.clean", f1 != NULL ? 1 : 0, (long)(before - mid),
          (long)(before - after));
    note3("far.clean.walk", n1, n2, r);
}

/* 同樣在乾淨的 far heap 上，但這次配兩塊，逐步放掉看可用量怎麼變 */
static void far_two(void)
{
    struct farheapinfo fhi;
    unsigned long before, a2, b1, b2;
    char far *f1, *f2;
    int n = 0, freed = 0, r;

    before = farcoreleft();
    f1 = (char far *)farmalloc(1000L);
    f2 = (char far *)farmalloc(1000L);
    a2 = farcoreleft();

    farfree(f2);                  /* 先放尾端那塊 */
    b1 = farcoreleft();
    farfree(f1);                  /* 再放剩下的唯一一塊 */
    b2 = farcoreleft();

    fhi.ptr = NULL;
    while ((r = farheapwalk(&fhi)) == _HEAPOK && n < 32) {
        if (!fhi.in_use)
            freed++;
        n++;
    }

    note3("far.two", (long)(before - a2), (long)(before - b1),
          (long)(before - b2));
    note3("far.two.walk", n, freed, r);
}

static void far_heap(void)
{
    char far *f1, *f2;
    unsigned long before, after;

    /* 此時 far heap 裡已經有 far_two 留下的 free 空間 */
    before = farcoreleft();
    f1 = (char far *)farmalloc(1000L);
    after = farcoreleft();
    note3("far.reuse", f1 != NULL ? 1 : 0, (long)(before - after), (long)FP_OFF(f1));

    f2 = (char far *)farmalloc(1000L);
    note3("far.gap", f2 != NULL ? 1 : 0, (long)FP_OFF(f2), 0);
    farfree(f2);
    farfree(f1);
}

/* 走一遍 far heap 的區塊序列：段是遞增還是遞減、每塊多大、用中還是自由 */
static void far_walk(void)
{
    struct farheapinfo fhi;
    char far *f1, *f2;
    unsigned firstseg = 0;
    int n = 0, r;

    f1 = (char far *)farmalloc(1000L);
    f2 = (char far *)farmalloc(1000L);
    fhi.ptr = NULL;
    while ((r = farheapwalk(&fhi)) == _HEAPOK && n < 32) {
        if (n == 0)
            firstseg = FP_SEG(fhi.ptr);
        fprintf(rep, "far.walk%d %ld %lu %d\n", n,
                (long)FP_SEG(fhi.ptr) - (long)firstseg, fhi.size, fhi.in_use);
        n++;
    }
    note3("far.walk", n, r, (f1 != NULL) + (f2 != NULL));
    farfree(f2);
    farfree(f1);
}

static void coreleft_shape(void)
{
    /* coreleft 的絕對值隨環境而變，只看它有沒有隨配置變少、放掉之後回來 */
#if defined(__SMALL__) || defined(__MEDIUM__)
    unsigned before, mid, after;
#else
    unsigned long before, mid, after;
#endif
    void *p;

    before = coreleft();
    p = malloc(4096);
    mid = coreleft();
    free(p);
    after = coreleft();
    note3("coreleft", before > mid ? 1 : 0, (long)(before - mid) >= 4096L,
          after == before ? 1 : 0);
    note3("coreleft.grain", (long)(before % 16), 0, 0);
}

int main(void)
{
    rep = fopen("OUT.TXT", "wb");
    fprintf(rep, "ptr %d %d\n", (int)sizeof(void *), (int)sizeof(void far *));
    block_layout();
    reuse_and_split();
    realloc_behaviour();
    calloc_and_errors();
    walk_and_check();
    far_clean();
    far_two();
    far_heap();
    far_walk();
    coreleft_shape();
    fclose(rep);
    return 0;
}
