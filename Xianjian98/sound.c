//
// Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "palcommon.h"
#include "common.h"
#include "sound.h"
#include "rixplay.h"
#include "util.h"

static BOOL gSndOpened = FALSE;

BOOL       g_fNoSound = FALSE;
BOOL       g_fNoMusic = FALSE;

#ifdef __SYMBIAN32__
INT        g_iVolume  = SDL_MIX_MAXVOLUME * 0.1;
#endif

typedef struct tagSNDPLAYER
{
   FILE                     *mkf;
   SDL_AudioSpec             spec;
   LPBYTE                    buf[2], pos[2];
   INT                       audio_len[2];
#ifdef PAL_HAS_CD
   SDL_CD                   *pCD;
#endif
} SNDPLAYER;

static SNDPLAYER gSndPlayer;

static SDL_AudioSpec *
SOUND_LoadVOCFromBuffer(
   LPCBYTE                lpVOC,
   SDL_AudioSpec         *lpSpec,
   LPBYTE                *lppBuffer
)
/*++
  Purpose:

    Load a VOC file in a buffer. Currently supports type 01 block only.

  Parameters:

    [IN]  lpVOC - pointer to the buffer of the VOC file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the VOC file.

    [OUT] lppBuffer - the output buffer.

  Return value:

    Pointer to the SDL_AudioSpec structure, NULL if failed.

--*/
{
   INT freq, len, x, i, l;

   //
   // Skip header
   //
   lpVOC += 0x1B;

   //
   // Length is 3 bytes long
   //
   len = (lpVOC[0] | (lpVOC[1] << 8) | (lpVOC[2] << 16)) - 2;
   lpVOC += 3;

   //
   // One byte for frequency
   //
   freq = 1000000 / (256 - *lpVOC);

#if 1

   lpVOC += 2;

   //
   // Convert the sample manually, as SDL doesn't like "strange" sample rates.
   //
   x = (INT)(len * ((FLOAT)PAL_SAMPLE_RATE / freq));

   *lppBuffer = (LPBYTE)calloc(1, x);
   if (*lppBuffer == NULL)
   {
      return NULL;
   }
   for (i = 0; i < x; i++)
   {
      l = (INT)(i * (freq / (FLOAT)PAL_SAMPLE_RATE));
      if (l >= len)
      {
         l = len - 1;
      }
      (*lppBuffer)[i] = lpVOC[l];
   }

   lpSpec->channels = 1;
   lpSpec->format = AUDIO_U8;
   lpSpec->freq = PAL_SAMPLE_RATE;
   lpSpec->size = x;

#else

   *lppBuffer = (unsigned char *)malloc(len);
   if (*lppBuffer == NULL)
   {
      return NULL;
   }

   lpSpec->channels = 1;
   lpSpec->format = AUDIO_U8;
   lpSpec->freq = freq;
   lpSpec->size = len;

   lpVOC += 2;
   memcpy(*lppBuffer, lpVOC, len);

#endif

   return lpSpec;
}

static VOID SDLCALL
SOUND_FillAudio(
   LPVOID          udata,
   LPBYTE          stream,
   INT             len
)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
   int        i;

   //
   // Play music
   //
   if (!g_fNoMusic)
   {
      RIX_FillBuffer(stream, len);
   }

   //
   // No current playing sound
   //
   if (g_fNoSound)
   {
      return;
   }

   for (i = 0; i < 2; i++)
   {
      //
      // Only play if we have data left
      //
      if (gSndPlayer.buf[i] == NULL)
      {
         continue;
      }

      if (gSndPlayer.audio_len[i] == 0)
      {
         //
         // Delete the audio buffer from memory
         //
         free(gSndPlayer.buf[i]);
         gSndPlayer.buf[i] = NULL;
         continue;
      }

      //
      // Mix as much data as possible
      //
      len = (len > gSndPlayer.audio_len[i]) ? gSndPlayer.audio_len[i] : len;
#ifdef __SYMBIAN32__
      SDL_MixAudio(stream, gSndPlayer.pos[i], len, g_iVolume);
#else
      SDL_MixAudio(stream, gSndPlayer.pos[i], len, SDL_MIX_MAXVOLUME);
#endif
      gSndPlayer.pos[i] += len;
      gSndPlayer.audio_len[i] -= len;
   }
}

INT
SOUND_OpenAudio(
   VOID
)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
   SDL_AudioSpec spec;

   if (gSndOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gSndOpened = FALSE;

   //
   // Load the MKF file.
   //
   gSndPlayer.mkf = fopen(va("%s%s", PAL_PREFIX, "VOC.MKF"), "rb");
   if (gSndPlayer.mkf == NULL)
   {
      return -2;
   }

   //
   // Open the sound subsystem.
   //
   gSndPlayer.spec.freq = PAL_SAMPLE_RATE;
   gSndPlayer.spec.format = AUDIO_S16;
   gSndPlayer.spec.channels = 1;
   gSndPlayer.spec.samples = 1024;
   gSndPlayer.spec.callback = SOUND_FillAudio;

   if (SDL_OpenAudio(&gSndPlayer.spec, &spec) < 0)
   {
      //
      // Failed
      //
      return -3;
   }

   memcpy(&gSndPlayer.spec, &spec, sizeof(SDL_AudioSpec));

   gSndPlayer.buf[0] = NULL;
   gSndPlayer.pos[0] = NULL;
   gSndPlayer.audio_len[0] = 0;

   gSndPlayer.buf[1] = NULL;
   gSndPlayer.pos[1] = NULL;
   gSndPlayer.audio_len[1] = 0;

   gSndOpened = TRUE;

   //
   // Initialize the music subsystem.
   //
   RIX_Init(va("%s%s", PAL_PREFIX, "mus.mkf"));

#ifdef PAL_HAS_CD
   //
   // Initialize the CD audio.
   //
   {
      int i;
      gSndPlayer.pCD = NULL;

      for (i = 0; i < SDL_CDNumDrives(); i++)
      {
         gSndPlayer.pCD = SDL_CDOpen(i);
         if (gSndPlayer.pCD != NULL)
         {
            if (!CD_INDRIVE(SDL_CDStatus(gSndPlayer.pCD)))
            {
               SDL_CDClose(gSndPlayer.pCD);
               gSndPlayer.pCD = NULL;
            }
            else
            {
               break;
            }
         }
      }
   }
#endif

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudio(0);

   return 0;
}

