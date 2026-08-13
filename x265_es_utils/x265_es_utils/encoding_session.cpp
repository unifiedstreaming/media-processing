/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x265_es_utils library.
 *
 * The x265_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x265_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x265_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "encoding_session.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/hexdump.hpp>
#include <cuti/stringprintf.hpp>
#include <x265_proto/types.hpp>

#include <string_view>

#include <x265.h>

#undef NDEBUG
#include <cassert>

namespace x265_es_utils
{

using namespace std::literals::string_view_literals;

namespace // anonymous
{

////////////////////////////////////////////////////////////////////////////////

int to_x265_bitdepth(x26x_proto::format_t format)
{
  switch(format)
  {
  case x26x_proto::format_t::YUV420P:
    return 8;
  case x26x_proto::format_t::YUV420P10LE:
    return 10;
  default:
    x265_exception_builder_t builder;
    builder << "unsupported x265_proto::frame.format_ value " <<
      to_string(format);
    builder.explode();
  }
}

////////////////////////////////////////////////////////////////////////////////

struct wrap_x265_api_t
{
  explicit wrap_x265_api_t(int bitdepth)
  : api_(x265_api_get(bitdepth))
  {
    if(api_ == nullptr)
    {
      x265_exception_builder_t builder;
      builder << "x265_api_get(" << bitdepth << ") failed";
      builder.explode();
    }
  }

  x265_api const* get() const
  {
    return api_;
  }

  x265_api const* operator->() const
  {
    return get();
  }

private:
  x265_api const* api_;
};

////////////////////////////////////////////////////////////////////////////////

char const* to_ptr(std::string const& str)
{
  return str.empty() ? nullptr : str.c_str();
}

std::string_view to_sv(std::string const& str)
{
  return str.empty() ? "(empty)"sv : std::string_view(str);
}

////////////////////////////////////////////////////////////////////////////////

struct cpuid_mask_t
{
  explicit cpuid_mask_t(int mask)
  : mask_(mask)
  { }

  int mask_;
};

std::ostream& operator<<(std::ostream& os, cpuid_mask_t const& rhs)
{
  bool sep = false;
  auto print_flag = [&](int flag, std::string_view desc)
  {
    if((rhs.mask_ & flag) != 0)
    {
      if(sep)
      {
        os << '|';
      }
      else
      {
        sep = true;
      }
      os << desc;
    }
  };
#if defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
  print_flag(X265_CPU_MMX,          "MMX");
  print_flag(X265_CPU_MMX2,         "MMX2");
  print_flag(X265_CPU_SSE,          "SSE");
  print_flag(X265_CPU_SSE2,         "SSE2");
  print_flag(X265_CPU_LZCNT,        "LZCNT");
  print_flag(X265_CPU_SSE3,         "SSE3");
  print_flag(X265_CPU_SSSE3,        "SSSE3");
  print_flag(X265_CPU_SSE4,         "SSE4");
  print_flag(X265_CPU_SSE42,        "SSE42");
  print_flag(X265_CPU_AVX,          "AVX");
  print_flag(X265_CPU_XOP,          "XOP");
  print_flag(X265_CPU_FMA4,         "FMA4");
  print_flag(X265_CPU_FMA3,         "FMA3");
  print_flag(X265_CPU_BMI1,         "BMI1");
  print_flag(X265_CPU_BMI2,         "BMI2");
  print_flag(X265_CPU_AVX2,         "AVX2");
  print_flag(X265_CPU_AVX512,       "AVX512");
  print_flag(X265_CPU_CACHELINE_32, "CACHELINE_32");
  print_flag(X265_CPU_CACHELINE_64, "CACHELINE_64");
  print_flag(X265_CPU_SSE2_IS_SLOW, "SSE2_IS_SLOW");
  print_flag(X265_CPU_SSE2_IS_FAST, "SSE2_IS_FAST");
  print_flag(X265_CPU_SLOW_SHUFFLE, "SLOW_SHUFFLE");
  print_flag(X265_CPU_STACK_MOD4,   "STACK_MOD4");
  print_flag(X265_CPU_SLOW_ATOM,    "SLOW_ATOM");
  print_flag(X265_CPU_SLOW_PSHUFB,  "SLOW_PSHUFB");
  print_flag(X265_CPU_SLOW_PALIGNR, "SLOW_PALIGNR");
#elif defined(__aarch64__) || defined(_M_ARM64)
  print_flag(X265_CPU_ARMV6,         "ARMV6");
  print_flag(X265_CPU_NEON,          "NEON");
  print_flag(X265_CPU_FAST_NEON_MRC, "FAST_NEON_MRC");
  print_flag(X265_CPU_SVE2,          "SVE2");
  print_flag(X265_CPU_SVE,           "SVE");
  print_flag(X265_CPU_NEON_DOTPROD,  "NEON_DOTPROD");
  print_flag(X265_CPU_NEON_I8MM,     "NEON_I8MM");
  print_flag(X265_CPU_SVE2_BITPERM,  "SVE2_BITPERM");
#else
#error CPU flags not implemented for this architecture
#endif
  return os;
}

struct log_level_t
{
  explicit log_level_t(int level)
  : level_(level)
  { }

  int level_;
};

std::ostream& operator<<(std::ostream& os, log_level_t const& rhs)
{
  switch(rhs.level_)
  {
  case X265_LOG_NONE:
    os << "NONE";
    break;
  case X265_LOG_ERROR:
    os << "ERROR";
    break;
  case X265_LOG_WARNING:
    os << "WARNING";
    break;
  case X265_LOG_INFO:
    os << "INFO";
    break;
  case X265_LOG_DEBUG:
    os << "DEBUG";
    break;
  case X265_LOG_FULL:
    os << "FULL";
    break;
  default:
    os << "Unknown x265 log level value " << rhs.level_;
    break;
  }
  return os;
}

struct search_method_t
{
  explicit search_method_t(int method)
  : method_(method)
  { }

  int method_;
};

std::ostream& operator<<(std::ostream& os, search_method_t const& rhs)
{
  switch(rhs.method_)
  {
  case X265_DIA_SEARCH:
    os << "Diamond";
    break;
  case X265_HEX_SEARCH:
    os << "Hexagon";
    break;
  case X265_UMH_SEARCH:
    os << "UMH";
    break;
  case X265_STAR_SEARCH:
    os << "Star";
    break;
  case X265_SEA:
    os << "SEA";
    break;
  case X265_FULL_SEARCH:
    os << "Full";
    break;
  default:
    os << "Unknown x265 search method value " << rhs.method_;
    break;
  }
  return os;
}

struct rc_mode_t
{
  explicit rc_mode_t(int mode)
  : mode_(mode)
  { }

  int mode_;
};

std::ostream& operator<<(std::ostream& os, rc_mode_t const& rhs)
{
  switch(rhs.mode_)
  {
  case X265_RC_ABR:
    os << "ABR";
    break;
  case X265_RC_CQP:
    os << "CQP";
    break;
  case X265_RC_CRF:
    os << "CRF";
    break;
  default:
    os << "Unknown x265 rate control mode value " << rhs.mode_;
    break;
  }
  return os;
}

struct aq_mode_t
{
  explicit aq_mode_t(int mode)
  : mode_(mode)
  { }

  int mode_;
};

std::ostream& operator<<(std::ostream& os, aq_mode_t const& rhs)
{
  switch(rhs.mode_)
  {
  case X265_AQ_NONE:
    os << "None";
    break;
  case X265_AQ_VARIANCE:
    os << "Variance";
    break;
  case X265_AQ_AUTO_VARIANCE:
    os << "Auto Varianc";
    break;
  case X265_AQ_AUTO_VARIANCE_BIASED:
    os << "Auto Variance Biased";
    break;
  case X265_AQ_EDGE:
    os << "Edge";
    break;
  default:
    os << "Unknown x265 adaptive quantization mode value " << rhs.mode_;
    break;
  }
  return os;
}

struct sar_idc_t
{
  explicit sar_idc_t(int idc)
  : idc_(idc)
  { }

  int idc_;
};

std::ostream& operator<<(std::ostream& os, sar_idc_t const& rhs)
{
  switch(rhs.idc_)
  {
  case 0:
    os << "unspecified";
    break;
  case 1:
    os << "1:1";
    break;
  case 2:
    os << "12:11";
    break;
  case 3:
    os << "10:11";
    break;
  case 4:
    os << "16:11";
    break;
  case 5:
    os << "40:33";
    break;
  case 6:
    os << "24:11";
    break;
  case 7:
    os << "20:11";
    break;
  case 8:
    os << "32:11";
    break;
  case 9:
    os << "80:33";
    break;
  case 10:
    os << "18:11";
    break;
  case 11:
    os << "15:11";
    break;
  case 12:
    os << "64:33";
    break;
  case 13:
    os << "160:99";
    break;
  case 14:
    os << "4:3";
    break;
  case 15:
    os << "3:2";
    break;
  case 16:
    os << "2:1";
    break;
  case 255:
    os << "EXTENDED_SAR";
    break;
  default:
    os << "Unknown aspect ratio indicator value " << rhs.idc_;
    break;
  }
  return os;
}

struct video_format_t
{
  explicit video_format_t(int format)
  : format_(format)
  { }

