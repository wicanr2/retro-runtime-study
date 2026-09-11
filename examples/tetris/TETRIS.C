/*
 * TETRIS.C － Borland C++ 2.0 + BGI 的俄羅斯方塊範例
 *
 * 編譯（small model，連結 BGI 繪圖庫）：
 *     BCC -ms TETRIS.C GRAPHICS.LIB
 * 執行時同一個目錄要有 EGAVGA.BGI（BGI 的 VGA 驅動，initgraph 會去載）。
 *
 * 操作：← → 移動、↑ 或 X 右轉、Z 左轉、↓ 加速、空白鍵直接落下、P 暫停、Esc 離開。
 *
 * 寫法刻意只用 BC++ 2.0 附的東西：graphics.h 畫圖、bios.h 的 bioskey() 讀鍵、
 * biostime() 讀 BIOS 計時器（每秒約 18.2 次）決定下落速度。
 * 畫面只重畫有變化的格子，避免整片重畫造成閃爍。
 */
#include <graphics.h>
#include <bios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS    10
#define ROWS    20
#define CELL    20
#define BOARD_X 220             /* 盤面左上角的像素位置 */
#define BOARD_Y 40
#define SIDE_X  (BOARD_X + COLS * CELL + 30)

/* bioskey(0) 回傳值的高位元組是掃描碼 */
#define SC_ESC   0x01
#define SC_UP    0x48
#define SC_LEFT  0x4B
#define SC_RIGHT 0x4D
#define SC_DOWN  0x50
#define SC_SPACE 0x39

#define GHOST    DARKGRAY       /* 落點預覽的顏色 */

/*
 * 七種方塊 × 四個方向。每個方向是一個 4×4 的點陣，壓成 16 位元：
 * 第 15 位元是 (列 0, 行 0)，往右往下依序遞減。
 */
static const unsigned int shapes[7][4] = {
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },   /* I */
    { 0x8E00, 0x6440, 0x0E20, 0x44C0 },   /* J */
    { 0x2E00, 0x4460, 0x0E80, 0xC440 },   /* L */
    { 0x6600, 0x6600, 0x6600, 0x6600 },   /* O */
    { 0x6C00, 0x4620, 0x06C0, 0x8C40 },   /* S */
    { 0x4E00, 0x4640, 0x0E40, 0x4C40 },   /* T */
    { 0xC600, 0x2640, 0x0C60, 0x4C80 }    /* Z */
};
static const int piece_color[7] = {
    LIGHTCYAN, LIGHTBLUE, BROWN, YELLOW, LIGHTGREEN, LIGHTMAGENTA, LIGHTRED
};
static const int line_score[5] = { 0, 100, 300, 500, 800 };

static unsigned char board[ROWS][COLS];   /* 已固定的方塊：0 是空格，其餘是顏色 */
static unsigned char shown[ROWS][COLS];   /* 目前畫在螢幕上的樣子 */

static int cur, rot, px, py;      /* 正在落下的方塊與位置 */
static int next_piece;
static long score;
static int lines, level;

static int cell_of(int s, int r, int row, int col)
{
    return (shapes[s][r] & (0x8000u >> (row * 4 + col))) != 0;
}

/* 方塊 s 以方向 r 放在 (x, y) 合不合法 */
static int fits(int s, int r, int x, int y)
{
    int row, col, bx, by;
    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++) {
            if (!cell_of(s, r, row, col))
                continue;
            bx = x + col;
            by = y + row;
            if (bx < 0 || bx >= COLS || by >= ROWS)
                return 0;
            if (by >= 0 && board[by][bx])
                return 0;
        }
    return 1;
}

/* 畫一格：有顏色的格子帶一圈亮邊與暗邊，看起來有立體感 */
static void draw_cell(int row, int col, int color)
{
    int x = BOARD_X + col * CELL, y = BOARD_Y + row * CELL;

    setfillstyle(SOLID_FILL, BLACK);
    bar(x, y, x + CELL - 1, y + CELL - 1);
    if (color == 0)
        return;
    if (color == GHOST) {
        setcolor(GHOST);
        rectangle(x + 2, y + 2, x + CELL - 3, y + CELL - 3);
        return;
    }
    setfillstyle(SOLID_FILL, color);
    bar(x + 1, y + 1, x + CELL - 2, y + CELL - 2);
    setcolor(WHITE);
    line(x + 1, y + 1, x + CELL - 2, y + 1);
    line(x + 1, y + 1, x + 1, y + CELL - 2);
    setcolor(DARKGRAY);
    line(x + CELL - 2, y + 2, x + CELL - 2, y + CELL - 2);
    line(x + 2, y + CELL - 2, x + CELL - 2, y + CELL - 2);
}

