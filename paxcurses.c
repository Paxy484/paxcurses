/*
 * Copyright 2026 Pax Project
 *
 * Licensed under the Pax Public License, Version 5.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: LicenseRef-Pax-5.0
 *
 * You may obtain a copy of the full License text within the Source or Object 
 * (or Binary) form materials provided with this distribution (typically in a 
 * "LICENSE" file, or "LICENSE-PAX" if there is more than one file named "LICENSE").
 */

#include "paxcurses.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Global runtime variables exposed via the header link */
WINDOW *stdscr = NULL;
int COLS = 80;
int LINES = 24;

/* 
 * 16-Color Attribute Memory Mapping Configuration Tracker 
 * Tracks maximum of 16 color pairs.
 */
static struct {
    uint8_t fg;
    uint8_t bg;
} color_pairs[16];

/* ==========================================================================
 * PLATFORM DETECTOR & RAW SYSTEM REGISTER WRAPPERS
 * ========================================================================== */
#if defined(__WATCOMC__) || defined(__DOS__)
    #define DOS_PLATFORM
    #include <conio.h>
    /* Direct VGA Text Mode Memory Address (80x25 / Flat 32-Bit Linear Address) */
    #define DOS_VGA_FAR_PTR ((unsigned char __far *)0x000B8000L)
#elif defined(_WIN32) || defined(__CYGWIN__)
    #define WIN32_PLATFORM
    #include <windows.h>
    #include <conio.h>
    static HANDLE hStdout = INVALID_HANDLE_VALUE;
    static HANDLE hStdin  = INVALID_HANDLE_VALUE;
    static DWORD origStdoutMode = 0;
    static DWORD origStdinMode  = 0;
#else
    #define POSIX_PLATFORM
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    static struct termios orig_termios;
#endif

/* Universal exit cleanup: Ensures terminal state is restored cleanly */
static void p_clean_exit(void) {
#if defined(POSIX_PLATFORM)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h\033[0m\n");
    fflush(stdout);
#elif defined(WIN32_PLATFORM)
    if (hStdout != INVALID_HANDLE_VALUE) SetConsoleMode(hStdout, origStdoutMode);
    if (hStdin != INVALID_HANDLE_VALUE)  SetConsoleMode(hStdin, origStdinMode);
    printf("\033[?25h\033[0m\n");
    fflush(stdout);
#endif
}

