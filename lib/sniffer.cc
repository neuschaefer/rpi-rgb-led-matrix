// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Copyright (C) 2024 J. Neuschäfer <j.ne@posteo.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

#include "sniffer.h"
#include "led-matrix.h"

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define PATH_VAR "RGBMATRIX_FB_FILE"
#define BMP_HEADER_SIZE 0x1c

namespace rgb_matrix {

static inline void put_le16(uint8_t *buf, uint16_t x)
{
  buf[0] = x;
  buf[1] = x >> 8;
}

static inline void put_le32(uint8_t *buf, uint32_t x)
{
  put_le16(&buf[0], x);
  put_le16(&buf[2], x >> 16);
}

void Sniffer::init() {
  if (init_done_)
    return;

  width_ = real_->width();         // pixels in a line
  stride_ = (width_ * 3 + 3) & ~3; // bytes in a line, 4-byte aligned
  height_ = real_->height();       // lines in the image
  size_ = BMP_HEADER_SIZE + height_ * stride_;

  char *path = getenv(PATH_VAR);
  if (path) {
    if (write_) {
      // open the file
      int fd = open(path, O_RDWR | O_CREAT, 0644);
      if (fd == -1) {
        fprintf(stderr, "Failed to open file \"%s\", specified in %s: %s\n", path, PATH_VAR, strerror(errno));
        exit(1);
      }

      // set size
      int res = ftruncate(fd, size_);
      if (res == -1) {
        fprintf(stderr, "Failed to resize file \"%s\", specified in %s: %s\n", path, PATH_VAR, strerror(errno));
        exit(1);
      }

      // mmap
      void *p = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (p == MAP_FAILED) {
        fprintf(stderr, "Failed to map file \"%s\", specified in %s: %s\n", path, PATH_VAR, strerror(errno));
        exit(1);
      }
      close(fd);
      buf_ = (uint8_t *)p;
    } else {
      buf_ = (uint8_t *)malloc(size_);
    }

    // write 24-bit BMP header
    //   https://github.com/corkami/pics/blob/master/binary/bmp1.png
    //   https://en.wikipedia.org/wiki/BMP_file_format#Example_1
    memset(buf_, 0, size_);
    buf_[0] = 'B';
    buf_[1] = 'M';
    put_le32(&buf_[0x02], size_);
    put_le32(&buf_[0x0a], BMP_HEADER_SIZE);
    put_le32(&buf_[0x0e], 0xc);
    put_le16(&buf_[0x12], width_);
    put_le16(&buf_[0x14], height_);
    put_le16(&buf_[0x16], 1);  // one plane
    put_le16(&buf_[0x18], 24); // 24 bpp

    if (write_)
      msync(buf_, size_, MS_SYNC);
  }

  init_done_ = true;
}

Sniffer::~Sniffer() {
  if (buf_) {
    if (write_)
      munmap(buf_, size_);
    else
      free(buf_);
    buf_ = NULL;
  }
}

int Sniffer::width() const {
  return real_->width();
}

int Sniffer::height() const {
  return real_->height();
}

void Sniffer::SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
  init();
  if (buf_) {
    DoSetPixel(x, y, red, green, blue);
    //msync(buf_, size_, MS_ASYNC);
  }
}

void Sniffer::Clear() {
  Fill(0, 0, 0);
}

void Sniffer::Fill(uint8_t red, uint8_t green, uint8_t blue) {
  init();
  if (buf_) {
    for (int y = 0; y < height_; y++)
      for (int x = 0; x < width_; x++)
        DoSetPixel(x, y, red, green, blue);
    //msync(buf_, size_, MS_ASYNC);
  }
}

void Sniffer::DoSetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t *p = &buf_[BMP_HEADER_SIZE + x * 3 + (height_ - 1 - y) * stride_];

  p[0] = blue;
  p[1] = green;
  p[2] = red;
}

void Sniffer::SwapOnVSync(FrameCanvas *other) {
  init();
  if (buf_) {
    memcpy(buf_, other->sniffer_->buf_, size_);
  }
}

}
