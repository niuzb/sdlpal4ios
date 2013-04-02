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

#include "main.h"

// Screen buffer
SDL_Surface              *gpScreen           = NULL;

// Backup screen buffer
SDL_Surface              *gpScreenBak        = NULL;

// The real screen surface
static SDL_Surface       *gpScreenReal       = NULL;

#if (defined (__SYMBIAN32__) && !defined (__S60_5X__)) || defined (PSP)
   static BOOL bScaleScreen = FALSE;
#else
   static BOOL bScaleScreen = TRUE;
#endif

// Initial screen size
 WORD               g_wInitialWidth    = 640;
 WORD               g_wInitialHeight   = 400;

// Shake times and level
static WORD               g_wShakeTime       = 0;
static WORD               g_wShakeLevel      = 0;

INT
VIDEO_Init(
   WORD             wScreenWidth,
   WORD             wScreenHeight,
   BOOL             fFullScreen
)
/*++
  Purpose:

    Initialze the video subsystem.

  Parameters:

    [IN]  wScreenWidth - width of the screen.

    [IN]  wScreenHeight - height of the screen.

    [IN]  fFullScreen - TRUE to use full screen mode, FALSE to use windowed mode.

  Return value:

    0 = success, -1 = fail to create the screen surface,
    -2 = fail to create screen buffer.

--*/
{
   g_wInitialWidth = wScreenWidth;
   g_wInitialHeight = wScreenHeight;

   //
   // Create the screen surface.
   //
#if defined (NDS)
   gpScreenReal = SDL_SetVideoMode(293, 196, 8, SDL_SWSURFACE | SDL_FULLSCREEN);
#elif defined (__SYMBIAN32__)
#ifdef __S60_5X__
   gpScreenReal = SDL_SetVideoMode(640, 360, 8,
      SDL_SWSURFACE | (fFullScreen ? SDL_FULLSCREEN : 0));
#else
   gpScreenReal = SDL_SetVideoMode(320, 240, 8,
      SDL_SWSURFACE | (fFullScreen ? SDL_FULLSCREEN : 0));
#endif
#elif defined (PSP)
   gpScreenReal = SDL_SetVideoMode(320, 240, 8, SDL_SWSURFACE | SDL_FULLSCREEN);
#else
   gpScreenReal = SDL_SetVideoMode(wScreenWidth, wScreenHeight, 8,
      SDL_SWSURFACE | (fFullScreen ? SDL_FULLSCREEN : 0));
#endif

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to 640x480 software mode.
      //
      gpScreenReal = SDL_SetVideoMode(640, 480, 8,
         SDL_SWSURFACE | (fFullScreen ? SDL_FULLSCREEN : 0));
   }

   //
   // Still fail?
   //
   if (gpScreenReal == NULL)
   {
      return -1;
   }

   //
   // Create the screen buffer and the backup screen buffer.
   //
   gpScreen = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);

   gpScreenBak = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);
    SDL_FillRect(gpScreenReal, 0, SDL_MapRGB(gpScreenReal->format, 0, 0, 0));
   //
   // Failed?
   //
   if (gpScreen == NULL || gpScreenBak == NULL)
   {
      if (gpScreen != NULL)
      {
         SDL_FreeSurface(gpScreen);
      }

      if (gpScreenBak != NULL)
      {
         SDL_FreeSurface(gpScreenBak);
      }

      SDL_FreeSurface(gpScreenReal);
      return -2;
   }

   if (fFullScreen)
   {
      SDL_ShowCursor(FALSE);
   }

   return 0;
}

