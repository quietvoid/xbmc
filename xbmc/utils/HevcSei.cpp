/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HevcSei.h"

void hevc_add_start_code_emulation_prevention_3_byte(std::vector<uint8_t>& buf)
{
  auto count = buf.size();
  size_t i = 0;

  while (i < count)
  {
    if (i > 2 && buf[i - 2] == 0 && buf[i - 1] == 0 && buf[i] <= 3)
    {
      buf.insert(buf.begin() + i, 3);
      count += 1;
    }

    i += 1;
  }
}

void hevc_clear_start_code_emulation_prevention_3_byte(const uint8_t* buf,
                                                       const size_t len,
                                                       std::vector<uint8_t>& out)
{
  size_t i = 0;

  if (len > 2)
  {
    out.reserve(len);

    out.push_back(buf[0]);
    out.push_back(buf[1]);

    for (i = 2; i < len; i++)
    {
      if (!(buf[i - 2] == 0 && buf[i - 1] == 0 && buf[i] == 3))
        out.push_back(buf[i]);
    }
  }
  else
  {
    out.assign(buf, buf + len);
  }
}

int CHevcSei::ParseSeiMessage(CBitstreamReader& br, std::vector<CHevcSei>& messages)
{
  CHevcSei sei{};
  sei.msg_offset = br.Position() / 8;
  sei.last_payload_type_byte = br.ReadBits(8);

  while (sei.last_payload_type_byte == 0xFF)
  {
    sei.last_payload_type_byte = br.ReadBits(8);
    sei.payload_type += 255;
  }

  sei.payload_type += sei.last_payload_type_byte;

  sei.last_payload_size_byte = br.ReadBits(8);
  while (sei.last_payload_size_byte == 0xFF)
  {
    sei.last_payload_size_byte = br.ReadBits(8);

    sei.payload_size += 255;
  }

  sei.payload_size += sei.last_payload_size_byte;
  sei.payload_offset = br.Position() / 8;

  // Invalid size
  if (sei.payload_size > br.AvailableBits())
    return 1;

  br.SkipBits(sei.payload_size * 8);
  messages.push_back(sei);

  return 0;
}

const std::vector<CHevcSei> CHevcSei::ParseSeiRbspInternal(const uint8_t* buf, const size_t len)
{
  std::vector<CHevcSei> messages;
  if (len > 4)
  {
    CBitstreamReader br(buf, len);

    // forbidden_zero_bit, nal_type, nuh_layer_id, temporal_id
    // nal_type == SEI_PREFIX should already be verified by caller
    br.SkipBits(16);

    while (true)
    {
      if (ParseSeiMessage(br, messages))
        break;

      if (br.AvailableBits() <= 8)
        break;
    }
  }

  return messages;
}

const std::vector<CHevcSei> CHevcSei::ParseSeiRbsp(const uint8_t* buf, const size_t len)
{
  return ParseSeiRbspInternal(buf, len);
}

const std::vector<CHevcSei> CHevcSei::ParseSeiRbspUnclearedEmulation(const uint8_t* in_data,
                                                                     const size_t in_data_len,
                                                                     std::vector<uint8_t>& buf)
{
  hevc_clear_start_code_emulation_prevention_3_byte(in_data, in_data_len, buf);
  return ParseSeiRbsp(buf.data(), buf.size());
}

const std::optional<const CHevcSei*> CHevcSei::FindHdr10PlusSeiMessage(
    const std::vector<uint8_t>& buf, const std::vector<CHevcSei>& messages)
{
  for (const CHevcSei& sei : messages)
  {
    if (sei.payload_type == 4 && sei.payload_size >= 7)
    {
      CBitstreamReader br(buf.data() + sei.payload_offset, sei.payload_size);
      auto itu_t_t35_country_code = br.ReadBits(8);
      auto itu_t_t35_terminal_provider_code = br.ReadBits(16);
      auto itu_t_t35_terminal_provider_oriented_code = br.ReadBits(16);

      if (itu_t_t35_country_code == 0xB5 && itu_t_t35_terminal_provider_code == 0x003C &&
          itu_t_t35_terminal_provider_oriented_code == 0x0001)
      {
        auto application_identifier = br.ReadBits(8);
        auto application_version = br.ReadBits(8);

        if (application_identifier == 4 && application_version <= 1)
          return &sei;
      }
    }
  }

  return {};
}

const std::pair<bool, const std::vector<uint8_t>> CHevcSei::RemoveHdr10PlusFromSeiNalu(
    const uint8_t* in_data, const size_t in_data_len)
{
  bool containsHdr10Plus = false;

  std::vector<uint8_t> buf;
  std::vector<CHevcSei> messages =
      CHevcSei::ParseSeiRbspUnclearedEmulation(in_data, in_data_len, buf);

  if (auto res = CHevcSei::FindHdr10PlusSeiMessage(buf, messages))
  {
    auto msg = *res;

    containsHdr10Plus = true;
    if (messages.size() > 1)
    {
      // Multiple SEI messages in NALU, remove only the HDR10+ one
      buf.erase(std::next(buf.begin(), msg->msg_offset),
                std::next(buf.begin(), msg->payload_offset + msg->payload_size));
      hevc_add_start_code_emulation_prevention_3_byte(buf);
    }
    else
    {
      // Single SEI message in NALU
      buf.clear();
    }
  }
  else
  {
    // No HDR10+
    buf.clear();
  }

  return std::make_pair(containsHdr10Plus, buf);
}