/* Zero-allocation UTF-8 encoder flushing raw bytes close to the silicon */
static inline void pax_putchar_utf8(uint32_t cp) {
#if defined(DOS_PLATFORM)
    putchar((char)(cp & 0xFF));
#else
    if (cp <= 0x7F) {
        putchar((char)cp);
    } else if (cp <= 0x7FF) {
        putchar((char)(0xC0 | ((cp >> 6) & 0x1F)));
        putchar((char)(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        putchar((char)(0xE0 | ((cp >> 12) & 0x0F)));
        putchar((char)(0x80 | ((cp >> 6) & 0x3F)));
        putchar((char)(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        putchar((char)(0xF0 | ((cp >> 18) & 0x07)));
        putchar((char)(0x80 | ((cp >> 12) & 0x3F)));
        putchar((char)(0x80 | ((cp >> 6) & 0x3F)));
        putchar((char)(0x80 | (cp & 0x3F)));
    }
#endif
}

/* Helper function to map packed uint8_t attributes into 16-color ANSI space */
static void apply_pax_attributes(uint8_t attr) {
#if !defined(DOS_PLATFORM)
    uint8_t pair = attr & 0x0F;
    uint8_t style = attr & 0xF0;

    uint8_t fg = (pair > 0) ? color_pairs[pair].fg : COLOR_WHITE;
    uint8_t bg = (pair > 0) ? color_pairs[pair].bg : COLOR_BLACK;

    uint8_t ansi_fg = (style & A_BOLD) ? (fg + 90) : (fg + 30);
    uint8_t ansi_bg = (style & A_BOLD) ? (bg + 100) : (bg + 40);

    printf("\033[0m\033[%d;%dm", ansi_fg, ansi_bg);

    if (style & A_UNDERLINE) printf("\033[4m");
    if (style & A_REVERSE)   printf("\033[7m");
    if (style & A_BLINK)     printf("\033[5m");
#endif
}

/* ==========================================================================
 * CORE ENGINE API IMPLEMENTATION
 * ========================================================================== */

WINDOW *initscr(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

#if defined(POSIX_PLATFORM)
    if (!isatty(STDIN_FILENO)) return NULL;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return NULL;
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return NULL;

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        COLS = ws.ws_col;
        LINES = ws.ws_row;
    }
#elif defined(WIN32_PLATFORM)
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin  = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE || hStdin == INVALID_HANDLE_VALUE) return NULL;

    GetConsoleMode(hStdout, &origStdoutMode);
    GetConsoleMode(hStdin, &origStdinMode);

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    DWORD rawStdout = origStdoutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    DWORD rawStdin  = origStdinMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

    SetConsoleMode(hStdout, rawStdout);
    SetConsoleMode(hStdin, rawStdin);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        COLS = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        LINES = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#elif defined(DOS_PLATFORM)
    COLS = 80;
    LINES = 25;
#endif

    atexit(p_clean_exit);

    stdscr = newwin(LINES, COLS, 0, 0);
    if (!stdscr) return NULL;

#if !defined(DOS_PLATFORM)
    printf("\033[2J\033[?25l\033[H");
#endif

    return stdscr;
}

int endwin(void) {
    p_clean_exit();
    return OK;
}

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x) {
    if (nlines <= 0) nlines = LINES - begin_y;
    if (ncols <= 0)  ncols  = COLS - begin_x;

    WINDOW *win = (WINDOW *)malloc(sizeof(WINDOW));
    if (!win) return NULL;

    win->x = begin_x;
    win->y = begin_y;
    win->width = ncols;
    win->height = nlines;
    win->cur_x = 0;
    win->cur_y = 0;
    win->scroll_y = 0;
    win->virtual_height = nlines;
    win->is_pad = false;
    win->nodelay = false;
    win->active_attr = A_NORMAL;

    size_t cell_count = (size_t)ncols * nlines;
    win->buffer = (PaxCell *)malloc(cell_count * sizeof(PaxCell));
    win->old_buffer = (PaxCell *)malloc(cell_count * sizeof(PaxCell));

    if (!win->buffer || !win->old_buffer) {
        free(win->buffer);
        free(win->old_buffer);
        free(win);
        return NULL;
    }

    for (size_t i = 0; i < cell_count; i++) {
        win->buffer[i].ch = ' ';
        win->buffer[i].attr = A_NORMAL;
        win->old_buffer[i].ch = 0;
        win->old_buffer[i].attr = 0xFF;
    }

    return win;
}

int delwin(WINDOW *win) {
    if (!win) return ERR;
    if (win == stdscr) stdscr = NULL;
    free(win->buffer);
    free(win->old_buffer);
    free(win);
    return OK;
}

/* ==========================================================================
 * PAD ENGINE IMPLEMENTATION
 * ========================================================================== */

WINDOW *newpad(int nlines, int ncols) {
    if (nlines <= 0 || ncols <= 0) return NULL;

    WINDOW *pad = (WINDOW *)malloc(sizeof(WINDOW));
    if (!pad) return NULL;

    pad->x = 0;
    pad->y = 0;
    pad->width = ncols;
    pad->height = nlines;
    pad->cur_x = 0;
    pad->cur_y = 0;
    pad->scroll_y = 0;
    pad->virtual_height = nlines;
    pad->is_pad = true;
    pad->nodelay = false;
    pad->active_attr = A_NORMAL;

    size_t cell_count = (size_t)ncols * nlines;
    pad->buffer = (PaxCell *)malloc(cell_count * sizeof(PaxCell));
    pad->old_buffer = (PaxCell *)malloc(cell_count * sizeof(PaxCell));

    if (!pad->buffer || !pad->old_buffer) {
        free(pad->buffer);
        free(pad->old_buffer);
        free(pad);
        return NULL;
    }

    for (size_t i = 0; i < cell_count; i++) {
        pad->buffer[i].ch = ' ';
        pad->buffer[i].attr = A_NORMAL;
        pad->old_buffer[i].ch = 0;
        pad->old_buffer[i].attr = 0xFF;
    }

    return pad;
}

WINDOW *subpad(WINDOW *orig, int nlines, int ncols, int begin_y, int begin_x) {
    (void)begin_y;
    (void)begin_x;
    if (!orig || !orig->is_pad) return NULL;
    return newpad(nlines, ncols);
}

int pnoutrefresh(WINDOW *pad, int pminrow, int pmincol, 
                 int sminrow, int smincol, int smaxrow, int smaxcol) {
    if (!pad || !pad->is_pad || !stdscr) return ERR;

    for (int sy = sminrow; sy <= smaxrow; sy++) {
        int py = pminrow + (sy - sminrow);
        if (py < 0 || py >= pad->height) continue;
        if (sy < 0 || sy >= stdscr->height) continue;

        for (int sx = smincol; sx <= smaxcol; sx++) {
            int px = pmincol + (sx - smincol);
            if (px < 0 || px >= pad->width) continue;
            if (sx < 0 || sx >= stdscr->width) continue;

            size_t pad_idx = (size_t)py * pad->width + px;
            size_t scr_idx = (size_t)sy * stdscr->width + sx;

            stdscr->buffer[scr_idx] = pad->buffer[pad_idx];
        }
    }

    return OK;
}

int prefresh(WINDOW *pad, int pminrow, int pmincol, 
             int sminrow, int smincol, int smaxrow, int smaxcol) {
    if (pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol) == ERR) {
        return ERR;
    }
    return refresh();
}

