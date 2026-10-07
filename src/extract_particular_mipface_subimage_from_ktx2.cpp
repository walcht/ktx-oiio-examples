/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Trivial example to read some given KTX2 file and write to given output
 * format.
 *
 * Example invocation:
 *  simple_read_from_ktx2 resources/ktx2/2d_uastc.ktx2 2d_uastc.png 0
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

int main(int argc, char *argv[]) {
#define PRINT_USAGE()                                                          \
  std::cout << "usage: " << argv[0]                                            \
            << " KTX2_INPUT_FILEPATH OUTPUT_FILEPATH [miplevel] [subimage]"    \
            << std::endl;

  if (argc < 3) {
    PRINT_USAGE();
    return 1;
  }

  int miplevel = 0;
  int subimage = 0;

  if (argc == 4) {
    try {
      miplevel = std::stoi(argv[3]);
    } catch (std::exception &e) {
      std::cerr << "failed to convert miplevel argv string to int: " << argv[3]
                << std::endl;
      return 1;
    }
  }

  if (argc == 5) {
    try {
      subimage = std::stoi(argv[4]);
    } catch (std::exception &e) {
      std::cerr << "failed to convert subimage argv string to int: " << argv[4]
                << std::endl;
      return 1;
    }
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

  // Call seek_subimage on the target miplevel so we know the size to allocate
  // for our buffer and so that we verify that the provided miplevel is valid.
  // Alternately, you can also use the attribute `ktx:miplevels` to get how
  // many miplevels the input KTX2 texture contains.
  if (!inp->seek_subimage(0, miplevel)) {
    std::cerr << "ImageInput::seek_subimage() to miplevel=" << miplevel
              << " failed (the provided miplevel is probably non-existant).\n";
    return 1;
  }

  const ImageSpec spec = inp->spec_dimensions(subimage, miplevel);
  const imagesize_t buffer_size = spec.image_bytes();
  assert(buffer_size ==
         static_cast<imagesize_t>(spec.width) * spec.height * spec.nchannels);
  std::vector<unsigned char> pixels(buffer_size);
  if (!inp->read_image(subimage, miplevel, 0, spec.nchannels,
                       make_span(pixels))) {
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
