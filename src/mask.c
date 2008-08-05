/*
  Copyright (C) 2002-2007 Ulf Ekstrom except for the bitcount function.
  This wrapper code was originally written by Danny van Bruggen(?) for
  the SCAM library, it was then converted by Ulf Ekstrom to wrap the
  bitmask library, a spinoff from SCAM.

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

*/

#include "pygame.h"
#include "pygamedocs.h"
#include "structmember.h"
#include "bitmask.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  PyObject_HEAD
  bitmask_t *mask;
} PyMaskObject;

staticforward PyTypeObject PyMask_Type;
#define PyMask_Check(x) ((x)->ob_type == &PyMask_Type)
#define PyMask_AsBitmap(x) (((PyMaskObject*)x)->mask)


/* mask object methods */

static PyObject* mask_get_size(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);

    if(!PyArg_ParseTuple(args, ""))
        return NULL;

    return Py_BuildValue("(ii)", mask->w, mask->h);
}

static PyObject* mask_get_at(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    int x, y, val;

    if(!PyArg_ParseTuple(args, "(ii)", &x, &y))
            return NULL;
    if (x >= 0 && x < mask->w && y >= 0 && y < mask->h) {
        val = bitmask_getbit(mask, x, y);
    } else {
        PyErr_Format(PyExc_IndexError, "%d, %d is out of bounds", x, y);
        return NULL;
    }

    return PyInt_FromLong(val);
}