int pechochar(WINDOW *pad, const chtype ch) {
    if (!pad || !pad->is_pad) return ERR;
    if (waddch(pad, ch) == ERR) return ERR;
    return prefresh(pad, pad->cur_y, pad->cur_x, 0, 0, pad->height - 1, pad->width - 1);
}

/* ==========================================================================
 * RENDERING ENGINE (VIEWPORT & DOUBLE-BUFFER DIFF)
 * ========================================================================== */

int wrefresh(WINDOW *win) {
    if (!win) return ERR;

    /* Pads must be refreshed strictly via prefresh() or pnoutrefresh() */
    if (win->is_pad) return ERR;

#if defined(DOS_PLATFORM)
    unsigned char __far *vga = DOS_VGA_FAR_PTR;

    for (int v_y = 0; v_y < win->height; v_y++) {
        int abs_y = win->scroll_y + v_y;
        if (abs_y >= win->virtual_height) break;

        for (int x = 0; x < win->width; x++) {
            size_t virt_idx = (size_t)abs_y * win->width + x;
            size_t view_idx = (size_t)v_y * win->width + x;

            PaxCell live = win->buffer[virt_idx];
            PaxCell old  = win->old_buffer[view_idx];

            if (live.ch == old.ch && live.attr == old.attr) continue;

            size_t vga_offset = ((win->y + v_y) * COLS + (win->x + x)) * 2;
            uint8_t pair = live.attr & 0x0F;
            uint8_t vga_attr = ((color_pairs[pair].bg & 0x07) << 4) | (color_pairs[pair].fg & 0x0F);

            vga[vga_offset]     = (unsigned char)(live.ch & 0xFF);
            vga[vga_offset + 1] = vga_attr;

            win->old_buffer[view_idx] = live;
        }
    }
#else
    uint8_t current_rendering_attr = 0xFF;
    int last_x = -1;
    int last_y = -1;

    for (int v_y = 0; v_y < win->height; v_y++) {
        int abs_y = win->scroll_y + v_y;
        if (abs_y >= win->virtual_height) break;

        for (int x = 0; x < win->width; x++) {
            size_t virt_idx = (size_t)abs_y * win->width + x;
            size_t view_idx = (size_t)v_y * win->width + x;

            PaxCell live = win->buffer[virt_idx];
            PaxCell old  = win->old_buffer[view_idx];

            if (live.ch == old.ch && live.attr == old.attr) continue;

            int target_screen_x = win->x + x + 1;
            int target_screen_y = win->y + v_y + 1;

            if (last_y != target_screen_y || last_x != target_screen_x) {
                printf("\033[%d;%dH", target_screen_y, target_screen_x);
            }

            if (live.attr != current_rendering_attr) {
                apply_pax_attributes(live.attr);
                current_rendering_attr = live.attr;
            }

            pax_putchar_utf8(live.ch);

            last_x = target_screen_x + 1;
            last_y = target_screen_y;

            win->old_buffer[view_idx] = live;
        }
    }

    int screen_cursor_y = win->cur_y - win->scroll_y;
    if (screen_cursor_y >= 0 && screen_cursor_y < win->height) {
        printf("\033[%d;%dH", win->y + screen_cursor_y + 1, win->x + win->cur_x + 1);
    }
    fflush(stdout);
#endif

    return OK;
}

int refresh(void) { return wrefresh(stdscr); }