/* 算出這一刻該有的盤面（固定方塊＋落點預覽＋落下中的方塊），只重畫不同的格子 */
static void render(void)
{
    unsigned char frame[ROWS][COLS];
    int row, col, gy;

    memcpy(frame, board, sizeof frame);
    gy = py;
    while (fits(cur, rot, px, gy + 1))
        gy++;
    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++) {
            if (!cell_of(cur, rot, row, col))
                continue;
            if (gy + row >= 0 && !frame[gy + row][px + col])
                frame[gy + row][px + col] = GHOST;
        }
    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++)
            if (cell_of(cur, rot, row, col) && py + row >= 0)
                frame[py + row][px + col] = (unsigned char)piece_color[cur];

    for (row = 0; row < ROWS; row++)
        for (col = 0; col < COLS; col++)
            if (frame[row][col] != shown[row][col]) {
                draw_cell(row, col, frame[row][col]);
                shown[row][col] = frame[row][col];
            }
}

static void draw_text_box(int y, const char *label, const char *value)
{
    setfillstyle(SOLID_FILL, BLACK);
    bar(SIDE_X, y, SIDE_X + 150, y + 30);
    setcolor(LIGHTGRAY);
    outtextxy(SIDE_X, y, (char *)label);
    setcolor(WHITE);
    outtextxy(SIDE_X, y + 14, (char *)value);
}

static void draw_side(void)
{
    char buf[16];
    int row, col, x, y;

    sprintf(buf, "%ld", score);
    draw_text_box(BOARD_Y + 110, "SCORE", buf);
    sprintf(buf, "%d", lines);
    draw_text_box(BOARD_Y + 150, "LINES", buf);
    sprintf(buf, "%d", level);
    draw_text_box(BOARD_Y + 190, "LEVEL", buf);

    /* 下一塊：縮小成 12 像素一格畫在框裡 */
    setfillstyle(SOLID_FILL, BLACK);
    bar(SIDE_X + 5, BOARD_Y + 20, SIDE_X + 5 + 4 * 12, BOARD_Y + 20 + 4 * 12);
    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++)
            if (cell_of(next_piece, 0, row, col)) {
                x = SIDE_X + 5 + col * 12;
                y = BOARD_Y + 20 + row * 12;
                setfillstyle(SOLID_FILL, piece_color[next_piece]);
                bar(x + 1, y + 1, x + 10, y + 10);
            }
}

static void draw_frame(void)
{
    cleardevice();
    setcolor(LIGHTGRAY);
    rectangle(BOARD_X - 3, BOARD_Y - 3, BOARD_X + COLS * CELL + 2, BOARD_Y + ROWS * CELL + 2);
    rectangle(SIDE_X, BOARD_Y + 14, SIDE_X + 58, BOARD_Y + 74);
    setcolor(WHITE);
    outtextxy(SIDE_X, BOARD_Y, "NEXT");

    settextstyle(DEFAULT_FONT, HORIZ_DIR, 3);
    setcolor(YELLOW);
    outtextxy(20, BOARD_Y, "TETRIS");
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    setcolor(LIGHTGRAY);
    outtextxy(20, BOARD_Y + 40, "Borland C++ 2.0");
    outtextxy(20, BOARD_Y + 52, "BGI on dosgolem");
    outtextxy(20, BOARD_Y + 100, "LEFT/RIGHT move");
    outtextxy(20, BOARD_Y + 114, "UP or X  rotate");
    outtextxy(20, BOARD_Y + 128, "Z   rotate left");
    outtextxy(20, BOARD_Y + 142, "DOWN  soft drop");
    outtextxy(20, BOARD_Y + 156, "SPACE hard drop");
    outtextxy(20, BOARD_Y + 170, "P pause  ESC quit");
    memset(shown, 0, sizeof shown);
}

/* 在盤面中央畫一行提示字，color 為 BLACK 時擦掉 */
static void banner(const char *msg, int color)
{
    int w = textwidth((char *)msg), x = BOARD_X + (COLS * CELL - w) / 2;
    int y = BOARD_Y + ROWS * CELL / 2;
    setfillstyle(SOLID_FILL, BLACK);
    bar(BOARD_X, y - 4, BOARD_X + COLS * CELL - 1, y + 12);
    if (color != BLACK) {
        setcolor(color);
        outtextxy(x, y, (char *)msg);
    } else {
        memset(shown[ROWS / 2 - 1], 0xFF, sizeof shown[0] * 2);   /* 讓那兩列下一次重畫 */
    }
}

static int spawn(void)
{
    cur = next_piece;
    next_piece = rand() % 7;
    rot = 0;
    px = 3;
    py = -1;
    draw_side();
    return fits(cur, rot, px, py);
}

