/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Trivial example to test if KTX2 output correctly implements AppendSubimage
 * mode where open() is called multiple times to append subimages (depth slices
 * for 3D volume or faces for cubemaps). To keep it simple, AppendMIPLevel is
 * not used here to add multiple mip levels per subimage. For testing
 * AppendSubimage + AppendMIPLevel see `append_mipmaps_to_subimages.cpp`.
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

#define PRINT_ERROR(FUNCTION_NAME)                                             \
  do {                                                                         \
    if (OIIO::has_error())                                                     \
      std::cerr << #FUNCTION_NAME << " failed. Reason: " << OIIO::geterror()   \
                << '\n';                                                       \
    else                                                                       \
      std::cerr << #FUNCTION_NAME << " failed." << '\n';                       \
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
  int nsubimages = spec.depth;

  std::cout << "number of subimages to write: " << nsubimages << std::endl;

  //
  // Read base mip level. All slices/depths should have the same dimensions,
  // nchannels, types, etc. KTX does not support different dimensions per
  // slice/face.
  //
  std::vector<unsigned char> pixels(spec.width * spec.height * spec.nchannels);

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  if (!out) {
    return 1;
  }

  // Be sure we can support mipmaps. Note that KtxOutput does not support
  // "appendsubimage" which means we have to specify, beforehand, the number of
  // subimages we willing to write.
  if (nsubimages <= 1 || !out->supports("multiimage")) {
    std::cerr << "cannot write a subimage\n";
    return 1;
  }

  // Set up spec for the highest resolution
  ImageSpec outspec(spec);

  // It is fine to just pass one ImageSpec element. Ktx does not support
  // different 'ImageSpec's for subimages. All provided subimages MUST have the
  // same spec as the first provided subimage.
  if (!out->open(out_fp, nsubimages, &outspec)) {
    PRINT_ERROR(out->open(mode : Create));
    return 1;
  }

  //
  // Write the base mip level for each subimage (volume depth slice or cubemap
  // face) To make things simple, only write base mip level (i.e., each
  // slice/face has exactly one mip level which is mip level 0)
  //
  for (int subimage = 0; subimage < nsubimages; ++subimage) {
    // First, read input subimage into our pixels buffer
    //
    // Q. Why not use read_image?
    // A. In the input is a 3D image, read_image() with a particular subimage,
    //    returns a 3D submimage and not a 2D slice.
    //
    if (!inp->read_scanlines(0, 0, 0, spec.height, subimage, 0, spec.nchannels,
                             TypeDesc::UINT8, pixels.data())) {
      // no need to call close() since it will be called in the destructor
      PRINT_ERROR(read_scanlines());
      return 1;
    }

    // Open the output KTX2 format in AppendSubimage mode
    // if (!out->open(out_fp, outspec, ImageOutput::AppendSubimage)) {
    //   PRINT_ERROR(out->open(mode : AppendSubimage));
    //   return 1;
    // }

    // Then write the previously-read subimage pixels buffer into the output
    // KTX2 format
    if (!out->write_scanlines(0, outspec.height, subimage, TypeDesc::UINT8, pixels.data())) {
      PRINT_ERROR(out->write_scanlines());
      return 1;
    }
  }

  // Will be called when inp goes out of scope. We do it here for better error
  // reporting
  if (!inp->close()) {
    PRINT_ERROR(inp->close);
    return 1;
  }

  // Will be called when out goes out of scope. We do it here for better error
  // reporting
  if (!out->close()) {
    PRINT_ERROR(out->close);
    return 1;
  }

  std::cout << "success" << '\n';

  return 0;
}