  int format_;
};

std::ostream& operator<<(std::ostream& os, video_format_t const& rhs)
{
  switch(rhs.format_)
  {
  case 0:
    os << "Component";
    break;
  case 1:
    os << "PAL";
    break;
  case 2:
    os << "NTSC";
    break;
  case 3:
    os << "SECAM";
    break;
  case 4:
    os << "MAC";
    break;
  case 5:
    os << "Unspecified";
    break;
  default:
    os << "Unknown video format value " << rhs.format_;
    break;
  }
  return os;
}

struct colour_primaries_t
{
  explicit colour_primaries_t(int value)
  : value_(value)
  { }

  int value_;
};

std::ostream& operator<<(std::ostream& os, colour_primaries_t const& rhs)
{
  switch(rhs.value_)
  {
  case 1:
    os << "BT_709";
    break;
  case 2:
    os << "unspecified";
    break;
  case 4:
    os << "BT_470_M";
    break;
  case 5:
    os << "BT_470_B_G";
    break;
  case 6:
    os << "BT_601";
    break;
  case 7:
    os << "SMPTE_240";
    break;
  case 8:
    os << "GENERIC_FILM";
    break;
  case 9:
    os << "BT_2020";
    break;
  case 10:
    os << "XYZ";
    break;
  case 11:
    os << "SMPTE_431";
    break;
  case 12:
    os << "SMPTE_432";
    break;
  case 22:
    os << "EBU_3213";
    break;
  default:
    os << "Unknown color primaries value " << rhs.value_;
    break;
  }
  return os;
}

struct transfer_characteristics_t
{
  explicit transfer_characteristics_t(int value)
  : value_(value)
  { }

  int value_;
};

std::ostream& operator<<(std::ostream& os, transfer_characteristics_t const& rhs)
{
  switch(rhs.value_)
  {
  case 1:
    os << "BT_709";
    break;
  case 2:
    os << "unspecified";
    break;
  case 4:
    os << "BT_470_M";
    break;
  case 5:
    os << "BT_470_B_G";
    break;
  case 6:
    os << "BT_601";
    break;
  case 7:
    os << "SMPTE_240";
    break;
  case 8:
    os << "LINEAR";
    break;
  case 9:
    os << "LOG_100";
    break;
  case 10:
    os << "LOG_100_SQRT10";
    break;
  case 11:
    os << "IEC_61966";
    break;
  case 12:
    os << "BT_1361";
    break;
  case 13:
    os << "SRGB";
    break;
  case 14:
    os << "SDR";
    break;
  case 16:
    os << "PQ";
    break;
  case 18:
    os << "HLG";
    break;
  default:
    os << "Unknown transfer characteristics value " << rhs.value_;
    break;
  }
  return os;
}

struct matrix_coefficients_t
{
  explicit matrix_coefficients_t(int value)
  : value_(value)
  { }

  int value_;
};

std::ostream& operator<<(std::ostream& os, matrix_coefficients_t const& rhs)
{
  switch(rhs.value_)
  {
  case 0:
    os << "IDENTITY";
    break;
  case 1:
    os << "BT_709";
    break;
  case 2:
    os << "unspecified";
    break;
  case 4:
    os << "FCC";
    break;
  case 5:
    os << "BT_470_B_G";
    break;
  case 6:
    os << "BT_601";
    break;
  case 7:
    os << "SMPTE_240";
    break;
  case 8:
    os << "SMPTE_YCGCO";
    break;
  case 9:
    os << "BT_2020_NCL";
    break;
  case 10:
    os << "BT_2020_CL";
    break;
  case 11:
    os << "SMPTE_2085";
    break;
  case 12:
    os << "CHROMAT_NCL";
    break;
  case 13:
    os << "CHROMAT_CL";
    break;
  case 14:
    os << "ICTCP";
    break;
  default:
    os << "Unknown matrix coefficients value " << rhs.value_;
    break;
  }
  return os;
}

struct ptr_value_t
{
  explicit ptr_value_t(void const* p)
  : p_(p)
  { }

  void const* p_;
};

std::ostream& operator<<(std::ostream& os, ptr_value_t const& rhs)
{
  if(rhs.p_ != nullptr)
  {
    os << rhs.p_;
  }
  else
  {
    os << "(null)";
  }
  return os;
}

////////////////////////////////////////////////////////////////////////////////

struct wrap_x265_param_t
{
  explicit wrap_x265_param_t(wrap_x265_api_t const& api)
  : api_(api)
  , param_(make_param(api_))
  { }

  wrap_x265_param_t(wrap_x265_api_t const& api,
    encoder_settings_t const& encoder_settings,
    x265_proto::session_params_t const& session_params)
  : api_(api)
  , param_(make_param(api_, encoder_settings.preset_, encoder_settings.tune_))
  { }

  x265_param const* get() const
  {
    return &param_;
  }

  x265_param* get()
  {
    return &param_;
  }

  x265_param const* operator->() const
  {
    return get();
  }

  x265_param* operator->()
  {
    return get();
  }

  void set_parameter(char const* name, char const* value)
  {
    auto result = api_->param_parse(get(), name, value);
    if(result != 0)
    {
      x265_exception_builder_t builder;
      builder << "x265_param_parse failed: ";
      switch(result)
      {
      case X265_PARAM_BAD_NAME:
        builder << "invalid parameter name " << name;
        break;
      case X265_PARAM_BAD_VALUE:
        builder << "invalid value " << value << " for parameter " << name;
        break;
      default:
        builder << "unknown return value " << result;
        break;
      }
      builder.explode();
    }
  }

  void apply_profile(char const* profile)
  {
    auto result = api_->param_apply_profile(get(), profile);
    if(result != 0)
    {
      x265_exception_builder_t builder;
      builder << "x265_param_apply_profile failed";
      builder.explode();
    }
  }

