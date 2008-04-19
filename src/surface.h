/*
  pygame - Python Game Library
  Copyright (C) 2000-2001  Pete Shinners
  Copyright (C) 2007 Marcus von Appen
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.
  
  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  
  Pete Shinners
  pete@shinners.org
*/

#ifndef SURFACE_H
#define SURFACE_H

#include <SDL.h>
#include "pygame.h"

#define PYGAME_BLEND_ADD  0x1
#define PYGAME_BLEND_SUB  0x2
#define PYGAME_BLEND_MULT 0x3
#define PYGAME_BLEND_MIN  0x4
#define PYGAME_BLEND_MAX  0x5

#define GET_PIXEL(pxl, bpp, source)               \
    switch (bpp)                                  \
    {                                             \
    case 2:                                       \
        pxl = *((Uint16 *) (source));             \
        break;                                    \
    case 4:                                       \
        pxl = *((Uint32 *) (source));             \
        break;                                    \
    default:                                      \
    {                                             \
        Uint8 *b = (Uint8 *) source;              \
        pxl = (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? \
            b[0] + (b[1] << 8) + (b[2] << 16) :   \
            (b[0] << 16) + (b[1] << 8) + b[2];    \
    }                                             \
    break;                                        \
    }

#define GET_PIXELVALS(_sR, _sG, _sB, _sA, px, fmt)                    \
    _sR = ((px & fmt->Rmask) >> fmt->Rshift);                         \
    _sR = (_sR << fmt->Rloss) + (_sR >> (8 - (fmt->Rloss << 1)));     \
    _sG = ((px & fmt->Gmask) >> fmt->Gshift);                         \
    _sG = (_sG << fmt->Gloss) + (_sG >> (8 - (fmt->Gloss << 1)));     \
    _sB = ((px & fmt->Bmask) >> fmt->Bshift);                         \
    _sB = (_sB << fmt->Bloss) + (_sB >> (8 - (fmt->Bloss << 1)));     \
    if (fmt->Amask)                                                   \
    {                                                                 \
        _sA = ((px & fmt->Amask) >> fmt->Ashift);                     \
        _sA = (_sA << fmt->Aloss) + (_sA >> (8 - (fmt->Aloss << 1))); \
    }                                                                 \
    else                                                              \
    {                                                                 \
        _sA = 255;                                                    \
    }

#define GET_PIXELVALS_1(sr, sg, sb, sa, _src, _fmt)    \
    sr = _fmt->palette->colors[*((Uint8 *) (_src))].r; \
    sg = _fmt->palette->colors[*((Uint8 *) (_src))].g; \
    sb = _fmt->palette->colors[*((Uint8 *) (_src))].b; \
    sa = 255;

#define CREATE_PIXEL(buf, r, g, b, a, bp, ft)     \
    switch (bp)                                   \
    {                                             \
    case 2:                                       \
        *((Uint16 *) (buf)) =                     \
            ((r >> ft->Rloss) << ft->Rshift) |    \
            ((g >> ft->Gloss) << ft->Gshift) |    \
            ((b >> ft->Bloss) << ft->Bshift) |    \
            ((a << ft->Aloss) << ft->Ashift);     \
        break;                                    \
    case 4:                                       \
        *((Uint32 *) (buf)) =                     \
            ((r >> ft->Rloss) << ft->Rshift) |    \
            ((g >> ft->Gloss) << ft->Gshift) |    \
            ((b >> ft->Bloss) << ft->Bshift) |    \
            ((a << ft->Aloss) << ft->Ashift);     \
        break;                                    \
    }

/* Pretty good idea from Tom Duff :-). */
#define LOOP_UNROLLED4(code, n, width) \
    n = (width + 3) / 4;               \
    switch (width & 3)                 \
    {                                  \
    case 0: do { code;                 \
        case 3: code;                  \
        case 2: code;                  \
        case 1: code;                  \
        } while (--n > 0);             \
    }

/* Used in the srcbpp == dstbpp == 1 blend functions */
#define REPEAT_3(code) \
    code;              \
    code;              \
    code;

#define BLEND_ADD(tmp, sR, sG, sB, sA, dR, dG, dB, dA)  \
    tmp = dR + sR; dR = (tmp <= 255 ? tmp : 255);       \
    tmp = dG + sG; dG = (tmp <= 255 ? tmp : 255);       \
    tmp = dB + sB; dB = (tmp <= 255 ? tmp : 255);       \
    tmp = dA + sA; dA = (tmp <= 255 ? tmp : 255);

#define BLEND_SUB(tmp, sR, sG, sB, sA, dR, dG, dB, dA) \
    tmp = dR - sR; dR = (tmp >= 0 ? tmp : 0);          \
    tmp = dG - sG; dG = (tmp >= 0 ? tmp : 0);          \
    tmp = dB - sB; dB = (tmp >= 0 ? tmp : 0);          \
    tmp = dA - sA; dA = (tmp >= 0 ? tmp : 0);

#define BLEND_MULT(sR, sG, sB, sA, dR, dG, dB, dA) \
    dR = (dR && sR) ? (dR * sR) >> 8 : 0;          \
    dG = (dG && sG) ? (dG * sG) >> 8 : 0;          \
    dB = (dB && sB) ? (dB * sB) >> 8 : 0;          \
    dA = (dA && sA) ? (dA * sA) >> 8 : 0;

#define BLEND_MIN(sR, sG, sB, sA, dR, dG, dB, dA) \
    if(sR < dR) { dR = sR; }                      \
    if(sG < dG) { dG = sG; }                      \
    if(sB < dB) { dB = sB; }                      \
    if(sA < dA) { dA = sA; }

#define BLEND_MAX(sR, sG, sB, sA, dR, dG, dB, dA) \
    if(sR > dR) { dR = sR; }                      \
    if(sG > dG) { dG = sG; }                      \
    if(sB > dB) { dB = sB; }                      \
    if(sA > dA) { dA = sA; }

#if 1
#define ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB, dA) \
    do {                                            \
        if (dA)                                     \
        {                                           \
            dR = (((sR - dR) * sA) >> 8) + dR;      \
            dG = (((sG - dG) * sA) >> 8) + dG;      \
            dB = (((sB - dB) * sA) >> 8) + dB;      \
            dA = sA + dA - ((sA * dA) / 255);       \
        }                                           \
        else                                        \
        {                                           \
            dR = sR;                                \
            dG = sG;                                \
            dB = sB;                                \
            dA = sA;                                \
        }                                           \
    } while(0)
#elif 0
#define ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB, dA)    \
    do {                                               \
        if(sA){                                        \
            if(dA && sA < 255){                        \
                int dContrib = dA*(255 - sA)/255;      \
                dA = sA+dA - ((sA*dA)/255);            \
                dR = (dR*dContrib + sR*sA)/dA;         \
                dG = (dG*dContrib + sG*sA)/dA;         \
                dB = (dB*dContrib + sB*sA)/dA;         \
            }else{                                     \
                dR = sR;                               \
                dG = sG;                               \
                dB = sB;                               \
                dA = sA;                               \
            }                                          \
        }                                              \
    } while(0)
#endif

int
surface_fill_blend (SDL_Surface *surface, SDL_Rect *rect, Uint32 color,
                    int blendargs);

int 
pygame_AlphaBlit (SDL_Surface * src, SDL_Rect * srcrect,
                  SDL_Surface * dst, SDL_Rect * dstrect);

int 
pygame_Blit (SDL_Surface * src, SDL_Rect * srcrect,
             SDL_Surface * dst, SDL_Rect * dstrect, int the_args);

#endif /* SURFACE_H */