int wclear(WINDOW *win) {
    if (!win) return ERR;
    size_t cell_count = (size_t)win->width * win->virtual_height;
    for (size_t i = 0; i < cell_count; i++) {
        win->buffer[i].ch = ' ';
        win->buffer[i].attr = A_NORMAL;
    }
    win->cur_x = 0;
    win->cur_y = 0;
    win->scroll_y = 0;
    return OK;
}

int clear(void)  { return wclear(stdscr); }
int werase(WINDOW *win) { return wclear(win); }

/* ==========================================================================
 * TEXT INJECTION & STATE MANIPULATION
 * ========================================================================== */

int wmove(WINDOW *win, int y, int x) {
    if (!win || x < 0 || x >= win->width || y < 0 || y >= win->virtual_height) return ERR;
    win->cur_x = x;
    win->cur_y = y;
    return OK;
}

int move(int y, int x) { return wmove(stdscr, y, x); }

int wattron(WINDOW *win, int attrs) {
    WINDOW *target = win ? win : stdscr;
    if (!target) return ERR;

    if ((attrs & 0x0F) != 0) {
        target->active_attr = (target->active_attr & 0xF0) | (attrs & 0x0F);
    }
    target->active_attr |= (attrs & 0xF0);
    return OK;
}

int wattroff(WINDOW *win, int attrs) {
    WINDOW *target = win ? win : stdscr;
    if (!target) return ERR;

    if ((attrs & 0x0F) != 0) {
        target->active_attr &= 0xF0;
    }
    target->active_attr &= ~(attrs & 0xF0);
    return OK;
}

int wattrset(WINDOW *win, int attrs) {
    WINDOW *target = win ? win : stdscr;
    if (!target) return ERR;

    target->active_attr = (uint8_t)attrs;
    return OK;
}

int attron(int attrs) { return wattron(stdscr, attrs); }

int attroff(int attrs) { return wattroff(stdscr, attrs); }

int attrset(int attrs) { return wattrset(stdscr, attrs); }

int init_pair(int16_t pair, int16_t f, int16_t b) {
    if (pair < 0 || pair >= 16) return ERR;
    color_pairs[pair].fg = (uint8_t)(f & 0x07);
    color_pairs[pair].bg = (uint8_t)(b & 0x07);
    return OK;
}

int waddch(WINDOW *win, const chtype ch) {
    if (!win) return ERR;

    if (win->cur_x < 0 || win->cur_x >= win->width || 
        win->cur_y < 0 || win->cur_y >= win->virtual_height) {
        return ERR;
    }

    if (ch == '\n') {
        win->cur_x = 0;
        win->cur_y++;
        if (!win->is_pad && win->cur_y >= win->scroll_y + win->height) {
            win->scroll_y = win->cur_y - win->height + 1;
        }
        return OK;
    }

    size_t idx = (size_t)win->cur_y * win->width + win->cur_x;
    win->buffer[idx].ch = ch;
    win->buffer[idx].attr = win->active_attr;

    win->cur_x++;
    if (win->cur_x >= win->width) {
        win->cur_x = 0;
        win->cur_y++;
        if (!win->is_pad && win->cur_y >= win->scroll_y + win->height) {
            win->scroll_y = win->cur_y - win->height + 1;
        }
    }

    return OK;
}

int addch(const chtype ch) { return waddch(stdscr, ch); }
int mvaddch(int y, int x, const chtype ch) { if (move(y, x) == ERR) return ERR; return waddch(stdscr, ch); }
int mvwaddch(WINDOW *win, int y, int x, const chtype ch) { if (wmove(win, y, x) == ERR) return ERR; return waddch(win, ch); }

int waddstr(WINDOW *win, const char *str) {
    if (!win || !str) return ERR;
    while (*str) {
        if (waddch(win, (chtype)(unsigned char)*str++) == ERR) return ERR;
    }
    return OK;
}

int addstr(const char *str) { return waddstr(stdscr, str); }
int mvaddstr(int y, int x, const char *str) { if (move(y, x) == ERR) return ERR; return waddstr(stdscr, str); }
int mvwaddstr(WINDOW *win, int y, int x, const char *str) { if (wmove(win, y, x) == ERR) return ERR; return waddstr(win, str); }

static int p_vprintw_engine(WINDOW *win, const char *fmt, va_list args) {
    char temp_str_buf[1024];
    int printed_len = vsnprintf(temp_str_buf, sizeof(temp_str_buf), fmt, args);
    if (printed_len < 0) return ERR;

    for (int i = 0; temp_str_buf[i] != '\0'; i++) {
        if (waddch(win, (chtype)(unsigned char)temp_str_buf[i]) == ERR) return ERR;
    }
    return OK;
}

