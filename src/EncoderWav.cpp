/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <kodi/addon-instance/AudioEncoder.h>
#include <stdio.h>
#include <string.h>

#define WAVE_FORMAT_PCM 0x0001

// structure for WAV
typedef struct
{
  uint8_t  riff[4];         /* must be "RIFF"    */
  uint32_t len;             /* #bytes + 44 - 8   */
  uint8_t  cWavFmt[8];      /* must be "WAVEfmt " */
  uint32_t dwHdrLen;
  uint16_t wFormat;
  uint16_t wNumChannels;
  uint32_t dwSampleRate;
  uint32_t dwBytesPerSec;
  uint16_t wBlockAlign;
  uint16_t wBitsPerSample;
  uint8_t  cData[4];        /* must be "data"   */
  uint32_t dwDataLen;       /* #bytes           */
}
WAVHDR;

class CEncoderWav : public kodi::addon::CInstanceAudioEncoder
{
public:
  CEncoderWav(KODI_HANDLE instance);

  virtual bool Start(int inChannels,
                     int inRate,
                     int inBits,
                     const std::string& title,
                     const std::string& artist,
                     const std::string& albumartist,
                     const std::string& album,
                     const std::string& year,
                     const std::string& track,
                     const std::string& genre,
                     const std::string& comment,
                     int trackLength) override;
  int Encode(int numBytesRead, const uint8_t* stream) override;
  virtual bool Finish() override;

private:
  WAVHDR m_wav;
  uint32_t m_audiosize;
};

CEncoderWav::CEncoderWav(KODI_HANDLE instance)
  : CInstanceAudioEncoder(instance)
{
  memset(&m_wav, 0, sizeof(m_wav));
}

bool CEncoderWav::Start(int inChannels, int inRate, int inBits,
                        const std::string& title, const std::string& artist,
                        const std::string& albumartist, const std::string& album,
                        const std::string& year, const std::string& track, const std::string& genre,
                        const std::string& comment, int trackLength)
{
  // we accept only 2ch / 16 bit atm
  if (inChannels != 2 || inBits != 16)
    return false;

  // setup and write out our wav header
  memcpy(m_wav.riff, "RIFF", 4);
  memcpy(m_wav.cWavFmt, "WAVEfmt ", 8);
  m_wav.dwHdrLen = 16;
  m_wav.wFormat = WAVE_FORMAT_PCM;
  m_wav.wBlockAlign = 4;
  memcpy(m_wav.cData, "data", 4);
  m_wav.wNumChannels = inChannels;
  m_wav.dwSampleRate = inRate;
  m_wav.wBitsPerSample = inBits;
  m_wav.dwBytesPerSec = inRate * inChannels * (inBits >> 3);

  Write((uint8_t*)&m_wav, sizeof(m_wav));
  return true;
}

int CEncoderWav::Encode(int numBytesRead, const uint8_t* stream)
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

  if (Seek(0, 0) == 0)
  {
    Write((uint8_t*)&m_wav, sizeof(m_wav));
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

class CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override;
};

ADDON_STATUS CMyAddon::CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
{
  addonInstance = new CEncoderWav(instance);
  return ADDON_STATUS_OK;
}

ADDONCREATOR(CMyAddon)
