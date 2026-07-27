#include <OpenImageIO/span.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <OpenImageIO/imageio.h>

using namespace OIIO;

int main(int argc, char *argv[]) {
#define PRINT_USAGE()                                                          \
  std::cerr << "usage: " << argv[0] << " INPUT_FILEPATH OUTPUT_FILEPATH"  \
            << '\n'

  if (argc != 3) {
    PRINT_USAGE();
    return 1;
  }

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

  const int miplvl = 0;
  const int subimage = 0;

  const ImageSpec &spec = inp->spec();
  const int xres = std::max(spec.width >> miplvl, 1);
  const int yres = std::max(spec.height >> miplvl, 1);
  [[maybe_unused]] const int zres = std::max(spec.depth >> miplvl, 1);
  const int nchannels = spec.nchannels;

  if (auto nlayers_ptr = spec.find_attribute("ktx:nlayers"); nlayers_ptr) {
    std::cout << "nbr layers: "
              << *static_cast<const uint32_t *>(nlayers_ptr->data()) << '\n';
  }
  std::vector<unsigned char> pixels(xres * yres * nchannels);
  if (!inp->read_image(subimage, miplvl, 0, nchannels, make_span(pixels))) {
    std::cerr << "ImageInput::read_image() failed. Reason: " << OIIO::geterror()
              << '\n';
    // no need to call close() since it will be called in the destructor
    return 1;
  }
  if (!inp->close()) {
    std::cerr << "inp->close() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }

  std::unique_ptr<ImageOutput> out = ImageOutput::create(out_fp);
  if (!out) {
    std::cerr << "out->create() failed." << '\n';
    return 1;
  }

  ImageSpec outspec = ImageSpec(xres, yres, nchannels, TypeDesc::UINT8);
  if (!out->open(out_fp, outspec)) {
    std::cerr << "out->open() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }

  if (!out->write_image(make_cspan(pixels))) {
    std::cerr << "out->write_image() failed. Reason: " << OIIO::geterror()
              << '\n';
    return 1;
  }

  if (!out->close()) {
    std::cerr << "out->close() failed. Reason: " << OIIO::geterror() << '\n';
    return 1;
  }
  return 0;
}