static PyObject* mask_set_at(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    int x, y, value = 1;

    if(!PyArg_ParseTuple(args, "(ii)|i", &x, &y, &value))
            return NULL;
    if (x >= 0 && x < mask->w && y >= 0 && y < mask->h) {
        if (value) {
            bitmask_setbit(mask, x, y);
        } else {
          bitmask_clearbit(mask, x, y);
        }
    } else {
        PyErr_Format(PyExc_IndexError, "%d, %d is out of bounds", x, y);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* mask_overlap(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_t *othermask;
    PyObject *maskobj;
    int x, y, val;
    int xp,yp;

    if(!PyArg_ParseTuple(args, "O!(ii)", &PyMask_Type, &maskobj, &x, &y))
            return NULL;
    othermask = PyMask_AsBitmap(maskobj);

    val = bitmask_overlap_pos(mask, othermask, x, y, &xp, &yp);
    if (val) {
      return Py_BuildValue("(ii)", xp,yp);
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}


static PyObject* mask_overlap_area(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_t *othermask;
    PyObject *maskobj;
    int x, y, val;

    if(!PyArg_ParseTuple(args, "O!(ii)", &PyMask_Type, &maskobj, &x, &y)) {
        return NULL;
    }
    othermask = PyMask_AsBitmap(maskobj);

    val = bitmask_overlap_area(mask, othermask, x, y);
    return PyInt_FromLong(val);
}

static PyObject* mask_overlap_mask(PyObject* self, PyObject* args)
{
    int x, y;
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_t *output = bitmask_create(mask->w, mask->h);
    bitmask_t *othermask;
    PyObject *maskobj;
    PyMaskObject *maskobj2 = PyObject_New(PyMaskObject, &PyMask_Type);

    if(!PyArg_ParseTuple(args, "O!(ii)", &PyMask_Type, &maskobj, &x, &y)) {
        return NULL;
    }
    othermask = PyMask_AsBitmap(maskobj);
    
    bitmask_overlap_mask(mask, othermask, output, x, y);
    
    if(maskobj2)
        maskobj2->mask = output;

    return (PyObject*)maskobj2;
}

static PyObject* mask_fill(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    
    bitmask_fill(mask);
        
    Py_RETURN_NONE;
}

static PyObject* mask_clear(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    
    bitmask_clear(mask);
        
    Py_RETURN_NONE;
}

static PyObject* mask_invert(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    
    bitmask_invert(mask);
        
    Py_RETURN_NONE;
}

static PyObject* mask_scale(PyObject* self, PyObject* args)
{
    int x, y;
    bitmask_t *input = PyMask_AsBitmap(self);
    bitmask_t *output;
    PyMaskObject *maskobj = PyObject_New(PyMaskObject, &PyMask_Type);
    
    if(!PyArg_ParseTuple(args, "(ii)", &x, &y)) {
        return NULL;
    }

    output = bitmask_scale(input, x, y);
    
    if(maskobj)
        maskobj->mask = output;

    return (PyObject*)maskobj;
}

static PyObject* mask_draw(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_t *othermask;
    PyObject *maskobj;
    int x, y;

    if(!PyArg_ParseTuple(args, "O!(ii)", &PyMask_Type, &maskobj, &x, &y)) {
        return NULL;
    }
    othermask = PyMask_AsBitmap(maskobj);

    bitmask_draw(mask, othermask, x, y);
    
    Py_RETURN_NONE;
}

static PyObject* mask_erase(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_t *othermask;
    PyObject *maskobj;
    int x, y;

    if(!PyArg_ParseTuple(args, "O!(ii)", &PyMask_Type, &maskobj, &x, &y)) {
        return NULL;
    }
    othermask = PyMask_AsBitmap(maskobj);

    bitmask_erase(mask, othermask, x, y);
    
    Py_RETURN_NONE;
}

static PyObject* mask_count(PyObject* self, PyObject* args)
{
    bitmask_t *m = PyMask_AsBitmap(self);
    
    return PyInt_FromLong(bitmask_count(m));
}

static PyObject* mask_centroid(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    int x, y;
    long int m10, m01, m00;

    m10 = m01 = m00 = 0;
    
    for (x = 0; x < mask->w; x++) {
        for (y = 0; y < mask->h; y++) {
            if (bitmask_getbit(mask, x, y)) {
               m10 += x;
               m01 += y;
               m00++;
            }
        }
    }
    
    if (m00) {
        return Py_BuildValue ("(OO)", PyInt_FromLong(m10/m00),
                              PyInt_FromLong(m01/m00));
    } else {
        return Py_BuildValue ("(OO)", PyInt_FromLong(0), PyInt_FromLong(0));
    }
}

static PyObject* mask_angle(PyObject* self, PyObject* args)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    int x, y, xc, yc;
    long int m10, m01, m00, m20, m02, m11;
    double theta;

    m10 = m01 = m00 = m20 = m02 = m11 = 0;
    
    for (x = 0; x < mask->w; x++) {
        for (y = 0; y < mask->h; y++) {
            if (bitmask_getbit(mask, x, y)) {
               m10 += x;
               m20 += x*x;
               m11 += x*y;
               m02 += y*y;
               m01 += y;
               m00++;
            }
        }
    }
    
    if (m00) {
        xc = m10/m00;
        yc = m01/m00;
        theta = -90.0*atan2(2*(m11/m00 - xc*yc),(m20/m00 - xc*xc)-(m02/m00 - yc*yc))/M_PI;
        return Py_BuildValue ("O", PyFloat_FromDouble(theta));
    } else {
        return Py_BuildValue ("O", PyFloat_FromDouble(0));
    }
}

static PyObject* mask_outline(PyObject* self, PyObject* args)
{
    bitmask_t* c = PyMask_AsBitmap(self);
    bitmask_t* m = bitmask_create(c->w + 2, c->h + 2); 
    PyObject* plist;
    int x, y, every, e, firstx, firsty, secx, secy, currx, curry, nextx, nexty, n;
    int a[14], b[14];
    a[0] = a[1] = a[7] = a[8] = a[9] = b[1] = b[2] = b[3] = b[9] = b[10] = b[11]= 1;
    a[2] = a[6] = a[10] = b[4] = b[0] = b[12] = b[8] = 0;
    a[3] = a[4] = a[5] = a[11] = a[12] = a[13] = b[5] = b[6] = b[7] = b[13] = -1;
    
    plist = NULL;
    plist = PyList_New (0);
    if (!plist)
        return NULL;
        
    every = 1;
    n = firstx = firsty = secx = x = 0;

    if(!PyArg_ParseTuple(args, "|i", &every)) {
        return NULL;
    }
    
    /* by copying to a new, larger mask, we avoid having to check if we are at
       a border pixel every time.  */
    bitmask_draw(m, c, 1, 1);
    
    e = every;
    
    /* find the first set pixel in the mask */
    for (y = 1; y < m->h-1; y++) {
        for (x = 1; x < m->w-1; x++) {
            if (bitmask_getbit(m, x, y)) {
                 firstx = x;
                 firsty = y;
                 PyList_Append(plist, Py_BuildValue("(ii)", x-1, y-1));
                 break;
            }
        }
        if (bitmask_getbit(m, x, y))
            break;
    }
    
    
    
    /* covers the mask having zero pixels or only the final pixel */
    if ((x == m->w-1) && (y == m->h-1)) {
        bitmask_free(m);
        return plist;
    }        
    
    /* check just the first pixel for neighbors */
    for (n = 0;n < 8;n++) {
        if (bitmask_getbit(m, x+a[n], y+b[n])) {
            currx = secx = x+a[n];
            curry = secy = y+b[n];
            e--;
            if (!e) {
                e = every;
                PyList_Append(plist, Py_BuildValue("(ii)", secx-1, secy-1));
            }
            break;
        }
    }       
    
    /* if there are no neighbors, return */
    if (!secx) {
        bitmask_free(m);
        return plist;
    }
    
    /* the outline tracing loop */
    for (;;) {
        /* look around the pixel, it has to have a neighbor */
        for (n = (n + 6) & 7;;n++) {
            if (bitmask_getbit(m, currx+a[n], curry+b[n])) {
                nextx = currx+a[n];
                nexty = curry+b[n];
                e--;
                if (!e) {
                    e = every;
                    PyList_Append(plist, Py_BuildValue("(ii)", nextx-1, nexty-1));
                }
                break;
            }
        }   
        /* if we are back at the first pixel, and the next one will be the
           second one we visited, we are done */
        if ((curry == firsty && currx == firstx) && (secx == nextx && secy == nexty)) {
            break;
        }

        curry = nexty;
        currx = nextx;
    }
    
    bitmask_free(m);
    
    return plist;
}

static PyObject* mask_from_surface(PyObject* self, PyObject* args)
{
    bitmask_t *mask;
    SDL_Surface* surf;

    PyObject* surfobj;
    PyMaskObject *maskobj;

    int x, y, threshold, ashift, usethresh;
    Uint8 *pixels;

    SDL_PixelFormat *format;
    Uint32 color, amask;
    Uint8 *pix;
    Uint8 a;

    /* set threshold as 127 default argument. */
    threshold = 127;

    /* get the surface from the passed in arguments. 
     *   surface, threshold
     */

    if (!PyArg_ParseTuple (args, "O!|i", &PySurface_Type, &surfobj, &threshold)) {
        return NULL;
    }

    surf = PySurface_AsSurface(surfobj);

    /* lock the surface, release the GIL. */
    PySurface_Lock (surfobj);

    Py_BEGIN_ALLOW_THREADS;

    /* get the size from the surface, and create the mask. */
    mask = bitmask_create(surf->w, surf->h);


    if(!mask) {
        /* Py_END_ALLOW_THREADS;
         */
        return NULL; /*RAISE(PyExc_Error, "cannot create bitmask");*/
    }

    pixels = (Uint8 *) surf->pixels;
    format = surf->format;
    amask = format->Amask;
    ashift = format->Ashift;
    usethresh = !(surf->flags & SDL_SRCCOLORKEY);

    for(y=0; y < surf->h; y++) {
        pixels = (Uint8 *) surf->pixels + y*surf->pitch;
        for(x=0; x < surf->w; x++) {
            /* Get the color.  TODO: should use an inline helper 
             *   function for this common function. */
            switch (format->BytesPerPixel)
            {
                case 1:
                    color = (Uint32)*((Uint8 *) pixels);
                    pixels++;
                    break;
                case 2:
                    color = (Uint32)*((Uint16 *) pixels);
                    pixels += 2;
                    break;
                case 3:
                    pix = ((Uint8 *) pixels);
                    pixels += 3;
                #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                    color = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
                #else
                    color = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
                #endif
                    break;
                default:                  /* case 4: */
                    color = *((Uint32 *) pixels);
                    pixels += 4;
                    break;
            }


            if (usethresh) {
                a = (color & amask) >> ashift;
                /* no colorkey, so we check the threshold of the alpha */
                if (a > threshold) {
                    bitmask_setbit(mask, x, y);
                }
            } else {
                /*  test against the colour key. */
                if (format->colorkey != color) {
                    bitmask_setbit(mask, x, y);
                }
            }
        }
    }

    Py_END_ALLOW_THREADS;

    /* unlock the surface, release the GIL.
     */
    PySurface_Unlock (surfobj);

    /*create the new python object from mask*/        
    maskobj = PyObject_New(PyMaskObject, &PyMask_Type);
    if(maskobj)
        maskobj->mask = mask;


    return (PyObject*)maskobj;
}

void bitmask_threshold (bitmask_t *m, SDL_Surface *surf, SDL_Surface *surf2, 
                        Uint32 color,  Uint32 threshold)
{
    int x, y, rshift, gshift, bshift, rshift2, gshift2, bshift2;
    int rloss, gloss, bloss, rloss2, gloss2, bloss2;
    Uint8 *pixels, *pixels2;
    SDL_PixelFormat *format, *format2;
    Uint32 the_color, the_color2, rmask, gmask, bmask, rmask2, gmask2, bmask2;
    Uint8 *pix;
    Uint8 r, g, b, a;
    Uint8 tr, tg, tb, ta;

    pixels = (Uint8 *) surf->pixels;
    format = surf->format;
    rmask = format->Rmask;
    gmask = format->Gmask;
    bmask = format->Bmask;
    rshift = format->Rshift;
    gshift = format->Gshift;
    bshift = format->Bshift;
    rloss = format->Rloss;
    gloss = format->Gloss;
    bloss = format->Bloss;

    if(surf2) {
        format2 = surf2->format;
        rmask2 = format2->Rmask;
        gmask2 = format2->Gmask;
        bmask2 = format2->Bmask;
        rshift2 = format2->Rshift;
        gshift2 = format2->Gshift;
        bshift2 = format2->Bshift;
        rloss2 = format2->Rloss;
        gloss2 = format2->Gloss;
        bloss2 = format2->Bloss;
        pixels2 = (Uint8 *) surf2->pixels;
    } else { /* make gcc stop complaining */
        rmask2 = gmask2 = bmask2 = 0;
        rshift2 = gshift2 = bshift2 = 0;
        rloss2 = gloss2 = bloss2 = 0;
        format2 = NULL;
        pixels2 = NULL;
    }

    SDL_GetRGBA (color, format, &r, &g, &b, &a);
    SDL_GetRGBA (threshold, format, &tr, &tg, &tb, &ta);

    for(y=0; y < surf->h; y++) {
        pixels = (Uint8 *) surf->pixels + y*surf->pitch;
        if (surf2) {
            pixels2 = (Uint8 *) surf2->pixels + y*surf2->pitch;
        }
        for(x=0; x < surf->w; x++) {
            /* the_color = surf->get_at(x,y) */
            switch (format->BytesPerPixel)
            {
            case 1:
                the_color = (Uint32)*((Uint8 *) pixels);
                pixels++;
                break;
            case 2:
                the_color = (Uint32)*((Uint16 *) pixels);
                pixels += 2;
                break;
            case 3:
                pix = ((Uint8 *) pixels);
                pixels += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                the_color = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                the_color = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                break;
            default:                  /* case 4: */
                the_color = *((Uint32 *) pixels);
                pixels += 4;
                break;
            }

            if (surf2) {
                switch (format2->BytesPerPixel) {
                    case 1:
                        the_color2 = (Uint32)*((Uint8 *) pixels2);
                        pixels2++;
                        break;
                    case 2:
                        the_color2 = (Uint32)*((Uint16 *) pixels2);
                        pixels2 += 2;
                        break;
                    case 3:
                        pix = ((Uint8 *) pixels2);
                        pixels2 += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                        the_color2 = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                        the_color2 = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                        break;
                    default:                  /* case 4: */
                        the_color2 = *((Uint32 *) pixels2);
                        pixels2 += 4;
                        break;
                }              
                if ((abs((((the_color2 & rmask2) >> rshift2) << rloss2) - (((the_color & rmask) >> rshift) << rloss)) < tr) & 
                    (abs((((the_color2 & gmask2) >> gshift2) << gloss2) - (((the_color & gmask) >> gshift) << gloss)) < tg) & 
                    (abs((((the_color2 & bmask2) >> bshift2) << bloss2) - (((the_color & bmask) >> bshift) << bloss)) < tb)) {
                    /* this pixel is within the threshold of othersurface. */
                    bitmask_setbit(m, x, y);
                }
            } else if ((abs((((the_color & rmask) >> rshift) << rloss) - r) < tr) & 
                       (abs((((the_color & gmask) >> gshift) << gloss) - g) < tg) & 
                       (abs((((the_color & bmask) >> bshift) << bloss) - b) < tb)) {
                /* this pixel is within the threshold of the color. */
                bitmask_setbit(m, x, y);
            }
        }
    }
}

static PyObject* mask_from_threshold(PyObject* self, PyObject* args)
{
    PyObject *surfobj, *surfobj2;
    PyMaskObject *maskobj;
    bitmask_t* m;
    SDL_Surface* surf, *surf2;
    int bpp;
    PyObject *rgba_obj_color, *rgba_obj_threshold;
    Uint8 rgba_color[4];
    Uint8 rgba_threshold[4];
    Uint32 color;
    Uint32 color_threshold;

    surf2 = surf = NULL;
    surfobj2 = NULL;
    rgba_obj_threshold = NULL;
    rgba_threshold[0] = 0; rgba_threshold[1] = 0; rgba_threshold[2] = 0; rgba_threshold[4] = 255;

    if (!PyArg_ParseTuple (args, "O!O|OO!", &PySurface_Type, &surfobj,
                           &rgba_obj_color,  &rgba_obj_threshold,
                           &PySurface_Type, &surfobj2))
        return NULL;

    surf = PySurface_AsSurface (surfobj);
    if(surfobj2) {
        surf2 = PySurface_AsSurface (surfobj2);
    }

    if (PyInt_Check (rgba_obj_color)) {
        color = (Uint32) PyInt_AsLong (rgba_obj_color);
    } else if (PyLong_Check (rgba_obj_color)) {
        color = (Uint32) PyLong_AsUnsignedLong (rgba_obj_color);
    } else if (RGBAFromColorObj (rgba_obj_color, rgba_color)) {
        color = SDL_MapRGBA (surf->format, rgba_color[0], rgba_color[1],
            rgba_color[2], rgba_color[3]);
    } else {
        return RAISE (PyExc_TypeError, "invalid color argument");
    }

    if(rgba_obj_threshold) {
        if (PyInt_Check (rgba_obj_threshold))
            color_threshold = (Uint32) PyInt_AsLong (rgba_obj_threshold);
        else if (PyLong_Check (rgba_obj_threshold))
            color_threshold = (Uint32) PyLong_AsUnsignedLong
                (rgba_obj_threshold);
        else if (RGBAFromColorObj (rgba_obj_threshold, rgba_threshold))
            color_threshold = SDL_MapRGBA (surf->format, rgba_threshold[0], rgba_threshold[1], rgba_threshold[2], rgba_threshold[3]);
        else
            return RAISE (PyExc_TypeError, "invalid threshold argument");
    } else {
        color_threshold = SDL_MapRGBA (surf->format, rgba_threshold[0], rgba_threshold[1], rgba_threshold[2], rgba_threshold[3]);
    }

    bpp = surf->format->BytesPerPixel;
    m = bitmask_create(surf->w, surf->h);
	
    PySurface_Lock(surfobj);
    if(surfobj2) {
        PySurface_Lock(surfobj2);
    }
    
    Py_BEGIN_ALLOW_THREADS;
    bitmask_threshold (m, surf, surf2, color,  color_threshold);
    Py_END_ALLOW_THREADS;

    PySurface_Unlock(surfobj);
    if(surfobj2) {
        PySurface_Unlock(surfobj2);
    }

    maskobj = PyObject_New(PyMaskObject, &PyMask_Type);
    if(maskobj)
        maskobj->mask = m;

    return (PyObject*)maskobj;
}

/* the initial labelling phase of the connected components algorithm */
unsigned int cc_label(bitmask_t *input, unsigned int* image, unsigned int* ufind, unsigned int* largest)
{
    unsigned int *buf;
    unsigned int x, y, w, h, root, aroot, croot, temp, label;
    
    label = 0;
    w = input->w;
    h = input->h;

    ufind[0] = 0;
    buf = image;

    /* special case for first pixel */
    if(bitmask_getbit(input, 0, 0)) { /* process for a new connected comp: */
        label++;              /* create a new label */
        *buf = label;         /* give the pixel the label */
        ufind[label] = label; /* put the label in the equivalence array */
        largest[label] = 1;   /* the label has 1 pixel associated with it */
    } else {
        *buf = 0;
    }
    buf++;
    /* special case for first row */
    for(x = 1; x < w; x++) {
        if (bitmask_getbit(input, x, 0)) {
            if (*(buf-1)) {                    /* d label */
                *buf = *(buf-1);
            } else {                           /* create label */
                label++;
                *buf = label;
                ufind[label] = label;
                largest[label] = 0;
            }
            largest[*buf]++;
        } else {
            *buf = 0;
        }
        buf++;
    }
    /* the rest of the image */
    for(y = 1; y < h; y++) {
        /* first pixel of the row */
        if (bitmask_getbit(input, 0, y)) {
            if (*(buf-w)) {                    /* b label */
                *buf = *(buf-w);
            } else if (*(buf-w+1)) {           /* c label */
                *buf = *(buf-w+1);
            } else {                           /* create label */
                label++;
                *buf = label;
                ufind[label] = label;
                largest[label] = 0;
            }
            largest[*buf]++;
        } else {
            *buf = 0;
        }
        buf++;
        /* middle pixels of the row */
        for(x = 1; x < (w-1); x++) {
            if (bitmask_getbit(input, x, y)) {
                if (*(buf-w)) {                /* b label */
                    *buf = *(buf-w);
                } else if (*(buf-w+1)) {       /* c branch of tree */
                    if (*(buf-w-1)) {                      /* union(c, a) */
                        croot = root = *(buf-w+1);
                        while (ufind[root] < root) {       /* find root */
                            root = ufind[root];
                        }
                        if (croot != *(buf-w-1)) {
                            temp = aroot = *(buf-w-1);
                            while (ufind[aroot] < aroot) { /* find root */
                                aroot = ufind[aroot];
                            }
                            if (root > aroot) {
                                root = aroot;
                            }
                            while (ufind[temp] > root) {   /* set root */
                                aroot = ufind[temp];
                                ufind[temp] = root;
                                temp = aroot;
                            }
                        }
                        while (ufind[croot] > root) {      /* set root */
                            temp = ufind[croot];
                            ufind[croot] = root;
                            croot = temp;
                        }
                        *buf = root;
                    } else if (*(buf-1)) {                 /* union(c, d) */
                        croot = root = *(buf-w+1);
                        while (ufind[root] < root) {       /* find root */
                            root = ufind[root];
                        }
                        if (croot != *(buf-1)) {
                            temp = aroot = *(buf-1);
                            while (ufind[aroot] < aroot) { /* find root */
                                aroot = ufind[aroot];
                            }
                            if (root > aroot) {
                                root = aroot;
                            }
                            while (ufind[temp] > root) {   /* set root */
                                aroot = ufind[temp];
                                ufind[temp] = root;
                                temp = aroot;
                            }
                        }
                        while (ufind[croot] > root) {      /* set root */
                            temp = ufind[croot];
                            ufind[croot] = root;
                            croot = temp;
                        }
                        *buf = root;
                    } else {                   /* c label */
                        *buf = *(buf-w+1);
                    }
                } else if (*(buf-w-1)) {       /* a label */
                    *buf = *(buf-w-1);
                } else if (*(buf-1)) {         /* d label */
                    *buf = *(buf-1);
                } else {                       /* create label */
                    label++;
                    *buf = label;
                    ufind[label] = label;
                    largest[label] = 0;
                }
                largest[*buf]++;
            } else {
                *buf = 0;
            }
            buf++;
        }
        /* last pixel of the row, if its not also the first pixel of the row */
        if (w > 1) {
            if (bitmask_getbit(input, x, y)) {
                if (*(buf-w)) {                /* b label */
                    *buf = *(buf-w);
                } else if (*(buf-w-1)) {       /* a label */
                    *buf = *(buf-w-1);
                } else if (*(buf-1)) {         /* d label */
                    *buf = *(buf-1);
                } else {                       /* create label */
                    label++;
                    *buf = label;
                    ufind[label] = label;
                    largest[label] = 0;
                }
                largest[*buf]++;
            } else {
                *buf = 0;
            }
            buf++;
        }
    }
    
    return label;
}

/* Connected component labeling based on the SAUF algorithm by Kesheng Wu,
   Ekow Otoo, and Kenji Suzuki.  The algorithm is best explained by their paper,
   "Two Strategies to Speed up Connected Component Labeling Algorithms", but in
   summary, it is a very efficient two pass method for 8-connected components.
   It uses a decision tree to minimize the number of neighbors that need to be
   checked.  It stores equivalence information in an array based union-find.
   This implementation also has a final step of finding bounding boxes. */
static GAME_Rect* get_bounding_rects(bitmask_t *input, int *num_bounding_boxes)
{
    unsigned int *image, *ufind, *largest, *buf;
    int x, y, w, h, temp, label, relabel;
    GAME_Rect* rects;
    
    label = 0;
    w = input->w;
    h = input->h;

    /* a temporary image to assign labels to each bit of the mask */
    image = (unsigned int *) malloc(sizeof(int)*w*h);
    /* allocate enough space for the maximum possible connected components */
    /* the union-find array. see wikipedia for info on union find */
    ufind = (unsigned int *) malloc(sizeof(int)*(w/2 + 1)*(h/2 + 1));
    largest = (unsigned int *) malloc(sizeof(int)*(w/2 + 1)*(h/2 + 1));
    
    /* do the initial labelling */
    label = cc_label(input, image, ufind, largest);

    relabel = 0;
    /* flatten and relabel the union-find equivalence array */
    for (x = 1; x <= label; x++) {
         if (ufind[x] < x) {             /* is it a union find root? */
             ufind[x] = ufind[ufind[x]]; /* relabel it to its root */
         } else {                 /* its a root */
             relabel++;                      
             ufind[x] = relabel;  /* assign the lowest label available */
         }
    }

    *num_bounding_boxes = relabel;
    /* the bounding rects, need enough space for the number of labels */
    rects = (GAME_Rect *) malloc(sizeof(GAME_Rect) * (relabel + 1));
    for (temp = 0; temp <= relabel; temp++) {
        rects[temp].h = 0;        /* so we know if its a new rect or not */
    }
    
    /* find the bounding rect of each connected component */
    buf = image;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (ufind[*buf]) {         /* if the pixel is part of a component */
                if (rects[ufind[*buf]].h) {   /* the component has a rect */
                    temp = rects[ufind[*buf]].x;
                    rects[ufind[*buf]].x = MIN(x, temp);
                    rects[ufind[*buf]].y = MIN(y, rects[ufind[*buf]].y);
                    rects[ufind[*buf]].w = MAX(rects[ufind[*buf]].w + temp, x + 1) - rects[ufind[*buf]].x;
                    rects[ufind[*buf]].h = MAX(rects[ufind[*buf]].h, y - rects[ufind[*buf]].y + 1);
                } else {                      /* otherwise, start the rect */
                    rects[ufind[*buf]].x = x;
                    rects[ufind[*buf]].y = y;
                    rects[ufind[*buf]].w = 1;
                    rects[ufind[*buf]].h = 1;
                }
            }
            buf++;
        }
    }
    
    free(image);
    free(ufind);

    return rects;
}