  void print(std::ostream& os) const
  {
    os << "cpuid="                                                << cpuid_mask_t(param_.cpuid);
    os << "\nframeNumThreads="                                    << param_.frameNumThreads;
    os << "\nnumaPools="                                          << param_.numaPools;
    os << "\nbEnableWavefront="                                   << param_.bEnableWavefront;
    os << "\nbDistributeModeAnalysis="                            << param_.bDistributeModeAnalysis;
    os << "\nbDistributeMotionEstimation="                        << param_.bDistributeMotionEstimation;
    os << "\nbThreadedME="                                        << param_.bThreadedME;
    os << "\nbLogCuStats="                                        << param_.bLogCuStats;
    os << "\nbEnablePsnr="                                        << param_.bEnablePsnr;
    os << "\nbEnableSsim="                                        << param_.bEnableSsim;
    os << "\nlogLevel="                                           << log_level_t(param_.logLevel);
    os << "\ncsvLogLevel="                                        << param_.csvLogLevel;
    os << "\ncsvfn="                                              << param_.csvfn;
    os << "\ninternalBitDepth="                                   << param_.internalBitDepth;
    os << "\ninternalCsp="                                        << param_.internalCsp;
    os << "\nfpsNum="                                             << param_.fpsNum;
    os << "\nfpsDenom="                                           << param_.fpsDenom;
    os << "\nsourceWidth="                                        << param_.sourceWidth;
    os << "\nsourceHeight="                                       << param_.sourceHeight;
    os << "\ninterlaceMode="                                      << param_.interlaceMode;
    os << "\ntotalFrames="                                        << param_.totalFrames;
    os << "\nlevelIdc="                                           << param_.levelIdc;
    os << "\nbHighTier="                                          << param_.bHighTier;
    os << "\nuhdBluray="                                          << param_.uhdBluray;
    os << "\nmaxNumReferences="                                   << param_.maxNumReferences;
    os << "\nbAllowNonConformance="                               << param_.bAllowNonConformance;
    os << "\nbRepeatHeaders="                                     << param_.bRepeatHeaders;
    os << "\nbAnnexB="                                            << param_.bAnnexB;
    os << "\nbEnableAccessUnitDelimiters="                        << param_.bEnableAccessUnitDelimiters;
    os << "\nbEmitHRDSEI="                                        << param_.bEmitHRDSEI;
    os << "\nbEmitInfoSEI="                                       << param_.bEmitInfoSEI;
    os << "\ndecodedPictureHashSEI="                              << param_.decodedPictureHashSEI;
    os << "\nbEnableTemporalSubLayers="                           << param_.bEnableTemporalSubLayers;
    os << "\nbOpenGOP="                                           << param_.bOpenGOP;
    os << "\ncraNal="                                             << param_.craNal;
    os << "\nkeyframeMin="                                        << param_.keyframeMin;
    os << "\nkeyframeMax="                                        << param_.keyframeMax;
    os << "\nbframes="                                            << param_.bframes;
    os << "\nbFrameAdaptive="                                     << param_.bFrameAdaptive;
    os << "\nbBPyramid="                                          << param_.bBPyramid;
    os << "\nbFrameBias="                                         << param_.bFrameBias;
    os << "\nlookaheadDepth="                                     << param_.lookaheadDepth;
    os << "\nlookaheadSlices="                                    << param_.lookaheadSlices;
    os << "\nscenecutThreshold="                                  << param_.scenecutThreshold;
    os << "\nbIntraRefresh="                                      << param_.bIntraRefresh;
    os << "\nmaxCUSize="                                          << param_.maxCUSize;
    os << "\nminCUSize="                                          << param_.minCUSize;
    os << "\nbEnableRectInter="                                   << param_.bEnableRectInter;
    os << "\nbEnableAMP="                                         << param_.bEnableAMP;
    os << "\nmaxTUSize="                                          << param_.maxTUSize;
    os << "\ntuQTMaxInterDepth="                                  << param_.tuQTMaxInterDepth;
    os << "\ntuQTMaxIntraDepth="                                  << param_.tuQTMaxIntraDepth;
    os << "\nlimitTU="                                            << param_.limitTU;
    os << "\nrdoqLevel="                                          << param_.rdoqLevel;
    os << "\nbEnableSignHiding="                                  << param_.bEnableSignHiding;
    os << "\nbEnableTransformSkip="                               << param_.bEnableTransformSkip;
    os << "\nnoiseReductionIntra="                                << param_.noiseReductionIntra;
    os << "\nnoiseReductionInter="                                << param_.noiseReductionInter;
    os << "\nscalingLists="                                       << param_.scalingLists;
    os << "\nbEnableConstrainedIntra="                            << param_.bEnableConstrainedIntra;
    os << "\nbEnableStrongIntraSmoothing="                        << param_.bEnableStrongIntraSmoothing;
    os << "\nmaxNumMergeCand="                                    << param_.maxNumMergeCand;
    os << "\nlimitReferences="                                    << param_.limitReferences;
    os << "\nlimitModes="                                         << param_.limitModes;
    os << "\nsearchMethod="                                       << search_method_t(param_.searchMethod);
    os << "\nsubpelRefine="                                       << param_.subpelRefine;
    os << "\nsearchRange="                                        << param_.searchRange;
    os << "\nbEnableTemporalMvp="                                 << param_.bEnableTemporalMvp;
    os << "\nbEnableHME="                                         << param_.bEnableHME;
    for(int i = 0; i < 3; ++i)
    {
      os << "\nhmeSearchMethod[" << i << "]="                     << search_method_t(param_.hmeSearchMethod[i]);
    }
    os << "\nbEnableWeightedPred="                                << param_.bEnableWeightedPred;
    os << "\nbEnableWeightedBiPred="                              << param_.bEnableWeightedBiPred;
    os << "\nbSourceReferenceEstimation="                         << param_.bSourceReferenceEstimation;
    os << "\nbEnableLoopFilter="                                  << param_.bEnableLoopFilter;
    os << "\ndeblockingFilterTCOffset="                           << param_.deblockingFilterTCOffset;
    os << "\ndeblockingFilterBetaOffset="                         << param_.deblockingFilterBetaOffset;
    os << "\nbEnableSAO="                                         << param_.bEnableSAO;
    os << "\nbSaoNonDeblocked="                                   << param_.bSaoNonDeblocked;
    os << "\nselectiveSAO="                                       << param_.selectiveSAO;
    os << "\nrdLevel="                                            << param_.rdLevel;
    os << "\nbEnableEarlySkip="                                   << param_.bEnableEarlySkip;
    os << "\nrecursionSkipMode="                                  << param_.recursionSkipMode;
    os << "\nbEnableFastIntra="                                   << param_.bEnableFastIntra;
    os << "\nbEnableTSkipFast="                                   << param_.bEnableTSkipFast;
    os << "\nbCULossless="                                        << param_.bCULossless;
    os << "\nbIntraInBFrames="                                    << param_.bIntraInBFrames;
    os << "\nrdPenalty="                                          << param_.rdPenalty;
    os << "\npsyRd="                                              << param_.psyRd;
    os << "\npsyRdoq="                                            << param_.psyRdoq;
    os << "\nbEnableRdRefine="                                    << param_.bEnableRdRefine;
    os << "\nanalysisReuseMode="                                  << param_.analysisReuseMode;
    os << "\nanalysisReuseFileName="                              << param_.analysisReuseFileName;
    os << "\nbLossless="                                          << param_.bLossless;
    os << "\ncbQpOffset="                                         << param_.cbQpOffset;
    os << "\ncrQpOffset="                                         << param_.crQpOffset;
    os << "\npreferredTransferCharacteristics="                   << param_.preferredTransferCharacteristics;
    os << "\npictureStructure="                                   << param_.pictureStructure;

    os << "\nrc.rateControlMode="                                 << rc_mode_t(param_.rc.rateControlMode);
    os << "\nrc.qp="                                              << param_.rc.qp;
    os << "\nrc.bitrate="                                         << param_.rc.bitrate;
    os << "\nrc.qCompress="                                       << param_.rc.qCompress;
    os << "\nrc.ipFactor="                                        << param_.rc.ipFactor;
    os << "\nrc.pbFactor="                                        << param_.rc.pbFactor;
    os << "\nrc.rfConstant="                                      << param_.rc.rfConstant;
    os << "\nrc.qpStep="                                          << param_.rc.qpStep;
    os << "\nrc.aqMode="                                          << aq_mode_t(param_.rc.aqMode);
    os << "\nrc.hevcAq="                                          << param_.rc.hevcAq;
    os << "\nrc.aqStrength="                                      << param_.rc.aqStrength;
    os << "\nrc.qpAdaptationRange="                               << param_.rc.qpAdaptationRange;
    os << "\nrc.vbvMaxBitrate="                                   << param_.rc.vbvMaxBitrate;
    os << "\nrc.vbvBufferSize="                                   << param_.rc.vbvBufferSize;
    os << "\nrc.vbvBufferInit="                                   << param_.rc.vbvBufferInit;
    os << "\nrc.cuTree="                                          << param_.rc.cuTree;
    os << "\nrc.rfConstantMax="                                   << param_.rc.rfConstantMax;
    os << "\nrc.rfConstantMin="                                   << param_.rc.rfConstantMin;
    os << "\nrc.bStatWrite="                                      << param_.rc.bStatWrite;
    os << "\nrc.bStatRead="                                       << param_.rc.bStatRead;
    os << "\nrc.statFileName="                                    << param_.rc.statFileName;
    os << "\nrc.qblur="                                           << param_.rc.qblur;
    os << "\nrc.complexityBlur="                                  << param_.rc.complexityBlur;
    os << "\nrc.bEnableSlowFirstPass="                            << param_.rc.bEnableSlowFirstPass;
    os << "\nrc.zoneCount="                                       << param_.rc.zoneCount;
    os << "\nrc.zones="                                           << param_.rc.zones;
    os << "\nrc.zonefileCount="                                   << param_.rc.zonefileCount;
    os << "\nrc.lambdaFileName="                                  << param_.rc.lambdaFileName;
    os << "\nrc.bStrictCbr="                                      << param_.rc.bStrictCbr;
    os << "\nrc.qgSize="                                          << param_.rc.qgSize;
    os << "\nrc.bEnableGrain="                                    << param_.rc.bEnableGrain;
    os << "\nrc.qpMax="                                           << param_.rc.qpMax;
    os << "\nrc.qpMin="                                           << param_.rc.qpMin;
    os << "\nrc.bEnableConstVbv="                                 << param_.rc.bEnableConstVbv;
    os << "\nrc.bEncFocusedFramesOnly="                           << param_.rc.bEncFocusedFramesOnly;
    os << "\nrc.dataShareMode="                                   << param_.rc.dataShareMode;
    os << "\nrc.sharedMemName="                                   << param_.rc.sharedMemName;

    os << "\nvui.aspectRatioIdc="                                 << sar_idc_t(param_.vui.aspectRatioIdc);
    os << "\nvui.sarWidth="                                       << param_.vui.sarWidth;
    os << "\nvui.sarHeight="                                      << param_.vui.sarHeight;
    os << "\nvui.bEnableOverscanInfoPresentFlag="                 << param_.vui.bEnableOverscanInfoPresentFlag;
    os << "\nvui.bEnableOverscanAppropriateFlag="                 << param_.vui.bEnableOverscanAppropriateFlag;
    os << "\nvui.bEnableVideoSignalTypePresentFlag="              << param_.vui.bEnableVideoSignalTypePresentFlag;
    os << "\nvui.videoFormat="                                    << video_format_t(param_.vui.videoFormat);
    os << "\nvui.bEnableVideoFullRangeFlag="                      << param_.vui.bEnableVideoFullRangeFlag;
    os << "\nvui.bEnableColorDescriptionPresentFlag="             << param_.vui.bEnableColorDescriptionPresentFlag;
    os << "\nvui.colorPrimaries="                                 << colour_primaries_t(param_.vui.colorPrimaries);
    os << "\nvui.transferCharacteristics="                        << transfer_characteristics_t(param_.vui.transferCharacteristics);
    os << "\nvui.matrixCoeffs="                                   << matrix_coefficients_t(param_.vui.matrixCoeffs);
    os << "\nvui.bEnableChromaLocInfoPresentFlag="                << param_.vui.bEnableChromaLocInfoPresentFlag;
    os << "\nvui.chromaSampleLocTypeTopField="                    << param_.vui.chromaSampleLocTypeTopField;
    os << "\nvui.chromaSampleLocTypeBottomField="                 << param_.vui.chromaSampleLocTypeBottomField;
    os << "\nvui.bEnableDefaultDisplayWindowFlag="                << param_.vui.bEnableDefaultDisplayWindowFlag;
    os << "\nvui.defDispWinLeftOffset="                           << param_.vui.defDispWinLeftOffset;
    os << "\nvui.defDispWinRightOffset="                          << param_.vui.defDispWinRightOffset;
    os << "\nvui.defDispWinTopOffset="                            << param_.vui.defDispWinTopOffset;
    os << "\nvui.defDispWinBottomOffset="                         << param_.vui.defDispWinBottomOffset;

    os << "\nmasteringDisplayColorVolume="                        << param_.masteringDisplayColorVolume;
    os << "\nmaxCLL="                                             << param_.maxCLL;
    os << "\nmaxFALL="                                            << param_.maxFALL;
    os << "\nminLuma="                                            << param_.minLuma;
    os << "\nmaxLuma="                                            << param_.maxLuma;
    os << "\nlog2MaxPocLsb="                                      << param_.log2MaxPocLsb;
    os << "\nbEmitVUITimingInfo="                                 << param_.bEmitVUITimingInfo;
    os << "\nbEmitVUIHRDInfo="                                    << param_.bEmitVUIHRDInfo;
    os << "\nmaxSlices="                                          << param_.maxSlices;
    os << "\nbOptQpPPS="                                          << param_.bOptQpPPS;
    os << "\nbOptRefListLengthPPS="                               << param_.bOptRefListLengthPPS;
    os << "\nbMultiPassOptRPS="                                   << param_.bMultiPassOptRPS;
    os << "\nscenecutBias="                                       << param_.scenecutBias;
    os << "\nlookaheadThreads="                                   << param_.lookaheadThreads;
    os << "\nbOptCUDeltaQP="                                      << param_.bOptCUDeltaQP;
    os << "\nanalysisMultiPassRefine="                            << param_.analysisMultiPassRefine;
    os << "\nanalysisMultiPassDistortion="                        << param_.analysisMultiPassDistortion;
    os << "\nbAQMotion="                                          << param_.bAQMotion;
    os << "\nbSsimRd="                                            << param_.bSsimRd;
    os << "\ndynamicRd="                                          << param_.dynamicRd;
    os << "\nbEmitHDRSEI="                                        << param_.bEmitHDRSEI;
    os << "\nbHDROpt="                                            << param_.bHDROpt;
    os << "\nanalysisReuseLevel="                                 << param_.analysisReuseLevel;
    os << "\nbLimitSAO="                                          << param_.bLimitSAO;
    os << "\ntoneMapFile="                                        << param_.toneMapFile;
    os << "\nbDhdr10opt="                                         << param_.bDhdr10opt;
    os << "\nbCTUInfo="                                           << param_.bCTUInfo;
    os << "\nbUseRcStats="                                        << param_.bUseRcStats;
    os << "\nscaleFactor="                                        << param_.scaleFactor;
    os << "\nintraRefine="                                        << param_.intraRefine;
    os << "\ninterRefine="                                        << param_.interRefine;
    os << "\nmvRefine="                                           << param_.mvRefine;
    os << "\nmaxLog2CUSize="                                      << param_.maxLog2CUSize;
    os << "\nmaxCUDepth="                                         << param_.maxCUDepth;
    os << "\nunitSizeDepth="                                      << param_.unitSizeDepth;
    os << "\nnum4x4Partitions="                                   << param_.num4x4Partitions;
    os << "\nbUseAnalysisFile="                                   << param_.bUseAnalysisFile;
    os << "\ncsvfpt="                                             << param_.csvfpt;
    os << "\nforceFlush="                                         << param_.forceFlush;
    os << "\nbEnableSplitRdSkip="                                 << param_.bEnableSplitRdSkip;
    os << "\nbDisableLookahead="                                  << param_.bDisableLookahead;
    os << "\nbLowPassDct="                                        << param_.bLowPassDct;
    os << "\nvbvBufferEnd="                                       << param_.vbvBufferEnd;
    os << "\nvbvEndFrameAdjust="                                  << param_.vbvEndFrameAdjust;
    os << "\nbAnalysisType="                                      << param_.bAnalysisType;
    os << "\nbCopyPicToFrame="                                    << param_.bCopyPicToFrame;
    os << "\ngopLookahead="                                       << param_.gopLookahead;
    os << "\nanalysisSave="                                       << param_.analysisSave;
    os << "\nanalysisLoad="                                       << param_.analysisLoad;
    os << "\nradl="                                               << param_.radl;
    os << "\nmaxAUSizeFactor="                                    << param_.maxAUSizeFactor;
    os << "\nbEmitIDRRecoverySEI="                                << param_.bEmitIDRRecoverySEI;
    os << "\nbDynamicRefine="                                     << param_.bDynamicRefine;
    os << "\nbSingleSeiNal="                                      << param_.bSingleSeiNal;
    os << "\nchunkStart="                                         << param_.chunkStart;
    os << "\nchunkEnd="                                           << param_.chunkEnd;
    os << "\nnaluFile="                                           << param_.naluFile;
    os << "\ndolbyProfile="                                       << param_.dolbyProfile;
    os << "\nbEnableHRDConcatFlag="                               << param_.bEnableHRDConcatFlag;
    os << "\nctuDistortionRefine="                                << param_.ctuDistortionRefine;
    os << "\nbEnableSvtHevc="                                     << param_.bEnableSvtHevc;
    os << "\nsvtHevcParam="                                       << param_.svtHevcParam;
    os << "\nbEnableFades="                                       << param_.bEnableFades;
    os << "\nbField="                                             << param_.bField;
    os << "\nbEmitCLL="                                           << param_.bEmitCLL;
    os << "\nbEnableFrameDuplication="                            << param_.bEnableFrameDuplication;
    os << "\ndupThreshold="                                       << param_.dupThreshold;
    os << "\nsourceBitDepth="                                     << param_.sourceBitDepth;
    os << "\nreconfigWindowSize="                                 << param_.reconfigWindowSize;
    os << "\nbResetZoneConfig="                                   << param_.bResetZoneConfig;
    os << "\nbNoResetZoneConfig="                                 << param_.bNoResetZoneConfig;
    os << "\nbEnableSceneCutAwareQp="                             << param_.bEnableSceneCutAwareQp;
    os << "\nfwdMaxScenecutWindow="                               << param_.fwdMaxScenecutWindow;
    for(int i = 0; i < 6; ++i)
    {
      os << "\nfwdScenecutWindow[" << i << "]="                   << param_.fwdScenecutWindow[i];
    }
    for(int i = 0; i < 6; ++i)
    {
      os << "\nfwdRefQpDelta[" << i << "]="                       << param_.fwdRefQpDelta[i];
    }
    for(int i = 0; i < 6; ++i)
    {
      os << "\nfwdNonRefQpDelta[" << i << "]="                    << param_.fwdNonRefQpDelta[i];
    }
    os << "\nbHistBasedSceneCut="                                 << param_.bHistBasedSceneCut;
    for(int i = 0; i < 3; ++i)
    {
      os << "\nhmeRange[" << i << "]="                            << param_.hmeRange[i];
    }
    os << "\nbHDR10Opt="                                          << param_.bHDR10Opt;
    os << "\nbEmitHDR10SEI="                                      << param_.bEmitHDR10SEI;
    os << "\nanalysisSaveReuseLevel="                             << param_.analysisSaveReuseLevel;
    os << "\nanalysisLoadReuseLevel="                             << param_.analysisLoadReuseLevel;
    os << "\nconfWinRightOffset="                                 << param_.confWinRightOffset;
    os << "\nconfWinBottomOffset="                                << param_.confWinBottomOffset;
    os << "\nedgeVarThreshold="                                   << param_.edgeVarThreshold;
    os << "\ndecoderVbvMaxRate="                                  << param_.decoderVbvMaxRate;
    os << "\nbliveVBV2pass="                                      << param_.bliveVBV2pass;
    os << "\nminVbvFullness="                                     << param_.minVbvFullness;
    os << "\nmaxVbvFullness="                                     << param_.maxVbvFullness;
    os << "\nbwdMaxScenecutWindow="                               << param_.bwdMaxScenecutWindow;
    for(int i = 0; i < 6; ++i)
    {
      os << "\nbwdScenecutWindow[" << i << "]="                   << param_.bwdScenecutWindow[i];
    }
    for(int i = 0; i < 6; ++i)
    {
      os << "\nbwdRefQpDelta[" << i << "]="                       << param_.bwdRefQpDelta[i];
    }
    for(int i = 0; i < 6; ++i)
    {
      os << "\nbwdNonRefQpDelta[" << i << "]="                    << param_.bwdNonRefQpDelta[i];
    }
    os << "\nvideoSignalTypePreset="                              << param_.videoSignalTypePreset;
    os << "\nbEnableEndOfBitstream="                              << param_.bEnableEndOfBitstream;
    os << "\nbEnableEndOfSequence="                               << param_.bEnableEndOfSequence;
    os << "\nfilmGrain="                                          << ptr_value_t(param_.filmGrain);
    os << "\naomFilmGrain="                                       << ptr_value_t(param_.aomFilmGrain);
    os << "\nbEnableTemporalFilter="                              << param_.bEnableTemporalFilter;
    os << "\nmcstfFrameRange="                                    << param_.mcstfFrameRange;
    os << "\nbSelectiveMCSTF="                                    << param_.bSelectiveMCSTF;
    os << "\ntmeTaskBlockSize="                                   << param_.tmeTaskBlockSize;
    os << "\ntmeNumBufferRows="                                   << param_.tmeNumBufferRows;
    os << "\ntmeNumThreads="                                      << param_.tmeNumThreads;
    os << "\nbEnableSBRC="                                        << param_.bEnableSBRC;
    os << "\nbEnableAlpha="                                       << param_.bEnableAlpha;
    os << "\nnumScalableLayers="                                  << param_.numScalableLayers;
    os << "\nnumViews="                                           << param_.numViews;
    os << "\nformat="                                             << param_.format;
    os << "\nnumLayers="                                          << param_.numLayers;
    os << "\nbEnableSCC="                                         << param_.bEnableSCC;
    os << "\nbConfigRCFrame="                                     << param_.bConfigRCFrame;
    os << "\nisAbrLadderEnable="                                  << param_.isAbrLadderEnable;
    os << "\ntune="                                               << ptr_value_t(param_.tune);
    os << "\nfoveaGazeX="                                         << param_.foveaGazeX;
    os << "\nfoveaGazeY="                                         << param_.foveaGazeY;
    os << "\nfoveaDelta="                                         << param_.foveaDelta;
    os << "\nfoveaSigma="                                         << param_.foveaSigma;
    os << "\nfoveaGazeFile="                                      << ptr_value_t(param_.foveaGazeFile);
  }

private:
  static x265_param make_param(wrap_x265_api_t const& api)
  {
    x265_param param;
    api->param_default(&param);
    return param;
  }

