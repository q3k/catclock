// Copyright 1985 Massachusetts Institute of Technology
// Copyright 2018 Sergiusz Bazanski

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

int wind = 1;

#include "catback.p"
#include "eyes.p"

#define CATWID 150            /* width of body bitmap */
#define CATHGT 300            /* height of body bitmap */
#define TAILWID 150           /* width of tail bitmap */
#define TAILHGT 89            /* height of tail bitmap */
#define MINULEN 27            /* length of minute hand */
#define HOURLEN 15            /* length of hour hand */
#define HANDWID 4             /* width of clock hands */
#define UPDATE (1000 / NTAIL) /* ms/update -- tail waves at roughly 1/2 hz */

#define BLACK (~0)
#define WHITE 0

#define NTP 7
Point tp[NTP] = {
    /* tail polygon */
    0, 0, 0, 76, 3, 82, 10, 84, 18, 82, 21, 76, 21, 70,
};

#define NTAIL 64
Image *eye[NTAIL + 1];
Image *tail[NTAIL + 1];
Image *cat;                /* cat body */
Image *eyes;               /* eye background */

Point toffs = {74, -15};   /* tail polygon offset */
Point tailoffs = {0, 211}; /* tail bitmap offset, relative to body */
Point eyeoffs = {49, 30};  /* eye bitmap offset, relative to body */
Point catoffs;             /* cat offset, relative to screen */

int xredraw;
int crosseyed;

void drawclock(void);
void drawhand(int, int, double);
void init(void);
Image *draweye(double);
Image *drawtail(double);

Image *eballoc(Rectangle r, int chan) {
  Image *b = allocimage(display, r, chan, 0, DWhite);
  if (b == 0) {
    fprint(2, "catclock: can't allocate bitmap\n");
    exits("allocimage");
  }
  return b;
}

void eloadimage(Image *i, Rectangle r, uchar *d, int nd) {
  int n;
  n = loadimage(i, r, d, nd);
  if (n < nd) {
    fprint(2, "loadimage fails: %r\n");
    exits("loadimage");
  }
}

int round_(double x) { return x >= 0. ? x + .5 : x - .5; }

void redraw(Image *screen) {
  Rectangle r = Rect(0, 0, Dx(screen->r), Dy(screen->r));
  catoffs.x = (Dx(r) - CATWID) / 2;
  catoffs.y = (Dy(r) - CATHGT) / 2;
  if (!ptinrect(catoffs, r))
    fprint(2, "catclock: window too small, resize!\n");
  xredraw = 1;
}

void eresized(int new) {
  if (new &&getwindow(display, Refmesg) < 0)
    fprint(2, "can't reattach to window");
  redraw(screen);
}

void main(int argc, char *argv[]) {
  int i;
  ARGBEGIN {
  case 'c':
    crosseyed++;
    break;
  default:
    fprint(2, "Usage: %s [-c]\n", argv0);
    exits("usage");
  }
  ARGEND
  initdraw(0, 0, "cat clock");
  einit(Emouse);
  redraw(screen);
  for (i = 0; i < nelem(catback_bits); i++)
    catback_bits[i] ^= 0xFF;
  for (i = 0; i < nelem(eyes_bits); i++)
    eyes_bits[i] ^= 0xFF;
  cat = eballoc(Rect(0, 0, CATWID, CATHGT), GREY1);
  eloadimage(cat, cat->r, catback_bits, sizeof(catback_bits));
  for (i = 0; i <= NTAIL; i++) {
    tail[i] = drawtail(i * PI / NTAIL);
    eye[i] = draweye(i * PI / NTAIL);
  }
  for (;;) {
    if (ecanmouse())
      emouse(); /* don't get resize events without this! */
    drawclock();
    flushimage(display, 1);
    sleep(UPDATE);
  }
}

/*
 * Draw a clock hand, theta is clockwise angle from noon
 */
void drawhand(int length, int width, double theta) {
  double c = cos(theta), s = sin(theta);
  double ws = width * s, wc = width * c;
  Point vhand[4];
  vhand[0] =
      addpt(screen->r.min, addpt(catoffs, Pt(CATWID / 2 + round_(length * s),
                                         CATHGT / 2 - round_(length * c))));
  vhand[1] = addpt(screen->r.min, addpt(catoffs, Pt(CATWID / 2 - round_(ws + wc),
                                                CATHGT / 2 + round_(wc - ws))));
  vhand[2] = addpt(screen->r.min, addpt(catoffs, Pt(CATWID / 2 - round_(ws - wc),
                                                CATHGT / 2 + round_(wc + ws))));
  vhand[3] = vhand[0];
  fillpoly(screen, vhand, 4, wind, display->white,
           addpt(screen->r.min, vhand[0]));
  poly(screen, vhand, 4, Endsquare, Endsquare, 0, display->black,
       addpt(screen->r.min, vhand[0]));
}

/*
 * draw a cat tail, t is time (mod 1 second)
 */
