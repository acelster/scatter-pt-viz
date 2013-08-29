// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "bmp.h"
#include "color.h"
#include "batch.h"
#include "settings.h"

void write_bmp_bw(unsigned char* data, int width, int height){
  struct bmp_id id;
  id.magic1 = 0x42;
  id.magic2 = 0x4D;

  struct bmp_header header;
  header.file_size = width*height+54 + 2;
  header.pixel_offset = 1078;

  struct bmp_dib_header dib_header;
  dib_header.header_size = 40;
  dib_header.width = width;
  dib_header.height = height;
  dib_header.num_planes = 1;
  dib_header.bit_pr_pixel = 8;
  dib_header.compress_type = 0;
  dib_header.data_size = width*height;
  dib_header.hres = 0;
  dib_header.vres = 0;
  dib_header.num_colors = 256;
  dib_header.num_imp_colors = 0;

  char padding[2];

  unsigned char* color_table = (unsigned char*)malloc(1024);
  for(int c= 0; c < 256; c++){
    color_table[c*4] = (unsigned char) c;
    color_table[c*4+1] = (unsigned char) c;
    color_table[c*4+2] = (unsigned char) c;
    color_table[c*4+3] = 0;
  }


  FILE* fp = fopen("out.bmp", "w+");
  fwrite((void*)&id, 1, 2, fp);
  fwrite((void*)&header, 1, 12, fp);
  fwrite((void*)&dib_header, 1, 40, fp);
  fwrite((void*)color_table, 1, 1024, fp);
  fwrite((void*)data, 1, width*height, fp);
  fwrite((void*)&padding,1,2,fp);
  fclose(fp);
}

void write_bmp(Display_color* data, int width, int height, Batch* b){

  struct bmp_id id;
  id.magic1 = 0x42;
  id.magic2 = 0x4D;

  struct bmp_header header;
  header.file_size = 4*width*height + 54 + 2;
  header.pixel_offset = 54;

  struct bmp_dib_header dib_header;
  dib_header.header_size = 40;
  dib_header.width = width;
  dib_header.height = height;
  dib_header.num_planes = 1;
  dib_header.bit_pr_pixel = 24;
  dib_header.compress_type = 0;
  dib_header.data_size = 4*width*height;
  dib_header.hres = 0;
  dib_header.vres = 0;
  dib_header.num_colors = 0;
  dib_header.num_imp_colors = 0;

  char padding[2];

  char name[50];
  if(BATCH){
    sprintf(name, "%04d.bmp", b->pic_number);
  }
  else{
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo;
    timeinfo = localtime(&rawtime);
    sprintf(name, "rsw_%d-%d-%d_%d-%d-%d.bmp",
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec,
        timeinfo->tm_mday,
        timeinfo->tm_mon,
        timeinfo->tm_year + 1900);
  }


  FILE* fp = fopen(name, "w+");
  if(fp == NULL){
    fprintf(stderr, "ERROR: Not able to open 'out.bmp' for writing.\n");
    return;
  }
  fwrite((void*)&id, 1, 2, fp);
  fwrite((void*)&header, 1, 12, fp);
  fwrite((void*)&dib_header, 1, 40, fp);

  //Ugly, but it works, and I couldn't get the bitmask thing to work...
  for(int c = 0; c < width*height; c++){
    fwrite((void*)&data[c].b, 1,1,fp);
    fwrite((void*)&data[c].g, 1,1,fp);
    fwrite((void*)&data[c].r, 1,1,fp);
  }

  fwrite((void*)&padding,1,2,fp);
  fclose(fp);

  printf("Successfully created '%s'\n", name);
}
