/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Extract volume slices from given KTX2 file into a given directory.
 *
 * Showcases how to read 3D (i.e., volume) textures using OpenImageIO.
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

  if (!std::filesystem::exists(out_dir)) {
    if (!std::filesystem::create_directory(out_dir)) {
      std::cerr << "failed to create output directory at: " << out_dir
                << std::endl;
      return 1;
    }
  }

  if (!std::filesystem::is_directory(out_dir)) {
    std::cerr << "output directory has to be provided and be existant: "
              << out_dir << std::endl;
    return 1;
  }

  if (!std::filesystem::is_empty(out_dir)) {
    std::cerr << "output directory should be empty" << std::endl;
    return 1;
  }

  auto inp = ImageInput::open(inp_fp);
  CHECK_OIIO_RESULT_G(inp);

  // Is the input KTX2 texture a volume texture?
  if (auto Q = inp->spec().find_attribute("textureformat", TypeDesc::STRING)) {
    if (!Strutil::iequals(Q->get_string(), "Volume Texture")) {
      std::cerr << "input KTX2 texture is not a volume texture" << std::endl;
      return 1;
    }
  } else {
    // for KTX2, this attribute is ALWAYS set so this should never occur
    std::cerr << "attribute \"textureformat\" is not set" << std::endl;
    return 1;
  }

  const int subimage = 0;
  int miplevel = 0;

  bool rv = true;

  // Do NOT use spec->image_bytes() as this will return the size of a whole 3D
  // volume in bytes. What we want instead is the size of a slice in bytes of
  // the base miplevel volume.
  std::vector<uint8_t> pixels(inp->spec().width * inp->spec().height *
                              inp->spec().pixel_bytes());
  while (inp->seek_subimage(subimage, miplevel)) {
    const ImageSpec &curr_spec = inp->spec_dimensions(subimage, miplevel);

    // For 3D volumes, we read a particular slice using tile-based OIIO API
    // calls. This nay seem weird (and it does!) but this is the only way to
    // achieve this using the current OIIO API. A much better alternative could
    // have been setting the 'z' parameter for some read_image() calls but there
    // is none that accepts it while also accepting span-based data (some major
    // limitation if you were to ask me).
    assert(curr_spec.tile_width == curr_spec.width);
    assert(curr_spec.tile_height == 1);
    assert(curr_spec.tile_depth == 1);

    const size_t slice_size_in_bytes =
        curr_spec.width * curr_spec.height * curr_spec.pixel_bytes();

    for (int slice_idx = 0; slice_idx < curr_spec.depth; ++slice_idx) {
      rv = inp->read_tiles(subimage, miplevel, 0, curr_spec.tile_width, 0,
                           curr_spec.width, slice_idx, slice_idx + 1, 0,
                           curr_spec.nchannels,
                           make_span(pixels.data(), slice_size_in_bytes));
      CHECK_OIIO_RESULT(rv, inp);

      const auto out_fp =
          out_dir / fmt::format("{}_miplvl_{}_slice_{}.png",
                                inp_fp.stem().c_str(), miplevel, slice_idx);
      std::unique_ptr<ImageOutput> out = ImageOutput::create("png");
      CHECK_OIIO_RESULT_G(out);

      ImageSpec out_spec(curr_spec.width, curr_spec.height,
                         curr_spec.nchannels);
      rv = out->open(out_fp, out_spec);
      CHECK_OIIO_RESULT(rv, out);

      rv = out->write_image(make_span(pixels.data(), slice_size_in_bytes));
      CHECK_OIIO_RESULT(rv, out);

      rv = out->close();
      CHECK_OIIO_RESULT(rv, out);
    }
    ++miplevel;
  }

  rv = inp->close();
  CHECK_OIIO_RESULT(rv, inp);

  return 0;
}