  static x265_param make_param(wrap_x265_api_t const& api,
    std::string const& preset, std::string const& tune)
  {
    x265_param param;
    auto result = api->param_default_preset(&param,
      to_ptr(preset), to_ptr(tune));
    if(result != 0)
    {
      x265_exception_builder_t builder;
      builder << "x265_param_default_preset failed for preset " <<
        to_sv(preset) << " and tune " << to_sv(tune);
      builder.explode();
    }
    return param;
  }

  wrap_x265_api_t const& api_;
  x265_param param_;
};

inline
std::ostream& operator<<(std::ostream& os, wrap_x265_param_t const& rhs)
{
  rhs.print(os);
  return os;
}

////////////////////////////////////////////////////////////////////////////////

struct slice_type_t
{
  explicit slice_type_t(int type)
  : type_(type)
  { }

  int type_;
};

std::ostream& operator<<(std::ostream& os, slice_type_t const& rhs)
{
  switch(rhs.type_)
  {
  case X265_TYPE_AUTO:
    os << "AUTO";
    break;
  case X265_TYPE_IDR:
    os << "IDR";
    break;
  case X265_TYPE_I:
    os << "I";
    break;
  case X265_TYPE_P:
    os << "P";
    break;
  case X265_TYPE_BREF:
    os << "BREF";
    break;
  case X265_TYPE_B:
    os << "B";
    break;
  default:
    os << "Unknown x265 slice type value " << rhs.type_;
    break;
  }
  return os;
}

////////////////////////////////////////////////////////////////////////////////

struct color_space_t
{
  explicit color_space_t(int space)
  : space_(space)
  { }

