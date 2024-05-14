/*
 * Copyright (C) 2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "encoding_session.hpp"

#include <cuti/exception_builder.hpp>
#include <x264_proto/types.hpp>

#undef NDEBUG
#include <cassert>
#include <iomanip>
#include <thread>
#include <x264.h>

namespace x264_es_utils
{

namespace // anonymous
{

struct x264_deleter_t
{
  void operator()(x264_t* encoder) const
  {
    x264_encoder_close(encoder);
  }
};

using x264_handle_t = std::unique_ptr<x264_t, x264_deleter_t>;

struct wrap_x264_param_t
{
  wrap_x264_param_t(
    cuti::logging_context_t const& logging_context,
    encoder_settings_t const& encoder_settings,
    x264_proto::session_params_t const& session_params);

  wrap_x264_param_t(wrap_x264_param_t const&) = delete;
  wrap_x264_param_t operator=(wrap_x264_param_t const&) = delete;

  void print(std::ostream& os) const;

  x264_handle_t create_x264_handle();

  ~wrap_x264_param_t();

private :
  explicit wrap_x264_param_t(
    cuti::logging_context_t const& logging_context,
    encoder_settings_t const& encoder_settings);

private :
  cuti::logging_context_t const& logging_context_;
  x264_param_t param_;
};

inline
std::ostream& operator<<(std::ostream& os, wrap_x264_param_t const& rhs)
{
  rhs.print(os);
  return os;
}

struct x264_output_t
{
  x264_output_t()
  : nals_(nullptr)
  , n_nals_(0)
  {
    std::memset(&pic_, 0, sizeof pic_);
  }

  x264_output_t(x264_output_t const&) = delete;
  x264_output_t& operator=(x264_output_t const&) = delete;

  int size() const
  {
    int result = 0;
    for(int i = 0; i < n_nals_; ++i)
    {
      result += nals_[i].i_payload;
    }
    return result;
  }

  x264_nal_t* nals_;
  int n_nals_;
  x264_picture_t pic_;
};

struct input_picture_t
{
public:
  input_picture_t(
    cuti::logging_context_t const& logging_context,
    x264_proto::frame_t const& frame);

  input_picture_t(input_picture_t const&) = delete;
  input_picture_t& operator=(input_picture_t const&) = delete;

  x264_picture_t* get()
  {
    return &picture_;
  }

  void print(std::ostream& os) const;

  ~input_picture_t();

private :
  cuti::logging_context_t const& logging_context_;
  x264_picture_t picture_;
};

inline
std::ostream& operator<<(std::ostream& os, input_picture_t const& rhs)
{
  rhs.print(os);
  return os;
}

struct wrap_x264_encoder_t
{
  wrap_x264_encoder_t(
    cuti::logging_context_t const& logging_context,
    encoder_settings_t const& encoder_settings,
    x264_proto::session_params_t const& session_params);

  wrap_x264_encoder_t(const wrap_x264_encoder_t&) = delete;
  wrap_x264_encoder_t& operator=(const wrap_x264_encoder_t&) = delete;

  int headers(x264_nal_t** headers_out, int *bytes_out) const;

  int encode(x264_output_t& output, input_picture_t& pic_in) const;

  int delayed_frames() const;

  int flush(x264_output_t& output) const;

private :
  cuti::logging_context_t const& logging_context_;
  wrap_x264_param_t param_;
  x264_handle_t handle_;
};

cuti::loglevel_t x264_log_level_to_cuti(int x264_log_level)
{
  switch(x264_log_level)
  {
  case X264_LOG_ERROR:
    return cuti::loglevel_t::error;
  case X264_LOG_WARNING:
    return cuti::loglevel_t::warning;
  case X264_LOG_INFO:
    return cuti::loglevel_t::info;
  case X264_LOG_DEBUG:
    return cuti::loglevel_t::debug;
  default:
    // NOTE: since this function will be called from a C callback, throwing
    // exceptions is not the best idea.
    return cuti::loglevel_t::debug;
  }
}

std::string vstringprintf(char const* fmt, va_list args)
{
  // NOTE: a copy of the incoming va_list must be saved, before the first
  // vsnprintf() call uses up the arguments, otherwise we cannot retry.
  va_list retry_args;
  va_copy(retry_args, args);
  std::vector<char> buffer(1024);
  int length = vsnprintf(buffer.data(), buffer.size(), fmt, args);
  if(length >= 0 && static_cast<size_t>(length) >= buffer.size())
  {
    buffer.resize(static_cast<size_t>(length) + 1);
    length = vsnprintf(buffer.data(), buffer.size(), fmt, retry_args);
  }
  va_end(retry_args);
  assert(length >= 0);
  return {buffer.data(), static_cast<size_t>(length)};
}

void x264_log_callback(void* ctx, int x264_level, char const *fmt, va_list args)
{
  assert(ctx != nullptr);
  auto const& logging_context =
    *static_cast<cuti::logging_context_t const*>(ctx);
  cuti::loglevel_t cuti_level = x264_log_level_to_cuti(x264_level);

  if(auto msg = logging_context.message_at(cuti_level))
  {
    auto text = vstringprintf(fmt, args);
    if(!text.empty() && text.back() == '\n')
    {
      text.pop_back();
      if(!text.empty() && text.back() == '\r')
      {
        text.pop_back();
      }
    }
    *msg << "libx264: " << text;
  }
}

char const* x264_profile_name(x264_proto::profile_t profile)
{
  switch(profile)
  {
  case x264_proto::profile_t::BASELINE:
    return "baseline";
  case x264_proto::profile_t::MAIN:
    return "main";
  case x264_proto::profile_t::HIGH:
    return "high";
  case x264_proto::profile_t::HIGH10:
    return "high10";
  case x264_proto::profile_t::HIGH422:
    return "high422";
  case x264_proto::profile_t::HIGH444_PREDICTIVE:
    return "high444";
  default:
    x264_exception_builder_t builder;
    builder << "bad x264_proto::profile_t value " << cuti::to_serialized(profile);
    builder.explode();
  }
}

// delegate ctor; ensures dtor is called when throwing in delegating ctor
wrap_x264_param_t::wrap_x264_param_t(
  cuti::logging_context_t const& logging_context,
  encoder_settings_t const& encoder_settings)
: logging_context_(logging_context)
{
  auto const& preset = encoder_settings.preset_;
  auto const& tune = encoder_settings.tune_;
  if(x264_param_default_preset(&param_,
    preset.empty() ? nullptr : preset.c_str(),
    tune.empty() ? nullptr : tune.c_str()) < 0)
  {
    x264_exception_builder_t builder;
    builder << "libx264 failed to apply preset: " <<
      (preset.empty() ? "default" : preset);
    if(!tune.empty())
    {
      builder << ", with tune: " << tune;
    }
    builder.explode();
  }
}

// delegating ctor
wrap_x264_param_t::wrap_x264_param_t(
  cuti::logging_context_t const& logging_context,
  encoder_settings_t const& encoder_settings,
  x264_proto::session_params_t const& session_params)
: wrap_x264_param_t(logging_context, encoder_settings) // -> dtor called on further exceptions
{
  if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
  {
    *msg << "encoding to avc profile=" << to_string(session_params.profile_idc_)
      << " level=" << session_params.level_idc_
      << " bitrate=" << session_params.bitrate_
      << " width=" << session_params.width_
      << " height=" << session_params.height_;
  }

  if(session_params.bitrate_ == 0)
  {
    x264_exception_builder_t builder;
    builder << "bad x264_proto::session_params.bitrate_ value " <<
      session_params.bitrate_;
    builder.explode();
  }

  // CPU flags
  if(encoder_settings.deterministic_)
  {
    param_.i_threads = 1;
    param_.i_lookahead_threads = 1;
    param_.b_sliced_threads = 0;
    param_.b_deterministic = 1;
    param_.b_cpu_independent = 1;
  }

  // Video properties
  param_.i_width  = session_params.width_;
  param_.i_height = session_params.height_;
  param_.i_csp = X264_CSP_NV12;
  param_.i_bitdepth = 8;
  param_.i_level_idc = session_params.level_idc_;

  // VUI parameters
  param_.vui.i_sar_width = session_params.sar_width_;
  param_.vui.i_sar_height = session_params.sar_height_;

  if(session_params.overscan_appropriate_flag_)
  {
    // 0=undef, 1=no overscan, 2=overscan
    param_.vui.i_overscan = *session_params.overscan_appropriate_flag_ ? 2 : 1;
  }
  if(session_params.video_format_)
  {
    param_.vui.i_vidformat = *session_params.video_format_;
  }
  if(session_params.video_full_range_flag_)
  {
    param_.vui.b_fullrange =
      *session_params.video_full_range_flag_;
  }
  if(session_params.colour_primaries_)
  {
    param_.vui.i_colorprim = *session_params.colour_primaries_;
  }
  if(session_params.transfer_characteristics_)
  {
    param_.vui.i_transfer = *session_params.transfer_characteristics_;
  }
  if(session_params.matrix_coefficients_)
  {
    param_.vui.i_colmatrix = *session_params.matrix_coefficients_;
  }
  if(session_params.chroma_sample_loc_type_top_field_ !=
    session_params.chroma_sample_loc_type_bottom_field_)
  {
    x264_exception_builder_t builder;
    builder << "libx264 does not support different chroma sample locations for "
      "top and bottom fields";
    builder.explode();
  }
  else if(session_params.chroma_sample_loc_type_top_field_)
  {
    param_.vui.i_chroma_loc =
      *session_params.chroma_sample_loc_type_top_field_;
  }

  // Bitstream parameters
#ifndef ALLOW_ENTROPY_CODING
  // To suppress entropy coding, and force use of CAVLC, turn off CABAC.
  param_.b_cabac = 0;
#endif // ALLOW_ENTROPY_CODING

  // Logging parameters
  param_.pf_log = x264_log_callback;
  param_.p_log_private = const_cast<cuti::logging_context_t*>(&logging_context_);
  param_.i_log_level = X264_LOG_DEBUG;

  // Rate control parameters
  param_.rc.i_rc_method = X264_RC_ABR;
  param_.rc.i_bitrate = (session_params.bitrate_ + 500) / 1000;

  // Muxing parameters
  param_.b_repeat_headers = 0;
  param_.b_annexb = 1;
  param_.b_vfr_input = 1;

  if(session_params.framerate_num_ == 0)
  {
    x264_exception_builder_t builder;
    builder << "bad x264_proto::session_params.framerate_num_ value " <<
      session_params.framerate_num_;
    builder.explode();
  }
  param_.i_fps_num = session_params.framerate_num_;
    
  if(session_params.framerate_den_ == 0)
  {
    x264_exception_builder_t builder;
    builder << "bad x264_proto::session_params.framerate_den_ value " <<
      session_params.framerate_den_;
    builder.explode();
  }
  param_.i_fps_den = session_params.framerate_den_;
    
  param_.i_timebase_num = 1;
  param_.i_timebase_den = session_params.timescale_;

  // Adjust keyint_{min,max} based on fps
  param_.i_keyint_min =
    session_params.framerate_num_ / session_params.framerate_den_;
  param_.i_keyint_max = 10 * param_.i_keyint_min;

  // Turn off automatic insertion of keyframes on scenecuts.
  param_.i_scenecut_threshold = 0;

  auto const* profile_name = x264_profile_name(session_params.profile_idc_);
  if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
  {
    *msg << "applying x264 profile " << profile_name;
  }
  if(x264_param_apply_profile(&param_, profile_name) < 0)
  {
    x264_exception_builder_t builder;
    builder << "libx264 failed to apply the " << profile_name << " profile";
    builder.explode();
  }
}

void wrap_x264_param_t::print(std::ostream& os) const
{
  os << "{x264_param_t at " << &param_ << ":"
     << " cpu=0x"                       << std::hex << param_.cpu << std::dec
     << " i_threads="                   << param_.i_threads
     << " i_lookahead_threads="         << param_.i_lookahead_threads
     << " b_sliced_threads="            << param_.b_sliced_threads
     << " b_deterministic="             << param_.b_deterministic
     << " b_cpu_independent="           << param_.b_cpu_independent
     << " i_sync_lookahead="            << param_.i_sync_lookahead
     << " i_width="                     << param_.i_width
     << " i_height="                    << param_.i_height
     << " i_csp="                       << param_.i_csp
     << " i_level_idc="                 << param_.i_level_idc
     << " i_frame_total="               << param_.i_frame_total
     << " i_nal_hrd="                   << param_.i_nal_hrd
     << " vui.i_sar_height="            << param_.vui.i_sar_height
     << " vui.i_sar_width="             << param_.vui.i_sar_width
     << " vui.i_overscan="              << param_.vui.i_overscan
     << " vui.i_vidformat="             << param_.vui.i_vidformat
     << " vui.b_fullrange="             << param_.vui.b_fullrange
     << " vui.i_colorprim="             << param_.vui.i_colorprim
     << " vui.i_transfer="              << param_.vui.i_transfer
     << " vui.i_colmatrix="             << param_.vui.i_colmatrix
     << " vui.i_chroma_loc="            << param_.vui.i_chroma_loc
     << " i_frame_reference="           << param_.i_frame_reference
     << " i_dpb_size="                  << param_.i_dpb_size
     << " i_keyint_max="                << param_.i_keyint_max
     << " i_keyint_min="                << param_.i_keyint_min
     << " i_scenecut_threshold="        << param_.i_scenecut_threshold
     << " b_intra_refresh="             << param_.b_intra_refresh
     << " i_bframe="                    << param_.i_bframe
     << " i_bframe_adaptive="           << param_.i_bframe_adaptive
     << " i_bframe_bias="               << param_.i_bframe_bias
     << " i_bframe_pyramid="            << param_.i_bframe_pyramid
     << " b_open_gop="                  << param_.b_open_gop
     << " b_bluray_compat="             << param_.b_bluray_compat
     << " b_deblocking_filter="         << param_.b_deblocking_filter
     << " i_deblocking_filter_alphac0=" << param_.i_deblocking_filter_alphac0
     << " i_deblocking_filter_beta="    << param_.i_deblocking_filter_beta
     << " b_cabac="                     << param_.b_cabac
     << " i_cabac_init_idc="            << param_.i_cabac_init_idc
     << " b_interlaced="                << param_.b_interlaced
     << " b_constrained_intra="         << param_.b_constrained_intra
     << " i_cqm_preset="                << param_.i_cqm_preset
     << " psz_cqm_file="
     << static_cast<void const*>(param_.psz_cqm_file)
     << " pf_log="
     << reinterpret_cast<void const*>(param_.pf_log)
     << " p_log_private="
     << static_cast<void const*>(param_.p_log_private)
     << " i_log_level="                 << param_.i_log_level
     << " b_full_recon="                << param_.b_full_recon
     << " psz_dump_yuv="
     << static_cast<void const*>(param_.psz_dump_yuv)
     << " analyse.intra="               << param_.analyse.intra
     << " analyse.inter="               << param_.analyse.inter
     << " analyse.b_transform_8x8="     << param_.analyse.b_transform_8x8
     << " analyse.i_weighted_pred="     << param_.analyse.i_weighted_pred
     << " analyse.b_weighted_bipred="   << param_.analyse.b_weighted_bipred
     << " analyse.i_direct_mv_pred="    << param_.analyse.i_direct_mv_pred
     << " analyse.i_chroma_qp_offset="  << param_.analyse.i_chroma_qp_offset
     << " analyse.i_me_method="         << param_.analyse.i_me_method
     << " analyse.i_me_range="          << param_.analyse.i_me_range
     << " analyse.i_mv_range="          << param_.analyse.i_mv_range
     << " analyse.i_mv_range_thread="   << param_.analyse.i_mv_range_thread
     << " analyse.i_subpel_refine="     << param_.analyse.i_subpel_refine
     << " analyse.b_chroma_me="         << param_.analyse.b_chroma_me
     << " analyse.b_mixed_references="  << param_.analyse.b_mixed_references
     << " analyse.i_trellis="           << param_.analyse.i_trellis
     << " analyse.b_fast_pskip="        << param_.analyse.b_fast_pskip
     << " analyse.b_dct_decimate="      << param_.analyse.b_dct_decimate
     << " analyse.i_noise_reduction="   << param_.analyse.i_noise_reduction
     << " analyse.f_psy_rd="            << param_.analyse.f_psy_rd
     << " analyse.f_psy_trellis="       << param_.analyse.f_psy_trellis
     << " analyse.b_psy="               << param_.analyse.b_psy
     << " analyse.i_luma_deadzone[0]="  << param_.analyse.i_luma_deadzone[0]
     << " analyse.i_luma_deadzone[1]="  << param_.analyse.i_luma_deadzone[1]
     << " analyse.b_psnr="              << param_.analyse.b_psnr
     << " analyse.b_ssim="              << param_.analyse.b_ssim
     << " rc.i_rc_method="              << param_.rc.i_rc_method
     << " rc.i_qp_constant="            << param_.rc.i_qp_constant
     << " rc.i_qp_min="                 << param_.rc.i_qp_min
     << " rc.i_qp_max="                 << param_.rc.i_qp_max
     << " rc.i_qp_step="                << param_.rc.i_qp_step
     << " rc.i_bitrate="                << param_.rc.i_bitrate
     << " rc.f_rf_constant="            << param_.rc.f_rf_constant
     << " rc.f_rf_constant_max="        << param_.rc.f_rf_constant_max
     << " rc.f_rate_tolerance="         << param_.rc.f_rate_tolerance
     << " rc.i_vbv_max_bitrate="        << param_.rc.i_vbv_max_bitrate
     << " rc.i_vbv_buffer_size="        << param_.rc.i_vbv_buffer_size
     << " rc.f_vbv_buffer_init="        << param_.rc.f_vbv_buffer_init
     << " rc.f_ip_factor="              << param_.rc.f_ip_factor
     << " rc.f_pb_factor="              << param_.rc.f_pb_factor
     << " rc.i_aq_mode="                << param_.rc.i_aq_mode
     << " rc.f_aq_strength="            << param_.rc.f_aq_strength
     << " rc.b_mb_tree="                << param_.rc.b_mb_tree
     << " rc.i_lookahead="              << param_.rc.i_lookahead
     << " rc.b_stat_write="             << param_.rc.b_stat_write
     << " rc.psz_stat_out="             << param_.rc.psz_stat_out
     << " rc.b_stat_read="              << param_.rc.b_stat_read
     << " rc.psz_stat_in="              << param_.rc.psz_stat_in
     << " rc.f_qcompress="              << param_.rc.f_qcompress
     << " rc.f_qblur="                  << param_.rc.f_qblur
     << " rc.f_complexity_blur="        << param_.rc.f_complexity_blur
     << " rc.zones="
     << static_cast<void const*>(param_.rc.zones)
     << " rc.i_zones="                  << param_.rc.i_zones
     << " rc.psz_zones="
     << static_cast<void const*>(param_.rc.psz_zones)
     << " crop_rect.i_left="            << param_.crop_rect.i_left
     << " crop_rect.i_top="             << param_.crop_rect.i_top
     << " crop_rect.i_right="           << param_.crop_rect.i_right
     << " crop_rect.i_bottom="          << param_.crop_rect.i_bottom
     << " i_frame_packing="             << param_.i_frame_packing
     << " b_aud="                       << param_.b_aud
     << " b_repeat_headers="            << param_.b_repeat_headers
     << " b_annexb="                    << param_.b_annexb
     << " i_sps_id="                    << param_.i_sps_id
     << " b_vfr_input="                 << param_.b_vfr_input
     << " b_pulldown="                  << param_.b_pulldown
     << " i_fps_num="                   << param_.i_fps_num
     << " i_fps_den="                   << param_.i_fps_den
     << " i_timebase_num="              << param_.i_timebase_num
     << " i_timebase_den="              << param_.i_timebase_den
     << " b_tff="                       << param_.b_tff
     << " b_pic_struct="                << param_.b_pic_struct
     << " b_fake_interlaced="           << param_.b_fake_interlaced
     << " i_slice_max_size="            << param_.i_slice_max_size
     << " i_slice_max_mbs="             << param_.i_slice_max_mbs
     << " i_slice_count="               << param_.i_slice_count
     << " param_free="
     << reinterpret_cast<void const *>(param_.param_free)
     << " nalu_process="
     << reinterpret_cast<void const *>(param_.nalu_process)
     << '}'
     ;
}

x264_handle_t wrap_x264_param_t::create_x264_handle()
{
  if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
  {
    *msg << "creating x264 encoder, param=" << *this;
  }
  x264_handle_t result(x264_encoder_open(&param_));
  if(result == nullptr)
  {
    x264_exception_builder_t builder;
    builder << "failed to create x264 encoder";
    builder.explode();
  }

  // x264 can adjust incoming parameters, so retrieve those.
  x264_encoder_parameters(result.get(), &param_);
  if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
  {
    *msg << "adjusted x264 encoder param=" << *this;
  }

  return result;
}

wrap_x264_param_t::~wrap_x264_param_t()
{
  x264_param_cleanup(&param_);
}

std::ostream& operator<<(std::ostream& os, x264_image_t const& rhs)
{
  os << "{x264_image_t at " << &rhs << ":"
    << " i_csp="   << rhs.i_csp
    << " i_plane=" << rhs.i_plane
    ;
  for(int i = 0; i < rhs.i_plane; ++i)
  {
    os << " i_stride[" << i << "]=" << rhs.i_stride[i];
  }
  for(int i = 0; i < rhs.i_plane; ++i)
  {
    os << " plane[" << i << "]=" << static_cast<void const*>(rhs.plane[i]);
  }
  os << '}';
  return os;
}

std::string x264_type_to_string(int type)
{
  switch(type)
  {
  case X264_TYPE_AUTO:
    return "X264_TYPE_AUTO";
  case X264_TYPE_IDR:
    return "X264_TYPE_IDR";
  case X264_TYPE_I:
    return "X264_TYPE_I";
  case X264_TYPE_P:
    return "X264_TYPE_P";
  case X264_TYPE_BREF:
    return "X264_TYPE_BREF";
  case X264_TYPE_B:
    return "X264_TYPE_B";
  case X264_TYPE_KEYFRAME:
    return "X264_TYPE_KEYFRAME";
  default:
    return std::to_string(type);
  }
}

std::ostream& operator<<(std::ostream& os, x264_picture_t const& rhs)
{
  os << "{x264_picture_t at " << &rhs << ":"
    << " i_type="       << x264_type_to_string(rhs.i_type)
    << " i_qpplus1="    << rhs.i_qpplus1
    << " i_pic_struct=" << rhs.i_pic_struct
    << " b_keyframe="   << rhs.b_keyframe
    << " i_pts="        << rhs.i_pts
    << " i_dts="        << rhs.i_dts
    << " param="        << static_cast<void const*>(rhs.param)
    << " img="          << rhs.img
//  << " prop="         << rhs.prop
//  << " hrd_timing="   << rhs.hrd_timing
//  << " extra_sei="    << rhs.extra_sei
    << " opaque="       << static_cast<void const*>(rhs.opaque)
    << '}';
    ;
  return os;
}

input_picture_t::input_picture_t(
  cuti::logging_context_t const& logging_context,
  x264_proto::frame_t const& frame)
: logging_context_(logging_context)
{
  // Verify frame. For now we always assume NV12 format.
  if(frame.format_ != x264_proto::format_t::NV12)
  {
    x264_exception_builder_t builder;
    builder << "unsupported x264_proto::frame.format_ value " <<
      cuti::to_serialized(frame.format_);
    builder.explode();
  }

  size_t img_size = static_cast<size_t>(frame.width_) * frame.height_ * 3 / 2;
  if(frame.data_.size() != img_size)
  {
    x264_exception_builder_t builder;
    builder << "unexpected x264_proto::frame.data_ size " <<
      frame.data_.size();
    builder.explode();
  }

  if(x264_picture_alloc(&picture_, X264_CSP_NV12,
                        frame.width_, frame.height_) < 0)
  {
    x264_exception_builder_t builder;
    builder << "libx264 failed to allocate picture of "
      << frame.width_ << "x" << frame.height_;
    builder.explode();
  }

  picture_.i_type = frame.keyframe_ ? X264_TYPE_IDR : X264_TYPE_AUTO;
  picture_.i_pts = frame.pts_;

  // Copy NV12 format, which is all the Y bytes, followed by all the UV words,
  // without padding, into x264 format (which is also not padded).
  std::copy(frame.data_.begin(), frame.data_.end(), picture_.img.plane[0]);
}

void input_picture_t::print(std::ostream& os) const
{
  os << picture_;
}

input_picture_t::~input_picture_t()
{
  x264_picture_clean(&picture_);
}

// Utility class for easy hex dumping
struct hexdump_t
{
  static constexpr size_t default_columns = 16;

  hexdump_t(uint8_t const* data, size_t size,
            size_t columns = default_columns)
  : data_(data)
  , size_(size)
  , columns_(columns)
  {
  }

  hexdump_t(std::vector<uint8_t> const& blob,
            size_t columns = default_columns)
  : data_(blob.data())
  , size_(blob.size())
  , columns_(columns)
  {
  }

  uint8_t const* data_;
  size_t size_;
  size_t columns_;
};

std::ostream& operator<<(std::ostream& os, hexdump_t const& hex)
{
  std::ios_base::fmtflags fmtflags = os.flags();
  for(size_t i = 0; i < hex.size_; i += hex.columns_)
  {
    if(i > 0)
    {
      os << '\n';
    }
    os << std::hex << std::setfill('0') << std::setw(8) << i << ':';
    for(size_t j = i, k = i + hex.columns_; j < k; ++j)
    {
      if(j < hex.size_)
      {
        unsigned char ch = hex.data_[j];
        os << ' ' << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<unsigned>(ch);
      }
      else
      {
        os << "   ";
      }
    }
    os << "  |";
    for(size_t j = i, k = i + hex.columns_; j < k; ++j)
    {
      if(j < hex.size_)
      {
        unsigned char ch = hex.data_[j];
        os << (std::isprint(ch) != 0 ? ch : static_cast<unsigned char>('.'));
      }
      else
      {
        break;
      }
    }
    os << '|';
  }
  os.flags(fmtflags);
  return os;
}

std::ostream& operator<<(std::ostream& os, x264_nal_t const& rhs)
{
  os << "{x264_nal_t at "    << &rhs << ":"
     << " i_ref_idc="        << rhs.i_ref_idc
     << " i_type="           << rhs.i_type
     << " b_long_startcode=" << rhs.b_long_startcode;
  if(rhs.i_type >= NAL_SLICE && rhs.i_type <= NAL_SLICE_IDR)
  {
    os << " i_first_mb="     << rhs.i_first_mb
       << " i_last_mb="      << rhs.i_last_mb;
  }
  os << " i_payload="        << rhs.i_payload
     << " p_payload:\n"
     << hexdump_t(rhs.p_payload, std::min(64, rhs.i_payload))
     << '}';
  return os;
}

wrap_x264_encoder_t::wrap_x264_encoder_t(
  cuti::logging_context_t const& logging_context,
  encoder_settings_t const& encoder_settings,
  x264_proto::session_params_t const& session_params)
: logging_context_(logging_context)
, param_(logging_context, encoder_settings, session_params)
, handle_(param_.create_x264_handle())
{
}

int wrap_x264_encoder_t::headers(x264_nal_t** headers, int* num_headers) const
{
  return x264_encoder_headers(handle_.get(), headers, num_headers);
}

int wrap_x264_encoder_t::encode(x264_output_t& output, input_picture_t& pic_in) const
{
  return x264_encoder_encode(handle_.get(),
    &output.nals_, &output.n_nals_, pic_in.get(), &output.pic_);
}

int wrap_x264_encoder_t::delayed_frames() const
{
  return x264_encoder_delayed_frames(handle_.get());
}

int wrap_x264_encoder_t::flush(x264_output_t& output) const
{
  return x264_encoder_encode(handle_.get(),
    &output.nals_, &output.n_nals_, nullptr, &output.pic_);
}

} // anonymous namespace

x264_exception_t::x264_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

x264_exception_t::~x264_exception_t()
{ }

struct encoding_session_t::impl_t
{
  impl_t(cuti::logging_context_t const& logging_context,
         encoder_settings_t const& encoder_settings,
         x264_proto::session_params_t const& session_params)
  : logging_context_(logging_context)
  , encoder_(logging_context_, encoder_settings, session_params)
  , frame_count_(0)
  , sample_count_(0)
  , flush_called_(false)
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: created";
    }
  }

  ~impl_t()
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: destroying";
    }
  }

  x264_proto::sample_headers_t sample_headers() const
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: retrieving sample headers";
    }

    x264_nal_t* headers;
    int num_headers;
    int num_bytes = encoder_.headers(&headers, &num_headers);
    if(num_bytes < 0)
    {
      x264_exception_builder_t builder;
      builder << "libx264 failed to retrieve sample headers";
      builder.explode();
    }
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      for(int i = 0; i < num_headers; ++i)
      {
        if(i > 0)
        {
          *msg << '\n';
        }
        *msg << "nal[" << i << "]=" << headers[i];
      }
    }

    // Sanity checks: libx264 is supposed to return SPS, PPS and SEI
    // (disposable, containing libx264 copyleft and parameter text).
    assert(num_headers == 3);

    assert(headers[0].i_type == NAL_SPS);
    assert(headers[0].i_payload > 4);
    assert(headers[0].p_payload[0] == 0x00);
    assert(headers[0].p_payload[1] == 0x00);
    assert(headers[0].p_payload[2] == 0x00);
    assert(headers[0].p_payload[3] == 0x01);
    assert((headers[0].p_payload[4] & 0x1f) == NAL_SPS);

    assert(headers[1].i_type == NAL_PPS);
    assert(headers[1].i_payload > 4);
    assert(headers[1].p_payload[0] == 0x00);
    assert(headers[1].p_payload[1] == 0x00);
    assert(headers[1].p_payload[2] == 0x00);
    assert(headers[1].p_payload[3] == 0x01);
    assert((headers[1].p_payload[4] & 0x1f) == NAL_PPS);

    assert(headers[2].i_type == NAL_SEI);
    assert(headers[2].i_payload > 4);
    assert(headers[2].p_payload[0] == 0x00);
    assert(headers[2].p_payload[1] == 0x00);
    assert(headers[2].p_payload[2] == 0x01);
    assert((headers[2].p_payload[3] & 0x1f) == NAL_SEI);

    x264_proto::sample_headers_t sample_headers;
    sample_headers.sps_.insert(sample_headers.sps_.end(),
      headers[0].p_payload, headers[0].p_payload + headers[0].i_payload);
    sample_headers.pps_.insert(sample_headers.pps_.end(),
      headers[1].p_payload, headers[1].p_payload + headers[1].i_payload);

    return sample_headers;
  }

  std::optional<x264_proto::sample_t> encode(x264_proto::frame_t frame)
  {
    assert(! flush_called_);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: encoding frame " << frame_count_;
    }
    ++frame_count_;

    x264_output_t output;
    input_picture_t pic_in(logging_context_, frame);
    int num_bytes = encoder_.encode(output, pic_in);
    if(num_bytes < 0)
    {
      x264_exception_builder_t builder;
      builder << "libx264 failed to encode frame";
      builder.explode();
    }
    else if(num_bytes == 0)
    {
      if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
      {
        *msg << "encoding_session[" << this << "]: no sample available yet";
      }

      // No sample has been produced yet.
      return std::nullopt;
    }

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: returning sample";
    }

    return generate_sample(num_bytes, output);
  }

  std::optional<x264_proto::sample_t> flush()
  {
    flush_called_ = true;

    int delayed_frames = encoder_.delayed_frames();
    assert(delayed_frames >= 0);
    if(delayed_frames == 0)
    {
      if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
      {
        *msg << "encoding_session[" << this << "]: no more samples";
      }

      // End of samples.
      return std::nullopt;
    }

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: flushing sample";
    }

    x264_output_t output;
    int num_bytes;
    while((num_bytes = encoder_.flush(output)) == 0)
    {
      // Unfortunately, x264 requires a busy loop here.
      std::this_thread::yield();
    }

    if(num_bytes < 0)
    {
      x264_exception_builder_t builder;
      builder << "libx264 failed to flush sample";
      builder.explode();
    }

    return generate_sample(num_bytes, output);
  }

private :
  static x264_handle_t create_x264_handle(
    cuti::logging_context_t const& logging_context,
    encoder_settings_t const& encoder_settings,
    x264_proto::session_params_t const& session_params)
  {
    wrap_x264_param_t param(logging_context, encoder_settings, session_params);
    return param.create_x264_handle();
  }

  x264_proto::sample_t generate_sample(
    int size, x264_output_t const& output)
  {
    assert(output.nals_ != nullptr);
    assert(output.n_nals_ > 0);
    assert(output.size() == size);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "sample[" << sample_count_ << "]"
        << " dts=" << output.pic_.i_dts
        << " pts=" << output.pic_.i_pts
        << " size=" << size
        << " pic type=" << x264_type_to_string(output.pic_.i_type);
      for(int i = 0; i < output.n_nals_; ++i)
      {
        *msg << " nal[" << i << "] type=" << output.nals_[i].i_type;
      }
    }
    ++sample_count_;

    x264_proto::sample_t sample;
    sample.dts_ = output.pic_.i_dts;
    sample.pts_ = output.pic_.i_pts;
    switch(output.pic_.i_type)
    {
    case X264_TYPE_IDR:
      sample.type_ = x264_proto::sample_t::type_t::i;
      break;
    case X264_TYPE_I:
    case X264_TYPE_P:
      sample.type_ = x264_proto::sample_t::type_t::p;
      break;
    case X264_TYPE_B:
      sample.type_ = x264_proto::sample_t::type_t::b;
      break;
    case X264_TYPE_BREF:
      sample.type_ = x264_proto::sample_t::type_t::b_ref;
      break;
    default:
      x264_exception_builder_t builder;
      builder << "unexpected x264 picture type "
        << x264_type_to_string(output.pic_.i_type);
      builder.explode();
    }
    // x264_encoder_encode's documentation says: the payloads of all output NALs
    // are guaranteed to be sequential in memory.
    sample.data_.insert(sample.data_.end(),
      output.nals_[0].p_payload,
      output.nals_[0].p_payload + size);

    return sample;
  }

private :
  cuti::logging_context_t const& logging_context_;
  wrap_x264_encoder_t encoder_;
  uint64_t frame_count_;
  uint64_t sample_count_;
  bool flush_called_;
};

encoding_session_t::encoding_session_t(
  cuti::logging_context_t const& logging_context,
  encoder_settings_t const& encoder_settings,
  x264_proto::session_params_t const& session_params)
: impl_(std::make_unique<impl_t>(logging_context, encoder_settings, session_params))
{
}

x264_proto::sample_headers_t encoding_session_t::sample_headers() const
{
  return impl_->sample_headers();
}

std::optional<x264_proto::sample_t>
encoding_session_t::encode(x264_proto::frame_t frame)
{
  return impl_->encode(std::move(frame));

}

std::optional<x264_proto::sample_t>
encoding_session_t::flush()
{
  return impl_->flush();
}

encoding_session_t::~encoding_session_t()
{
}

} // x264_es_utils
