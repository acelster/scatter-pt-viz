// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <GL/freeglut.h>

#include "raycreator.h"
#include "point.h"
#include "wireframe.h"
#include "settings.h"


void draw_axis(Raycreator* rc, Ranges* r){
}

void drawStrokeText(char*string,int x,int y,int z)
{
char *c;
glPushMatrix();
glTranslatef(x, y+8,z);
glScalef(0.09f,-0.08f,z);
for (c=string; *c != '\0'; c++)
{
glutStrokeCharacter(GLUT_STROKE_ROMAN , *c);
}
glPopMatrix();
}

void draw_wireframe(Raycreator* rc, Ranges* r){
  if(GRID_ON == 0){
    return;
  }

  double base_x = r->xmin, base_y = r->ymin, base_z = r->zmin;
  if(GRID_BASE == 0){
    base_x = get_base(r->xmin);
    base_y = get_base(r->ymin);
    base_z = get_base(r->zmin);
  }

  if(GRID_SPACING < 0){
    GRID_SPACING = (r->xmax - r->xmin)/3.0;
  }
  int ys = floor((r->ymax - base_y)/GRID_SPACING) + 1;
  int xs = floor((r->xmax - base_x)/GRID_SPACING) + 1;
  int zs = floor((r->zmax - base_z)/GRID_SPACING) + 1;

  Coord max_corner;
  double max_distance = -1;
  for(int corner_x = 0; corner_x < 2; corner_x++){
    for(int corner_y = 0; corner_y < 2; corner_y++){
      for(int corner_z = 0; corner_z < 2; corner_z++){
        Coord corner;
        corner.x = (corner_x == 0) ? r->xmin : r->xmax;
        corner.y = (corner_y == 0) ? r->ymin : r->ymax;
        corner.z = (corner_z == 0) ? r->zmin : r->zmax;

        double new_distance = distance_Coord(rc->eye, corner);
        if(max_distance < new_distance){
          max_corner = corner;
          max_distance = new_distance;
        }
      }
    }
  }

  Coord start, end;
  Color color = {1.0,1.0,1.0,1.0};

  start = max_corner;
  start.x = r->xmin;
  end = max_corner;
  end.x = r->xmax;

  draw_line(rc, start, end, color);

  color.a = 0.3;
  for(int c = 0; c < xs; c++){
    start = max_corner;
    end = max_corner;

    start.x = base_x + c * GRID_SPACING;
    end.x = start.x;
    end.x = start.x;

    start.z = r->zmin;
    end.z = r->zmax;

    draw_line(rc, start, end, color);

    start = max_corner;
    end = max_corner;

    start.x = base_x + c * GRID_SPACING;
    end.x = start.x;

    start.y = r->ymin;
    end.y = r->ymax;

    draw_line(rc, start, end, color);
  }


  start = max_corner;
  start.y = r->ymin;
  end = max_corner;
  end.y = r->ymax;

  color.a = 1.0;
  draw_line(rc, start, end, color);
  color.a = 0.3;

  for(int c = 0; c < ys; c++){
    start = max_corner;
    end = max_corner;

    start.y = base_y + c * GRID_SPACING;
    end.y = start.y;

    start.z = r->zmin;
    end.z = r->zmax;

    draw_line(rc, start, end, color);

    start = max_corner;
    end = max_corner;

    start.y = base_y + c * GRID_SPACING;
    end.y = start.y;

    start.x = r->xmin;
    end.x = r->xmax;

    draw_line(rc, start, end, color);
  }


  start = max_corner;
  start.z = r->zmin;
  end = max_corner;
  end.z = r->zmax;

  color.a = 1.0;
  draw_line(rc, start, end, color);
  color.a  = 0.3;

  for(int c = 0; c < zs; c++){
    start = max_corner;
    end = max_corner;

    start.z = base_z + c * GRID_SPACING;
    end.z = start.z;

    start.x = r->xmin;
    end.x = r->xmax;

    draw_line(rc, start, end, color);

    start = max_corner;
    end = max_corner;

    start.z = base_z + c * GRID_SPACING;
    end.z = start.z;

    start.y = r->ymin;
    end.y = r->ymax;

    draw_line(rc, start, end, color);
  }

  char n[20];
  if(NUMBERS){
    glRasterPos2f(-1,-1);
    glColor4f(1.0,1.0,1.0,1.0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, "HACK");

    Coord pos;
    double p[2];
    pos = max_corner;
    for(double c = 0; c < xs; c++){
      pos.x = base_x + c * GRID_SPACING;
      get_screen_pos(rc, pos, p);
      glRasterPos2f(p[0],p[1]);
      glColor4f(1.0,1.0,1.0,1.0);
      sprintf(n, "%2.2f", pos.x);
      glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);
      //drawStrokeText(n, p[0], p[1], 0);
    }

    pos = max_corner;
    for(double c = 0; c < ys; c++){
      pos.y = base_y + c * GRID_SPACING;
      get_screen_pos(rc, pos, p);
      glRasterPos2f(p[0],p[1]);
      glColor4f(1.0,1.0,1.0,1.0);
      sprintf(n, "%2.2f", pos.y);
      glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);
    }

    pos = max_corner;
    for(double c = 0; c < zs; c++){
      pos.z = base_z + c * GRID_SPACING;
      get_screen_pos(rc, pos, p);
      glRasterPos2f(p[0],p[1]);
      glColor4f(1.0,1.0,1.0,1.0);
      sprintf(n, "%2.2f", pos.z);
      glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);
    }

    pos = max_corner;
    pos.x += pos.x > r->xmin ? ((-r->xmax + r->xmin)*1.05) : ((r->xmax - r->xmin)*1.05);
    get_screen_pos(rc, pos, p);
    glRasterPos2f(p[0], p[1]);
    glColor4f(1.0,1.0,1.0,1.0);
    sprintf(n, "Qx");
    glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);

    pos = max_corner;
    pos.y += pos.y > r->ymin ? ((-r->ymax + r->ymin)*1.05) : ((r->ymax - r->ymin)*1.05);
    get_screen_pos(rc, pos, p);
    glRasterPos2f(p[0], p[1]);
    glColor4f(1.0,1.0,1.0,1.0);
    sprintf(n, "Qy");
    glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);

    pos = max_corner;
    pos.z += pos.z > r->zmin ? ((-r->zmax + r->zmin)*1.05) : ((r->zmax - r->zmin)*1.05);
    get_screen_pos(rc, pos, p);
    glRasterPos2f(p[0], p[1]);
    glColor4f(1.0,1.0,1.0,1.0);
    sprintf(n, "Qz");
    glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, n);
  }
}


