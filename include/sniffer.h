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

// A sniffer that writes pixel data into a file. For your SD card's sake, it
// should be an in-memory file.

#ifndef RPI_SNIFFER_H
#define RPI_SNIFFER_H

#include "canvas.h"

namespace rgb_matrix {

class FrameCanvas;

class Sniffer : public Canvas {
  public:
    Sniffer(Canvas *real, bool write=true) : real_(real), write_(write), init_done_(false) {}
    ~Sniffer();

    virtual int width() const;
    virtual int height() const;
    virtual void SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue);
    virtual void Clear();
    virtual void Fill(uint8_t red, uint8_t green, uint8_t blue);

    void SwapOnVSync(FrameCanvas *other);

  private:
    Canvas *const real_;
    bool write_;
    bool init_done_;
    uint8_t *buf_;
    int width_;
    int stride_;
    int height_;
    int size_;

    void init();
    void DoSetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue);
};
}

#endif
