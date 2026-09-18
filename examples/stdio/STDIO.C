/* 讓 BC++ 2.0 的 stdio 與低階檔案層把邊界行為跑出來，結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/stdio-file-io.md。
   報告檔以 "wb" 開啟，輸出不經過文字模式轉換。 */
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static FILE *rep;

static void note(char *tag, long a, long b, long c)
{
    fprintf(rep, "%s %ld %ld %ld\n", tag, a, b, c);
}

/* 把整個檔案以二進位讀回來，回傳長度，內容寫進 buf（最多 n）。 */
static int slurp(char *name, char *buf, int n)
{
    FILE *f = fopen(name, "rb");
    int got;
    if (!f) return -1;
    got = fread(buf, 1, n, f);
    fclose(f);
    return got;
}

/* 把位元組寫成可讀的十六進位字串。 */
static void hexline(char *tag, char *buf, int n)
{
    int i;
    fprintf(rep, "%s", tag);
    for (i = 0; i < n; i++)
        fprintf(rep, " %02X", (unsigned char)buf[i]);
    fprintf(rep, "\n");
}

static void write_modes(void)
{
    FILE *f;
    char buf[32];
    int n;

    f = fopen("T1.TXT", "w");                 /* 文字模式：\n 會變成 \r\n */
    fputs("a\nb\n", f);
    fclose(f);
    n = slurp("T1.TXT", buf, sizeof buf);
    hexline("textwrite", buf, n);

    f = fopen("T2.TXT", "wb");                /* 二進位：原樣寫出 */
    fputs("a\nb\n", f);
    fclose(f);
    n = slurp("T2.TXT", buf, sizeof buf);
    hexline("binwrite", buf, n);
}

static void read_translation(void)
{
    FILE *f;
    char buf[32];
    int n;

    f = fopen("T3.TXT", "wb");                /* \r\n、孤立的 \r、後面還有資料 */
    fwrite("x\r\ny\rz\r\n", 1, 8, f);
    fclose(f);
    f = fopen("T3.TXT", "r");
    n = fread(buf, 1, sizeof buf, f);
    note("textread", n, feof(f) ? 1 : 0, ftell(f));
    hexline("textread.bytes", buf, n);
    fclose(f);

    f = fopen("T4.TXT", "wb");                /* Ctrl-Z 之後還有資料 */
    fwrite("abc\032def", 1, 7, f);
    fclose(f);
    f = fopen("T4.TXT", "r");
    n = fread(buf, 1, sizeof buf, f);
    note("ctrlz", n, feof(f) ? 1 : 0, ftell(f));
    hexline("ctrlz.bytes", buf, n);
    fclose(f);

    f = fopen("T4.TXT", "rb");                /* 二進位不理會 Ctrl-Z */
    n = fread(buf, 1, sizeof buf, f);
    note("ctrlz.bin", n, feof(f) ? 1 : 0, 0L);
    fclose(f);
}

static void tell_and_seek(void)
{
    FILE *f;
    char buf[64];
    long t1, t2;
    int n;

    f = fopen("T5.TXT", "w");                 /* 文字模式下 ftell 報的是檔案裡的位置 */
    fputs("12\n", f);
    t1 = ftell(f);
    fflush(f);
    t2 = ftell(f);
    fclose(f);
    n = slurp("T5.TXT", buf, sizeof buf);
    note("ftell.text", t1, t2, n);

    f = fopen("T6.BIN", "wb");                /* 跳過檔尾再寫，中間的洞由 DOS 補 0 */
    fwrite("AB", 1, 2, f);
    fseek(f, 6L, SEEK_SET);
    fwrite("Z", 1, 1, f);
    fclose(f);
    n = slurp("T6.BIN", buf, sizeof buf);
    hexline("seekgap", buf, n);

    f = fopen("T6.BIN", "rb");                /* 讀到檔尾之後：短讀、feof、ftell */
    n = fread(buf, 1, 4, f);
    t1 = ftell(f);
    fseek(f, 0L, SEEK_END);
    t2 = ftell(f);
    n = fread(buf, 1, 4, f);
    note("readpasteof", t1, t2, n);
    note("eofflag", feof(f) ? 1 : 0, ferror(f) ? 1 : 0, 0L);
    clearerr(f);
    note("clearerr", feof(f) ? 1 : 0, ferror(f) ? 1 : 0, 0L);
    fclose(f);
}

