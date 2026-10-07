/*
 * Copyright 2026 Walid Chtioui @ walid.chtioui.main@gmail.com
 * SPDX-License-Identifier: MIT
 */

/*
 * Trivial example to write 123U (half gray) values to default KTX2 output.
 * To make this more interesting, input array is of type float (this is done to
 * replicate imageinout_test.cpp behaviour in OIIO testing suite).
 */

#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <filesystem>

using namespace OIIO;

int main() {
  const auto fn = "uastc_from_blank_input.ktx2";
  const auto fp = std::filesystem::current_path() / "output" / fn;
  const int xres = 40, yres = 40, nchannels = 3;
  const size_t pixels_pitch = xres * nchannels;
  std::vector<float> pixels(pixels_pitch * yres, 0.5f);

  std::unique_ptr<ImageOutput> out = ImageOutput::create("ktx2");
  if (!out) {
    std::cerr << "ImageOutput::create(\"ktx2\") failed." << '\n';
    return 1;
  }
  ImageSpec outspec(xres, yres, nchannels, TypeDesc::UINT8);
  if (!out->open(fp, outspec)) {
    if (OIIO::has_error())
      std::cerr << "out->close() failed. Reason: " << OIIO::geterror() << '\n';
    else
      std::cerr << "out->close() failed." << '\n';
    return 1;
  }
  if (!out->write_image(TypeDesc::FLOAT, pixels.data())) {
    if (OIIO::has_error())
      std::cerr << "out->write_image() failed. Reason: " << OIIO::geterror()
                << '\n';
    else
      std::cerr << "out->write_image() failed." << '\n';
    return 1;
  }
  if (!out->close()) {
    if (OIIO::has_error())
      std::cerr << "out->close() failed. Reason: " << OIIO::geterror() << '\n';
    else
      std::cerr << "out->close() failed." << '\n';
    return 1;
  }
  return 0;
}
