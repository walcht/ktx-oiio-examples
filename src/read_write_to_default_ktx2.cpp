/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trivial example to read any supported format by OIIO and write default KTX2
 * output.
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

int main(int argc, char **argv) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " INPUT_FILEPATH OUTPUT_FILEPATH" << '\n'

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

  const auto inp_fp = std::filesystem::path(argv[1]);
  const auto out_fp = argv[2];
  if (!std::filesystem::exists(inp_fp)) {
    std::cerr << "provided input file does not exist: " << inp_fp << '\n';
    return 1;
  }

  auto inp = ImageInput::open(inp_fp);
  if (!inp) {
    std::cerr << "ImageInput::open failed. Reason: " << OIIO::geterror()
              << '\n';
    return 1;
  }

  const ImageSpec &spec = inp->spec();

  std::vector<unsigned char> pixels(spec.width * spec.height * spec.nchannels);
  if (!inp->read_image(0, 0, 0, spec.nchannels, make_span(pixels))) {
    std::cerr << "ImageInput::read_image() failed. Reason: " << OIIO::geterror()
              << '\n';
    // no need to call close() since it will be called in the destructor
    return 1;
  }

  if (!inp->close()) {
    std::cerr << "inp->close() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  if (!out) {
    return 1;
  }

  // Use default KTX2 output (should use UASTC LDR 4x4)
  ImageSpec outspec(spec);
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
