#! /bin/bash

# Copyright (c) 2013, Thomas L. Falch
# For conditions of distribution and use, see the accompanying LICENSE and README files

# This file is a part of the Scattered Point Visualization application
# developed at the Norwegian University of Science and Technology

for bmpfile in `ls *.bmp`; do
    convert -quality 100 $bmpfile `basename $bmpfile bmp`jpg;
done

ffmpeg -r 24 -i %04d.jpg -b 3000k -minrate 3000k rsw_`date +%H-%M-%S_%d-%m-%y`.mp4

rm -f *.jpg