  int space_;
};

std::ostream& operator<<(std::ostream& os, color_space_t const& rhs)
{
  switch(rhs.space_)
  {
  case X265_CSP_I400:
    os << "4:0:0";
    break;
  case X265_CSP_I420:
    os << "4:2:0";
    break;
  case X265_CSP_I422:
    os << "4:2:2";
    break;
  case X265_CSP_I444:
    os << "4:4:4";
    break;
  default:
    os << "Unknown x265 color space value " << rhs.space_;
    break;
  }
  return os;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, x265_analysis_data const& rhs)
{
  os << "x265_analysis_data at " << static_cast<void const*>(&rhs);
  return os;
}

std::ostream& operator<<(std::ostream& os, x265_frame_stats const& rhs)
{
  os << "x265_frame_stats at " << static_cast<void const*>(&rhs);
  return os;
}

std::ostream& operator<<(std::ostream& os, x265_sei_payload const& rhs)
{
  os << "payloadSize=" << rhs.payloadSize;
  os << " payloadType=" << rhs.payloadType;
  os << " payload:";
  os << cuti::hexdump(rhs.payload, rhs.payloadSize);
  return os;
}

std::ostream& operator<<(std::ostream& os, x265_sei const& rhs)
{
  os << "numPayloads=" << rhs.numPayloads;
  for(int i = 0; i < rhs.numPayloads; ++i)
  {
    os << "\npayloads[" << i << "]=" << rhs.payloads[i];
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, x265_dolby_vision_rpu const& rhs)
{
  os << "payloadSize=" << rhs.payloadSize;
  os << " payload:";
  os << cuti::hexdump(rhs.payload, rhs.payloadSize);
  return os;
}

////////////////////////////////////////////////////////////////////////////////

struct wrap_x265_picture_t
{
  wrap_x265_picture_t(wrap_x265_api_t const& api, wrap_x265_param_t& param)
  : api_(api)
  , picture_()
  {
    api_->picture_init(param.get(), get());
  }

  x265_picture const* get() const
  {
    return &picture_;
  }

  x265_picture* get()
  {
    return &picture_;
  }

  x265_picture const* operator->() const
  {
    return get();
  }

  x265_picture* operator->()
  {
    return get();
  }

  void print(std::ostream& os) const
  {
    os << "pts="                     << picture_.pts;
    os << "\ndts="                   << picture_.dts;
    os << "\nvbvEndFlag="            << picture_.vbvEndFlag;
    os << "\nuserData="              << picture_.userData;
    for(int i = 0; i < 4; ++i)
    {
      os << "\nplanes[" << i << "]=" << picture_.planes[i];
    }
    for(int i = 0; i < 4; ++i)
    {
      os << "\nstride[" << i << "]=" << picture_.stride[i];
    }
    os << "\nbitDepth="              << picture_.bitDepth;
    os << "\nsliceType="             << slice_type_t(picture_.sliceType);
    os << "\npoc="                   << picture_.poc;
    os << "\ncolorSpace="            << color_space_t(picture_.colorSpace);
    os << "\nforceqp="               << picture_.forceqp;
    os << "\nanalysisData="          << picture_.analysisData;
    os << "\nquantOffsets="          << picture_.quantOffsets;
    os << "\nframeData="             << picture_.frameData;
    os << "\nuserSEI="               << picture_.userSEI;
    os << "\nrcData="                << picture_.rcData;
    os << "\nframesize="             << picture_.framesize;
    os << "\nheight="                << picture_.height;
    os << "\nreorderedPts="          << picture_.reorderedPts;
    os << "\nrpu="                   << picture_.rpu;
    os << "\nfieldNum="              << picture_.fieldNum;
    os << "\npicStruct="             << picture_.picStruct;
    os << "\nwidth="                 << picture_.width;
    os << "\nlayerID="               << picture_.layerID;
    os << "\nformat="                << picture_.format;
  }

private:
  wrap_x265_api_t const& api_;
  x265_picture picture_;
};

inline
std::ostream& operator<<(std::ostream& os, wrap_x265_picture_t const& rhs)
{
  rhs.print(os);
  return os;
}

////////////////////////////////////////////////////////////////////////////////

struct x265_input_picture_t
{
  x265_input_picture_t(wrap_x265_api_t const& api, wrap_x265_param_t& param,
                       x26x_proto::frame_t&& frame)
  : api_(api)
  , picture_(api_, param)
  , frame_(std::move(frame))
  {
    assert((frame_.format_ == x26x_proto::format_t::YUV420P &&
      api_->bit_depth == 8) ||
      (frame_.format_ == x26x_proto::format_t::YUV420P10LE &&
      api_->bit_depth == 10));
    uint32_t elem_size = frame_.format_ == x26x_proto::format_t::YUV420P10LE ?
      2 : 1;
    uint32_t const y_stride = frame_.width_ * elem_size;
    uint32_t const y_size = y_stride * frame_.height_;
    uint32_t const u_stride = frame_.width_ / 2 * elem_size;
    uint32_t const u_size = u_stride * frame_.height_ / 2;
    assert(y_size + u_size * 2 == frame_.data_.size());

    picture_->pts = frame_.pts_;
    picture_->dts = frame_.pts_;
    picture_->planes[0] = frame_.data_.data();
    picture_->planes[1] = frame_.data_.data() + y_size;
    picture_->planes[2] = frame_.data_.data() + y_size + u_size;
    picture_->stride[0] = y_stride;
    picture_->stride[1] = u_stride;
    picture_->stride[2] = u_stride;
    picture_->sliceType = frame_.keyframe_ ? X265_TYPE_IDR : X265_TYPE_AUTO;
    picture_->framesize = frame_.data_.size();
    picture_->height = frame_.height_;
    picture_->width = frame_.width_;
  }

  x265_input_picture_t(x265_input_picture_t const&) = delete;
  x265_input_picture_t& operator=(x265_input_picture_t const&) = delete;

  x265_picture const* get() const
  {
    return picture_.get();
  }

  x265_picture* get()
  {
    return picture_.get();
  }

private:
  wrap_x265_api_t const& api_;
  wrap_x265_picture_t picture_;
  x26x_proto::frame_t frame_;
};

////////////////////////////////////////////////////////////////////////////////

struct x265_output_t
{
  x265_output_t(wrap_x265_api_t const& api, wrap_x265_param_t& param)
  : nals_(nullptr)
  , num_nals_(0)
  , picture_(api, param)
  { }

  x265_output_t(x265_output_t const&) = delete;
  x265_output_t& operator=(x265_output_t const&) = delete;

  x265_nal* nals_;
  uint32_t num_nals_;
  wrap_x265_picture_t picture_;
};

////////////////////////////////////////////////////////////////////////////////

struct wrap_x265_encoder_t
{
  wrap_x265_encoder_t(wrap_x265_api_t const& api, wrap_x265_param_t& param)
  : api_(api)
  , encoder_(api_->encoder_open(param.get()))
  {
    if(encoder_ == nullptr)
    {
      x265_exception_builder_t builder;
      builder << "x265_encoder_open failed";
      builder.explode();
    }
  }

  ~wrap_x265_encoder_t()
  {
    api_->encoder_close(get());
  }

  x265_encoder* get() const
  {
    return encoder_;
  }

  x265_encoder* operator->() const
  {
    return get();
  }

  void parameters(x265_param* out) const
  {
    api_->encoder_parameters(get(), out);
  }

  int headers(x265_nal **nals_out, uint32_t *num_nals_out) const
  {
    return api_->encoder_headers(get(), nals_out, num_nals_out);
  }

  int encode(x265_nal** nals_out, uint32_t* num_nals_out,
             x265_picture* pic_in, x265_picture* pic_out) const
  {
    return api_->encoder_encode(get(), nals_out, num_nals_out, pic_in, pic_out);
  }

  int reconfig(x265_param* param_in) const
  {
    return api_->encoder_reconfig(get(), param_in);
  }

private:
  wrap_x265_api_t const& api_;
  x265_encoder* encoder_;
};


////////////////////////////////////////////////////////////////////////////////

cuti::loglevel_t x265_log_level_to_cuti(int x265_log_level)
{
  switch(x265_log_level)
  {
  case X265_LOG_NONE:
  case X265_LOG_ERROR:
    return cuti::loglevel_t::error;
  case X265_LOG_WARNING:
    return cuti::loglevel_t::warning;
  case X265_LOG_INFO:
    return cuti::loglevel_t::info;
  case X265_LOG_DEBUG:
    return cuti::loglevel_t::debug;
  case X265_LOG_FULL:
  default:
    // NOTE: since this function will be called from a C callback, throwing
    // exceptions is not the best idea.
    return cuti::loglevel_t::debug;
  }
}

void x265_log_callback(void* context, const char* caller, int level,
  const char* fmt, va_list args)
{
  if(context != nullptr)
  {
    auto const& logging_context =
      *static_cast<cuti::logging_context_t const*>(context);
    auto cuti_level = x265_log_level_to_cuti(level);

    if(auto msg = logging_context.message_at(cuti_level))
    {
      auto text = cuti::vstringprintf(fmt, args);

      // Remove extraneous newlines at front and back
      auto lpos = text.find_first_not_of("\r\n");
      lpos = lpos == std::string::npos ? 0 : lpos;

      auto rpos = text.find_last_not_of("\r\n");
      rpos = rpos == std::string::npos ? text.size() : rpos + 1;

      *msg << (caller == nullptr ? "x265" : caller) << ": " <<
        std::string_view(text.begin() + lpos, text.begin() + rpos);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

char const* x265_profile_name(x265_proto::profile_t profile)
{
  switch(profile)
  {
  case x265_proto::profile_t::MAIN:
    return "main";
  case x265_proto::profile_t::MAIN10:
    return "main10";
  default:
    x265_exception_builder_t builder;
    builder << "unsupported x265 profile value " << to_string(profile);
    builder.explode();
  }
}

////////////////////////////////////////////////////////////////////////////////

struct x265_encoder_t
{
  x265_encoder_t(
    cuti::logging_context_t const& logging_context,
    encoder_settings_t const& encoder_settings,
    x265_proto::session_params_t const& session_params)
  : logging_context_(logging_context)
  , api_(to_x265_bitdepth(session_params.common_.format_))
  , param_(api_, encoder_settings, session_params)
  , encoder_()
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding to hevc profile="
        << to_string(session_params.general_profile_idc_)
        << " tier=" << session_params.general_tier_flag_
        << " level=" << session_params.general_level_idc_
        << " bitrate=" << session_params.common_.bitrate_
        << " width=" << session_params.common_.width_
        << " height=" << session_params.common_.height_
        << " format=" << to_string(session_params.common_.format_);
      if(!encoder_settings.preset_.empty())
      {
        *msg << " preset=" << encoder_settings.preset_;
      }
      if(!encoder_settings.tune_.empty())
      {
        *msg << " tune=" << encoder_settings.tune_;
      }
    }

    if(session_params.common_.bitrate_ == 0)
    {
      x265_exception_builder_t builder;
      builder << "bad x265_proto::session_params.common_.bitrate_ value " <<
        session_params.common_.bitrate_;
      builder.explode();
    }

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "initial param:\n" << param_;
    }

    param_->frameNumThreads = encoder_settings.frame_threads_;
    param_->logCallback = x265_log_callback;
    param_->logContext = const_cast<cuti::logging_context_t*>(&logging_context_);
    assert(param_->internalBitDepth == api_->bit_depth);
    assert(param_->internalCsp == X265_CSP_I420);
    if(session_params.common_.framerate_)
    {
      param_->fpsNum = session_params.common_.framerate_->first;
      param_->fpsDenom = session_params.common_.framerate_->second;
    }
    else
    {
      // x265 requires fps fields to be filled in.
      param_->fpsNum = 25;
      param_->fpsDenom = 1;
    }
    param_->sourceWidth = session_params.common_.width_;;
    param_->sourceHeight = session_params.common_.height_;
    // ISO/IEC 23008-2, A.4.1: "general_level_idc and sub_layer_level_idc[i]
    // shall be set equal to a value of 30 times the level number specified in
    // Table A.8."
    //
    // Meanwhile, x265 multiplies this value again with 10. So e.g. level 5.1
    // becomes becomes general_level_idc 153, and netint_libxcoder then expects
    // the integer value 51 in x265_param::levelIdc.
    param_->levelIdc = session_params.general_level_idc_ / 3;
    param_->bHighTier = session_params.general_tier_flag_;
    param_->bEmitInfoSEI = 0;
    param_->bOpenGOP = 0;
    param_->keyframeMax = -1;
    if(param_->sourceHeight < 720)
    {
      // Avoid x265 warning about disabling lookahead-slices.
      param_->lookaheadSlices = 0;
    }
    param_->rc.rateControlMode = X265_RC_ABR;
    // NOTE: x265's bitrate is specified in kbps.
    param_->rc.bitrate = (session_params.common_.bitrate_ + 500) / 1000;
    param_->vui.aspectRatioIdc = X265_EXTENDED_SAR;
    param_->vui.sarWidth = session_params.common_.sar_width_;
    param_->vui.sarHeight = session_params.common_.sar_height_;
    if(session_params.vui_overscan_appropriate_flag_)
    {
      param_->vui.bEnableOverscanInfoPresentFlag = 1;
      param_->vui.bEnableOverscanAppropriateFlag =
        *session_params.vui_overscan_appropriate_flag_;
    }
    if(session_params.vui_video_format_)
    {
      param_->vui.bEnableVideoSignalTypePresentFlag = 1;
      param_->vui.videoFormat = *session_params.vui_video_format_;
    }
    if(session_params.vui_video_full_range_flag_)
    {
      param_->vui.bEnableVideoFullRangeFlag =
        *session_params.vui_video_full_range_flag_;
    }
    if(session_params.vui_colour_primaries_)
    {
      param_->vui.colorPrimaries = *session_params.vui_colour_primaries_;
    }
    if(session_params.vui_transfer_characteristics_)
    {
      param_->vui.colorPrimaries =
        *session_params.vui_transfer_characteristics_;
    }
    if(session_params.vui_matrix_coefficients_)
    {
      param_->vui.matrixCoeffs = *session_params.vui_matrix_coefficients_;
    }
    if(session_params.vui_chroma_sample_loc_type_top_field_)
    {
      param_->vui.bEnableChromaLocInfoPresentFlag = 1;
      param_->vui.chromaSampleLocTypeTopField =
        *session_params.vui_chroma_sample_loc_type_top_field_;
    }
    if(session_params.vui_chroma_sample_loc_type_bottom_field_)
    {
      param_->vui.chromaSampleLocTypeBottomField =
        *session_params.vui_chroma_sample_loc_type_bottom_field_;
    }
    if(session_params.vui_def_disp_win_left_offset_)
    {
      param_->vui.bEnableDefaultDisplayWindowFlag = 1;
      param_->vui.defDispWinLeftOffset =
        *session_params.vui_def_disp_win_left_offset_;
    }
    if(session_params.vui_def_disp_win_right_offset_)
    {
      param_->vui.defDispWinRightOffset =
        *session_params.vui_def_disp_win_right_offset_;
    }
    if(session_params.vui_def_disp_win_top_offset_)
    {
      param_->vui.defDispWinTopOffset =
        *session_params.vui_def_disp_win_top_offset_;
    }
    if(session_params.vui_def_disp_win_bottom_offset_)
    {
      param_->vui.defDispWinBottomOffset =
        *session_params.vui_def_disp_win_bottom_offset_;
    }

    auto const* profile_name =
      x265_profile_name(session_params.general_profile_idc_);
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "applying x265 profile: " << profile_name;
    }
    param_.apply_profile(profile_name);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "adjusted param:\n" << param_;
    }
    encoder_ = std::make_unique<wrap_x265_encoder_t>(api_, param_);

    wrap_x265_param_t encoder_param(api_);
    encoder_->parameters(encoder_param.get());
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoder param:\n" << encoder_param;
    }
  }

