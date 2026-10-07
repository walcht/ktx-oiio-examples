/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Extract faces from given cubemap KTX2 file into a given directory.
 * Showcases how to read particular cubemap faces.
 *
 * Like DDS, reading a cubemap slice is done via `read_tile(s)`.
 *
 * KTX supports incomplete cubemaps but these are rarely used and are NOT
 * supported by OIIO KTX pluging.
 *
 * KTX Cubemap faces are always stored in the order: +X, -X, +Y, -Y, +Z, -Z in a
 * left-handed coordinate system with +Y up and, with the +Z face forward, +X on
 * the on the right.
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
    std::cerr << "usage: " << argv[0] << " KTX2_INPUT_FILEPATH OUTPUT_DIR"     \
              << '\n';                                                         \
  } while (0)

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

  const auto inp_fp = std::filesystem::path(argv[1]);
  const auto out_dir = std::filesystem::path(argv[2]);

  if (!std::filesystem::exists(inp_fp)) {
    std::cerr << "provided input KTX2 file does not exist: " << inp_fp << '\n';
    return 1;
  }

  if (!std::filesystem::is_directory(out_dir)) {
    std::cerr << "output directory has to be provided and be existant: "
              << out_dir << std::endl;
    return 1;
  }

  auto inp = ImageInput::open(inp_fp);
  CHECK_OIIO_RESULT_G(inp);

  // Is the input KTX2 texture a cubemap texture?
  if (auto Q = inp->spec().find_attribute("textureformat", TypeDesc::STRING)) {
    if (!Strutil::iequals(Q->get_string(), "CubeFace Environment")) {
      std::cerr << "input KTX2 texture is not a cubemap texture" << std::endl;
      return 1;
    }
  } else {
    // for KTX2, this attribute is ALWAYS
    std::cerr << "attribute \"textureformat\" is not set" << std::endl;
    return 1;
  }

  int miplevel = 0;
  const int subimage = 0;

  // Faces in KTX2 are all of the same dimensions.
  // Simply allocate a single buffer of size base mipmap (all other mips are
  // smaller so this is sufficient).
  std::vector<uint8_t> pixels(inp->spec().tile_bytes());
  assert(inp->spec().tile_bytes() == inp->spec().tile_width *
                                         inp->spec().tile_height *
                                         inp->spec().pixel_bytes());
  while (inp->seek_subimage(subimage, miplevel)) {
    bool rv = true;
    const ImageSpec &curr_spec = inp->spec_dimensions(subimage, miplevel);
    for (int i = 0; i < 6; ++i) {
      rv = inp->read_tiles(subimage, miplevel, 0, curr_spec.tile_width,
                           i * curr_spec.tile_height,
                           (i + 1) * curr_spec.tile_height, 0, 1, 0,
                           curr_spec.nchannels,
                           make_span(pixels.data(), curr_spec.tile_bytes()));
      CHECK_OIIO_RESULT(rv, inp);

      const auto out_fp =
          out_dir / fmt::format("{}_miplvl_{}_face_{}.png",
                                inp_fp.stem().c_str(), miplevel, i);
      std::unique_ptr<ImageOutput> out = ImageOutput::create("png");
      CHECK_OIIO_RESULT_G(out);

      ImageSpec out_spec(curr_spec.tile_width, curr_spec.tile_height,
                         curr_spec.nchannels);
      rv = out->open(out_fp, out_spec);
      CHECK_OIIO_RESULT(rv, out);

      rv = out->write_image(make_span(pixels.data(), out_spec.image_bytes()));
      CHECK_OIIO_RESULT(rv, out);

      rv = out->close();
      CHECK_OIIO_RESULT(rv, out);
    }
    ++miplevel;
  }

  auto rv = inp->close();
  CHECK_OIIO_RESULT(rv, inp);

  return 0;
}
