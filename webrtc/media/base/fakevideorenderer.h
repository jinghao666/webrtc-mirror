/*
 * libjingle
 * Copyright 2011 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEBRTC_MEDIA_BASE_FAKEVIDEORENDERER_H_
#define WEBRTC_MEDIA_BASE_FAKEVIDEORENDERER_H_

#include "webrtc/base/logging.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/media/base/videoframe.h"
#include "webrtc/media/base/videorenderer.h"

namespace cricket {

// Faked video renderer that has a callback for actions on rendering.
class FakeVideoRenderer : public VideoRenderer {
 public:
  FakeVideoRenderer()
      : errors_(0),
        width_(0),
        height_(0),
        num_rendered_frames_(0),
        black_frame_(false) {
  }

  virtual bool RenderFrame(const VideoFrame* frame) {
    rtc::CritScope cs(&crit_);
    // TODO(zhurunz) Check with VP8 team to see if we can remove this
    // tolerance on Y values.
    black_frame_ = CheckFrameColorYuv(6, 48, 128, 128, 128, 128, frame);
    // Treat unexpected frame size as error.
    if (!frame) {
      LOG(LS_WARNING) << "RenderFrame expected non-null frame.";
      ++errors_;
      return false;
    }
    ++num_rendered_frames_;
    width_ = static_cast<int>(frame->GetWidth());
    height_ = static_cast<int>(frame->GetHeight());
    SignalRenderFrame(frame);
    return true;
  }

  int errors() const { return errors_; }
  int width() const {
    rtc::CritScope cs(&crit_);
    return width_;
  }
  int height() const {
    rtc::CritScope cs(&crit_);
    return height_;
  }
  int num_rendered_frames() const {
    rtc::CritScope cs(&crit_);
    return num_rendered_frames_;
  }
  bool black_frame() const {
    rtc::CritScope cs(&crit_);
    return black_frame_;
  }

  sigslot::signal3<int, int, int> SignalSetSize;
  sigslot::signal1<const VideoFrame*> SignalRenderFrame;

 private:
  static bool CheckFrameColorYuv(uint8_t y_min,
                                 uint8_t y_max,
                                 uint8_t u_min,
                                 uint8_t u_max,
                                 uint8_t v_min,
                                 uint8_t v_max,
                                 const cricket::VideoFrame* frame) {
    if (!frame) {
      return false;
    }
    // Y
    size_t y_width = frame->GetWidth();
    size_t y_height = frame->GetHeight();
    const uint8_t* y_plane = frame->GetYPlane();
    const uint8_t* y_pos = y_plane;
    int32_t y_pitch = frame->GetYPitch();
    for (size_t i = 0; i < y_height; ++i) {
      for (size_t j = 0; j < y_width; ++j) {
        uint8_t y_value = *(y_pos + j);
        if (y_value < y_min || y_value > y_max) {
          return false;
        }
      }
      y_pos += y_pitch;
    }
    // U and V
    size_t chroma_width = frame->GetChromaWidth();
    size_t chroma_height = frame->GetChromaHeight();
    const uint8_t* u_plane = frame->GetUPlane();
    const uint8_t* v_plane = frame->GetVPlane();
    const uint8_t* u_pos = u_plane;
    const uint8_t* v_pos = v_plane;
    int32_t u_pitch = frame->GetUPitch();
    int32_t v_pitch = frame->GetVPitch();
    for (size_t i = 0; i < chroma_height; ++i) {
      for (size_t j = 0; j < chroma_width; ++j) {
        uint8_t u_value = *(u_pos + j);
        if (u_value < u_min || u_value > u_max) {
          return false;
        }
        uint8_t v_value = *(v_pos + j);
        if (v_value < v_min || v_value > v_max) {
          return false;
        }
      }
      u_pos += u_pitch;
      v_pos += v_pitch;
    }
    return true;
  }

  int errors_;
  int width_;
  int height_;
  int num_rendered_frames_;
  bool black_frame_;
  rtc::CriticalSection crit_;
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_BASE_FAKEVIDEORENDERER_H_