static PyObject* mask_get_bounding_rects(PyObject* self, PyObject* args)
{
    GAME_Rect *regions;
    GAME_Rect *aregion;
    int num_bounding_boxes, i;
    PyObject* ret;
    PyObject* rect;

    bitmask_t *mask = PyMask_AsBitmap(self);

    ret = NULL;
    num_bounding_boxes = 0;

    ret = PyList_New (0);
    if (!ret)
        return NULL;

    Py_BEGIN_ALLOW_THREADS;

    regions = get_bounding_rects(mask, &num_bounding_boxes);

    Py_END_ALLOW_THREADS;

    /* build a list of rects to return.  */
    for(i=1; i <= num_bounding_boxes; i++) {
        aregion = regions + i;
        rect = PyRect_New4 ( aregion->x, aregion->y, aregion->w, aregion->h );
        PyList_Append (ret, rect);
        Py_DECREF (rect);
    }

    free(regions);

    return ret;
}


/* Connected component labeling based on the SAUF algorithm by Kesheng Wu,
   Ekow Otoo, and Kenji Suzuki.  The algorithm is best explained by their paper,
   "Two Strategies to Speed up Connected Component Labeling Algorithms", but in
   summary, it is a very efficient two pass method for 8-connected components.
   It uses a decision tree to minimize the number of neighbors that need to be
   checked.  It stores equivalence information in an array based union-find.
   This implementation also tracks the number of pixels in each label, finding 
   the biggest one while flattening the union-find equivalence array.  It then 
   writes an output mask containing only the largest connected component. */
