/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Trivial example to write 123U (half gray) values to default KTX2 output.
 * To make this more interesting, input array is of type float (this is done to
 * replicate imageinout_test.cpp behaviour in OIIO testing suite).
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

#define CHECK_OIIO_RESULT(rv, interface)                                       \
  do {                                                                         \
    if (!rv) {                                                                 \
      if (interface->has_error()) {                                            \
        std::cerr << "fatal error: " << interface->geterror() << std::endl;    \
        return 1;                                                              \
      }                                                                        \
      std::cerr << "some error encountered: " << std::endl;                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define CHECK_OIIO_RESULT_G(rv)                                                \
  do {                                                                         \
    if (!rv) {                                                                 \
      std::cerr << "fatal error: " << OIIO::geterror() << std::endl;           \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(int argc, char **argv) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " KTX2_OUTPUT_FILEPATH" << std::endl

  if (argc != 2) {
    PRINT_USAGE();
    return 1;
  }

  const auto out_fp = std::filesystem::path(argv[1]);
  if (std::filesystem::exists(out_fp)) {
    std::cerr << "output file already exists" << std::endl;
    return 1;
  }

  const int xres = 40, yres = 40, nchannels = 3;
  const size_t pixels_pitch = xres * nchannels;
  std::vector<float> pixels(pixels_pitch * yres, 0.5f);

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  CHECK_OIIO_RESULT_G(out);

  ImageSpec outspec(xres, yres, nchannels, TypeDesc::UINT8);
  bool rv = out->open(out_fp, outspec);
  CHECK_OIIO_RESULT(rv, out);

  rv = out->write_image(TypeDesc::FLOAT, pixels.data());
  CHECK_OIIO_RESULT(rv, out);

  rv = out->close();
  CHECK_OIIO_RESULT(rv, out);

  return 0;
}