int wprintw(WINDOW *win, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = p_vprintw_engine(win, fmt, args);
    va_end(args);
    return res;
}

int printw(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = p_vprintw_engine(stdscr, fmt, args);
    va_end(args);
    return res;
}

int mvprintw(int y, int x, const char *fmt, ...) {
    if (move(y, x) == ERR) return ERR;
    va_list args;
    va_start(args, fmt);
    int res = p_vprintw_engine(stdscr, fmt, args);
    va_end(args);
    return res;
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...) {
    if (wmove(win, y, x) == ERR) return ERR;
    va_list args;
    va_start(args, fmt);
    int res = p_vprintw_engine(win, fmt, args);
    va_end(args);
    return res;
}

/* ==========================================================================
 * EXTENSIONS & HELPERS
 * ========================================================================== */

int nodelay(WINDOW *win, bool bf) {
    if (!win) return ERR;
    win->nodelay = bf;
    return OK;
}

int box(WINDOW *win, uint32_t verch, uint32_t horch) {
    if (!win) return ERR;

    if (verch == 0) verch = 0x2502; /* '│' */
    if (horch == 0) horch = 0x2500; /* '─' */

    int w = win->width;
    int h = win->height;

    for (int x = 1; x < w - 1; x++) {
        mvwaddch(win, 0, x, horch);
        mvwaddch(win, h - 1, x, horch);
    }
    for (int y = 1; y < h - 1; y++) {
        mvwaddch(win, y, 0, verch);
        mvwaddch(win, y, w - 1, verch);
    }

    mvwaddch(win, 0, 0, 0x250C);         /* '┌' */
    mvwaddch(win, 0, w - 1, 0x2510);     /* '┐' */
    mvwaddch(win, h - 1, 0, 0x2514);     /* '└' */
    mvwaddch(win, h - 1, w - 1, 0x2518); /* '┘' */

    return OK;
}

int wscroll_up(WINDOW *win, int n) {
    if (!win || win->is_pad) return ERR;
    win->scroll_y -= n;
    if (win->scroll_y < 0) win->scroll_y = 0;
    return OK;
}

int wscroll_down(WINDOW *win, int n) {
    if (!win || win->is_pad) return ERR;
    int max_scroll = win->virtual_height - win->height;
    if (max_scroll < 0) max_scroll = 0;
    win->scroll_y += n;
    if (win->scroll_y > max_scroll) win->scroll_y = max_scroll;
    return OK;
}

/* ==========================================================================
 * UNBUFFERED INPUT PROCESSING PIPELINE
 * ========================================================================== */

int wgetch(WINDOW *win) {
    WINDOW *target_win = win ? win : stdscr;
    bool is_nonblocking = target_win ? target_win->nodelay : false;
    int c = ERR;

#if defined(POSIX_PLATFORM)
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    if (is_nonblocking) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) {
            return ERR;
        }
    }

    char ch;
    ssize_t bytes_read = read(STDIN_FILENO, &ch, 1);
    if (bytes_read <= 0) return ERR;
    c = (unsigned char)ch;

    if (c == 27) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) > 0 && read(STDIN_FILENO, &seq[1], 1) > 0) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': c = KEY_UP;    break;
                        case 'B': c = KEY_DOWN;  break;
                        case 'C': c = KEY_RIGHT; break;
                        case 'D': c = KEY_LEFT;  break;
                    }
                }
            }
        }
    }
#elif defined(WIN32_PLATFORM) || defined(DOS_PLATFORM)
    if (is_nonblocking) {
        if (!_kbhit()) return ERR;
    } else {
        while (!_kbhit()) {}
    }

    c = _getch();
    if (c == 0 || c == 0xE0 || c == 224) {
        int second_byte = _getch();
        switch (second_byte) {
            case 72: c = KEY_UP;    break;
            case 80: c = KEY_DOWN;  break;
            case 77: c = KEY_RIGHT; break;
            case 75: c = KEY_LEFT;  break;
        }
    }
#endif

    return c;
}

int getch(void) { return wgetch(stdscr); }

int cbreak(void)                { return OK; }
int nocbreak(void)              { return OK; }
int echo(void)                  { return OK; }
int noecho(void)                { return OK; }
int keypad(WINDOW *win, int bf) { (void)win; (void)bf; return OK; }
int curs_set(int visibility)    { (void)visibility; return OK; }
int start_color(void)           { return OK; }
int has_colors(void)            { return TRUE; }