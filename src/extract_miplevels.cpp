/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Trivial example to read some given KTX2 file and extract its mipmaps as PNGs
 * into given folder.
 */

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/span.h>
#include <filesystem>

using namespace OpenImageIO;

#define PRINT_USAGE()                                                          \
  do {                                                                         \
    std::cout << "usage: " << argv[0] << " KTX2_INPUT_FILEPATH OUTPUT_DIR"     \
              << std::endl;                                                    \
  } while (0)

#define CHECK_OIIO_RESULT(call, msg, reason)                                   \
  do {                                                                         \
    if (!call) {                                                               \
      std::cerr << fmt::format("{} failed. Reason: {}", msg, reason)           \
                << std::endl;                                                  \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(int argc, char *argv[]) {
  if (argc < 3) {
    PRINT_USAGE();
    return 1;
  }

  const auto inp_fp = std::filesystem::path(argv[1]);
  const auto out_dir = std::filesystem::path(argv[2]);

  if (!std::filesystem::exists(inp_fp)) {
    std::cerr << "provided input KTX2 file does not exist: " << inp_fp
              << std::endl;
    return 1;
  }

  if (!std::filesystem::is_directory(out_dir)) {
    std::cerr << "output directory has to be provided and be existant: "
              << out_dir << std::endl;
    return 1;
  }

  const auto base_fn = inp_fp.stem();

  const std::unique_ptr<ImageInput> inp = ImageInput::open(inp_fp);
  CHECK_OIIO_RESULT(inp, "inp->open()", OIIO::geterror());

  ImageSpec spec = inp->spec_dimensions(0, 0);

  // Allocate a single array for the base mip level

  // construct a single buffer that can hold the base miplvl (subsequently, all
  // other mips as well)
  const size_t base_miplvl_size_bytes = spec.image_bytes();
  std::vector<uint8_t> pixels(base_miplvl_size_bytes);
  int miplevel = 0;
  while (inp->seek_subimage(0, miplevel)) {
    bool rv = true;
    const ImageSpec curr_spec = inp->spec_dimensions(0, miplevel);
    const size_t buffer_size_bytes = curr_spec.image_bytes();
    assert(buffer_size_bytes == static_cast<imagesize_t>(curr_spec.width) *
                                    curr_spec.height * curr_spec.pixel_bytes());

    rv = inp->read_image(0, miplevel, 0, curr_spec.nchannels,
                         make_span(pixels.data(), buffer_size_bytes));
    CHECK_OIIO_RESULT(rv, "inp->read_image()", inp->geterror());

    std::filesystem::path mip_fn =
        out_dir / fmt::format("{}_miplvl_{}.png", base_fn.c_str(), miplevel);
    const std::unique_ptr<ImageOutput> out = ImageOutput::create("png");
    rv = out->open(mip_fn, curr_spec);
    CHECK_OIIO_RESULT(rv, "out->open()", out->geterror());

    rv = out->write_image(make_span(pixels.data(), buffer_size_bytes));
    CHECK_OIIO_RESULT(rv, "out->write_image()", out->geterror());

    rv = out->close();
    CHECK_OIIO_RESULT(rv, "out->close()", out->geterror());

    ++miplevel;
  }

  CHECK_OIIO_RESULT(inp->close(), "inp->close()", inp->geterror());
  return 0;
}