/* 把落下中的方塊固定到盤面、消行、計分；回傳消了幾行 */
static int lock_piece(void)
{
    int row, col, r, cleared = 0;

    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++)
            if (cell_of(cur, rot, row, col) && py + row >= 0)
                board[py + row][px + col] = (unsigned char)piece_color[cur];

    for (row = ROWS - 1; row >= 0; row--) {
        for (col = 0; col < COLS && board[row][col]; col++)
            ;
        if (col < COLS)
            continue;
        for (r = row; r > 0; r--)
            memcpy(board[r], board[r - 1], COLS);
        memset(board[0], 0, COLS);
        cleared++;
        row++;                          /* 同一列再檢查一次：上面掉下來的可能也滿了 */
    }
    if (cleared) {
        score += (long)line_score[cleared] * level;
        lines += cleared;
        level = 1 + lines / 10;
    }
    return cleared;
}

/* 下落間隔（BIOS tick）：等級越高越快，最快每 tick 一格 */
static int drop_ticks(void)
{
    int t = 10 - level;
    return t < 1 ? 1 : t;
}

/* 等一個鍵，順便數迴圈次數當亂數種子：玩家按鍵的時機每次都不同 */
static int wait_key(unsigned *spin)
{
    while (!bioskey(1))
        (*spin)++;
    return bioskey(0) >> 8;
}

static int play(void)
{
    long last;
    int key, paused = 0;

    memset(board, 0, sizeof board);
    score = 0;
    lines = 0;
    level = 1;
    draw_frame();
    next_piece = rand() % 7;
    if (!spawn())
        return 0;
    last = biostime(0, 0L);

    for (;;) {
        if (bioskey(1)) {
            key = bioskey(0);
            if ((key >> 8) == SC_ESC)
                return 1;
            if ((key & 0xFF) == 'p' || (key & 0xFF) == 'P') {
                paused = !paused;
                banner("PAUSED", paused ? YELLOW : BLACK);
                last = biostime(0, 0L);
                continue;
            }
            if (paused)
                continue;
            switch (key >> 8) {
            case SC_LEFT:
                if (fits(cur, rot, px - 1, py)) px--;
                break;
            case SC_RIGHT:
                if (fits(cur, rot, px + 1, py)) px++;
                break;
            case SC_DOWN:
                if (fits(cur, rot, px, py + 1)) { py++; score++; }
                break;
            case SC_SPACE:
                while (fits(cur, rot, px, py + 1)) { py++; score += 2; }
                last = 0;                   /* 下一輪立刻固定 */
                break;
            default:
                if ((key >> 8) == SC_UP || (key & 0xFF) == 'x' || (key & 0xFF) == 'X' ||
                    (key & 0xFF) == 'z' || (key & 0xFF) == 'Z') {
                    int nr = ((key & 0xFF) == 'z' || (key & 0xFF) == 'Z') ? (rot + 3) % 4 : (rot + 1) % 4;
                    /* 靠牆時試著左右推一格再轉（簡單的踢牆） */
                    if (fits(cur, nr, px, py)) rot = nr;
                    else if (fits(cur, nr, px - 1, py)) { px--; rot = nr; }
                    else if (fits(cur, nr, px + 1, py)) { px++; rot = nr; }
                }
                break;
            }
        }
        if (!paused && biostime(0, 0L) - last >= drop_ticks()) {
            last = biostime(0, 0L);
            if (fits(cur, rot, px, py + 1)) {
                py++;
            } else {
                lock_piece();
                if (!spawn()) {
                    render();
                    return 0;
                }
                draw_side();
            }
        }
        if (!paused)
            render();
    }
}

int main(void)
{
    int gd = DETECT, gm, err, key;
    unsigned spin = 0;

    initgraph(&gd, &gm, "");
    err = graphresult();
    if (err != grOk) {
        printf("BGI error: %s\n", grapherrormsg(err));
        return 1;
    }

    draw_frame();
    banner("PRESS ANY KEY", WHITE);
    wait_key(&spin);
    srand(spin ^ (unsigned)biostime(0, 0L));

    while (play() == 0) {           /* 0 ＝ 疊到頂了 */
        banner("GAME OVER", LIGHTRED);
        setcolor(WHITE);
        outtextxy(BOARD_X + 20, BOARD_Y + ROWS * CELL / 2 + 20, "ENTER: again");
        outtextxy(BOARD_X + 20, BOARD_Y + ROWS * CELL / 2 + 34, "ESC:   quit");
        do {
            key = wait_key(&spin);
        } while (key != 0x1C && key != SC_ESC);
        if (key == SC_ESC)
            break;
    }
    closegraph();
    return 0;
}