VOID
SOUND_CloseAudio(
   VOID
)
/*++
  Purpose:

    Close the audio subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_CloseAudio();

   if (gSndPlayer.buf[0] != NULL)
   {
      free(gSndPlayer.buf[0]);
      gSndPlayer.buf[0] = NULL;
   }

   if (gSndPlayer.buf[1] != NULL)
   {
      free(gSndPlayer.buf[1]);
      gSndPlayer.buf[1] = NULL;
   }

   if (gSndPlayer.mkf != NULL)
   {
      fclose(gSndPlayer.mkf);
      gSndPlayer.mkf = NULL;
   }

   RIX_Shutdown();

#ifdef PAL_HAS_CD
   if (gSndPlayer.pCD != NULL)
   {
      SOUND_PlayCDA(-1);
      SDL_CDClose(gSndPlayer.pCD);
   }
#endif
}

#ifdef __SYMBIAN32__

VOID
SOUND_AdjustVolume(
   INT    iDirectory
)
/*++
  Purpose:

    SDL sound volume adjust function.

  Parameters:

    [IN]  iDirectory - value, Increase (>0) or decrease (<=0) 3% volume.

  Return value:

    None.

--*/
{
   if (iDirectory > 0)
   {
      if (g_iVolume <= SDL_MIX_MAXVOLUME)
      {
         g_iVolume += SDL_MIX_MAXVOLUME * 0.03;
      }
      else
      {
         g_iVolume = SDL_MIX_MAXVOLUME;
      }
   }
   else
   {
      if (g_iVolume > 0)
      {
         g_iVolume -= SDL_MIX_MAXVOLUME * 0.03;
      }
      else
      {
         g_iVolume = 0;
      }
   }
}

#endif

VOID
SOUND_PlayChannel(
   INT    iSoundNum,
   INT    iChannel
)
/*++
  Purpose:

    Play a sound in voc.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound.

    [IN]  iChannel - the number of channel (0 or 1).

  Return value:

    None.

--*/
{
   SDL_AudioCVT    wavecvt;
   SDL_AudioSpec   wavespec;
   LPBYTE          buf, bufdec;
   UINT            samplesize;
   int             len;

   if (!gSndOpened || g_fNoSound)
   {
      return;
   }

   //
   // Stop playing current sound.
   //
   if (gSndPlayer.buf[iChannel] != NULL)
   {
      LPBYTE p = gSndPlayer.buf[iChannel];
      gSndPlayer.buf[iChannel] = NULL;
      free(p);
   }

   if (iSoundNum < 0)
   {
      return;
   }

   //
   // Get the length of the sound file.
   //
   len = PAL_MKFGetChunkSize(iSoundNum, gSndPlayer.mkf);
   if (len <= 0)
   {
      return;
   }

   buf = (LPBYTE)calloc(len, 1);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the sound file from the MKF archive.
   //
   PAL_MKFReadChunk(buf, len, iSoundNum, gSndPlayer.mkf);

   SOUND_LoadVOCFromBuffer(buf, &wavespec, &bufdec);
   free(buf);

   //
   // Build the audio converter and create conversion buffers
   //
   if (SDL_BuildAudioCVT(&wavecvt, wavespec.format, wavespec.channels, wavespec.freq,
      gSndPlayer.spec.format, gSndPlayer.spec.channels, gSndPlayer.spec.freq) < 0)
   {
      free(bufdec);
      return;
   }

   samplesize = ((wavespec.format & 0xFF) / 8) * wavespec.channels;
   wavecvt.len = wavespec.size & ~(samplesize - 1);
   wavecvt.buf = (LPBYTE)malloc(wavecvt.len * wavecvt.len_mult);
   if (wavecvt.buf == NULL)
   {
      free(bufdec);
      return;
   }
   memcpy(wavecvt.buf, bufdec, wavespec.size);
   free(bufdec);

   //
   // Run the audio converter
   //
   if (SDL_ConvertAudio(&wavecvt) < 0)
   {
      return;
   }

   gSndPlayer.buf[iChannel] = wavecvt.buf;
   gSndPlayer.audio_len[iChannel] = wavecvt.len * wavecvt.len_mult;
   gSndPlayer.pos[iChannel] = wavecvt.buf;
}

BOOL
SOUND_PlayCDA(
   INT    iNumTrack
)
/*++
  Purpose:

    Play a CD Audio Track.

  Parameters:

    [IN]  iNumTrack - number of the CD Audio Track.

  Return value:

    TRUE if the track can be played, FALSE if not.

--*/
{
#ifdef PAL_HAS_CD
   if (gSndPlayer.pCD != NULL)
   {
      if (CD_INDRIVE(SDL_CDStatus(gSndPlayer.pCD)))
      {
         SDL_CDStop(gSndPlayer.pCD);

         if (iNumTrack != -1)
         {
            RIX_Play(-1, FALSE, 0);

            if (SDL_CDPlayTracks(gSndPlayer.pCD, iNumTrack - 1, 0, 1, 0) == 0)
            {
               return TRUE;
            }
         }
      }
   }
#endif

   return FALSE;
}