void largest_connected_comp(bitmask_t* input, bitmask_t* output, int ccx, int ccy)
{
    unsigned int *image, *ufind, *largest, *buf;
    unsigned int max, x, y, w, h, label;
    
    w = input->w;
    h = input->h;

    /* a temporary image to assign labels to each bit of the mask */
    image = (unsigned int *) malloc(sizeof(int)*w*h);
    /* allocate enough space for the maximum possible connected components */
    /* the union-find array. see wikipedia for info on union find */
    ufind = (unsigned int *) malloc(sizeof(int)*(w/2 + 1)*(h/2 + 1));
    /* an array to track the number of pixels associated with each label */
    largest = (unsigned int *) malloc(sizeof(int)*(w/2 + 1)*(h/2 + 1));
    
    /* do the initial labelling */
    label = cc_label(input, image, ufind, largest);

    max = 1;
    /* flatten the union-find equivalence array */
    for (x = 2; x <= label; x++) {
         if (ufind[x] != x) {                 /* is it a union find root? */
             largest[ufind[x]] += largest[x]; /* add its pixels to its root */
             ufind[x] = ufind[ufind[x]];      /* relabel it to its root */
         }
         if (largest[ufind[x]] > largest[max]) { /* is it the new biggest? */
             max = ufind[x];
         }
    }
    
    /* write out the final image */
    buf = image;
    if (ccx >= 0)
        max = ufind[*(buf+ccy*w+ccx)];
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (ufind[*buf] == max) {         /* if the label is the max one */
                bitmask_setbit(output, x, y); /* set the bit in the mask */
            }
            buf++;
        }
    }
    
    free(image);
    free(ufind);
    free(largest);
}

