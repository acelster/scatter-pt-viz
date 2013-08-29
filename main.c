// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <pthread.h>
#include <GL/freeglut.h>

#include "point.h"
#include "loader.h"
#include "ray.h"
#include "bmp.h"
#include "node.h"
#include "color.h"
#include "quad.h"
#include "transfer.h"
#include "raycreator.h"
#include "wireframe.h"
#include "global.h"
#include "filter.h"
#include "settings.h"
#include "lasso.h"
#include "batch.h"
#include "thread_pool.h"
#include "bin_node.h"
#include "timer.h"
#include "raytrace_kernel.h"
#include "raytrace_kernel_grid.h"
#include "grid.h"
#include "root.h"

#ifdef COUNT
unsigned long raysTraced = 0;
unsigned long zeroLengthRays = 0;
unsigned long rangeSearches = 0;
unsigned long preTracedRays = 0;
unsigned long samplesAccessed = 0;
unsigned long samplesFullyAccessed = 0;
#endif

Transfer_Overlay* transfer_overlay;
Tree* tree;
Root* root;
Ray* rays;
Display_color** images;

static pthread_mutex_t work_mutex;
static pthread_cond_t work_cond;
static int work = 0;
static Lasso lasso;
static int lasso_enabled = 0;
static int new_image_ready = -1;
static pthread_mutex_t new_image_ready_mutex;
static pthread_t background_thread;
static Raycreator* rc;
static int num_images;
static Batch batch;
static Thread_pool* tp;
static Thread_args** tas;
static int total_num_points;
static GLuint* textures;

static int SHOW_HUD = 0;
static double progress_time = 100;
static double progress = 0;

void print_params(){
  printf("At: (%2.2f,%2.2f,%2.2f)\n", rc->eye.x, rc->eye.y, rc->eye.z);
  printf("Looking in direction: (%2.2f,%2.2f,%2.2f)\n",
      rc->forward.x,
      rc->forward.y,
      rc->forward.z);
  printf("FOV: %f\n", rc->fov);
}

void screenshot(){
  void* data = malloc(4 * RESOLUTION * RESOLUTION);
  glReadPixels(0,
      0,
      RESOLUTION,
      RESOLUTION,
      GL_RGBA,
      GL_UNSIGNED_INT_8_8_8_8,
      data);

  write_bmp((Display_color*)data, RESOLUTION, RESOLUTION, &batch);
  //write_bmp(images[6], 1024, 1024, &batch);
  free(data);
}

void* generate_image(){


  while(1){
    pthread_mutex_lock(&work_mutex);
    while(work == 0){
      pthread_cond_wait(&work_cond, &work_mutex);
    }
    work = 0;
    pthread_mutex_unlock(&work_mutex);


#ifdef USE_CUDA
    update_raycreator(rc);
    if(MULTIGPU == 1){
        timer_start("\nCreating rays... \n");
        create_rays(rc, rays);
        timer_end();
    }

    timer_start("Tracing rays...\n");
    root->launch_kernel(root->s, rc);
    timer_end();

    pthread_mutex_lock(&new_image_ready_mutex);
    new_image_ready = log2(RESOLUTION) -4;
    pthread_mutex_unlock(&new_image_ready_mutex);
#else

    timer_start("\nCreating rays... \n");
    update_raycreator(rc);
    create_rays(rc, rays);
    timer_end();

    int c = 0;
    int start = 16;
    if(SINGLE || BATCH){
      start = RESOLUTION;
    }
    for(int res = start; res <= RESOLUTION; res*=2){
      timer_start("Tracing rays");
      printf("(%d)... ", res);
      fflush(stdout);

      thread_pool_exec(tp, res, res*res);

      progress_time = timer_end();
      progress = 0;
#ifdef COUNT
			printf("---COUNTS---\n");
			printf("Rays traced: %lu\n", raysTraced);
			printf("Zero length rays: %lu\n", zeroLengthRays);
			printf("Range searches: %lu\n", rangeSearches);
			printf("Pre traced rays: %lu\n", preTracedRays);
			printf("Samples accesed: %lu\n", samplesAccessed);
			printf("Samples fully accessed: %lu\n", samplesFullyAccessed);
			printf("\n");
			raysTraced = 0;
			zeroLengthRays = 0;
			rangeSearches = 0;
			preTracedRays = 0;
			samplesAccessed = 0;
			samplesFullyAccessed = 0;
#endif
      if(SINGLE || BATCH){
        c = log2(RESOLUTION) - 4;
      }
      //Maybe i should lock it...
      if(work == 1){
        break;
      }

      pthread_mutex_lock(&new_image_ready_mutex);
      new_image_ready = c;
      pthread_mutex_unlock(&new_image_ready_mutex);
      c++;
    }

#endif
    print_params();
  }
}