VOID
VIDEO_Shutdown(
   VOID
)
/*++
  Purpose:

    Shutdown the video subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpScreen != NULL)
   {
      SDL_FreeSurface(gpScreen);
   }
   gpScreen = NULL;

   if (gpScreenBak != NULL)
   {
      SDL_FreeSurface(gpScreenBak);
   }
   gpScreenBak = NULL;

   if (gpScreenReal != NULL)
   {
      SDL_FreeSurface(gpScreenReal);
   }
   gpScreenReal = NULL;
}

VOID
VIDEO_UpdateScreen(
   const SDL_Rect  *lpRect
)
/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/
{
   SDL_Rect        srcrect, dstrect;
   short           offset = 240 - 200;
   short           screenRealHeight = gpScreenReal->h;
   short           screenRealY = 0;

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   if (lpRect != NULL)
   {
      dstrect.x = (SHORT)((INT)(lpRect->x) * gpScreenReal->w / gpScreen->w);
      dstrect.y = (SHORT)((INT)(screenRealY + lpRect->y) * screenRealHeight / gpScreen->h);
      dstrect.w = (WORD)((DWORD)(lpRect->w) * gpScreenReal->w / gpScreen->w);
      dstrect.h = (WORD)((DWORD)(lpRect->h) * screenRealHeight / gpScreen->h);

      SDL_SoftStretch(gpScreen, (SDL_Rect *)lpRect, gpScreenReal, &dstrect);

      if (SDL_MUSTLOCK(gpScreenReal))
      {
         SDL_UnlockSurface(gpScreenReal);
      }

      SDL_UpdateRect(gpScreenReal, dstrect.x, dstrect.y, dstrect.w, dstrect.h);
   }
   else if (g_wShakeTime != 0)
   {
      //
      // Shake the screen
      //
      srcrect.x = 0;
      srcrect.y = 0;
      srcrect.w = 320;
      srcrect.h = 200 - g_wShakeLevel;

      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
      dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

      if (g_wShakeTime & 1)
      {
         srcrect.y = g_wShakeLevel;
      }
      else
      {
         dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }

      SDL_SoftStretch(gpScreen, &srcrect, gpScreenReal, &dstrect);

      if (g_wShakeTime & 1)
      {
         dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }
      else
      {
         dstrect.y = screenRealY;
      }

      dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

      SDL_FillRect(gpScreenReal, &dstrect, 0);

      if (SDL_MUSTLOCK(gpScreenReal))
      {
         SDL_UnlockSurface(gpScreenReal);
      }

      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);

      g_wShakeTime--;
   }
   else
   {
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

      SDL_SoftStretch(gpScreen, NULL, gpScreenReal, &dstrect);

      if (SDL_MUSTLOCK(gpScreenReal))
      {
         SDL_UnlockSurface(gpScreenReal);
      }

      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
   }
}

VOID
VIDEO_SetPalette(
   SDL_Color        rgPalette[256]
)
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{
   SDL_SetPalette(gpScreenReal, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
#if (defined (__SYMBIAN32__))
   {
      static UINT32 time = 0;
      if (SDL_GetTicks() - time > 50)
      {
	      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
	      time = SDL_GetTicks();
      }
   }
#endif
}

VOID
VIDEO_Resize(
   INT             w,
   INT             h
)
/*++
  Purpose:

    This function is called when user resized the window.

  Parameters:

    [IN]  w - width of the window after resizing.

    [IN]  h - height of the window after resizing.

  Return value:

    None.

--*/
{
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i;

   //
   // Get the original palette.
   //
   for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
   {
      palette[i] = gpScreenReal->format->palette->colors[i];
   }

   //
   // Create the screen surface.
   //
   flags = gpScreenReal->flags;

   SDL_FreeSurface(gpScreenReal);
   gpScreenReal = SDL_SetVideoMode(w, h, 8, flags);

   if (gpScreenReal == NULL)
   {
#ifdef __SYMBIAN32__
#ifdef __S60_5X__
      gpScreenReal = SDL_SetVideoMode(640, 360, 8, SDL_SWSURFACE);
#else
      gpScreenReal = SDL_SetVideoMode(320, 240, 8, SDL_SWSURFACE);
#endif
#else
      //
      // Fall back to 640x480 software windowed mode.
      //
      gpScreenReal = SDL_SetVideoMode(640, 480, 8, SDL_SWSURFACE);
#endif
   }

   SDL_SetPalette(gpScreenReal, SDL_PHYSPAL | SDL_LOGPAL, palette, 0, i);
   VIDEO_UpdateScreen(NULL);
}

SDL_Color *
VIDEO_GetPalette(
   VOID
)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
   return gpScreenReal->format->palette->colors;
}

