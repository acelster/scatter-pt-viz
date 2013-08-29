// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "raycreator.h"
#include "ray.h"
#include "settings.h"
#include "color.h"
#include "lasso.h"
#include "raycreator_kernel.h"

Raycreator* init_raycreator(Ranges* ranges){
  Raycreator* rc = (Raycreator*)malloc(sizeof(Raycreator));
  rc->resolution = RESOLUTION;
  rc->ranges = ranges;
  rc->fov = 3.14/16;

  Coord center;
  center.x = rc->ranges->xmin + (rc->ranges->xmax - rc->ranges->xmin)/2.0;
  center.y = rc->ranges->ymin + (rc->ranges->ymax - rc->ranges->ymin)/2.0;
  center.z = rc->ranges->zmin + (rc->ranges->zmax - rc->ranges->zmin)/2.0;

  if(isnan(EYE_INITIAL.x) && isnan(FORWARD_INITIAL.x)){

    real_t theta = 3.14159/4;
    real_t phi = 0.3;
    real_t r = 5*(rc->ranges->xmax - rc->ranges->xmin);


    rc->eye.x = center.x + cos(phi)*sin(theta)*r;
    rc->eye.y = center.y + sin(phi)*sin(theta)*r;
    rc->eye.z = center.z + cos(theta)*r;

    rc->forward.x = center.x - rc->eye.x;
    rc->forward.y = center.y - rc->eye.y;
    rc->forward.z = center.z - rc->eye.z;
  }
  else if(isnan(EYE_INITIAL.x)){
    rc->forward = FORWARD_INITIAL;
    normalize_Coord(&rc->forward);
    rc->eye = add_scaled_Coord(center, rc->forward, -5);
  }
  else if(isnan(FORWARD_INITIAL.x)){
    rc->eye = EYE_INITIAL;
    rc->forward = sub_Coord(center, rc->eye);
  }

  else{
    rc->eye = EYE_INITIAL;
    rc->forward = FORWARD_INITIAL;
  }

  return rc;
}

void update_camera(Raycreator* rc, Lasso lasso){
  
  //Relative screen coordinates
  real_t dx1 = ((real_t)lasso.start_x)/RESOLUTION - 0.5;
  real_t dx2 = ((real_t)lasso.end_x)/RESOLUTION - 0.5;
  real_t dy1 = ((real_t)lasso.end_y)/RESOLUTION - 0.5;
  real_t dy2 = ((real_t)lasso.start_y)/RESOLUTION - 0.5;

  //Absolute screen coordinates
  real_t screen_width = 2*0.1*tan(rc->fov);
  dx1 = dx1*screen_width;
  dx2 = dx2*screen_width;
  dy1 = dy1*screen_width;
  dy2 = dy2*screen_width;

  //Angle from center
  real_t ax1 = atan(dx1/0.1);
  real_t ax2 = atan(dx2/0.1);
  real_t ay1 = atan(dy1/0.1);
  real_t ay2 = atan(dy2/0.1);

  //Angles for new window
  real_t angle_x = fabs(ax2 - ax1);
  real_t angle_y = fabs(ay2 - ay1);

  real_t angle = (angle_x > angle_y) ? angle_x : angle_y;

  //Angle to center of new window
  real_t acx = (ax1 + ax2)/2.0;
  real_t acy = (ay1 + ay2)/2.0;

  real_t dcx = 0.1 * tan(acx);
  real_t dcy = 0.1 * tan(acy);

  Coord new_center = add_scaled_Coord(rc->eye, rc->forward, 0.1);
  new_center = add_scaled_Coord(new_center, rc->right, dcx);
  new_center = add_scaled_Coord(new_center, rc->up, -dcy);

  rc->forward = sub_Coord(new_center, rc->eye);
  rc->fov = angle/2.0;
}

void update_raycreator(Raycreator* rc){
  Coord eye = rc->eye;

  real_t eye_screen_d = 0.1;

  rc->pixel_width = 2*eye_screen_d*tan(rc->fov)/((real_t)rc->resolution);

  Coord forward = rc->forward;

  Coord right;
  Coord up;

  if(forward.x == 0 && forward.y == 0){
    //handle special case
    right.x = 1; right.y = 0; right.z = 0;
    up.x = 0; up.y = 1; up.z = 0;
  }
  else{
    right.x = forward.y;
    right.y = -forward.x;
    right.z = 0;

    /*
    right.x = forward.z;
    right.z = -forward.x;
    right.y = 0;
    */

    up.x = right.y*forward.z - right.z*forward.y;
    up.y = right.z*forward.x - right.x*forward.z;
    up.z = right.x*forward.y - right.y*forward.x;
  }

  normalize_Coord(&right);
  normalize_Coord(&up);
  normalize_Coord(&forward);

  rc->eye = eye;
  rc->forward = forward;
  rc->right = right;
  rc->up = up;

  rc->screen_center = add_scaled_Coord(eye, forward, eye_screen_d);
}

void create_rays(Raycreator* rc, Ray* rays){

	//Hack...
	Coord screen_center = rc->screen_center;
	Coord up = rc->up;
	Coord right = rc->right;
	Coord eye = rc->eye;
	real_t pixel_width = rc->pixel_width;

//#ifdef USE_CUDA
//  launch_kernel(rays, rc, screen_center, pixel_width);
//#else

  Color color = {-1,-1,-1,-1};
  int half_res = rc->resolution/2;
  
#pragma omp parallel for
  for(int c = -half_res; c < half_res; c++){
    for(int d = -half_res; d < half_res; d++){
      int i = (c+half_res)*rc->resolution + (d + half_res);
      Coord end;
      end.x = screen_center.x + c*pixel_width*up.x + d*pixel_width*right.x;
      end.y = screen_center.y + c*pixel_width*up.y + d*pixel_width*right.y;
      end.z = screen_center.z + c*pixel_width*up.z + d*pixel_width*right.z;

      rays[i].start = end;
      rays[i].dir = sub_Coord(end, eye);
      rays[i].color = color;
      find_intersections(&rays[i], rc->ranges);
    }
  }

//#endif
}

void find_intersections(Ray* ray, Ranges* ranges){
  Coord top = {ranges->xmax, ranges->ymax, ranges->zmax};
  Coord bottom = {ranges->xmin, ranges->ymin, ranges->zmin};

  Coord t1 = div_Coord(sub_Coord(top, ray->start), ray->dir, 1);
  Coord t2 = div_Coord(sub_Coord(bottom, ray->start), ray->dir, -1);

  Coord tmin = min_Coord(t1, t2);
  Coord tmax = max_Coord(t1, t2);

  real_t tnear = fmax(fmax(tmin.x, tmin.y), fmax(tmin.x, tmin.z));
  real_t tfar = fmin(fmin(tmax.x, tmax.y), fmin(tmax.x, tmax.z));

  if(tfar <= tnear){
    ray->distance = 0;
    return;
  }

  if(tfar <=0){
    ray->distance = -1;
    return;
  }

  Coord far = add_scaled_Coord(ray->start, ray->dir, tfar);

  tnear = fmax(0, tnear);
  ray->start = add_scaled_Coord(ray->start, ray->dir, tnear);

  ray->distance = distance_Coord(ray->start, far);
}

