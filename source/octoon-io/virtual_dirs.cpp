// File: virtual_dirs.h
// Author: PENGUINLIONG
#include <fstream>
#include <filesystem>
#include <cassert>
#include "octoon/io/virtual_dirs.h"
#include "octoon/io/mstream.h"
#include "zipper/unzipper.h"

namespace octoon {
namespace io {

// LocalDir

namespace detail {

unsigned int
translate_opts(const OpenOptions& opts) {
  unsigned int rv = std::ios_base::binary;
  if (opts.options.read) {
    rv |= std::ios_base::in;
  }
  if (opts.options.write)
    rv |= std::ios_base::out;
    // `opts.options.create` is not checked, since `std::ios_base::out` always
    // try to create the file.
    if (opts.options.append) {
      rv |= std::ios_base::app;
    }
    if (opts.options.truncate) {
      rv |= std::ios_base::trunc;
    }
  }
  return rv;
}
class LocalFileStream : public stream {
public:
  bool open(const std::string& path, const OpenOptions& opts) {
    inner_.open(path, translate_opts(opts));
    if (inner_.is_open()) {
      opts_ = opts;
      return true;
    } else {
      return false;
    }
  }
  bool can_read() {
    return opts_.options.read;
  }
  bool can_write() {
    return opts_.options.write;
  }
  bool can_seek() {
    return true;
  }
  size_t read(uint8_t* buf, size_t size) {
    try {
      inner_.read((char*)buf, size);
    }
    catch (const std::ios_base::failure&) {
      return 0;
    }
    inner_.seekp(inner_.tellg(), std::ios_base::beg);
    return inner_.gcount();
  }
  size_t write(const uint8_t* buf, size_t size) {
    try {
      inner_.write((char*)buf, size);
    } catch (const std::ios_base::failure&) {
      return 0;
    }
    inner_.seekg(inner_.tellp(), std::ios_base::beg);
    return size;
  }
  bool seek(long dist, SeekOrigin ori) {
    try {
      switch (ori) {
      case octoon::io::SeekOrigin::Start:
        inner_.seekg(dist, std::ios_base::beg);
        inner_.seekp(dist, std::ios_base::beg);
        break;
      case octoon::io::SeekOrigin::End:
        inner_.seekg(dist, std::ios_base::end);
        inner_.seekp(dist, std::ios_base::end);
        break;
      case octoon::io::SeekOrigin::Current:
        inner_.seekg(dist, std::ios_base::cur);
        inner_.seekp(dist, std::ios_base::cur);
        break;
      }
      return true;
    }
    catch (const std::ios_base::failure&) {
      return false;
    }
  }
private:
  std::fstream inner_;
  OpenOptions opts_;
};
} // namespace detail

LocalDir::LocalDir(const std::string& base_dir) noexcept :
base_dir_(base_dir) {
}
std::unique_ptr<stream>
LocalDir::open(const Orl& orl, const OpenOptions& options) {
  auto rv_stream = std::make_unique<detail::LocalFileStream>();
  if (rv_stream->open(make_path(orl), options)) {
    return rv_stream;
  } else {
    return nullptr;
  }
}
bool
LocalDir::remove(const Orl& orl, ItemType type) {
  auto path = make_path(orl);
  auto status = std::filesystem::status(path);
  if (std::filesystem::file_type::not_found) {
    return false;
  }
  if (type == ItemType::File &&
    status.type() == std::filesystem::file_type::regular) {
    return std::filesystem::remove(path);
  }
  if (type == ItemType::Directory &&
    status.type() == std::filesystem::file_type::directory) {
    return std::filesystem::remove_all(path);
  }
  return false;
}
bool
LocalDir::exists(const Orl& orl) {
  auto status = std::filesystem::status(make_path(orl));
  switch (status.type()) {
   case std::filesystem::file_type::regular:
    return ItemType::File;
   case std::filesystem::file_type::directory:
    return ItemType::Directory;
   default:
    return ItemType::NA;
  }
}

std::string
LocalDir::make_path(const Orl& orl) const {
  std::string rv;
  auto path = orl.path();
  if (path.back() != '/') {
    rv.reserve(base_dir_.size() + 1 + path.size());
    rv.append(base_dir_);
    rv.push_back('/');
    rv.append(path);
  } else {
    rv.reserve(base_dir_.size() + path.size());
    rv.append(base_dir_);
    rv.append(path);
  }
  return rv;
}

// ZipArchive

ZipArchive::ZipArchive(const std::string& zip_file) :
  unzipper_(nullptr),
  entries_(nullptr) {
  try {
    unzipper_ = new zipper::Unzipper(zip_file);
  } catch (const std::exception&) {
  }
  entries_ = new std::vector<zipper::ZipEntry>(std::move(unzipper_.entries()));
}
ZipArchive::~ZipArchive() {
  delete ((zipper::Unzipper*)unzipper_);
  delete ((std::vector<zipper::ZipEntry>*)entries_);
}
std::unique_ptr<stream>
ZipArchive::open(const Orl& orl, const OpenOptions& opts) {
  std::vector<uint8_t> buf;
  // Zip archives are read-only.
  if (opts.options.write) {
    return nullptr;
  }
  if (unzipper_ == nullptr) return nullptr;
  assert(reinterpret_cast<zipper::Unzipper*>(unzipper_)
    ->extractEntryToMemory(orl.path(), buf));
  return std::make_unique<mstream>(buf);
}
bool
ZipArchive::remove(const Orl& orl, ItemType type) {
  return false;
}
ItemType
ZipArchive::exists(const Orl& orl) {
  for (auto entry : entries) {
    if (entry.name == orl.path()) {
      return ItemType::File;
    }
  }
  return ItemType::NA;
}

} // namespace io
} // namespace octoon
