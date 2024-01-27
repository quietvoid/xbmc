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
private:
  // Used when parsing
  uint8_t last_payload_type_byte;
  uint8_t last_payload_size_byte;

  static const std::vector<CHevcSei> ParseSeiRbspInternal(const uint8_t* buf, const size_t len);
};