void restart_thread(){
  pthread_mutex_lock(&work_mutex);
  thread_pool_stop(tp);
  work = 1;
  pthread_cond_signal(&work_cond);
  pthread_mutex_unlock(&work_mutex);
}

void display(void){

  int local_ready = new_image_ready;
  pthread_mutex_lock(&new_image_ready_mutex);
  new_image_ready = -1;
  pthread_mutex_unlock(&new_image_ready_mutex);

  if(local_ready != -1){
    glBindTexture(GL_TEXTURE_2D, textures[local_ready]);
    glTexSubImage2D(GL_TEXTURE_2D,
        0,
        0,
        0,
        pow(2, (local_ready + 4)),
        pow(2, (local_ready + 4)),
        GL_RGBA,
        GL_UNSIGNED_INT_8_8_8_8,
        images[local_ready]);

    	//glDrawPixels(1024,1024,GL_RGBA,GL_UNSIGNED_INT_8_8_8_8, images[local_ready]);
  }


  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw_wireframe(rc, &root->ranges);



  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glColor4f(1.0,1.0,1.0,1.0);
  glTexCoord2f(0.0f,0.0f); 
  glVertex2f(0.0f,0.0f);

  glColor4f(1.0,1.0,1.0,1.0);
  glTexCoord2f(1.0f,0.0f); 
  glVertex2f(1.0f,0.0f);

  glColor4f(1.0,1.0,1.0,1.0);
  glTexCoord2f(1.0f,1.0f); 
  glVertex2f(1.0f,1.0f);

  glColor4f(1.0,1.0,1.0,1.0);
  glTexCoord2f(0.0f,1.0f); 
  glVertex2f(0.0f,1.0f);
  glEnd();
  glDisable(GL_TEXTURE_2D);



  render_transfer_overlay(transfer_overlay);

  if(lasso.active){
    render_lasso(lasso);
  }

  if(SHOW_HUD){
    char n[30];
    double h = 15/(double)RESOLUTION;
    glColor4f(1.0,1.0,1.0,1.0);

    glRasterPos2f(0.01,1-h);
    sprintf(n, "At: %2.2f %2.2f %2.2f", rc->eye.x, rc->eye.y, rc->eye.z);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, n);

    glRasterPos2f(0.01,1-2*h);
    sprintf(n, "In: %2.2f %2.2f %2.2f", rc->forward.x,rc->forward.y,rc->forward.z);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, n);

    glRasterPos2f(0.01,1-3*h);
    sprintf(n, "FOV: %2.2f", rc->fov);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, n);

    Color col;
    col.r = 0.5; col.g = 0.5; col.b = 0.5; col.a = 0.5;

    Quad bkg;
    Quad prog;
    bkg.x = 0.01; bkg.y = 1-5*h; bkg.w = 0.15; bkg.h = 1.1*h;
    bkg.c = col;

    prog.x = 0.01; prog.y = 1-5*h; prog.w = 0.25 * 0.15 * progress/progress_time; prog.h = 1.1*h;
    col.r = 1.0; col.g = 1.0; col.b = 1.0;
    prog.c = col;

    render(&bkg);
    render(&prog);

    glColor4f(1.0,1.0,1.0,1.0-0.15*progress/progress_time);
    glRasterPos2f(0.01,1-6*h);
    sprintf(n, "%2.2f", progress_time);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, n);

  }

  glutSwapBuffers();

  if(SINGLE == 1 && local_ready + 4 == log2(RESOLUTION)){
    screenshot();
    exit(0);
  }

  if(BATCH){
    if(local_ready + 4 == log2(RESOLUTION)){
      screenshot();
      if(batch_has_next(&batch)){
        batch_next(&batch, rc);
        restart_thread();
      }
      else{
        int made_movie = system("./moviemaker.sh");
        if(made_movie != 0){
          fprintf(stderr, "ERROR: Problems making movie. Exiting.\n");
        }
        exit(0);
      }
    }
  }
}

