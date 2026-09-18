/* 讓 BC++ 2.0 的 conio 把文字畫面的行為跑出來，結果寫到 OUT.TXT。
   本 repo 自己寫的測試程式；說明見 docs/10-borland-crtl/conio-screen.md。
   畫面本身交給 dosgolem 的文字畫面 dump 去看，這裡只記錄座標、屬性與畫面上取回的字元。 */
#include <stdio.h>
#include <conio.h>
#include <string.h>

static FILE *rep;

static void note3(char *tag, long a, long b, long c)
{
    fprintf(rep, "%s %ld %ld %ld\n", tag, a, b, c);
}

/* 從畫面取一個位置的字元與屬性（gettext 用絕對座標） */
static void cell(char *tag, int x, int y)
{
    unsigned buf;

    gettext(x, y, x, y, &buf);
    fprintf(rep, "%s %u %u\n", tag, buf & 0xFF, (buf >> 8) & 0xFF);
}

static void window_and_cursor(void)
{
    struct text_info ti;

    gettextinfo(&ti);
    note3("screen", ti.screenwidth, ti.screenheight, ti.currmode);
    note3("win.full", ti.winleft, ti.wintop, ti.winright);
    note3("attr.init", ti.attribute, ti.normattr, 0);

    /* 開一個視窗，游標應該跳到視窗的 1,1 */
    window(10, 5, 40, 15);
    gettextinfo(&ti);
    note3("win.set", ti.winleft, ti.wintop, ti.winright);
    note3("win.cursor", wherex(), wherey(), ti.winbottom);

    /* 視窗座標是 1 起算、相對視窗 */
    gotoxy(3, 2);
    note3("goto.in", wherex(), wherey(), 0);

    /* 越界的 gotoxy 應該整個被忽略 */
    gotoxy(100, 100);
    note3("goto.out", wherex(), wherey(), 0);

    /* 無效的 window 也應該整個被忽略 */
    window(1, 1, 200, 200);
    gettextinfo(&ti);
    note3("win.bad", ti.winleft, ti.wintop, ti.winright);
}

static void write_and_wrap(void)
{
    struct text_info ti;
    int x1, y1;

    window(1, 1, 80, 25);
    clrscr();
    window(5, 3, 14, 6);          /* 10 欄 × 4 列的小視窗 */
    clrscr();

    gotoxy(1, 1);
    cputs("AB");
    note3("cputs", wherex(), wherey(), 0);
    cell("cell.A", 5, 3);
    cell("cell.B", 6, 3);

    /* \n 只換列不回欄 */
    gotoxy(1, 1);
    cputs("XY\nZ");
    note3("lf", wherex(), wherey(), 0);
    cell("cell.Z", 7, 4);

    /* \r 回到視窗左界 */
    gotoxy(5, 1);
    cputs("\r");
    note3("cr", wherex(), wherey(), 0);

    /* \b 退一格但不清字元，且不會退出視窗 */
    gotoxy(1, 1);
    cputs("\b\b");
    note3("bs.left", wherex(), wherey(), 0);

    /* 寫滿一列會折到下一列的左界 */
    gotoxy(1, 2);
    cputs("0123456789");          /* 正好 10 欄 */
    x1 = wherex(); y1 = wherey();
    note3("wrap", x1, y1, 0);

    /* 寫到視窗最後一列的尾巴會捲動視窗 */
    window(5, 3, 14, 6);
    clrscr();
    gotoxy(1, 1);
    cputs("top");
    gotoxy(1, 4);
    cputs("0123456789");          /* 最後一列寫滿 */
    note3("scroll.pos", wherex(), wherey(), 0);
    cell("scroll.top", 5, 3);     /* 原本的 top 應該被捲掉 */

    gettextinfo(&ti);
    note3("scroll.win", ti.winleft, ti.wintop, ti.winbottom);
}

static void attributes(void)
{
    unsigned buf;

    window(1, 1, 80, 25);
    clrscr();
    textattr(0);
    textcolor(LIGHTGREEN);
    textbackground(BLUE);
    gotoxy(1, 1);
    cputs("C");
    gettext(1, 1, 1, 1, &buf);
    note3("attr.fgbg", (buf >> 8) & 0xFF, LIGHTGREEN + (BLUE << 4), 0);

    textattr(BLINK + YELLOW + (RED << 4));
    gotoxy(2, 1);
    cputs("D");
    gettext(2, 1, 2, 1, &buf);
    note3("attr.blink", (buf >> 8) & 0xFF, BLINK + YELLOW + (RED << 4), 0);

    /* clrscr 用的是目前的屬性 */
    clrscr();
    gettext(40, 12, 40, 12, &buf);
    note3("attr.clrscr", (buf >> 8) & 0xFF, buf & 0xFF, 0);
}

