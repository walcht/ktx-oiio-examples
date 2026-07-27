/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trivial example to showcase how to use OpenImageIO to write (append) mipmaps.
 * open() with AppendMIPLevel mode is called multiple times (up until max
 * allowed MIP level) to append mipmaps. To keep it simple, only 2D input images
 * are accepted. If you want to append mipmaps to 3D images, then you need to
 * use 3D downsamplers which are currently not supported by OIIO.
 */

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/span.h>

#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

int main(int argc, char **argv) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " INPUT_FILEPATH OUTPUT_KTX2_FILEPATH"  \
            << '\n'

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

#define PRINT_ERROR(FUNCTION_NAME, ptr)                                        \
  do {                                                                         \
    if (ptr->has_error())                                                      \
      std::cerr << FUNCTION_NAME << " failed. Reason: " << ptr->geterror()     \
                << '\n';                                                       \
    else                                                                       \
      std::cerr << FUNCTION_NAME << " failed." << '\n';                        \
  } while (0)

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

  if (spec.depth > 1)
    std::cout << "volume input; ignoring volume slices of index >= 1" << '\n';

  // Read base mip level image (in case of volume, just reserve size of one
  // slice)
  std::vector<unsigned char> pixels(spec.width * spec.height * spec.nchannels);
  //
  // Q. Why not use read_image?
  // A. In the input is a 3D image, read_image() with a particular subimage,
  //    returns a 3D submimage and not a 2D slice.
  //
  if (!inp->read_scanlines(0, 0, 0, spec.height, 0, 0, spec.nchannels,
                           TypeDesc::UINT8, pixels.data())) {
    // no need to call close() since it will be called in the destructor
    PRINT_ERROR("read_scanlines()", inp);
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

  // Be sure we can support mipmaps
  if (!out->supports("mipmap")) {
    std::cerr << "cannot write a MIP-map\n";
    return 1;
  }

  // Set up spec for the highest resolution
  ImageSpec outspec(spec);
  // Can only perform 2D filtering (input image could be 3D)
  outspec.depth = 1;

  // Wrap subimage 0 in an ImageBuf (nothing is copied, nothing is owned)
  ImageBuf base_miplevel(outspec, make_cspan(pixels));

  // Write base image (MIP level 0)
  if (!out->open(out_fp, outspec, ImageOutput::Create)) {
    PRINT_ERROR("out->open()", out);
    return 1;
  }

  if (!out->write_image(TypeDesc::UINT8,
                        base_miplevel.localpixels_as_byte_image_span())) {
    PRINT_ERROR("out->write_image()", out);
    return 1;
  }

  // Write images, halving every time, until we're down to 1 pixel in either
  // dimension
  while ((outspec.width >>= 1) >= 1 && (outspec.height >>= 1) >= 1) {
    ROI roi(0, outspec.height, 0, outspec.width, 0, 1, 0, spec.nchannels);

    //
    // Important notes about OIIO filters vs. ktx tools' filters
    //  - the default filter used by ktx tools is lanczos4 which is not
    //  available in OIIO
    //  - a lot of other filters available in ktx tools (around 50%) are not
    //  available in OIIO
    //  - filter scaling option in ktx tools' cannot be used in OIIO (or I don't
    //  know how)
    //
    // So, if your input is a ktx2 file and you want to write a ktx2 file
    // (which, in general, is a very bad idea because each rewrite cycle
    // significantly worsens the quality), and if said input has mipmaps, you
    // will likely not be able to regenerate same mipmaps (at least using same
    // parameters).
    //
    ImageBuf dst = ImageBufAlgo::resize(base_miplevel,
                                        {{"filtername", "lanczos3"}}, roi, 1);
    if (!out->open(out_fp, outspec, ImageOutput::AppendMIPLevel)) {
      PRINT_ERROR("out->open()", out);
      return 1;
    }
    if (!out->write_image(TypeDesc::UINT8,
                          dst.localpixels_as_byte_image_span())) {
      PRINT_ERROR("out->write_image()", out);
      return 1;
    }
  }

  if (!out->close()) {
    PRINT_ERROR("out->close()", out);
    return 1;
  }

  std::cout << "success" << '\n';

  return 0;
}