void keyboard(unsigned char key, int x, int y){
  int changed = 0;
  double step = 0.1;
  switch(key){
    case 'd':
      rc->eye = add_scaled_Coord(rc->eye, rc->right, step);
      changed = 1;
      break;
    case 'a':
      rc->eye = add_scaled_Coord(rc->eye, rc->right, -step);
      changed = 1;
      break;
    case 'w':
      rc->eye = add_scaled_Coord(rc->eye, rc->forward, step);
      changed = 1;
      break;
    case 's':
      rc->eye = add_scaled_Coord(rc->eye, rc->forward, -step);
      changed = 1;
      break;
    case 'q':
      rc->eye = add_scaled_Coord(rc->eye, rc->up, step);
      changed = 1;
      break;
    case 'z':
      rc->eye = add_scaled_Coord(rc->eye, rc->up, -step);
      changed = 1;
      break;
    case 'x':
      rc->fov -= 0.02;
      if(rc->fov <=0){
        rc->fov = 0;
      }
      changed = 1;
      break;
    case 'h':
      SHOW_HUD = (SHOW_HUD * -1) + 1;
      glutPostRedisplay();
      break;
    case 'c':
      rc->fov += 0.02;
      if(rc->fov > 1.57){
        rc->fov = 1.57;
      }
      changed = 1;
      break;
    case 'X':
      rc->fov = 3.14/16;
      changed = 1;
      break;
    case 'r':
      load_config_file(1);
      changed = 1;
      break;
    case 'g':
      GRID_ON = (GRID_ON * -1) + 1;
      glutPostRedisplay();
      break;
    case 'n':
      NUMBERS = (NUMBERS * -1) + 1;
      glutPostRedisplay();
      break;
    case 't':
      transfer_overlay->visible = (transfer_overlay->visible * -1) + 1;
      if(transfer_overlay->visible){
        glutPostRedisplay();
      }
      else{
        changed = 1;
      }
      break;
    case 'b':
      screenshot();
      break;
    case 'y':
      dump_colors(transfer_overlay);
      break;
    case 'l':
      lasso_enabled = (lasso_enabled * -1) + 1;
      if(lasso_enabled){
        printf("Zoom mode\n");
      }
      else{
        printf("Trace mode\n");
      }
      break;
    case '\033':
#ifdef USE_CUDA
      freeAndReset();
#endif
      printf("\n\nAborted by user. Exiting\n");
      exit(0);
      break;
  }

  if(changed == 1){
    restart_thread();
  }
}

void special_keyboard(int key, int x, int y){
  double angle = 0.02;
  switch(key){
    case GLUT_KEY_UP:
      rc->forward = add_scaled_Coord(rc->forward, rc->up, angle);
      break;
    case GLUT_KEY_DOWN:
      rc->forward = add_scaled_Coord(rc->forward, rc->up, -angle);
      break;
    case GLUT_KEY_LEFT:
      rc->forward = add_scaled_Coord(rc->forward, rc->right, -angle);
      break;
    case GLUT_KEY_RIGHT:
      rc->forward = add_scaled_Coord(rc->forward, rc->right, angle);
      break;
  }
  restart_thread();
}

void mouse(int button, int state, int x, int y){

  int window_w = glutGet(GLUT_WINDOW_WIDTH);
  int window_h = glutGet(GLUT_WINDOW_HEIGHT);

  double norm_x = (double)x/(double)window_w;
  double norm_y = 1.0 - (double)y/(double)window_h;

  handle_mouse(transfer_overlay, norm_x, norm_y, state);

  if(transfer_overlay->visible == 0){
    if(state == GLUT_DOWN){
      if(lasso_enabled){
      lasso.active = 1;
      lasso.start_x = x;
      lasso.start_y = y;
      }
      else{
        int raynumber = (RESOLUTION - y)*RESOLUTION + x;
        printf("Tracing ray %d\n", raynumber);
        trace_ray(&rays[raynumber], root, 1);
        printf("'raydumt.txt' successfully created\n");
      }
    }

    if(state == GLUT_UP && lasso.active){
      lasso.end_x = x;
      lasso.end_y = y;
      lasso.active = 0;

      update_camera(rc, lasso);

      restart_thread();
    }
  }
  glutPostRedisplay();
}

void mouse_motion(int x, int y){
  if(lasso.active){
    lasso.end_x = x;
    lasso.end_y = y;
    glutPostRedisplay();
  }
}

void timer(){
  if(new_image_ready != -1 || SHOW_HUD){
    progress += 0.03;
    glutPostRedisplay();
  }
  glutTimerFunc(30, timer, 0);
}

