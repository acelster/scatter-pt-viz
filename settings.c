// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include "settings.h"

real_t INTERPOLATION_RADIUS;
int RESOLUTION;
char DEFAULT_FOLDER[100];
int NEIGHBOUR_MODE;
int INTERPOLATION_MODE;
real_t STEP_SIZE;
real_t ANISO_MATRIX[9];
int ANISOTROPIC;
int NUMBERS;
int GRID_ON;
int BATCH;
int FILEARG;
char* FILENAME;
Coord EYE_INITIAL;
Coord FORWARD_INITIAL;
int GRID_BASE;
real_t GRID_SPACING;
int NUM_THREADS;
int VERBOSE;
real_t FILTERING_THRESHOLD;
int MEDIAN_FILTERING;
int COLOR_BANDS;
int DATA_STRUCTURE;
int SINGLE;
real_t STEP_FACTOR;
real_t STEP_LIMIT;
real_t OPACITY_THRESHOLD;
real_t THRESHOLD_MEDIAN_FILTER;
real_t VARIOGRAM_RADIUS;
real_t BATCH_SIZE;
real_t MAX_INTENSITY;
int MULTIGPU;

float stored_work_fractions[2] = {0,0};

#define NUM_SETTINGS 5

#ifdef USE_FLOAT
#define aniso_format "%f %f %f %f %f %f %f %f %f"
#define coord_format "%f %f %f"
#define real_format "%f"
#else
#define aniso_format "%lf %lf %lf %lf %lf %lf %lf %lf %lf"
#define coord_format "%lf %lf %lf"
#define real_format "%lf"
#endif

char keys[NUM_SETTINGS*2][6] = {"RESOLU", "%d",
"COLOR_", "%d",
"STEP_S", real_format,
"NUM_TH", "%d",
"DATA_S", "%d"};
void* values[NUM_SETTINGS] = {&RESOLUTION,
&COLOR_BANDS,
&STEP_SIZE,
&NUM_THREADS,
&DATA_STRUCTURE};


void load_config_file(int reload){

  if(!reload){
    set_default_values();
  }
  char line[100];

  FILE* config_file = fopen("config.txt", "r");

  if(config_file == NULL){
    fprintf(stderr, "WARNING: Did not find configuration file. Using default values\n");
    return;
  }

  while(fgets(line, 100, config_file) != NULL){
    if(strlen(line) < 6 || line[0] == '#'){
      continue;
    }
    if(!reload){

      for(int c= 0; c < NUM_SETTINGS; c++){
        if(strncmp(line, keys[c*2], 6) == 0){
          sscanf(strpbrk(line, "=") + 1, keys[c*2+1], values[c]);
        }
      }

      if(strncmp(line, "EYE_IN", 6) == 0){
        sscanf(strpbrk(line, "=") + 1, coord_format, &EYE_INITIAL.x,
            &EYE_INITIAL.y,
            &EYE_INITIAL.z);
      }
      if(strncmp(line, "FORWAR", 6) == 0){
        sscanf(strpbrk(line, "=") + 1, coord_format, 
            &FORWARD_INITIAL.x, 
            &FORWARD_INITIAL.y,
            &FORWARD_INITIAL.z);
      }
      if(strncmp(line, "DEFAUL", 6) == 0){
        int l = strlen(line+15);
        strncpy(DEFAULT_FOLDER, line+15, l-1);
        DEFAULT_FOLDER[l-1] = '\0';
      }
      if(strncmp(line, "INTERP", 6) == 0){
        if(strncmp(line, "INTERPOLATION_R", 15) == 0){
          sscanf(strpbrk(line, "=") + 1, real_format, &INTERPOLATION_RADIUS);
        }
      }
    }

    if(strncmp(line, "INTERP", 6) == 0){
      if(strncmp(line, "INTERPOLATION_M", 15) == 0){
        sscanf(strpbrk(line, "=") + 1, "%d", &INTERPOLATION_MODE);
      }
    }
    if(strncmp(line, "NEIGHB", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, "%d", &NEIGHBOUR_MODE);
    }
    if(strncmp(line, "ANISO_", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, aniso_format,
          &ANISO_MATRIX[0],
          &ANISO_MATRIX[1],
          &ANISO_MATRIX[2],
          &ANISO_MATRIX[3],
          &ANISO_MATRIX[4],
          &ANISO_MATRIX[5],
          &ANISO_MATRIX[6],
          &ANISO_MATRIX[7],
          &ANISO_MATRIX[8]);

      if(ANISO_MATRIX[0] != 1.0 || ANISO_MATRIX[4] != 1.0 || ANISO_MATRIX[8] != 1.0){
        ANISOTROPIC = 1;
      }
      if(ANISO_MATRIX[1] != 0.0 ||
          ANISO_MATRIX[2] != 0.0 ||
          ANISO_MATRIX[3] != 0.0 ||
          ANISO_MATRIX[5] != 0.0 ||
          ANISO_MATRIX[6] != 0.0 ||
          ANISO_MATRIX[7] != 0.0){
        ANISOTROPIC = 2;
      }
    }
    if(strncmp(line, "GRID_B", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, "%d", &GRID_BASE);
    }
    if(strncmp(line, "GRID_S", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &GRID_SPACING);
    }
    if(strncmp(line, "VERBOS", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, "%d", &VERBOSE);
    }
    if(strncmp(line, "FILTER", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &FILTERING_THRESHOLD);
      if(FILTERING_THRESHOLD < 0){
        printf("WARNING: Illegal value for option 'FILTERING_THRESHOLD'. Using default value (0.0).\n");
        FILTERING_THRESHOLD = 0;
      }
    }
    if(strncmp(line, "MEDIAN", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, "%d", &MEDIAN_FILTERING);
    }
    if(strncmp(line, "STEP_F", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &STEP_FACTOR);
    }
    if(strncmp(line, "STEP_L", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &STEP_LIMIT);
    }
    if(strncmp(line, "OPACIT", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &OPACITY_THRESHOLD);
    }
    if(strncmp(line, "THRESH", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &THRESHOLD_MEDIAN_FILTER);
    }
    if(strncmp(line, "VARIOG", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &VARIOGRAM_RADIUS);
    }
    if(strncmp(line, "BATCH_", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, real_format, &BATCH_SIZE);
    }
    if(strncmp(line, "MULTIG", 6) == 0){
      sscanf(strpbrk(line, "=") + 1, "%d", &MULTIGPU);
    }
  }

  if(INTERPOLATION_MODE == 0 && MEDIAN_FILTERING == 1){
    fprintf(stderr, "ERROR: Median filtering cannot be used in combination with kriging. Exiting.\n");
    exit(-1);
  }


#ifdef USE_CUDA
  if(NUM_THREADS > 1){
    fprintf(stderr, "WARNING: Illegal value for option 'NUM_THREADS' when using CUDA. Using default value (1).\n");
    NUM_THREADS = 1;
  }
#endif
#ifdef COUNT
	if(NUM_THREADS > 1){
    fprintf(stderr, "WARNING: Illegal value for option 'NUM_THREADS' when using COUNT. Using default value (1).\n");
    NUM_THREADS = 1;
  }
#endif
}

