/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Extract layers (i.e., subimages) from given array KTX2 file into a given
 * directory.
 *
 * Showcases how to read array textures.
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

  const int miplevel = 0;
  int subimage = 0;

  // Textures in arrays in KTX2 are all required to have the same dimensions.
  // Simply allocate a single buffer of size base mipmap (all other mips are
  // smaller so this is sufficient).
  std::vector<uint8_t> pixels(inp->spec().image_bytes());
  assert(inp->spec().image_bytes() ==
         inp->spec().width * inp->spec().height * inp->spec().pixel_bytes());
  while (inp->seek_subimage(subimage, miplevel)) {
    bool rv = true;
    const ImageSpec &curr_spec = inp->spec_dimensions(subimage, miplevel);

    rv = inp->read_image(subimage, miplevel, 0, curr_spec.nchannels,
                         make_span(pixels));
    CHECK_OIIO_RESULT(rv, inp);

    const auto out_fp =
        out_dir / fmt::format("{}_miplvl_{}_layer_{}.png",
                              inp_fp.stem().c_str(), miplevel, subimage);
    std::unique_ptr<ImageOutput> out = ImageOutput::create("png");
    CHECK_OIIO_RESULT_G(out);

    rv = out->open(out_fp, curr_spec);
    CHECK_OIIO_RESULT(rv, out);

    rv = out->write_image(make_span(pixels.data(), curr_spec.image_bytes()));
    CHECK_OIIO_RESULT(rv, out);

    rv = out->close();
    CHECK_OIIO_RESULT(rv, out);

    ++subimage;
  }

  auto rv = inp->close();
  CHECK_OIIO_RESULT(rv, inp);

  return 0;
}
