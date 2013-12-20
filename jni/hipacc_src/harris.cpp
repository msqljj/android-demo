//
// Copyright (c) 2013, University of Erlangen-Nuremberg
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <iostream>
#include <vector>
#include <numeric>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "hipacc.hpp"

using namespace hipacc;


// Kernel description in HIPAcc
class Harris : public Kernel<float> {
  private:
    Accessor<float> &inputX;
    Accessor<float> &inputY;
    Domain &dom;
    Mask<float> &mask;
    float k;

  public:
    Harris(IterationSpace<float> &iter,
           Accessor<float> &inputX, Accessor<float> &inputY,
           Domain &dom, Mask<float> &mask, float k)
          : Kernel(iter), inputX(inputX), inputY(inputY),
            dom(dom), mask(mask), k(k) {
      addAccessor(&inputX);
      addAccessor(&inputY);
    }

    void kernel() {
      float sumX = 0.0f;
      float sumY = 0.0f;
      float sumXY = 0.0f;

      iterate(dom, [&] () {
        float dx = inputX(dom);
        float dy = inputY(dom);
        sumX += mask(dom) * dx * dx;
        sumY += mask(dom) * dy * dy;
        sumXY += mask(dom) * dx * dy;
      });

      output() = ((sumX * sumY) - (sumXY * sumXY))      /* det   */
                 - (k * (sumX + sumY) * (sumX + sumY)); /* trace */
    }
};

class HarrisDeriv : public Kernel<float> {
  private:
    Accessor<uchar> &input;
    Mask<float> &mask;

  public:
    HarrisDeriv(IterationSpace<float> &iter,
                Accessor<uchar> &input, Mask<float> &mask)
          : Kernel(iter), input(input), mask(mask) {
      addAccessor(&input);
    }

    void kernel() {
      output() = convolve(mask, HipaccSUM, [&] () -> float {
                   return input(mask) * mask();
                 });
    }
};


// Main function
#ifdef HIPACC
int w = 1024;
int h = 1024;
uchar4 *in;
uchar4 *out;
int main(int argc, const char **argv) {
#else
#ifndef FILTERSCRIPT
int runRSHarris(int w, int h, uchar4 *in, uchar4 *out) {
#else
int runFSHarris(int w, int h, uchar4 *in, uchar4 *out) {
#endif
#endif
  float k = 0.04f;
  float threshold = 20000.0f;
  const int width = w;
  const int height = h;
  const int size_x = SIZE_X;
  const int size_y = SIZE_Y;
  float timing = 0.0f;

  // only filter kernel sizes 3x3, 5x5, and 7x7 implemented
  if (size_x != size_y || !(size_x == 3 || size_x == 5 || size_x == 7)) {
      return -1;
  }

  uchar4 *host_in = in;
  uchar4 *host_out = out;
  uchar *filter_in = new uchar[width * height];
  float *result = new float[width * height];

  for (int i = 0; i < width * height; ++i) {
      filter_in[i] = .2126 * in[i].x + .7152 * in[i].y + .0722 * in[i].z;
  }

  // convolution filter masks
  const float mask_x[9] = {
    -0.166666667f,          0.0f,  0.166666667f,
    -0.166666667f,          0.0f,  0.166666667f,
    -0.166666667f,          0.0f,  0.166666667f
  };
  const float mask_y[9] = {
    -0.166666667f, -0.166666667f, -0.166666667f,
             0.0f,          0.0f,          0.0f,
     0.166666667f,  0.166666667f,  0.166666667f
  };
  const float gauss[] = {
#if SIZE_X == 3
    0.057118f, 0.124758f, 0.057118f,
    0.124758f, 0.272496f, 0.124758f,
    0.057118f, 0.124758f, 0.057118f
#endif
#if SIZE_X == 5
    0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f,
    0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f,
    0.026151f, 0.090339f, 0.136565f, 0.090339f, 0.026151f,
    0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f,
    0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f
#endif
#if SIZE_X == 7
    0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841,
    0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010,
    0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471,
    0.008351, 0.029902, 0.064280, 0.082959, 0.064280, 0.029902, 0.008351,
    0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471,
    0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010,
    0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841
#endif
  };

  // input and output image of width x height pixels
  Image<uchar> IN(width, height);
  Image<float> RES(width, height);
  Image<float> DX(width, height);
  Image<float> DY(width, height);

  IN = filter_in;
  RES = result;

  Domain D(size_x, size_y);

  Mask<float> MX(3, 3);
  Mask<float> MY(3, 3);
  MX = mask_x;
  MY = mask_y;

  Mask<float> G(size_x, size_y);
  G = gauss;

  BoundaryCondition<uchar> BcInClamp(IN, 3, 3, BOUNDARY_CLAMP);
  Accessor<uchar> AccInClamp(BcInClamp);
  IterationSpace<float> IsDx(DX);
  HarrisDeriv dx(IsDx, AccInClamp, MX);

  dx.execute();
  timing = hipaccGetLastKernelTiming();

  IterationSpace<float> IsDy(DY);
  HarrisDeriv dy(IsDy, AccInClamp, MY);

  dy.execute();
  timing += hipaccGetLastKernelTiming();

  BoundaryCondition<float> BcDxClamp(DX, G, BOUNDARY_CLAMP);
  Accessor<float> AccDx(BcDxClamp);
  BoundaryCondition<float> BcDyClamp(DY, G, BOUNDARY_CLAMP);
  Accessor<float> AccDy(BcDyClamp);
  IterationSpace<float> IsRes(RES);
  Harris filter(IsRes, AccDx, AccDy, D, G, k);

  filter.execute();
  timing += hipaccGetLastKernelTiming();

  // get results
  result = RES.getData();

  // draw output
  memcpy(host_out, host_in, sizeof(uchar4) * width * height);

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; y++) {
      int pos = y*width+x;
      if (result[pos] > threshold) {
        for (int i = -10; i <= 10; ++i) {
          if (x+i >= 0 && x+i < width) {
            host_out[pos+i].x =
                host_out[pos+i].y =
                host_out[pos+i].z = 255;
          }
        }
        for (int i = -10; i <= 10; ++i) {
          if (y+i > 0 && y+i < height) {
            host_out[pos+(i*width)].x =
                host_out[pos+(i*width)].y =
                host_out[pos+(i*width)].z = 255;
          }
        }
      }
    }
  }

  delete[] result;
  delete[] filter_in;

  return timing;
}