void get_screen_pos(Raycreator* rc, Coord origin, double* screen_pos){
  Coord toOrigin = sub_Coord(origin, rc->eye);
  double angle = angle_between_Coords(rc->forward, toOrigin);
  double toOriginPixelDistance = 0.1/cos(angle);
  normalize_Coord(&toOrigin);

  Coord pixelPoint = add_scaled_Coord(rc->eye, toOrigin, toOriginPixelDistance);
  normalize_Coord(&rc->forward);
  Coord screenCenter = add_scaled_Coord(rc->eye, rc->forward, 0.1);
  Coord screenCenterToOrigin = sub_Coord(pixelPoint, screenCenter);

  double angle2 = angle_between_Coords(screenCenterToOrigin, rc->up);

  double d =length_Coord(screenCenterToOrigin);
  double w = d * sin(angle2);
  double h = d * cos(angle2);
  double screen_width = 2*0.1*tan(rc->fov);
  double angle3 = angle_between_Coords(screenCenterToOrigin, rc->right);

  double sign = (angle3 < 3.14/2) ? 1 : -1;

  double origin_x = 0.5 + sign*w/screen_width;
  double origin_y = 0.5 + h/screen_width;

  screen_pos[0] = origin_x;
  screen_pos[1] = origin_y;
}

double get_base(double limit){
  double m = fmod(limit, GRID_SPACING);
  double r = limit - m;
  if(limit / fabs(limit) == 1){
    r += GRID_SPACING;
  }

  return r;
}

void draw_line(Raycreator* rc, Coord start, Coord end, Color color){
  double s[2], e[2];

  get_screen_pos(rc, start, s);
  get_screen_pos(rc, end, e);

  glColor4f(color.r,color.g,color.b,color.a);
  glBegin(GL_LINES);
  glVertex3f(s[0], s[1], 0);
  glVertex3f(e[0],e[1], 0);
  glEnd();
}
