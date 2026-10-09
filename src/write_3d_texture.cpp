/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Write input slices into a 3D KTX2 texture file.
 *
 * Showcases how to use write_tiles() to write slices to an opened KTX2 output
 * 3D texture file. write_image() CANNOT be used here!
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

int main(int argc, char *argv[]) {
#define PRINT_USAGE()                                                          \
  do {                                                                         \
    std::cerr << "usage: " << argv[0]                                          \
              << " SLICE_0 [SLINE_N...] KTX2_OUTPUT_FILE" << '\n';             \
  } while (0)

  if (argc < 3) {
    PRINT_USAGE();
    return 1;
  }

  const int nbr_slices = argc - 2;
  std::vector<std::filesystem::path> inp_slices(nbr_slices);
  for (size_t slice_idx = 0; slice_idx < inp_slices.size(); ++slice_idx) {
    auto slice_fp = std::filesystem::path(argv[slice_idx + 1]);
    if (!std::filesystem::exists(slice_fp)) {
      std::cerr << fmt::format(
                       "provided filepath {} for slice {} does not exist",
                       slice_fp.c_str(), slice_idx)
                << std::endl;
      return 1;
    }
    inp_slices[slice_idx] = slice_fp;
  }

  const auto out_fp = std::filesystem::path(argv[argc - 1]);
  if (std::filesystem::exists(out_fp)) {
    std::cerr << "output KTX2 filepath does exist" << std::endl;
    return 1;
  }

  bool rv = true;

  ImageSpec slice_spec;

  // Before anything, we need to know the dimensions and we need to make sure
  // that all slices have the same dimension (this is required by KTX2).
  {
    {
      auto inp = ImageInput::open(inp_slices[0]);
      CHECK_OIIO_RESULT_G(inp);

      slice_spec = inp->spec_dimensions(0, 0);

      rv = inp->close();
      CHECK_OIIO_RESULT(rv, inp);
    }

    for (size_t slice_idx = 1; slice_idx < inp_slices.size(); ++slice_idx) {
      auto inp = ImageInput::open(inp_slices[slice_idx]);
      CHECK_OIIO_RESULT_G(inp);

      if (inp->spec().width != slice_spec.width ||
          inp->spec().height != slice_spec.height ||
          inp->spec().nchannels != slice_spec.nchannels) {
        std::cerr << "all input slices MUST have the same dimensions"
                  << std::endl;
        return 1;
      }

      rv = inp->close();
      CHECK_OIIO_RESULT(rv, inp);
    }
  }

  std::vector<uint8_t> pixels(slice_spec.width * slice_spec.height *
                              slice_spec.pixel_bytes());
  const int subimage = 0;
  const int miplevel = 0;

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  CHECK_OIIO_RESULT_G(out);

  ImageSpec out_spec(slice_spec.width, slice_spec.height, slice_spec.nchannels,
                     slice_spec.format);
  // MUST set the tile dimensions as follows (width, 1, 1). This is done because
  // writing 3D textures through OIIO API can only currently be done via
  // write_tile(s) API calls.
  out_spec.depth = out_spec.full_depth = nbr_slices;
  out_spec.tile_width = slice_spec.width;
  out_spec.tile_height = 1;
  out_spec.tile_depth = 1;
  out_spec.attribute("textureformat", "Volume Texture");

  rv = out->open(out_fp, out_spec);
  CHECK_OIIO_RESULT(rv, out);

  for (size_t slice_idx = 0; slice_idx < inp_slices.size(); ++slice_idx) {
    auto inp = ImageInput::open(inp_slices[slice_idx]);
    CHECK_OIIO_RESULT_G(inp);

    rv = inp->read_image(subimage, miplevel, 0, slice_spec.nchannels,
                         make_span(pixels));
    CHECK_OIIO_RESULT(rv, inp);

    rv = out->write_tiles(0, slice_spec.width, 0, slice_spec.height, slice_idx,
                          slice_idx + 1, make_span(pixels));
    CHECK_OIIO_RESULT(rv, out);

    rv = inp->close();
    CHECK_OIIO_RESULT(rv, inp);
  }

  rv = out->close();
  CHECK_OIIO_RESULT(rv, out);

  return 0;
}
