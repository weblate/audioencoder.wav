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

#include "xbmc_audioenc_dll.h"
#include <stdio.h>
#include <string.h>

extern "C" {

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

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

// structure for holding our context
class wav_context
{
public:
  wav_context(audioenc_callbacks &cb) :
    callbacks(cb), audiosize(0)
  {
    memset(&wav, 0, sizeof(wav));
  };

  audioenc_callbacks callbacks;
  WAVHDR             wav;
  uint32_t           audiosize;
};

void *Create(audioenc_callbacks *callbacks)
{
  if (callbacks && callbacks->write && callbacks->seek)
  {
    return new wav_context(*callbacks);
  }
  return NULL;
}

bool Start(void* ctx, int iInChannels, int iInRate, int iInBits,
           const char* title, const char* artist,
           const char* albumartist, const char* album,
           const char* year, const char* track, const char* genre,
           const char* comment, int iTrackLength)
{
  wav_context *context = (wav_context*)ctx;
  if (!context)
    return false;

  // we accept only 2ch / 16 bit atm
  if (iInChannels != 2 || iInBits != 16)
    return false;

  // setup and write out our wav header
  memcpy(context->wav.riff, "RIFF", 4);
  memcpy(context->wav.cWavFmt, "WAVEfmt ", 8);
  context->wav.dwHdrLen = 16;
  context->wav.wFormat = WAVE_FORMAT_PCM;
  context->wav.wBlockAlign = 4;
  memcpy(context->wav.cData, "data", 4);
  context->wav.wNumChannels = iInChannels;
  context->wav.dwSampleRate = iInRate;
  context->wav.wBitsPerSample = iInBits;
  context->wav.dwBytesPerSec = iInRate * iInChannels * (iInBits >> 3);

  context->callbacks.write(context->callbacks.opaque, (uint8_t*)&context->wav, sizeof(context->wav));

  return true;
}

int Encode(void* ctx, int nNumBytesRead, uint8_t* pbtStream)
{
  wav_context *context = (wav_context*)ctx;
  if (!context)
    return -1;

  // write the audio directly out to the file is all we need do here
  context->callbacks.write(context->callbacks.opaque, pbtStream, nNumBytesRead);
  context->audiosize += nNumBytesRead;

  return nNumBytesRead;
}

bool Finish(void* ctx)
{
  wav_context *context = (wav_context*)ctx;
  if (!context)
    return false;

  // seek back and fill in the wav header size
  context->wav.len = context->audiosize + sizeof(context->wav) - 8;
  context->wav.dwDataLen = context->audiosize;

  if (context->callbacks.seek(context->callbacks.opaque, 0, 0) == 0)
  {
    context->callbacks.write(context->callbacks.opaque, (uint8_t*)&context->wav, sizeof(context->wav));
    return true;
  }

  return false;
}

void Free(void *ctx)
{
  wav_context *context = (wav_context*)ctx;
  delete context;
}

}
