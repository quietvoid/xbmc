/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BitstreamReader.h"

#include <optional>
#include <vector>

class CHevcSei
{
public:
  CHevcSei() = default;
  ~CHevcSei() = default;

  uint8_t payload_type;
  size_t payload_size;

  // In relation to the input SEI rbsp payload
  size_t msg_offset;
  size_t payload_offset;

  // Parses single SEI message from the reader and pushes it to the list
  static int ParseSeiMessage(CBitstreamReader& br, std::vector<CHevcSei>& messages);

  // Parses SEI payload assumed to not have emulation prevention 3 bytes
  static const std::vector<CHevcSei> ParseSeiRbsp(const uint8_t* buf, const size_t len);

  // Clears emulation prevention 3 bytes and fills in the passed buf
  static const std::vector<CHevcSei> ParseSeiRbspUnclearedEmulation(const uint8_t* in_data,
                                                                    const size_t in_data_len,
                                                                    std::vector<uint8_t>& buf);

  // Returns a HDR10+ SEI message if present in the list
  static const std::optional<const CHevcSei*> FindHdr10PlusSeiMessage(
      const std::vector<uint8_t>& buf, const std::vector<CHevcSei>& messages);

  // Returns a pair with:
  //   1) a bool for whether or not the NALU SEI payload contains a HDR10+ SEI message.
  //   2) a vector of bytes:
  //      When not empty: the new NALU containing all but the HDR10+ SEI message.
  //      Otherwise: the NALU contained only one HDR10+ SEI and can be discarded.
  static const std::pair<bool, const std::vector<uint8_t>> RemoveHdr10PlusFromSeiNalu(
      const uint8_t* in_data, const size_t in_data_len);

private:
  // Used when parsing
  uint8_t last_payload_type_byte;
  uint8_t last_payload_size_byte;

  static const std::vector<CHevcSei> ParseSeiRbspInternal(const uint8_t* buf, const size_t len);
};
