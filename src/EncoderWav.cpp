/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include <kodi/addon-instance/AudioEncoder.h>
#include <stdio.h>
#include <string.h>

#define WAVE_FORMAT_PCM 0x0001

// structure for WAV
typedef struct
{
  uint8_t riff[4]; /* must be "RIFF"    */
  uint32_t len; /* #bytes + 44 - 8   */
  uint8_t cWavFmt[8]; /* must be "WAVEfmt " */
  uint32_t dwHdrLen;
  uint16_t wFormat;
  uint16_t wNumChannels;
  uint32_t dwSampleRate;
  uint32_t dwBytesPerSec;
  uint16_t wBlockAlign;
  uint16_t wBitsPerSample;
  uint8_t cData[4]; /* must be "data"   */
  uint32_t dwDataLen; /* #bytes           */
} WAVHDR;

class ATTR_DLL_LOCAL CEncoderWav : public kodi::addon::CInstanceAudioEncoder
{
public:
  CEncoderWav(const kodi::addon::IInstanceInfo& instance);

  bool Start(const kodi::addon::AudioEncoderInfoTag& tag) override;
  ssize_t Encode(const uint8_t* stream, size_t numBytesRead) override;
  bool Finish() override;

private:
  WAVHDR m_wav;
  size_t m_audiosize;
};

CEncoderWav::CEncoderWav(const kodi::addon::IInstanceInfo& instance)
  : CInstanceAudioEncoder(instance)
{
  memset(&m_wav, 0, sizeof(m_wav));
}

bool CEncoderWav::Start(const kodi::addon::AudioEncoderInfoTag& tag)
{
  // we accept only 2ch / 16 bit atm
  if (tag.GetChannels() != 2 || tag.GetBitsPerSample() != 16)
    return false;

  // setup and write out our wav header
  memcpy(m_wav.riff, "RIFF", 4);
  memcpy(m_wav.cWavFmt, "WAVEfmt ", 8);
  m_wav.dwHdrLen = 16;
  m_wav.wFormat = WAVE_FORMAT_PCM;
  m_wav.wBlockAlign = 4;
  memcpy(m_wav.cData, "data", 4);
  m_wav.wNumChannels = tag.GetChannels();
  m_wav.dwSampleRate = tag.GetSamplerate();
  m_wav.wBitsPerSample = tag.GetBitsPerSample();
  m_wav.dwBytesPerSec = tag.GetSamplerate() * tag.GetChannels() * (tag.GetBitsPerSample() >> 3);

  Write((uint8_t*)&m_wav, sizeof(m_wav));
  return true;
}

ssize_t CEncoderWav::Encode(const uint8_t* stream, size_t numBytesRead)
{
  // write the audio directly out to the file is all we need do here
  Write(stream, numBytesRead);
  m_audiosize += numBytesRead;
  return numBytesRead;
}

bool CEncoderWav::Finish()
{
  // seek back and fill in the wav header size
  m_wav.len = m_audiosize + sizeof(m_wav) - 8;
  m_wav.dwDataLen = m_audiosize;

  if (Seek(0, SEEK_SET) == 0)
  {
    Write((uint8_t*)&m_wav, sizeof(m_wav));
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override;
};

ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
                                      KODI_ADDON_INSTANCE_HDL& hdl)
{
  hdl = new CEncoderWav(instance);
  return ADDON_STATUS_OK;
}

ADDONCREATOR(CMyAddon)