static PyObject* mask_connected_component(PyObject* self, PyObject* args)
{
    bitmask_t *input = PyMask_AsBitmap(self);
    bitmask_t *output = bitmask_create(input->w, input->h);
    PyMaskObject *maskobj = PyObject_New(PyMaskObject, &PyMask_Type);   
    int x, y;
    
    x = -1;

    if(!PyArg_ParseTuple(args, "|(ii)", &x, &y)) {
        return NULL;
    }    

    if (x == -1 || bitmask_getbit(input, x, y))
        largest_connected_comp(input, output, x, y);
    
    if(maskobj)
        maskobj->mask = output;

    return (PyObject*)maskobj;
}


static PyMethodDef maskobj_builtins[] =
{
    { "get_size", mask_get_size, METH_VARARGS, DOC_MASKGETSIZE},
    { "get_at", mask_get_at, METH_VARARGS, DOC_MASKGETAT },
    { "set_at", mask_set_at, METH_VARARGS, DOC_MASKSETAT },
    { "overlap", mask_overlap, METH_VARARGS, DOC_MASKOVERLAP },
    { "overlap_area", mask_overlap_area, METH_VARARGS, DOC_MASKOVERLAPAREA },
    { "overlap_mask", mask_overlap_mask, METH_VARARGS, DOC_MASKOVERLAPMASK },
    { "fill", mask_fill, METH_NOARGS, DOC_MASKFILL },
    { "clear", mask_clear, METH_NOARGS, DOC_MASKCLEAR },
    { "invert", mask_invert, METH_NOARGS, DOC_MASKINVERT },
    { "scale", mask_scale, METH_VARARGS, DOC_MASKSCALE },
    { "draw", mask_draw, METH_VARARGS, DOC_MASKDRAW },
    { "erase", mask_erase, METH_VARARGS, DOC_MASKERASE },
    { "count", mask_count, METH_NOARGS, DOC_MASKCOUNT },
    { "centroid", mask_centroid, METH_NOARGS, DOC_MASKCENTROID },
    { "angle", mask_angle, METH_NOARGS, DOC_MASKANGLE },
    { "outline", mask_outline, METH_VARARGS, DOC_MASKOUTLINE },
    { "connected_component", mask_connected_component, METH_VARARGS,
      DOC_MASKCONNECTEDCOMPONENT },
    { "get_bounding_rects", mask_get_bounding_rects, METH_NOARGS,
      DOC_MASKGETBOUNDINGRECTS },

    { NULL, NULL, 0, NULL }
};