static void buffering(void)
{
    FILE *f;
    char buf[64];
    long before, after;

    f = fopen("T7.BIN", "wb");                /* 預設全緩衝：寫入還留在緩衝區 */
    fwrite("0123456789", 1, 10, f);
    before = slurp("T7.BIN", buf, sizeof buf);
    fflush(f);
    after = slurp("T7.BIN", buf, sizeof buf);
    fclose(f);
    note("fullbuf", before, after, 0L);

    f = fopen("T8.BIN", "wb");                /* 不緩衝：每次寫入直接落到檔案 */
    setvbuf(f, NULL, _IONBF, 0);
    fwrite("0123456789", 1, 10, f);
    before = slurp("T8.BIN", buf, sizeof buf);
    fclose(f);
    after = slurp("T8.BIN", buf, sizeof buf);
    note("nobuf", before, after, 0L);

    f = fopen("T9.BIN", "wb");                /* 自備緩衝區，比寫入量小 */
    setvbuf(f, buf, _IOFBF, 8);
    fwrite("0123456789", 1, 10, f);
    before = slurp("T9.BIN", buf + 32, 16);
    fclose(f);
    after = slurp("T9.BIN", buf + 32, 16);
    note("smallbuf", before, after, 0L);
}

static void append_and_update(void)
{
    FILE *f;
    char buf[64];
    int n;

    f = fopen("TA.BIN", "wb");
    fwrite("1234", 1, 4, f);
    fclose(f);

    f = fopen("TA.BIN", "ab");                /* 附加模式：寫入一律落在檔尾 */
    fseek(f, 0L, SEEK_SET);
    fwrite("X", 1, 1, f);
    fclose(f);
    n = slurp("TA.BIN", buf, sizeof buf);
    hexline("append", buf, n);

    f = fopen("TA.BIN", "r+b");               /* 更新模式：可以改寫中間 */
    fseek(f, 1L, SEEK_SET);
    fwrite("Y", 1, 1, f);
    fseek(f, 0L, SEEK_SET);
    n = fread(buf, 1, 8, f);
    fclose(f);
    hexline("update", buf, n);
}

static void pushback_and_lines(void)
{
    FILE *f;
    char line[16];
    long t0, t1, t2;
    int c;

    f = fopen("TC.TXT", "wb");
    fwrite("ab\r\ncd", 1, 6, f);              /* 最後一行沒有換行 */
    fclose(f);

    f = fopen("TC.TXT", "r");
    c = fgetc(f);
    t0 = ftell(f);
    ungetc(c, f);                             /* 推回一個字元 */
    t1 = ftell(f);
    c = fgetc(f);
    t2 = ftell(f);
    note("ungetc", t0, t1, t2);
    fclose(f);

    f = fopen("TC.TXT", "r");
    fgets(line, sizeof line, f);              /* 有換行的一行 */
    note("fgets1", strlen(line), line[strlen(line) - 1] == '\n' ? 1 : 0, ftell(f));
    fgets(line, sizeof line, f);              /* 檔尾沒有換行的一行 */
    note("fgets2", strlen(line), line[strlen(line) - 1] == '\n' ? 1 : 0, ftell(f));
    note("fgets3", fgets(line, sizeof line, f) == NULL ? 1 : 0, feof(f) ? 1 : 0, 0L);
    fclose(f);

    f = fopen("TC.TXT", "ab");                /* 附加模式開檔後的位置 */
    t0 = ftell(f);
    fclose(f);
    f = fopen("TC.TXT", "rb");
    fseek(f, 0L, SEEK_END);
    t1 = ftell(f);
    fclose(f);
    note("appendpos", t0, t1, 0L);
}

static void errors(void)
{
    FILE *f;
    int fd;

    errno = 0;
    f = fopen("NOSUCH.TXT", "r");             /* 開不存在的檔 */
    note("noent", f ? 1 : 0, errno, _doserrno);

    errno = 0;
    f = fopen("T1.TXT", "q");                 /* 模式字串第一個字元不合法 */
    note("badmode", f ? 1 : 0, errno, _doserrno);

    errno = 0;
    fd = open("NOSUCH.TXT", O_RDONLY);        /* 低階代號層的同一件事 */
    note("open.noent", fd, errno, _doserrno);

    errno = 0;
    note("read.badfd", read(9, (void *)0, 1), errno, _doserrno);
}

static void handles(void)
{
    FILE *f;
    int fd, dup1;
    long pos;

    f = fopen("TB.BIN", "wb");
    fwrite("abcdef", 1, 6, f);
    fclose(f);

    fd = open("TB.BIN", O_RDONLY | O_BINARY); /* FILE 之下就是 DOS 代號 */
    dup1 = dup(fd);
    lseek(fd, 2L, SEEK_SET);
    pos = tell(dup1);                         /* 複製的代號共用檔案位置 */
    note("dup.pos", fd >= 0 ? 1 : 0, pos, filelength(fd));
    close(dup1);
    close(fd);

    f = fopen("TB.BIN", "rb");
    fd = fileno(f);
    note("fileno", fd >= 3 ? 1 : 0, isatty(fd), eof(fd));
    fclose(f);
}

int main(void)
{
    rep = fopen("OUT.TXT", "wb");
    note("BUFSIZ", BUFSIZ, FOPEN_MAX, sizeof(FILE));
    write_modes();
    read_translation();
    tell_and_seek();
    buffering();
    append_and_update();
    pushback_and_lines();
    errors();
    handles();
    note("fmode", _fmode == O_TEXT ? 1 : 0, 0L, 0L);
    fclose(rep);
    return 0;
}
