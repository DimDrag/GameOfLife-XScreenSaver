/* xscreensaver, Copyright (c) 1992-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * 19971004: Johannes Keukelaar <johannes@nada.kth.se>: Use helix screen
 *           eraser.
 */

#include "screenhack.h"
#include <time.h>

struct field {
  __uint8_t* matrix;
  __uint8_t* new_matrix;
  int rows;
  int cols;
};

struct state {
  GC draw_gc;
  int sleep_time;
  int xlim, ylim;
  int cell_size;
  struct field field;
};

static void *
life_init (Display *dpy, Window window)
{
  struct state *st = (struct state *) calloc (1, sizeof(*st));
  XGCValues gcv;
  Colormap cmap;
  XWindowAttributes xgwa;
  XGetWindowAttributes (dpy, window, &xgwa);
  
  cmap = xgwa.colormap;
  gcv.foreground = get_pixel_resource (dpy, cmap, "foreground", "Foreground");
  gcv.background = get_pixel_resource (dpy, cmap, "background", "Background");
  st->draw_gc = XCreateGC (dpy, window, GCForeground, &gcv);
  st->sleep_time = get_integer_resource (dpy, "delay", "Delay");
  st->cell_size = get_integer_resource (dpy, "cellSize", "Cell size");
  st->xlim = xgwa.width;
  st->ylim = xgwa.height;
  st->field.cols = xgwa.width / st->cell_size;
  st->field.rows = xgwa.height / st->cell_size;
  
  st->field.new_matrix = (__uint8_t*)malloc(st->field.rows * st->field.cols * sizeof(__uint8_t));
  st->field.matrix = (__uint8_t*)malloc(st->field.rows * st->field.cols * sizeof(__uint8_t));
  for (int i = 0; i < st->field.rows; i++) {
    for (int j = 0; j < st->field.cols; j++) {
      st->field.matrix[i * st->field.cols + j] = (random() % 10) == 1;
    }
  }
  return st;
}

static void
life_reshape (Display *dpy, Window window, void *closure,
                   unsigned int width, unsigned int height)
{
}



static void life_draw_field(Display *dpy, Window window, void *closure, int x, int y) {
  struct state *st = (struct state *) closure;
  XRectangle *rectangles = (XRectangle*)malloc(st->field.rows * st->field.cols * sizeof(XRectangle));
  int rectangles_count = 0;
  for (int i = 0; i < st->field.rows; i++) {
    for (int j = 0; j < st->field.cols; j++) {
      if (st->field.matrix[i * st->field.cols + j]) {
        rectangles[rectangles_count++] =
          (XRectangle){ st->cell_size * j + x, st->cell_size * i + y, st->cell_size, st->cell_size };
      }
    }
  }
  XFillRectangles(dpy, window, st->draw_gc, rectangles, rectangles_count);
  free(rectangles);
}

static int get_number_of_neighbors(__uint8_t* matrix, int rows, int cols, int x, int y) {
  // n + 1 = (n + 1) % Nlen
  // n - 1 = (n - 1 + Nlen) % Nlen
  int res = 0;
  for (int i = x - 1; i <= x + 1; i++) {
	  for (int j = y - 1; j <= y + 1; j++) {
      int cp_i = i, cp_j = j;
      cp_i = (i + rows) % rows;
      cp_j = (j + cols) % cols;
	    if (y != j || x != i) {
        res += matrix[cp_i * cols + cp_j];
	    }
	  }
  }
  return res;
}

static void simulate_one_iteration(struct state* st) {
  memcpy(st->field.new_matrix, st->field.matrix, st->field.rows * st->field.cols * sizeof(__uint8_t));
  for (int i = 0; i < st->field.rows; i++) {
    for (int j = 0; j < st->field.cols; j++) {
      int number_of_neighbors = get_number_of_neighbors(st->field.new_matrix, st->field.rows, st->field.cols, i, j);
      st->field.matrix[i * st->field.cols + j] &= (number_of_neighbors == 2);
      st->field.matrix[i * st->field.cols + j] |= (number_of_neighbors == 3);
    }
  }
}

static unsigned long
life_draw (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;
  XClearWindow (dpy, window);
  life_draw_field(dpy, window, closure, 0, 0);
  simulate_one_iteration(st);
  
  return st->sleep_time;
}

static Bool
life_event (Display *dpy, Window window, void *closure, XEvent *event)
{
  return False;
}

static void
life_free (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;
  free(st->field.matrix);
  free(st->field.new_matrix);
  XFreeGC (dpy, st->draw_gc);
  free (st);
}

static const char *life_defaults [] = {
  ".background: black",
  ".foreground: green",
  "*delay:      100000",
  "*cellSize:      10",
#ifdef HAVE_MOBILE
  "*ignoreRotation: True",
#endif
  0
};

static XrmOptionDescRec life_options [] = {
  { "-delay", ".delay", XrmoptionSepArg, 0      },
  { "-cell-size", ".cellSize", XrmoptionSepArg, 0 },
  { 0, 0, 0, 0 }
};

XSCREENSAVER_MODULE ("Life", life)