/*mask object internals*/

static void mask_dealloc(PyObject* self)
{
    bitmask_t *mask = PyMask_AsBitmap(self);
    bitmask_free(mask);
    PyObject_DEL(self);
}


static PyObject* mask_getattr(PyObject* self, char* attrname)
{
    return Py_FindMethod(maskobj_builtins, self, attrname);
}


static PyTypeObject PyMask_Type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "pygame.mask.Mask",
    sizeof(PyMaskObject),
    0,
    mask_dealloc,
    0,
    mask_getattr,
    0,
    0,
    0,
    0,
    NULL,
    0, 
    (hashfunc)NULL,
    (ternaryfunc)NULL,
    (reprfunc)NULL,
    0L,0L,0L,0L,
    DOC_PYGAMEMASKMASK /* Documentation string */
};


/*mask module methods*/

static PyObject* Mask(PyObject* self, PyObject* args)
{
    bitmask_t *mask;
    int w,h;
    PyMaskObject *maskobj;
    if(!PyArg_ParseTuple(args, "(ii)", &w, &h))
        return NULL;
    mask = bitmask_create(w,h);

    if(!mask)
      return NULL; /*RAISE(PyExc_Error, "cannot create bitmask");*/
        
        /*create the new python object from mask*/        
    maskobj = PyObject_New(PyMaskObject, &PyMask_Type);
    if(maskobj)
        maskobj->mask = mask;
    return (PyObject*)maskobj;
}



static PyMethodDef mask_builtins[] =
{
    { "Mask", Mask, METH_VARARGS, DOC_PYGAMEMASKMASK },
    { "from_surface", mask_from_surface, METH_VARARGS,
      DOC_PYGAMEMASKFROMSURFACE},
    { "from_threshold", mask_from_threshold, METH_VARARGS,
      DOC_PYGAMEMASKFROMTHRESHOLD},
    { NULL, NULL, 0, NULL }
};

void initmask(void)
{
  PyObject *module, *dict;
  PyType_Init(PyMask_Type);
  
  /* create the module */
  module = Py_InitModule3("mask", mask_builtins, DOC_PYGAMEMASK);
  dict = PyModule_GetDict(module);
  PyDict_SetItemString(dict, "MaskType", (PyObject *)&PyMask_Type);
  import_pygame_base ();
  import_pygame_color ();
  import_pygame_surface ();
  import_pygame_rect ();
}

