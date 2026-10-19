/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

std::string construct_vkformat_str(int nchannels, TypeDesc typedesc,
                                   bool srgb = true, bool normalized = true) {
  const int type_size_in_bits = typedesc.size() * 8;
  std::string str{"VK_FORMAT_"};
  if (nchannels >= 1)
    str.append(1, 'R').append(1, type_size_in_bits);
  if (nchannels >= 2)
    str.append(1, 'G').append(1, type_size_in_bits);
  if (nchannels >= 3)
    str.append(1, 'B').append(1, type_size_in_bits);
  if (nchannels >= 4)
    str.append(1, 'A').append(1, type_size_in_bits);

  switch (typedesc.basetype) {
  case TypeDesc::UINT8:
    str += srgb ? "_SRGB" : normalized ? "_UNORM" : "_UINT";
    break;
  case TypeDesc::INT8:
    str += normalized ? "_SNORM" : "_SINT";
    break;
  case TypeDesc::UINT16:
    str += normalized ? "_UNORM" : "_UINT";
    break;
  case TypeDesc::INT16:
    str += normalized ? "_SNORM" : "_SINT";
    break;
  case TypeDesc::UINT32:
    [[fallthrough]];
  case TypeDesc::UINT64:
    str += "_UINT";
    break;
  case TypeDesc::INT32:
    [[fallthrough]];
  case TypeDesc::INT64:
    str += "_SINT";
    break;
  case TypeDesc::HALF:
    [[fallthrough]];
  case TypeDesc::FLOAT:
    [[fallthrough]];
  case TypeDesc::DOUBLE:
    str += "_SFLOAT";
    break;
  default:
    break;
  }

  return str;
}

#define CHECK_OIIO_RESULT(rv, interface)                                       \
  do {                                                                         \
    if (!rv) {                                                                 \
      if (interface->has_error()) {                                            \
        std::cerr << "fatal error: " << interface->geterror() << std::endl;    \
        return 1;                                                              \
      }                                                                        \
      std::cerr << "some unknown error encountered" << std::endl;              \
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

int main(int argc, char **argv) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " OUTPUT_DIR" << std::endl

  if (argc != 2) {
    PRINT_USAGE();
    return 1;
  }

  const auto out_dir = std::filesystem::path(argv[1]);

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

  const int xres = 40, yres = 40, nchannels = 4;
  const size_t pixels_pitch = xres * nchannels;
  std::vector<double> pixels(pixels_pitch * yres, 0.5);

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  CHECK_OIIO_RESULT_G(out);

  const std::array<TypeDesc, 11> target_types{
      TypeDesc::UINT8,  TypeDesc::INT8,   TypeDesc::UINT16, TypeDesc::INT16,
      TypeDesc::HALF,   TypeDesc::UINT32, TypeDesc::INT32,  TypeDesc::FLOAT,
      TypeDesc::UINT64, TypeDesc::INT64,  TypeDesc::DOUBLE};

  for (int nchannels_output = 1; nchannels_output <= 4; ++nchannels_output) {
    for (auto const &target_type : target_types) {
      std::filesystem::path out_fp =
          out_dir /
          fmt::format("grayscale_{}.ktx2",
                      construct_vkformat_str(nchannels_output, target_type));
      ImageSpec outspec(xres, yres, nchannels_output, target_type);
      bool rv = out->open(out_fp, outspec);
      CHECK_OIIO_RESULT(rv, out);

      rv = out->write_image(TypeDesc::DOUBLE, pixels.data(),
                            nchannels * sizeof(double));
      CHECK_OIIO_RESULT(rv, out);

      rv = out->close();
      CHECK_OIIO_RESULT(rv, out);
    }
  }

  return 0;
}