VOID
VIDEO_ToggleScaleScreen(
   VOID
)
/*++
  Purpose:

    Toggle scalescreen mode.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#ifdef __SYMBIAN32__
   bScaleScreen = !bScaleScreen;
   VIDEO_Resize(320, 240);
   VIDEO_UpdateScreen(NULL);
#endif
}

VOID
VIDEO_ToggleFullscreen(
   VOID
)
/*++
  Purpose:

    Toggle fullscreen mode.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i;

   //
   // Get the original palette.
   //
   for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
   {
      palette[i] = gpScreenReal->format->palette->colors[i];
   }

   //
   // Get the flags of the original screen surface
   //
   flags = gpScreenReal->flags;

   if (flags & SDL_FULLSCREEN)
   {
      //
      // Already in fullscreen mode. Remove the fullscreen flag.
      //
      flags &= ~SDL_FULLSCREEN;
      flags |= SDL_RESIZABLE;
      SDL_ShowCursor(TRUE);
   }
   else
   {
      //
      // Not in fullscreen mode. Set the fullscreen flag.
      //
      flags |= SDL_FULLSCREEN;
      SDL_ShowCursor(FALSE);
   }

   //
   // Free the original screen surface
   //
   SDL_FreeSurface(gpScreenReal);

   //
   // ... and create a new one
   //
   if (g_wInitialWidth == 640 && g_wInitialHeight == 400 && (flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 480, 8, flags);
   }
   else if (g_wInitialWidth == 640 && g_wInitialHeight == 480 && !(flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 400, 8, flags);
   }
   else
   {
      gpScreenReal = SDL_SetVideoMode(g_wInitialWidth, g_wInitialHeight, 8, flags);
   }

   VIDEO_SetPalette(palette);

   //
   // Update the screen
   //
   VIDEO_UpdateScreen(NULL);
}

VOID
VIDEO_SaveScreenshot(
   VOID
)
/*++
  Purpose:

    Save the screenshot of current screen to a BMP file.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int      iNumBMP = 0;
   FILE    *fp;

   //
   // Find a usable BMP filename.
   //
   for (iNumBMP = 0; iNumBMP <= 9999; iNumBMP++)
   {
      fp = fopen(va("%sscrn%.4d.bmp", PAL_PREFIX, iNumBMP), "rb");
      if (fp == NULL)
      {
         break;
      }
      fclose(fp);
   }

   if (iNumBMP > 9999)
   {
      return;
   }

   //
   // Save the screenshot.
   //
   SDL_SaveBMP(gpScreenReal, va("%sscrn%.4d.bmp", PAL_PREFIX, iNumBMP));
}

VOID
VIDEO_BackupScreen(
   VOID
)
/*++
  Purpose:

    Backup the screen buffer.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_BlitSurface(gpScreen, NULL, gpScreenBak, NULL);
}

VOID
VIDEO_RestoreScreen(
   VOID
)
/*++
  Purpose:

    Restore the screen buffer which has been saved with VIDEO_BackupScreen().

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_BlitSurface(gpScreenBak, NULL, gpScreen, NULL);
}

VOID
VIDEO_ShakeScreen(
   WORD           wShakeTime,
   WORD           wShakeLevel
)
/*++
  Purpose:

    Set the screen shake time and level.

  Parameters:

    [IN]  wShakeTime - how many times should we shake the screen.

    [IN]  wShakeLevel - level of shaking.

  Return value:

    None.

--*/
{
   g_wShakeTime = wShakeTime;
   g_wShakeLevel = wShakeLevel;
}

VOID
VIDEO_SwitchScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;

   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 6; i++)
   {
      for (j = rgIndex[i]; j < gpScreen->pitch * gpScreen->h; j += 6)
      {
         ((LPBYTE)(gpScreenBak->pixels))[j] = ((LPBYTE)(gpScreen->pixels))[j];
      }

      //
      // Draw the backup buffer to the screen
      //
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

      SDL_SoftStretch(gpScreenBak, NULL, gpScreenReal, &dstrect);
      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);

      UTIL_Delay(wSpeed);
   }
}

VOID
VIDEO_FadeScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Fade from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j, k;
   DWORD             time;
   BYTE              a, b;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;
   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   time = SDL_GetTicks();

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         PAL_ProcessEvent();
         while (SDL_GetTicks() <= time)
         {
            PAL_ProcessEvent();
            SDL_Delay(5);
         }
         time = SDL_GetTicks() + wSpeed;

         //
         // Blend the pixels in the 2 buffers, and put the result into the
         // backup buffer
         //
         for (k = rgIndex[j]; k < gpScreen->pitch * gpScreen->h; k += 6)
         {
            a = ((LPBYTE)(gpScreen->pixels))[k];
            b = ((LPBYTE)(gpScreenBak->pixels))[k];

            if (i > 0)
            {
               if ((a & 0x0F) > (b & 0x0F))
               {
                  b++;
               }
               else if ((a & 0x0F) < (b & 0x0F))
               {
                  b--;
               }
            }

            ((LPBYTE)(gpScreenBak->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
         }

         //
         // Draw the backup buffer to the screen
         //
         if (g_wShakeTime != 0)
         {
            //
            // Shake the screen
            //
            SDL_Rect srcrect, dstrect;

            srcrect.x = 0;
            srcrect.y = 0;
            srcrect.w = 320;
            srcrect.h = 200 - g_wShakeLevel;

            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
            dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

            if (g_wShakeTime & 1)
            {
               srcrect.y = g_wShakeLevel;
            }
            else
            {
               dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }

            SDL_SoftStretch(gpScreenBak, &srcrect, gpScreenReal, &dstrect);

            if (g_wShakeTime & 1)
            {
               dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }
            else
            {
               dstrect.y = screenRealY;
            }

            dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

            SDL_FillRect(gpScreenReal, &dstrect, 0);
            SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
            g_wShakeTime--;
         }
         else
         {
            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = gpScreenReal->w;
            dstrect.h = screenRealHeight;

            SDL_SoftStretch(gpScreenBak, NULL, gpScreenReal, &dstrect);
            SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
         }
      }
   }

   if (SDL_MUSTLOCK(gpScreenReal))
   {
      SDL_UnlockSurface(gpScreenReal);
   }

   //
   // Draw the result buffer to the screen as the final step
   //
   VIDEO_UpdateScreen(NULL);
}