static void blocks(void)
{
    unsigned buf[8];
    int i, ok = 1;

    window(1, 1, 80, 25);
    textattr(7);
    clrscr();
    gotoxy(1, 1);
    cputs("12345678");

    /* gettext 的緩衝區是一行一行的字元＋屬性 */
    gettext(1, 1, 8, 1, buf);
    for (i = 0; i < 8; i++)
        if ((buf[i] & 0xFF) != '1' + i)
            ok = 0;
    note3("gettext", ok, buf[0] & 0xFF, (buf[0] >> 8) & 0xFF);

    /* puttext 放到別的地方 */
    puttext(1, 3, 8, 3, buf);
    cell("puttext", 1, 3);

    /* movetext 搬一塊 */
    movetext(1, 1, 8, 1, 1, 5);
    cell("movetext", 1, 5);

    /* 多列的 gettext：緩衝區一列接一列，第二列從第 width 格開始 */
    gotoxy(1, 7);
    cputs("ab");
    gotoxy(1, 8);
    cputs("cd");
    gettext(1, 7, 2, 8, buf);
    note3("gettext.rows", buf[0] & 0xFF, buf[2] & 0xFF, buf[3] & 0xFF);

    /* 往下搬一塊會與自己重疊的區域 */
    gotoxy(1, 10);
    cputs("12345678");
    movetext(1, 10, 8, 11, 1, 11);
    cell("movetext.overlap", 1, 11);
}

/* 區塊函式的座標檢查：gettext／movetext 會擋，puttext 不會 */
static void bounds(void)
{
    unsigned buf[2];

    window(1, 1, 80, 25);
    textattr(7);
    buf[0] = 0x0741;              /* 白底黑字的 'A' */

    note3("bounds.get", gettext(1, 1, 100, 1, buf), gettext(1, 1, 1, 100, buf),
          gettext(5, 1, 1, 1, buf));
    note3("bounds.move", movetext(1, 1, 100, 1, 1, 2), movetext(1, 1, 8, 1, 75, 1), 0);
    /* puttext 不驗座標：同樣越界的呼叫它回 1，而且真的寫下去
       （這裡挑的是最後一列右緣外一點點，仍在顯示記憶體那一段裡） */
    note3("bounds.put", puttext(79, 25, 82, 25, buf),
          gettext(79, 25, 82, 25, buf), 0);
}

static void scroll_flag(void)
{
    window(5, 10, 14, 12);
    textattr(7);
    clrscr();

    /* 關掉自動換行與捲動 */
    _wscroll = 0;
    gotoxy(1, 1);
    cputs("0123456789ABC");       /* 超過視窗寬度 */
    note3("nowrap", wherex(), wherey(), _wscroll);
    _wscroll = 1;
}

/* 把 directvideo 關掉，同樣的動作改走 BIOS，看結果一不一樣 */
static void bios_path(void)
{
    unsigned buf[4], buf2[4];
    int i, same = 1;

    /* 只清自己要用的那幾列，前面幾段留在畫面上一起 dump */
    window(1, 17, 80, 20);
    textattr(7);
    clrscr();
    window(1, 1, 80, 25);

    /* 先用直接寫顯示記憶體的路徑寫一次 */
    directvideo = 1;
    gotoxy(1, 18);
    cputs("BIOS");
    gettext(1, 18, 4, 18, buf);

    /* 同樣的動作改走 BIOS */
    directvideo = 0;
    gotoxy(1, 19);
    cputs("BIOS");
    gettext(1, 19, 4, 19, buf2);

    for (i = 0; i < 4; i++)
        if (buf[i] != buf2[i])
            same = 0;
    note3("biospath", same, buf2[0] & 0xFF, (buf2[0] >> 8) & 0xFF);

    /* 捲動與清除也走一次 BIOS 路徑 */
    window(60, 18, 70, 20);
    clrscr();
    gotoxy(1, 1);
    cputs("x");
    gettext(60, 18, 60, 18, buf2);
    note3("biosclr", buf2[0] & 0xFF, (buf2[0] >> 8) & 0xFF, 0);

    directvideo = 1;
    window(1, 1, 80, 25);
}

int main(void)
{
    rep = fopen("OUT.TXT", "wb");
    note3("directvideo", directvideo, _wscroll, 0);
    window_and_cursor();
    write_and_wrap();
    attributes();
    blocks();
    bounds();
    scroll_flag();
    bios_path();
    window(1, 1, 80, 25);
    gotoxy(1, 20);
    fclose(rep);
    return 0;
}