Image *drawtail(double t) {
  Image *bp;
  double theta =
      .4 * sin(t + 3. * PIO2) - .08; /* an assymetric tail leans to one side */
  double s = sin(theta), c = cos(theta);
  Point rtp[NTP];
  int i;
  bp = eballoc(Rect(0, 0, TAILWID, TAILHGT), GREY1);
  for (i = 0; i != NTP; i++)
    rtp[i] =
        addpt(Pt(tp[i].x * c + tp[i].y * s, -tp[i].x * s + tp[i].y * c), toffs);
  fillpoly(bp, rtp, NTP, wind, display->black, rtp[0]);
  return bp;
}

/*
 * draw the cat's eyes, t is time (mod 1 second)
 */
Image *draweye(double t) {
  Image *bp;
  double u;
  double angle = 0.7 * sin(t + 3 * PIO2) + PI / 2.0; /* direction eyes point */
  Point pts[100];
  int i, j;
  struct {
    double x, y, z;
  } pt;
  if (eyes == 0) {
    eyes = eballoc(Rect(0, 0, eyes_width, eyes_height), GREY1);
    eloadimage(eyes, eyes->r, eyes_bits, sizeof(eyes_bits));
  }
  bp = eballoc(eyes->r, GREY1);
  draw(bp, bp->r, eyes, nil, ZP);
  for (i = 0, u = -PI / 2.0; u < PI / 2.0; i++, u += 0.25) {
    pt.x = cos(u) * cos(angle + PI / 7.0);
    pt.y = sin(u);
    pt.z = 2. + cos(u) * sin(angle + PI / 7.0);
    pts[i].x = (pt.z == 0.0 ? pt.x : pt.x / pt.z) * 23.0 + 12.0;
    pts[i].y = (pt.z == 0.0 ? pt.y : pt.y / pt.z) * 23.0 + 11.0;
  }
  for (u = PI / 2.0; u > -PI / 2.0; i++, u -= 0.25) {
    pt.x = cos(u) * cos(angle - PI / 7.0);
    pt.y = sin(u);
    pt.z = 2. + cos(u) * sin(angle - PI / 7.0);
    pts[i].x = (pt.z == 0.0 ? pt.x : pt.x / pt.z) * 23.0 + 12.0;
    pts[i].y = (pt.z == 0.0 ? pt.y : pt.y / pt.z) * 23.0 + 11.0;
  }
  fillpoly(bp, pts, i, wind, display->black, pts[0]);
  if (crosseyed) {
    angle = 0.7 * sin(PI - t + 3 * PIO2) + PI / 2.0;
    for (i = 0, u = -PI / 2.0; u < PI / 2.0; i++, u += 0.25) {
      pt.x = cos(u) * cos(angle + PI / 7.0);
      pt.y = sin(u);
      pt.z = 2. + cos(u) * sin(angle + PI / 7.0);
      pts[i].x = (pt.z == 0.0 ? pt.x : pt.x / pt.z) * 23.0 + 12.0;
      pts[i].y = (pt.z == 0.0 ? pt.y : pt.y / pt.z) * 23.0 + 11.0;
    }
    for (u = PI / 2.0; u > -PI / 2.0; i++, u -= 0.25) {
      pt.x = cos(u) * cos(angle - PI / 7.0);
      pt.y = sin(u);
      pt.z = 2. + cos(u) * sin(angle - PI / 7.0);
      pts[i].x = (pt.z == 0.0 ? pt.x : pt.x / pt.z) * 23.0 + 12.0;
      pts[i].y = (pt.z == 0.0 ? pt.y : pt.y / pt.z) * 23.0 + 11.0;
    }
  }
  for (j = 0; j < i; j++)
    pts[j].x += 31;
  fillpoly(bp, pts, i, wind, display->black, pts[0]);
  return bp;
}

void drawclock(void) {
  static int t = 0, dt = 1;
  static Tm otm;
  Tm tm = *localtime(time(0));
  tm.hour %= 12;
  if (xredraw || tm.min != otm.min || tm.hour != otm.hour) {
    if (xredraw) {
      draw(screen, screen->r, display->white, nil, ZP);
      border(screen, screen->r, 4, display->black, ZP);
    }
    draw(screen, screen->r, cat, nil, mulpt(catoffs, -1));
    flushimage(display, 1);
    drawhand(MINULEN, HANDWID, 2. * PI * tm.min / 60.);
    drawhand(HOURLEN, HANDWID, 2. * PI * (tm.hour + tm.min / 60.) / 12.);
    xredraw = 0;
  }
  draw(screen, screen->r, tail[t], nil, mulpt(addpt(catoffs, tailoffs), -1));
  draw(screen, screen->r, eye[t], nil, mulpt(addpt(catoffs, eyeoffs), -1));
  t += dt;
  if (t < 0 || t > NTAIL) {
    t -= 2 * dt;
    dt = -dt;
  }
  otm = tm;
}
