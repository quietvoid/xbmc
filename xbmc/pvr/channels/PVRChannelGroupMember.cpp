/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupMember.h"

#include "ServiceBroker.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "utils/DatabaseUtils.h"
#include "utils/SortUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace PVR;

CPVRChannelGroupMember::CPVRChannelGroupMember(int iGroupID,
                                               const std::string& groupName,
                                               const std::shared_ptr<CPVRChannel>& channel)
  : m_iGroupID(iGroupID),
    m_clientChannelNumber(channel->ClientChannelNumber()),
    m_iOrder(channel->ClientOrder())
{
  SetChannel(channel);
  SetGroupName(groupName);
}

void CPVRChannelGroupMember::Serialize(CVariant& value) const
{
  value["channelnumber"] = m_channelNumber.GetChannelNumber();
  value["subchannelnumber"] = m_channelNumber.GetSubChannelNumber();
}

void CPVRChannelGroupMember::SetChannel(const std::shared_ptr<CPVRChannel>& channel)
{
  // note: no need to set m_bChanged, as these values are not persisted in the db
  m_channel = channel;
  m_iClientID = channel->ClientID();
  m_iChannelUID = channel->UniqueID();
  m_bIsRadio = channel->IsRadio();
}

void CPVRChannelGroupMember::ToSortable(SortItem& sortable, Field field) const
{
  if (field == FieldChannelNumber)
  {
    sortable[FieldChannelNumber] = m_channelNumber.SortableChannelNumber();
  }
  else if (field == FieldClientChannelOrder)
  {
    if (m_iOrder)
      sortable[FieldClientChannelOrder] = m_iOrder;
    else
      sortable[FieldClientChannelOrder] = m_clientChannelNumber.SortableChannelNumber();
  }
}

void CPVRChannelGroupMember::SetGroupID(int iGroupID)
{
  if (m_iGroupID != iGroupID)
  {
    m_iGroupID = iGroupID;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetGroupName(const std::string& groupName)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientID);
  if (client)
    m_path = CPVRChannelsPath(m_bIsRadio, groupName, client->ID(), m_iChannelUID);
  else
    CLog::LogF(LOGERROR, "Unable to obtain instance for client id: {}", m_iClientID);
}

void CPVRChannelGroupMember::SetChannelNumber(const CPVRChannelNumber& channelNumber)
{
  if (m_channelNumber != channelNumber)
  {
    m_channelNumber = channelNumber;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetClientChannelNumber(const CPVRChannelNumber& clientChannelNumber)
{
  if (m_clientChannelNumber != clientChannelNumber)
  {
    m_clientChannelNumber = clientChannelNumber;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetClientPriority(int iClientPriority)
{
  if (m_iClientPriority != iClientPriority)
  {
    m_iClientPriority = iClientPriority;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetOrder(int iOrder)
{
  if (m_iOrder != iOrder)
  {
    m_iOrder = iOrder;
    m_bChanged = true;
  }
}

bool CPVRChannelGroupMember::QueueDelete()
{
  const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (!database)
    return false;

  return database->QueueDeleteQuery(*this);
}
