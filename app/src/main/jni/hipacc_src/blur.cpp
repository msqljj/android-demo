#include <cstring>

#include "hipacc.hpp"
#include "filter_name.hpp"

using namespace hipacc;


// Kernel description in Hipacc
class Blur : public Kernel<uchar4> {
  private:
    Accessor<uchar4> &input;
    Domain &dom;
    const int size_x, size_y;

  public:
    Blur(IterationSpace<uchar4> &iter, Accessor<uchar4> &input, Domain &dom,
         const int size_x, const int size_y)
            : Kernel(iter), input(input), dom(dom),
              size_x(size_x), size_y(size_y) {
        add_accessor(&input);
    }

    void kernel() {
        uint4 sum = reduce(dom, Reduce::SUM, [&] () -> uint4 {
                        return convert_uint4(input(dom));
                    });
        output() = convert_uchar4(convert_float4(sum)/(size_x*size_y));
    }
};


// Main function
FILTER_NAME(Blur) {
    const int width = w;
    const int height = h;
    const int size_x = SIZE_X;
    const int size_y = SIZE_Y;
    const int offset_x = size_x >> 1;
    const int offset_y = size_y >> 1;

    // input and output image of width x height pixels
    Image<uchar4> In(width, height, pin);
    Image<uchar4> Out(width, height, pout);

    // define domain for blur filter
    Domain D(size_x, size_y);

    BoundaryCondition<uchar4> BcInClamp(In, D, Boundary::CLAMP);
    Accessor<uchar4> AccInClamp(BcInClamp);

    IterationSpace<uchar4> IsOut(Out);
    Blur filter(IsOut, AccInClamp, D, size_x, size_y);

    filter.execute();
    float timing = hipacc_last_kernel_timing();

    // get pointer to result data
    uchar4 *result = Out.data();
    std::memcpy(pout, result, sizeof(uchar4) * width * height);

    return timing;
}