  x265_encoder_t(const x265_encoder_t&) = delete;
  x265_encoder_t& operator=(const x265_encoder_t&) = delete;

  wrap_x265_api_t const& api() const
  {
    return api_;
  }

  wrap_x265_param_t& param()
  {
    return param_;
  }

  int headers(x265_nal** headers_out, uint32_t *num_headers_out) const
  {
    return encoder_->headers(headers_out, num_headers_out);
  }

  int encode(x265_nal** nals_out, uint32_t* num_nals_out, x265_picture* pic_in,
    x265_picture* pic_out) const
  {
    return encoder_->encode(nals_out, num_nals_out, pic_in, pic_out);
  }

  int flush(x265_nal** nals_out, uint32_t* num_nals_out, x265_picture* pic_out)
    const
  {
    return encoder_->encode(nals_out, num_nals_out, nullptr, pic_out);
  }

private :
  cuti::logging_context_t const& logging_context_;
  wrap_x265_api_t api_;
  wrap_x265_param_t param_;
  std::unique_ptr<wrap_x265_encoder_t> encoder_;
};

////////////////////////////////////////////////////////////////////////////////

struct nal_type_t
{
  explicit nal_type_t(int type)
  : type_(type)
  { }

  int type_;
};

std::ostream& operator<<(std::ostream& os, nal_type_t const& rhs)
{
  switch(rhs.type_)
  {
  case NAL_UNIT_CODED_SLICE_TRAIL_N:
    os << "TRAIL_N";
    break;
  case NAL_UNIT_CODED_SLICE_TRAIL_R:
    os << "TRAIL_R";
    break;
  case NAL_UNIT_CODED_SLICE_TSA_N:
    os << "TSA_N";
    break;
  case NAL_UNIT_CODED_SLICE_TSA_R:
    os << "TSA_R";
    break;
  case NAL_UNIT_CODED_SLICE_STSA_N:
    os << "STSA_N";
    break;
  case NAL_UNIT_CODED_SLICE_STSA_R:
    os << "STSA_R";
    break;
  case NAL_UNIT_CODED_SLICE_RADL_N:
    os << "RADL_N";
    break;
  case NAL_UNIT_CODED_SLICE_RADL_R:
    os << "RADL_R";
    break;
  case NAL_UNIT_CODED_SLICE_RASL_N:
    os << "RASL_N";
    break;
  case NAL_UNIT_CODED_SLICE_RASL_R:
    os << "RASL_R";
    break;
  case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    os << "BLA_W_LP";
    break;
  case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    os << "BLA_W_RADL";
    break;
  case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    os << "BLA_N_LP";
    break;
  case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    os << "IDR_W_RADL";
    break;
  case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    os << "IDR_N_LP";
    break;
  case NAL_UNIT_CODED_SLICE_CRA:
    os << "CRA";
    break;
  case NAL_UNIT_VPS:
    os << "VPS";
    break;
  case NAL_UNIT_SPS:
    os << "SPS";
    break;
  case NAL_UNIT_PPS:
    os << "PPS";
    break;
  case NAL_UNIT_ACCESS_UNIT_DELIMITER:
    os << "AUD";
    break;
  case NAL_UNIT_EOS:
    os << "EOS";
    break;
  case NAL_UNIT_EOB:
    os << "EOB";
    break;
  case NAL_UNIT_FILLER_DATA:
    os << "FD";
    break;
  case NAL_UNIT_PREFIX_SEI:
    os << "PREFIX_SEI";
    break;
  case NAL_UNIT_SUFFIX_SEI:
    os << "SUFFIX_SEI";
    break;
  case NAL_UNIT_UNSPECIFIED:
    os << "UNSPECIFIED";
    break;
  case NAL_UNIT_INVALID:
    os << "INVALID";
    break;
  default:
    os << "Unknown x265 nal type value " << rhs.type_;
    break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, x265_nal const& rhs)
{
  os << "type=" << nal_type_t(rhs.type);
  os << " sizeBytes=" << rhs.sizeBytes;
  os << " payload:";
  os << cuti::hexdump(rhs.payload, rhs.sizeBytes);
  return os;
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////

x265_exception_t::x265_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

x265_exception_t::~x265_exception_t()
{ }

////////////////////////////////////////////////////////////////////////////////

struct encoding_session_t::impl_t
{
  impl_t(cuti::logging_context_t const& logging_context,
         encoder_settings_t const& encoder_settings,
         x265_proto::session_params_t const& session_params)
  : logging_context_(logging_context)
  , encoder_(logging_context_, encoder_settings, session_params)
  , frame_count_(0)
  , sample_count_(0)
  , first_cto_(std::nullopt)
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

  x265_proto::sample_headers_t sample_headers() const
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoding_session[" << this << "]: retrieving sample headers";
    }

    x265_nal* headers;
    uint32_t num_headers;
    int num_bytes = encoder_.headers(&headers, &num_headers);
    if(num_bytes < 0)
    {
      x265_exception_builder_t builder;
      builder << "libx265 failed to retrieve sample headers";
      builder.explode();
    }
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      for(uint32_t i = 0; i < num_headers; ++i)
      {
        if(i > 0)
        {
          *msg << '\n';
        }
        *msg << "nal[" << i << "]=" << headers[i];
      }
    }

    // Sanity checks: x265 is supposed to return VPS, SPS, PPS and optionally
    // PREFIX_SEI (disposable, containing x265 copyright banner and parameter
    // text, see the bEmitInfoSEI setting above).
    assert(num_headers == 3);

    assert(headers[0].type == NAL_UNIT_VPS);
    assert(headers[0].sizeBytes > 5);
    assert(headers[0].payload[0] == 0x00);
    assert(headers[0].payload[1] == 0x00);
    assert(headers[0].payload[2] == 0x00);
    assert(headers[0].payload[3] == 0x01);
    assert(headers[0].payload[4] == 0x40); // NAL_UNIT_VPS

    assert(headers[1].type == NAL_UNIT_SPS);
    assert(headers[1].sizeBytes > 5);
    assert(headers[1].payload[0] == 0x00);
    assert(headers[1].payload[1] == 0x00);
    assert(headers[1].payload[2] == 0x00);
    assert(headers[1].payload[3] == 0x01);
    assert(headers[1].payload[4] == 0x42); // NAL_UNIT_SPS

    assert(headers[2].type == NAL_UNIT_PPS);
    assert(headers[2].sizeBytes > 5);
    assert(headers[2].payload[0] == 0x00);
    assert(headers[2].payload[1] == 0x00);
    assert(headers[2].payload[2] == 0x00);
    assert(headers[2].payload[3] == 0x01);
    assert(headers[2].payload[4] == 0x44); // NAL_UNIT_PPS

#if 0
    assert(headers[3].type == NAL_UNIT_PREFIX_SEI);
    assert(headers[3].sizeBytes > 4);
    assert(headers[3].payload[0] == 0x00);
    assert(headers[3].payload[1] == 0x00);
    assert(headers[3].payload[2] == 0x01);
    assert(headers[3].payload[3] == 0x4e); // NAL_UNIT_PREFIX_SEI
#endif

    x265_proto::sample_headers_t sample_headers;
    sample_headers.vps_.insert(sample_headers.vps_.end(),
      headers[0].payload, headers[0].payload + headers[0].sizeBytes);
    sample_headers.sps_.insert(sample_headers.sps_.end(),
      headers[1].payload, headers[1].payload + headers[1].sizeBytes);
    sample_headers.pps_.insert(sample_headers.pps_.end(),
      headers[2].payload, headers[2].payload + headers[2].sizeBytes);

    return sample_headers;
  }

  std::optional<x26x_proto::sample_t> encode(x26x_proto::frame_t frame)
  {
    assert(! flush_called_);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoding_session[" << this << "]: encoding frame #" <<
        frame_count_ << " pts=" << frame.pts_ << '/' << frame.timescale_;
      if(frame.keyframe_)
      {
        *msg << " (keyframe)";
      }
    }
    ++frame_count_;

    x265_output_t output(encoder_.api(), encoder_.param());
    x265_input_picture_t pic_in(encoder_.api(), encoder_.param(),
      std::move(frame));
    auto result = encoder_.encode(&output.nals_, &output.num_nals_,
      pic_in.get(), output.picture_.get());
    if(result < 0)
    {
      x265_exception_builder_t builder;
      builder << "x265 failed to encode frame";
      builder.explode();
    }
    else if(result == 0)
    {
      if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
      {
        *msg << "encoding_session[" << this << "]: no sample available yet";
      }

      // No sample has been produced yet.
      return std::nullopt;
    }

    // x265 always returns one sample at a time.
    assert(result == 1);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoding_session[" << this << "]: encoded sample #" <<
        sample_count_;
    }

    return generate_sample(output);
  }

  std::optional<x26x_proto::sample_t> flush()
  {
    flush_called_ = true;

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoding_session[" << this << "]: flushing sample #" <<
        sample_count_;
    }

    x265_output_t output(encoder_.api(), encoder_.param());
    auto result = encoder_.flush(&output.nals_, &output.num_nals_,
      output.picture_.get());
    if(result < 0)
    {
      x265_exception_builder_t builder;
      builder << "x265 failed to flush sample";
      builder.explode();
    }
    else if(result == 0)
    {
      if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
      {
        *msg << "encoding_session[" << this << "]: no more samples";
      }

      // End of samples.
      return std::nullopt;
    }

    // x265 always returns one sample at a time.
    assert(result == 1);

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "encoding_session[" << this << "]: flushed sample #" <<
        sample_count_;
    }

    return generate_sample(output);
  }