void set_default_values(){
  for(int c = 0; c < 9; c++){
    ANISO_MATRIX[c] = 0.0;
  }
  ANISO_MATRIX[0] = 1.0;
  ANISO_MATRIX[4] = 1.0;
  ANISO_MATRIX[8] = 1.0;
  INTERPOLATION_RADIUS = 0.002;
  RESOLUTION = 512;
  NEIGHBOUR_MODE = 1;
  INTERPOLATION_MODE = 1;
  STEP_SIZE = 0.05;
  NUMBERS = 1;
  GRID_ON = 1;
  EYE_INITIAL.x = sqrt(-1);
  FORWARD_INITIAL.x = sqrt(-1);
  char* temp_default_folder = "/home/data";
  strcpy(DEFAULT_FOLDER, temp_default_folder);
  GRID_BASE = 1;
  GRID_SPACING = -1;
  NUM_THREADS = sysconf(_SC_NPROCESSORS_ONLN);
  VERBOSE = 1;
  FILTERING_THRESHOLD = 0;
  MEDIAN_FILTERING = 0;
  ANISOTROPIC = 0;
  COLOR_BANDS=11;
  DATA_STRUCTURE=0;
  STEP_LIMIT = 0.04;
  STEP_FACTOR = 2.0;
  OPACITY_THRESHOLD = 0.99;
  THRESHOLD_MEDIAN_FILTER = 15;
  VARIOGRAM_RADIUS = 0.00001;
  BATCH_SIZE = 1e9;
}

void parse_args(int argc, char** argv){
  BATCH = 0;
  FILEARG = 0;
  SINGLE = 0;

  opterr = 0;

  int c;
  while((c = getopt(argc, argv, "sbf:")) != -1){
    switch(c){
      case 's':
        SINGLE = 1;
        break;
      case 'b':
        BATCH = 1;
        break;
      case 'f':
        FILEARG = 1;
        FILENAME = optarg;
        break;
      case '?':
        if(optopt == 'f'){
          fprintf(stderr, "ERROR: Option -f requires an argument. Exiting.\n");
          exit(-1);
        }
        else if(isprint(optopt))
          fprintf(stderr, "ERROR: Unknown option '-%c'.\n", optopt);
        else
          fprintf(stderr, "ERROR: Unknown option character '\\x%x.\n", optopt);
        break;
      default:
        exit(-1);
    }
  }
}

