// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef SETTINGS
#define SETTINGS

#include "point.h"
#include "real.h"

extern real_t INTERPOLATION_RADIUS;
extern int RESOLUTION;
extern char DEFAULT_FOLDER[100];
extern int NEIGHBOUR_MODE;
extern int INTERPOLATION_MODE;
extern real_t STEP_SIZE;
extern real_t ANISO_MATRIX[9];
extern int ANISOTROPIC;
extern int NUMBERS;
extern int GRID_ON;
extern int BATCH;
extern int FILEARG;
extern char* FILENAME;
extern Coord EYE_INITIAL;
extern Coord FORWARD_INITIAL;
extern int GRID_BASE;
extern real_t GRID_SPACING;
extern int NUM_THREADS;
extern int VERBOSE;
extern real_t FILTERING_THRESHOLD;
extern int MEDIAN_FILTERING;
extern int COLOR_BANDS;
extern int DATA_STRUCTURE;
extern int SINGLE;
extern real_t STEP_FACTOR;
extern real_t STEP_LIMIT;
extern real_t OPACITY_THRESHOLD;
extern real_t THRESHOLD_MEDIAN_FILTER;
extern real_t VARIOGRAM_RADIUS;
extern real_t BATCH_SIZE;
extern real_t MAX_INTENSITY;
extern int MULTIGPU;

extern float stored_work_fractions[2];


void load_config_file(int reload);
void set_default_values();
void parse_args(int argc, char** argv);


#endif
