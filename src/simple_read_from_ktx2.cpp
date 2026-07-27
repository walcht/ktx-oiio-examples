/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trivial example to read some given KTX2 file and write to given output
 * format.
 *
 * Example invocation:
 *  simple_read_from_ktx2 resources/ktx2/2d_uastc.ktx2 2d_uastc.png
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

int main(int argc, char *argv[]) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " KTX2_INPUT_FILEPATH OUTPUT_FILEPATH"  \
            << '\n'

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

  const auto inp_fp = std::filesystem::path(argv[1]);
  const auto out_fp = argv[2];
  if (!std::filesystem::exists(inp_fp)) {
    std::cerr << "provided input KTX2 file does not exist: " << inp_fp << '\n';
    return 1;
  }

  auto inp = ImageInput::open(inp_fp);
  if (!inp) {
    std::cerr << "inp->open() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }

  const ImageSpec &spec = inp->spec();
  int xres = std::max(spec.width >> 0, 1);
  int yres = std::max(spec.height >> 0, 1);
  [[maybe_unused]] int zres = std::max(spec.depth >> 0, 1);
  int nchannels = spec.nchannels;
  int subimage = 0;

  std::vector<unsigned char> pixels(xres * yres * nchannels);
  if (!inp->read_image(subimage, 0, 0, nchannels, make_span(pixels))) {
    std::cerr << "ImageInput::read_image() failed. Reason: " << OIIO::geterror()
              << '\n';
    // no need to call close() since it will be called in the destructor
    return 1;
  }

  if (!inp->close()) {
    std::cerr << "inp->close() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }

  std::unique_ptr<ImageOutput> out = ImageOutput::create(out_fp);
  if (!out) {
    return 1;
  }
  ImageSpec outspec(xres, yres, nchannels, TypeDesc::UINT8);
  if (!out->open(out_fp, outspec)) {
    return 1;
  }
  if (!out->write_image(make_span(pixels))) {
    return 1;
  }
  if (!out->close()) {
    return 1;
  }
  return 0;
}
