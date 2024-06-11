/* Life by Dmitrii Anastasiadi <dmitrii.anastasiadi@yandex.ru> 2024
 *
 * Based on Conway's Life
 * 
 * https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
 * 
 * Based on several examples from the hacks directory of: 
 * xscreensaver, Copyright (c) 1992-2024 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "screenhack.h"
#include <time.h>

#define BLOCK_SIZE (int)10

struct field {
  __uint8_t** matrix;
  int rows;
  int cols;
};

struct state {
  GC draw_gc;
  int sleep_time;
  int xlim, ylim;
  Bool clock;
  struct field field;
};

static __uint8_t digits[10][7] = {
  { 1, 1, 1, 1, 1, 1, 0 }, // 0
  { 0, 1, 1, 0, 0, 0, 0 }, // 1
  { 1, 1, 0, 1, 1, 0, 1 }, // 2
  { 1, 1, 1, 1, 0, 0, 1 }, // 3
  { 0, 1, 1, 0, 0, 1, 1 }, // 4
  { 1, 0, 1, 1, 0, 1, 1 }, // 5
  { 1, 0, 1, 1, 1, 1, 1 }, // 6
  { 1, 1, 1, 0, 0, 0, 0 }, // 7
  { 1, 1, 1, 1, 1, 1, 1 }, // 8
  { 1, 1, 1, 1, 0, 1, 1 }  // 9
};

static void* life_init (Display *dpy, Window window) {
  struct state *st = (struct state *)calloc(1, sizeof(*st));
  XGCValues gcv;
  Colormap cmap;
  XWindowAttributes xgwa;
  XGetWindowAttributes (dpy, window, &xgwa);
  
  cmap = xgwa.colormap;
  gcv.foreground = get_pixel_resource (dpy, cmap, "foreground", "Foreground");
  gcv.background = get_pixel_resource (dpy, cmap, "background", "Background");
  st->draw_gc = XCreateGC (dpy, window, GCForeground, &gcv);
  st->sleep_time = get_integer_resource (dpy, "delay", "Delay");
  st->clock  = get_boolean_resource(dpy, "clock", "Boolean");
  st->xlim = xgwa.width;
  st->ylim = xgwa.height;
  st->field.cols = xgwa.width / BLOCK_SIZE;
  st->field.rows = xgwa.height / BLOCK_SIZE;

  if (st->clock) {
    st->field.cols -= 26;
    st->field.rows -= 28;
  }
  
  st->field.matrix = (__uint8_t**)malloc(st->field.cols * sizeof(__uint8_t*));
  for (int i = 0; i < st->field.cols; i++) {
    st->field.matrix[i] = (__uint8_t*)malloc(st->field.rows * sizeof(__uint8_t));
    for (int j = 0; j < st->field.rows; j++) {
      st->field.matrix[i][j] = random() % 2;
    }
  }
  return st;
}

static void life_reshape (Display *dpy, Window window, void *closure,
  unsigned int width, unsigned int height) {
}

static void life_draw_field(Display *dpy, Window window, void *closure, int x, int y) {
  struct state *st = (struct state *) closure;
  // draw edging
  if (st->clock) {
    XFillRectangle(dpy, window, st->draw_gc,
      x - BLOCK_SIZE,
      y - BLOCK_SIZE,
      (st->field.cols + 2) * BLOCK_SIZE,
      BLOCK_SIZE);
    XFillRectangle(dpy, window, st->draw_gc,
      x - BLOCK_SIZE + (st->field.cols + 1) * BLOCK_SIZE,
      y - BLOCK_SIZE,
      BLOCK_SIZE,
      (st->field.rows + 2) * BLOCK_SIZE);
    XFillRectangle(dpy, window, st->draw_gc,
      x - BLOCK_SIZE,
      y - BLOCK_SIZE + (st->field.rows + 1) * BLOCK_SIZE,
      (st->field.cols + 2) * BLOCK_SIZE,
      BLOCK_SIZE);
    XFillRectangle(dpy, window, st->draw_gc,
      x - BLOCK_SIZE,
      y - BLOCK_SIZE,
      BLOCK_SIZE,
      (st->field.rows + 2) * BLOCK_SIZE);
  }
  // draw field
  XRectangle *rectangles = (XRectangle*)malloc(st->field.cols * st->field.rows * sizeof(XRectangle));
  int rectangles_count = 0;
  for (int i = 0; i < st->field.cols; i++) {
    for (int j = 0; j < st->field.rows; j++) {
      if (st->field.matrix[i][j]) {
        rectangles[rectangles_count++] =
          (XRectangle){ BLOCK_SIZE * i + x, BLOCK_SIZE * j + y, BLOCK_SIZE, BLOCK_SIZE };
      }
    }
  }
  XFillRectangles(dpy, window, st->draw_gc, rectangles, rectangles_count);
  free(rectangles);
}

static int get_number_of_neighbors(struct field field, int x, int y) {
  // n + 1 = (n + 1) % Nlen
  // n - 1 = (n - 1 + Nlen) % Nlen
  int res = 0;
  for (int i = x - 1; i <= x + 1; i++) {
    for (int j = y - 1; j <= y + 1; j++) {
      int cp_i = i, cp_j = j;
      cp_i = (i + field.cols) % field.cols;
      cp_j = (j + field.rows) % field.rows;
      if (y != j || x != i) {
        res += field.matrix[cp_i][cp_j];
      }
    }
  }
  return res;
}

static void simulate_one_iteration(struct state* st) {
  __uint8_t** matrix_copy = (__uint8_t**)malloc(st->field.cols * sizeof(__uint8_t*));
  for (int i = 0; i < st->field.cols; i++) {
    matrix_copy[i] = (__uint8_t*)malloc(st->field.rows * sizeof(__uint8_t));
    for (int j = 0; j < st->field.rows; j++) {
      matrix_copy[i][j] = st->field.matrix[i][j];
    }
  }
  struct field field_copy = { matrix_copy, st->field.rows, st->field.cols };

  for (int i = 0; i < st->field.cols; i++) {
    for (int j = 0; j < st->field.rows; j++) {
      int number_of_neighbors = get_number_of_neighbors(field_copy, i, j);
      st->field.matrix[i][j] &= (number_of_neighbors == 2);
      st->field.matrix[i][j] |= (number_of_neighbors == 3);
    }
  }

  for (int i = 0; i < field_copy.cols; i++) {
    free(field_copy.matrix[i]);
  }
  free(field_copy.matrix);
}

static void draw_segment(Display *dpy, Window window, void *closure, int x,
                         int y, int segment_number) {
  int width = BLOCK_SIZE;
  int height = width * 5;
  struct state *st = (struct state *)closure;
  switch (segment_number) {
  case 0:
    XFillRectangle(dpy, window, st->draw_gc,
                   x, y, height, width);
    break;
  case 1:
    XFillRectangle(dpy, window, st->draw_gc,
                   x + height - width, y, width, height);
    break;
  case 2:
    XFillRectangle(dpy, window, st->draw_gc,
                   x + height - width, y + height - width, width, height);
    break;
  case 3:
    XFillRectangle(dpy, window, st->draw_gc,
                   x, y + (height - width) * 2, height, width);
    break;
  case 4:
    XFillRectangle(dpy, window, st->draw_gc,
                   x, y + height - width, width, height);
    break;
  case 5:
    XFillRectangle(dpy, window, st->draw_gc,
                   x, y, width, height);
    break;
  case 6:
    XFillRectangle(dpy, window, st->draw_gc,
                   x, y + height - width, height, width);
    break;
  default:
    break;
  }
}

static void draw_digit(Display *dpy, Window window, void *closure,
                       int x, int y, int digit) {
  for (int i = 0; i < 7; i++) {
    if (digits[digit][i]) {
      draw_segment(dpy, window, closure, x, y, i);
    }
  }
}

static void draw_clock(Display *dpy, Window window, void *closure,
                       int x, int y) {
  time_t now = time(0);
  struct tm* now_tm = localtime(&now);
  int hours = now_tm->tm_hour;
  int minutes = now_tm->tm_min;
    
  struct state *st = (struct state *) closure;
  int digit_width = 5 * BLOCK_SIZE;
  int margin = BLOCK_SIZE;
  draw_digit(dpy, window, closure, x, y,  hours / 10);
  draw_digit(dpy, window, closure, x + digit_width + margin, y, hours % 10);
  XFillRectangle(dpy, window, st->draw_gc,
    x + (digit_width + margin)*2,
    y + 2*BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
  XFillRectangle(dpy, window, st->draw_gc,
    x + (digit_width + margin)*2,
    y + 6*BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
  draw_digit(dpy, window, closure,
    x + (digit_width + margin) * 2 + margin * 2, y, minutes / 10);
  draw_digit(dpy, window, closure,
    x + (digit_width + margin) * 3 + margin * 2, y, minutes % 10);
}

static unsigned long life_draw(Display *dpy, Window window, void *closure) {
  struct state *st = (struct state *) closure;
  XClearWindow (dpy, window);
  if (st->clock) {
    int clock_size = 25 * BLOCK_SIZE;    
    draw_clock(dpy, window, closure,(st->xlim - clock_size) / 2, 5 * BLOCK_SIZE);
    life_draw_field(dpy, window, closure, 13 * BLOCK_SIZE, 20 * BLOCK_SIZE);
  } else {
    life_draw_field(dpy, window, closure, 0, 0);
  }
  simulate_one_iteration(st);
  
  return st->sleep_time;
}

static Bool life_event(Display *dpy, Window window, void *closure, XEvent *event) {
  return False;
}

static void life_free(Display *dpy, Window window, void *closure) {
  struct state *st = (struct state *)closure;
  for (int i = 0; i < st->field.cols; i++) {
    free(st->field.matrix[i]);
  }
  free(st->field.matrix);
  XFreeGC (dpy, st->draw_gc);
  free (st);
}


static const char *life_defaults [] = {
  ".background: black",
  ".foreground: green",
  "*clock:      False",
  "*delay:      100000",
#ifdef HAVE_MOBILE
  "*ignoreRotation: True",
#endif
  0
};

static XrmOptionDescRec life_options [] = {
  { "-delay", ".delay", XrmoptionSepArg, 0      },
  { "-clock", ".clock", XrmoptionNoArg,  "True" },
  { 0, 0, 0, 0 }
};

XSCREENSAVER_MODULE ("Life", life)