private :
  x26x_proto::sample_t generate_sample(x265_output_t const& output)
  {
    assert(output.nals_ != nullptr);
    assert(output.num_nals_ == 1);

    auto encoder_cto = output.picture_->pts - output.picture_->dts;
    assert(encoder_cto >= std::numeric_limits<int32_t>::min());
    assert(encoder_cto <= std::numeric_limits<int32_t>::max());
    if(!first_cto_)
    {
      first_cto_ = static_cast<int32_t>(encoder_cto);
    }
    auto dts = output.picture_->dts + *first_cto_;
    auto cto = output.picture_->pts - dts;

    if(auto msg = logging_context_.message_at(cuti::loglevel_t::debug))
    {
      *msg << "sample[" << sample_count_ << "]"
        << " dts=" << dts << " (was " << output.picture_->dts << ")"
        << " pts=" << output.picture_->pts
        << " cto=" << cto << " (was " << encoder_cto << ")"
        << " first_cto=" << *first_cto_
        << " size=" << output.nals_[0].sizeBytes
        << " pic type=" << slice_type_t(output.picture_->sliceType)
        << " nal type=" << nal_type_t(output.nals_[0].type);
    }
    ++sample_count_;

    x26x_proto::sample_t sample;
    assert(dts >= 0);
    assert(output.picture_->pts >= 0);
    sample.dts_ = dts;
    sample.pts_ = output.picture_->pts;
    switch(output.picture_->sliceType)
    {
    case X265_TYPE_IDR:
      sample.type_ = x26x_proto::sample_t::type_t::i;
      break;
    case X265_TYPE_I:
    case X265_TYPE_P:
      sample.type_ = x26x_proto::sample_t::type_t::p;
      break;
    case X265_TYPE_B:
      sample.type_ = x26x_proto::sample_t::type_t::b;
      break;
    case X265_TYPE_BREF:
      sample.type_ = x26x_proto::sample_t::type_t::b_ref;
      break;
    default:
      x265_exception_builder_t builder;
      builder << "unexpected x265 picture type " <<
        slice_type_t(output.picture_->sliceType);
      builder.explode();
    }
    sample.data_.insert(sample.data_.end(), output.nals_[0].payload,
      output.nals_[0].payload + output.nals_[0].sizeBytes);

    return sample;
  }

private :
  cuti::logging_context_t const& logging_context_;
  x265_encoder_t encoder_;
  uint64_t frame_count_;
  uint64_t sample_count_;
  std::optional<int32_t> first_cto_;
  bool flush_called_;
};

////////////////////////////////////////////////////////////////////////////////

encoding_session_t::encoding_session_t(
  cuti::logging_context_t const& logging_context,
  encoder_settings_t const& encoder_settings,
  x265_proto::session_params_t const& session_params)
: impl_(std::make_unique<impl_t>(
    logging_context, encoder_settings, session_params))
{
}

x265_proto::sample_headers_t encoding_session_t::sample_headers() const
{
  return impl_->sample_headers();
}

std::optional<x26x_proto::sample_t>
encoding_session_t::encode(x26x_proto::frame_t frame)
{
  return impl_->encode(std::move(frame));

}

std::optional<x26x_proto::sample_t>
encoding_session_t::flush()
{
  return impl_->flush();
}

encoding_session_t::~encoding_session_t()
{
}

} // x265_es_utils
