#include <cstring>

#include "hipacc.hpp"
#include "filter_name.hpp"

using namespace hipacc;


// Kernel description in Hipacc
class Gaussian : public Kernel<uchar4> {
  private:
    Accessor<uchar4> &input;
    Mask<float> &mask;

  public:
    Gaussian(IterationSpace<uchar4> &iter, Accessor<uchar4> &input,
             Mask<float> &mask)
            : Kernel(iter), input(input), mask(mask) {
        add_accessor(&input);
    }

    void kernel() {
        float4 sum = convolve(mask, Reduce::SUM, [&] () -> float4 {
                         return mask() * convert_float4(input(mask));
                     });
        output() = convert_uchar4(sum + 0.5f);
    }
};


// Main function
FILTER_NAME(Gaussian) {
    const int width = w;
    const int height = h;
    const int size_x = SIZE_X;
    const int size_y = SIZE_Y;
    const int offset_x = size_x >> 1;
    const int offset_y = size_y >> 1;

    // only filter kernel sizes 3x3, 5x5, and 7x7 implemented
    if (size_x != size_y || !(size_x == 3 || size_x == 5 || size_x == 7)) {
        return -1;
    }

    // convolution filter mask
    const float mask[size_y][size_x] = {
#if SIZE_X == 3
        { 0.057118f, 0.124758f, 0.057118f },
        { 0.124758f, 0.272496f, 0.124758f },
        { 0.057118f, 0.124758f, 0.057118f }
#endif
#if SIZE_X == 5
        { 0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f },
        { 0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f },
        { 0.026151f, 0.090339f, 0.136565f, 0.090339f, 0.026151f },
        { 0.017300f, 0.059761f, 0.090339f, 0.059761f, 0.017300f },
        { 0.005008f, 0.017300f, 0.026151f, 0.017300f, 0.005008f }
#endif
#if SIZE_X == 7
        { 0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841 },
        { 0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010 },
        { 0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471 },
        { 0.008351, 0.029902, 0.064280, 0.082959, 0.064280, 0.029902, 0.008351 },
        { 0.006471, 0.023169, 0.049806, 0.064280, 0.049806, 0.023169, 0.006471 },
        { 0.003010, 0.010778, 0.023169, 0.029902, 0.023169, 0.010778, 0.003010 },
        { 0.000841, 0.003010, 0.006471, 0.008351, 0.006471, 0.003010, 0.000841 }
#endif
    };

    // input and output image of width x height pixels
    Image<uchar4> In(width, height, pin);
    Image<uchar4> Out(width, height, pout);

    // filter mask
    Mask<float> M(mask);

    BoundaryCondition<uchar4> BcInClamp(In, M, Boundary::CLAMP);
    Accessor<uchar4> AccInClamp(BcInClamp);
    IterationSpace<uchar4> IsOut(Out);
    Gaussian filter(IsOut, AccInClamp, M);

    filter.execute();
    float timing = hipacc_last_kernel_timing();

    // get pointer to result data
    uchar4 *result = Out.data();
    std::memcpy(pout, result, sizeof(uchar4) * width * height);

    return timing;
}
