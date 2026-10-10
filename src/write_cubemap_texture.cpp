/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Write six input faces into a cubemap texture
 *
 * Showcases how to use write_tiles() to write faces to an opened KTX2 output
 * cubemap texture file. write_image() CANNOT be used here!
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
    std::cerr                                                                  \
        << "usage: " << argv[0]                                                \
        << " FACE_0 FACE_1 FACE_2 FACE_3 FACE_4 FACE_4 FACE_5 "                \
           "KTX2_OUTPUT_FILE"                                                  \
        << '\n'                                                                \
        << "In KTX2, faces are ALWAYS organized as follows: +X -X +Y -Y +Z -Z" \
        << std::endl;                                                          \
  } while (0)

  if (argc != 8) {
    PRINT_USAGE();
    return 1;
  }

  std::array<std::filesystem::path, 6> inp_faces{argv[1], argv[2], argv[3],
                                                 argv[4], argv[5], argv[6]};
  for (size_t face_idx = 0; face_idx < inp_faces.size(); ++face_idx) {
    if (!std::filesystem::exists(inp_faces[face_idx])) {
      std::cerr << fmt::format(
                       "provided filepath {} for face {} does not exist",
                       inp_faces[face_idx].c_str(), face_idx)
                << std::endl;
      return 1;
    }
  }

  const auto out_fp = std::filesystem::path(argv[argc - 1]);
  if (std::filesystem::exists(out_fp)) {
    std::cerr << "output KTX2 filepath does exist" << std::endl;
    return 1;
  }

  bool rv = true;

  ImageSpec face_spec;

  // Before anything, we need to know the dimensions and we need to make sure
  // that all input faces have the same dimensions (this is required by KTX2).
  {
    {
      auto inp = ImageInput::open(inp_faces[0]);
      CHECK_OIIO_RESULT_G(inp);

      face_spec = inp->spec_dimensions(0, 0);

      rv = inp->close();
      CHECK_OIIO_RESULT(rv, inp);
    }

    for (size_t face_idx = 1; face_idx < inp_faces.size(); ++face_idx) {
      auto inp = ImageInput::open(inp_faces[face_idx]);
      CHECK_OIIO_RESULT_G(inp);

      if (inp->spec().width != face_spec.width ||
          inp->spec().height != face_spec.height ||
          inp->spec().nchannels != face_spec.nchannels) {
        std::cerr << "all input faces MUST have the same dimensions"
                  << std::endl;
        return 1;
      }

      rv = inp->close();
      CHECK_OIIO_RESULT(rv, inp);
    }
  }

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  CHECK_OIIO_RESULT_G(out);

  // KTX2 tiles are ALWAYS organized as 1x6 layout (hence why the height is
  // multiplied by 6).
  ImageSpec out_spec(face_spec.width, face_spec.height * 6, face_spec.nchannels,
                     face_spec.format);
  out_spec.tile_width = face_spec.width;
  out_spec.tile_height = face_spec.height;

  std::vector<uint8_t> pixels(out_spec.tile_bytes());
  assert(out_spec.tile_bytes() ==
         out_spec.tile_width * out_spec.tile_height * out_spec.pixel_bytes());

  rv = out->open(out_fp, out_spec);
  CHECK_OIIO_RESULT(rv, out);

  for (size_t face_idx = 0; face_idx < 1; ++face_idx) {
    auto inp = ImageInput::open(inp_faces[face_idx]);
    CHECK_OIIO_RESULT_G(inp);

    const int subimage = 0;
    const int miplevel = 0;
    rv = inp->read_image(subimage, miplevel, 0, face_spec.nchannels,
                         make_span(pixels));
    CHECK_OIIO_RESULT(rv, inp);

    rv = out->write_tile(0, face_idx * out_spec.tile_height, 0,
                         make_span(pixels));
    CHECK_OIIO_RESULT(rv, out);

    rv = inp->close();
    CHECK_OIIO_RESULT(rv, inp);
  }

  rv = out->close();
  CHECK_OIIO_RESULT(rv, out);

  return 0;
}
