/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Read an input file and write it as a default KTX2 output texture.
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <OpenImageIO/imageio.h>

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

int main(int argc, char *argv[]) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " INPUT_FILEPATH OUTPUT_FILEPATH"       \
            << std::endl

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

  const auto inp_fp = std::filesystem::path(argv[1]);
  const auto out_fp = std::filesystem::path(argv[2]);

  if (!std::filesystem::exists(inp_fp)) {
    std::cerr << "provided input file does not exist: " << inp_fp << std::endl;
    return 1;
  }

  if (std::filesystem::exists(out_fp)) {
    std::cerr << "provided output file already exists" << std::endl;
    return 1;
  }

  auto inp = ImageInput::open(inp_fp);
  CHECK_OIIO_RESULT_G(inp);

  const int miplvl = 0;
  const int subimage = 0;

  bool rv = true;

  const ImageSpec &spec = inp->spec();
  std::vector<unsigned char> pixels(spec.image_bytes());
  rv = inp->read_image(subimage, miplvl, 0, spec.nchannels, make_span(pixels));
  CHECK_OIIO_RESULT(rv, inp);

  rv = inp->close();
  CHECK_OIIO_RESULT(rv, inp);

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  CHECK_OIIO_RESULT_G(out);

  rv = out->open(out_fp, spec);
  CHECK_OIIO_RESULT(rv, out);

  rv = out->write_image(make_cspan(pixels));
  CHECK_OIIO_RESULT(rv, out);

  rv = out->close();
  CHECK_OIIO_RESULT(rv, out);

  return 0;
}