void initGL(int argc, char** argv){
  glutInit(&argc, argv);
  glutInitWindowSize(RESOLUTION, RESOLUTION);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutCreateWindow("Resiprocal space viewer");

  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special_keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(mouse_motion);
  glutTimerFunc(0, timer, 0);


  glViewport(0, 0, rc->resolution, rc->resolution);
  glClearColor(0.0,0.0,0.0,1.0);
  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

  glGenTextures(num_images, textures);

  for(int c = 0; c < num_images; c++){
    glBindTexture(GL_TEXTURE_2D, textures[c]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        pow(2, (c + 4)),
        pow(2, (c + 4)),
        0,
        GL_LUMINANCE,
        GL_UNSIGNED_BYTE,
        NULL);
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void load_data(){
  timer_start("Loading data...\n");

  Loader l = init_loader(BATCH_SIZE);

  root = create_root(l.ranges);

  /*
  FILE* f = fopen("dump.txt", "w+");
  while(has_next(&l)){
      PointList* p = next(&l);
      for(int i = 0; i < p->index; i++){
          fprintf(f, "%f, %f, %f, %f\n", p->points[i].x,
                  p->points[i].y,
                  p->points[i].z,
                  p->points[i].intensity);
      }
  }
  exit(0);
  */

  while(has_next(&l)){
    root->insert_points(root->s, next(&l));
  }

  timer_end();

  root->finalize(root->s);
}

void allocate_buffers(){
  rays = (Ray*)malloc(sizeof(Ray) * RESOLUTION * RESOLUTION);

  num_images = (int)log2(RESOLUTION) - 3;
  images = (Display_color**)malloc(sizeof(Display_color*) * num_images);

  textures = (GLuint*)malloc(sizeof(GLuint) * num_images);

  int c = 0;
  for(int res = 16; res <= RESOLUTION; res *=2){
    images[c] = (Display_color*)malloc(sizeof(Display_color) * res * res);
    c++;
  }
}

void init_threads(){
  if(VERBOSE){
    printf("Using %d threads.\n", NUM_THREADS);
  }
  pthread_mutex_init(&new_image_ready_mutex, NULL);
  pthread_mutex_init(&work_mutex, NULL);
  pthread_cond_init(&work_cond, NULL);

  tp = new_thread_pool(NUM_THREADS);
  tas = new_thread_args(NUM_THREADS);
  create_threads(tp, tas);

  pthread_create(&background_thread, NULL, generate_image, NULL);
}

void pre_load_init(int argc, char** argv){
  parse_args(argc, argv);
  load_config_file(0);

  if(BATCH){
    init_batch(&batch);
  }

  init_threads();

  allocate_buffers();

  //transfer_overlay = init_transfer_overlay();
  init_lasso(&lasso);
}

void start_rendering(){
  if(BATCH){
    if(batch_has_next(&batch)){
      batch_next(&batch, rc);
    }
  }

  restart_thread();
}

void post_load_init(int argc, char** argv){
  transfer_overlay = init_transfer_overlay(&root->ranges);
  rc = init_raycreator(&root->ranges);
  initGL(argc, argv);
#ifdef USE_CUDA
  initDevice(root->s, rc);
#endif
}

void print_welcome(){
  if(VERBOSE >= 1){
    printf("\nThis is RSV 0.1\n");
    printf("Compiled at %s %s with:\n", __DATE__, __TIME__);
#ifdef USE_CUDA
    printf("\tCUDA\n");
#endif
#ifdef USE_FLOAT
    printf("\tfloat\n");
#else
    printf("\tdouble\n");
#endif
    printf("\n");
  }
}


void check_memory_usage(){
#ifdef USE_CUDA
  long int d_mem = root->get_memory_useage(root->s);
  long int image_mem = sizeof(Display_color) * RESOLUTION * RESOLUTION;
  long int rays_mem = sizeof(Ray) * RESOLUTION * RESOLUTION;
  long int color_mem = sizeof(Color) * transfer_overlay->color_table_size;

  long int total = image_mem + rays_mem + color_mem + d_mem;
  long int available = get_memory_size();

  if(VERBOSE >= 2){
    printf("Memory useage\n");
    printf("Points and datastructure: \t%11ld MB\n", d_mem/(1024*1024));
    printf("Image:\t\t\t%11ld MB\n", image_mem/(1024*1024));
    printf("Rays:\t\t\t%11ld MB\n", rays_mem/(1024*1024));
    printf("Color:\t\t\t%11ld MB\n", color_mem/(1024*1024));
    printf("Total:\t\t\t%11ld MB\n", total/(1024*1024));
    printf("Available:\t\t%11ld MB\n", available/(1024*1024));
  }

  if(total > available){
    fprintf(stderr, "ERROR: Not enough GPU memory available. Exiting.\n");
    exit(-1);
  }
#else
  root->get_memory_useage(root->s);
#endif
}

int main(int argc, char** argv){
  pre_load_init(argc, argv);

  print_welcome();

  load_data();

  post_load_init(argc, argv);
  
  check_memory_usage();

  start_rendering();
  
  glutMainLoop();

  return 0;
}
