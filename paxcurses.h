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

#ifndef PAXCURSES_H
#define PAXCURSES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * 1. STANDARD NCURSES TYPES & CONSTANTS
 * ========================================================================== */
#define FALSE 0
#define TRUE  1

#define OK    0
#define ERR   (-1)

typedef uint32_t chtype;
typedef uint8_t  attr_t;

/* Standard 8 Baseline Colors */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

/* Text Attribute Bitmasks packed into PaxCell attributes */
#define A_NORMAL     0x00u
#define A_COLOR      0x0Fu  /* Bitmask for color pair index (0-15) */
#define A_BOLD       0x10u  /* High-intensity foreground / bold */
#define A_UNDERLINE  0x20u  /* Underline attribute */
#define A_REVERSE    0x40u  /* Reverse video mode */
#define A_BLINK      0x80u  /* Blinking text */

/* Standard NCURSES Macro Wrappers for Color Pairs */
#define COLOR_PAIR(n)     ((attr_t)((n) & A_COLOR))
#define PAIR_NUMBER(a)    ((int)((a) & A_COLOR))

/* Key definitions matching standard ncurses expectations */
#define KEY_MIN       0x100
#define KEY_UP        0x101
#define KEY_DOWN      0x102
#define KEY_RIGHT     0x103
#define KEY_LEFT      0x104
#define KEY_TAB       0x105  /* Crucial link jumping macro */
#define KEY_BACKSPACE 0x106  /* Safe deletion tracker */
#define KEY_ENTER     0x107  /* Multi-platform enter fallback */
#define KEY_PPAGE     0x108  /* Page Up tracking buffer */
#define KEY_NPAGE     0x109  /* Page Down tracking buffer */
#define KEY_HOME      0x10A  /* Return to coordinates [0,0] */
#define KEY_END       0x10B  /* Jump to end of string matrix */
#define KEY_RESIZE    0x10C  /* Native viewport adjustment hook */
#define KEY_MAX       0x1FF

/* Macro Helpers */
#define getmaxyx(win, y, x) do { (y) = (win) ? (win)->height : 0; (x) = (win) ? (win)->width : 0; } while(0)

/* Dynamic Scrollback Control Macros */
#define wset_virtual_height(win, vh)   pax_set_virtual_height((win), (vh))
#define wget_virtual_height(win)       ((win) ? (win)->virtual_height : 0)
#define wreserve_scrollback(win, lines) pax_reserve_scrollback((win), (lines))
#define resize(l, c) resizeterm((l), (c))

/* Global window pointers and screen metrics */
extern struct _pax_win *stdscr;
extern int COLS;
extern int LINES;

/* ==========================================================================
 * 2. PAX INTERNAL STRUCTURES (CLOSE TO THE METAL)
 * ========================================================================== */
typedef struct {
    uint32_t ch;     /* Full 32-bit UTF-8 codepoint support */
    uint8_t  attr;   /* Bit 0-3: Color Pair index, Bit 4-7: Text Effects */
} PaxCell;

typedef struct _pax_win {
    int x, y;            /* Window physical screen offset */
    int width, height;   /* Window physical viewport bounds */
    int cur_x, cur_y;    /* Virtual cursor position tracking */

    uint8_t active_attr;
    
    /* Native Pax Viewport Engine (Used when is_pad == false) */
    int scroll_y;        /* Vertical scroll offset into virtual buffer */
    int virtual_height;  /* Total allocated virtual rows */

    /* Ncurses Compatibility Engine Guard & Settings */
    bool is_pad;         /* True: Pad API controls rendering */
    bool nodelay;        /* True: Non-blocking input mode for wgetch */

    PaxCell *buffer;     /* Virtual canvas (Flat 1D layout: y * width + x) */
    PaxCell *old_buffer; /* Historical canvas state (Used for double-buffer diffs) */
} WINDOW;

/* ==========================================================================
 * 3. COMPACT NCURSES-COMPATIBLE FUNCTION API
 * ========================================================================== */

/* Engine Life Cycle */
WINDOW  *initscr(void);
int      endwin(void);
int      cbreak(void);
int      nocbreak(void);
int      echo(void);
int      noecho(void);
int      keypad(WINDOW *win, int bf);
int      curs_set(int visibility);
int      has_colors(void);
int      start_color(void);

/* Core Drawing & Screen Updates */
int      refresh(void);
int      wrefresh(WINDOW *win);
int      clear(void);
int      wclear(WINDOW *win);
int      erase(void);
int      werase(WINDOW *win);
int      wresize(WINDOW *win, int lines, int columns);
int      resizeterm(int lines, int columns);

/* Coordinate and State Manipulation */
int      move(int y, int x);
int      wmove(WINDOW *win, int y, int x);
int      attron(int attrs);
int      attroff(int attrs);
int      wattron(WINDOW *win, int attrs);
int      wattroff(WINDOW *win, int attrs);
int      attrset(int attrs);
int      wattrset(WINDOW *win, int attrs);
int      init_pair(int16_t pair, int16_t f, int16_t b);

/* Character & String Injection */
int      addch(const chtype ch);
int      waddch(WINDOW *win, const chtype ch);
int      mvaddch(int y, int x, const chtype ch);
int      mvwaddch(WINDOW *win, int y, int x, const chtype ch);

int      addstr(const char *str);
int      waddstr(WINDOW *win, const char *str);
int      mvaddstr(int y, int x, const char *str);
int      mvwaddstr(WINDOW *win, int y, int x, const char *str);

int      printw(const char *fmt, ...);
int      wprintw(WINDOW *win, const char *fmt, ...);
int      mvprintw(int y, int x, const char *fmt, ...);
int      mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);

/* Wide Character Utilities */
int      box(WINDOW *win, uint32_t verch, uint32_t horch);

/* Dynamic Canvas & Scrollback Management */
int      pax_set_virtual_height(WINDOW *win, int new_vh);
int      pax_reserve_scrollback(WINDOW *win, int additional_lines);

/* Native Pax Viewport Navigation Helpers (Ignored on Pads) */
int      wscroll_up(WINDOW *win, int n);
int      wscroll_down(WINDOW *win, int n);

/* Input Telemetry & Settings */
int      getch(void);
int      wgetch(WINDOW *win);
int      nodelay(WINDOW *win, bool bf);

/* Window & Pad Management */
WINDOW  *newwin(int nlines, int ncols, int begin_y, int begin_x);
int      delwin(WINDOW *win);

WINDOW  *newpad(int nlines, int ncols);
WINDOW  *subpad(WINDOW *orig, int nlines, int ncols, int begin_y, int begin_x);
int      prefresh(WINDOW *pad, int pminrow, int pmincol, 
                  int sminrow, int smincol, int smaxrow, int smaxcol);
int      pnoutrefresh(WINDOW *pad, int pminrow, int pmincol, 
                      int sminrow, int smincol, int smaxrow, int smaxcol);
int      pechochar(WINDOW *pad, const chtype ch);

#ifdef __cplusplus
}
#endif

#endif /* PAXCURSES_H */