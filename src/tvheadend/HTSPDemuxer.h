/*
 *  Copyright (C) 2017-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <atomic>
#include <map>
#include <string>
#include <time.h>
#include <vector>

extern "C"
{
#include "libhts/htsmsg.h"
}

#include "Subscription.h"
#include "kodi/libXBMC_pvr.h"
#include "p8-platform/threads/mutex.h"
#include "p8-platform/util/buffer.h"
#include "status/DescrambleInfo.h"
#include "status/Quality.h"
#include "status/SourceInfo.h"
#include "status/TimeshiftStatus.h"

namespace tvheadend
{

class HTSPConnection;

/*
 * HTSP Demuxer - live streams
 */
class HTSPDemuxer
{
public:
  HTSPDemuxer(HTSPConnection& conn);
  ~HTSPDemuxer();

  bool ProcessMessage(const std::string& method, htsmsg_t* m);
  void Connected();

  bool Open(uint32_t channelId,
            tvheadend::eSubscriptionWeight weight = tvheadend::SUBSCRIPTION_WEIGHT_NORMAL);
  void Close();
  DemuxPacket* Read();
  void Trim();
  void Flush();
  void Abort();
  bool Seek(double time, bool backwards, double* startpts);
  void Speed(int speed);
  void FillBuffer(bool mode);
  void Weight(tvheadend::eSubscriptionWeight weight);

  PVR_ERROR CurrentStreams(PVR_STREAM_PROPERTIES* streams);
  PVR_ERROR CurrentSignal(PVR_SIGNAL_STATUS* sig);
  PVR_ERROR CurrentDescrambleInfo(PVR_DESCRAMBLE_INFO* info);

  bool IsTimeShifting() const;
  bool IsRealTimeStream() const;
  PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES* times) const;

  uint32_t GetSubscriptionId() const;
  uint32_t GetChannelId() const;
  time_t GetLastUse() const;
  bool IsPaused() const;

  /**
   * Tells each demuxer to use the specified profile for new subscriptions
   * @param profile the profile to use
   */
  void SetStreamingProfile(const std::string& profile);

private:
  void Close0();
  void Abort0();

  /**
   * Resets the signal, quality, timeshift info and optionally the starttime
   * @param resetStartTime if true, startTime will be reset
   */
  void ResetStatus(bool resetStartTime = true);

  void ParseMuxPacket(htsmsg_t* m);
  void ParseSourceInfo(htsmsg_t* m);
  void ParseSubscriptionStart(htsmsg_t* m);
  void ParseSubscriptionStop(htsmsg_t* m);
  void ParseSubscriptionSkip(htsmsg_t* m);
  void ParseSubscriptionSpeed(htsmsg_t* m);
  void ParseSubscriptionGrace(htsmsg_t* m);
  void ParseQueueStatus(htsmsg_t* m);
  void ParseSignalStatus(htsmsg_t* m);
  void ParseTimeshiftStatus(htsmsg_t* m);
  void ParseDescrambleInfo(htsmsg_t* m);

  bool AddTVHStream(uint32_t idx, const char* type, htsmsg_field_t* f);
  bool AddRDSStream(uint32_t audioIdx, uint32_t rdsIdx);
  void ProcessRDS(uint32_t idx, const void* bin, size_t binlen);

  mutable P8PLATFORM::CMutex m_mutex;
  HTSPConnection& m_conn;
  P8PLATFORM::SyncedBuffer<DemuxPacket*> m_pktBuffer;
  std::vector<PVR_STREAM_PROPERTIES::PVR_STREAM> m_streams;
  std::map<int, int> m_streamStat;
  int64_t m_seekTime;
  P8PLATFORM::CCondition<volatile int64_t> m_seekCond;
  bool m_seeking;
  tvheadend::status::SourceInfo m_sourceInfo;
  tvheadend::status::Quality m_signalInfo;
  tvheadend::status::TimeshiftStatus m_timeshiftStatus;
  tvheadend::status::DescrambleInfo m_descrambleInfo;
  tvheadend::Subscription m_subscription;
  std::atomic<time_t> m_lastUse;
  std::atomic<time_t> m_startTime;
  uint32_t m_rdsIdx;
  int32_t m_requestedSpeed = 1000;
  int32_t m_actualSpeed = 1000;
};

} // namespace tvheadend
