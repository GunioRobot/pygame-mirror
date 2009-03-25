/*
  pygame - Python Game Library
  Copyright (C) 2008 Marcus von Appen

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
/*
  This is a proposed SDL_gfx draw module for Pygame. It is backported
  from Pygame 2.
  
  TODO:
  - Add gfxdraw.doc file.
  - Unpdate Setup.in and config modules.
  - Move Python 3 compatibility macros to a header.
  - Determine if SDL video must be initiated for all routines to work.
    Add check if required, else remove ASSERT_VIDEO_INIT.
  - Unit tests.
  - Example (Maybe).
*/
#define PYGAME_SDLGFXPRIM_INTERNAL

#include "pygame.h"
#include "surface.h"
#include <SDL_gfxPrimitives.h>

#if PY_VERSION_HEX >= 0x03000000
#define PY3 1
#define MODINIT_RETURN(x) return x
#else
#define PY3 0
#define MODINIT_RETURN(x) return
#endif

static PyObject* _gfx_pixelcolor (PyObject *self, PyObject* args);
static PyObject* _gfx_hlinecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_vlinecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_rectanglecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_boxcolor (PyObject *self, PyObject* args);
static PyObject* _gfx_linecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_circlecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_arccolor (PyObject *self, PyObject* args);
static PyObject* _gfx_aacirclecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_filledcirclecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_ellipsecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_aaellipsecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_filledellipsecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_piecolor (PyObject *self, PyObject* args);
static PyObject* _gfx_trigoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_aatrigoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_filledtrigoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_polygoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_aapolygoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_filledpolygoncolor (PyObject *self, PyObject* args);
static PyObject* _gfx_texturedpolygon (PyObject *self, PyObject* args);
static PyObject* _gfx_beziercolor (PyObject *self, PyObject* args);

static PyMethodDef _gfxdraw_methods[] = {
    { "pixel", _gfx_pixelcolor, METH_VARARGS, "" },
    { "hline", _gfx_hlinecolor, METH_VARARGS, "" },
    { "vline", _gfx_vlinecolor, METH_VARARGS, "" },
    { "rectangle", _gfx_rectanglecolor, METH_VARARGS, "" },
    { "box", _gfx_boxcolor, METH_VARARGS, "" },
    { "line", _gfx_linecolor, METH_VARARGS, "" },
    { "arc", _gfx_arccolor, METH_VARARGS, "" },
    { "circle", _gfx_circlecolor, METH_VARARGS, "" },
    { "aacircle", _gfx_aacirclecolor, METH_VARARGS, "" },
    { "filled_circle", _gfx_filledcirclecolor, METH_VARARGS, "" },
    { "ellipse", _gfx_ellipsecolor, METH_VARARGS, "" },
    { "aaellipse", _gfx_aaellipsecolor, METH_VARARGS, "" },
    { "filled_ellipse", _gfx_filledellipsecolor, METH_VARARGS, "" },
    { "pie", _gfx_piecolor, METH_VARARGS, "" },
    { "trigon", _gfx_trigoncolor, METH_VARARGS, "" },
    { "aatrigon", _gfx_aatrigoncolor, METH_VARARGS, "" },
    { "filled_trigon", _gfx_filledtrigoncolor, METH_VARARGS, "" },
    { "polygon", _gfx_polygoncolor, METH_VARARGS, "" },
    { "aapolygon", _gfx_aapolygoncolor, METH_VARARGS, "" },
    { "filled_polygon", _gfx_filledpolygoncolor, METH_VARARGS, "" },
    { "textured_polygon", _gfx_texturedpolygon, METH_VARARGS, "" },
    { "bezier", _gfx_beziercolor, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL },
};

#define ASSERT_VIDEO_INIT(unused)  /* Is video really needed for gfxdraw? */

static int
Sint16FromObj (PyObject *item, Sint16 *val)
{
    PyObject* intobj;
    long tmp;
    
    if (PyNumber_Check (item))
    {
        if (!(intobj = PyNumber_Int (item)))
            return 0;
        tmp = PyInt_AsLong (intobj);
        Py_DECREF (intobj);
        if (tmp == -1 && PyErr_Occurred ())
            return 0;
        *val = (Sint16)tmp;
        return 1;
    }
    return 0;
}

static int
Sint16FromSeqIndex (PyObject* obj, Py_ssize_t _index, Sint16* val)
{
    int result = 0;
    PyObject* item;
    item = PySequence_GetItem (obj, _index);
    if (item)
    {
        result = Sint16FromObj (item, val);
        Py_DECREF (item);
    }
    return result;
}

static PyObject*
_gfx_pixelcolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhO:pixel", &surface, &x, &y, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (pixelRGBA (PySurface_AsSurface (surface), x, y,
		   rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_hlinecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x1, x2, y;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhO:hline", &surface, &x1, &x2, &y, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (hlineRGBA (PySurface_AsSurface (surface), x1, x2, y,
		   rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_vlinecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, _y1, y2;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhO:vline", &surface, &x, &_y1, &y2,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (vlineRGBA (PySurface_AsSurface (surface), x, _y1, y2,
		   rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_rectanglecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *rect;
    GAME_Rect sdlrect;
    Sint16 x1, x2, _y1, y2;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOO:rectangle", &surface, &rect, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!GameRect_FromObject (rect, &sdlrect))
        return NULL;

    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    x1 = sdlrect.x;
    _y1 = sdlrect.y;
    x2 = (Sint16) (sdlrect.x + sdlrect.w);
    y2 = (Sint16) (sdlrect.y + sdlrect.h);

    if (rectangleRGBA (PySurface_AsSurface (surface), x1, x2, _y1, y2,
		       rgba[0], rgba[1], rgba[2], rgba[3]) ==
        -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_boxcolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *rect;
    GAME_Rect sdlrect;
    Sint16 x1, x2, _y1, y2;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOO:box", &surface, &rect, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!GameRect_FromObject (rect, &sdlrect))
        return NULL;

    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    x1 = sdlrect.x;
    _y1 = sdlrect.y;
    x2 = (Sint16) (sdlrect.x + sdlrect.w);
    y2 = (Sint16) (sdlrect.y + sdlrect.h);

    if (rectangleRGBA (PySurface_AsSurface (surface), x1, x2, _y1, y2,
		       rgba[0], rgba[1], rgba[2], rgba[3]) ==
        -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_linecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x1, x2, _y1, y2;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhO:line", &surface, &x1, &_y1, &x2, &y2,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (lineRGBA (PySurface_AsSurface (surface), x1, _y1, x2, y2,
		  rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_circlecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, r;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhO:circle", &surface, &x, &y, &r, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (circleRGBA (PySurface_AsSurface (surface), x, y, r,
		    rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_arccolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, r, start, end;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhhO:arc", &surface, &x, &y, &r,
            &start, &end, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (arcRGBA (PySurface_AsSurface (surface), x, y, r, start, end,
		 rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_aacirclecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, r;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhO:aacircle", &surface, &x, &y, &r,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (aacircleRGBA (PySurface_AsSurface (surface), x, y, r,
		      rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_filledcirclecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, r;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhO:filledcircle", &surface, &x, &y, &r,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (filledCircleRGBA (PySurface_AsSurface (surface), x, y, r,
			  rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_ellipsecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, rx, ry;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhO:ellipse", &surface, &x, &y, &rx, &ry,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (ellipseRGBA (PySurface_AsSurface (surface), x, y, rx, ry,
		     rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_aaellipsecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, rx, ry;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhO:aaellipse", &surface, &x, &y, &rx, &ry,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (aaellipseRGBA (PySurface_AsSurface (surface), x, y, rx, ry,
		       rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_filledellipsecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, rx, ry;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhO:filled_ellipse", &surface, &x, &y,
            &rx, &ry, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (filledEllipseRGBA (PySurface_AsSurface (surface), x, y, rx, ry,
			   rgba[0], rgba[1], rgba[2], rgba[3]) ==
        -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_piecolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x, y, r, start, end;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhhO:pie", &surface, &x, &y, &r,
            &start, &end, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (pieRGBA (PySurface_AsSurface (surface), x, y, r, start, end,
		 rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_trigoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x1, x2, x3, _y1, y2, y3;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhhhO:trigon", &surface, &x1, &_y1, &x2,
            &y2, &x3, &y3, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (trigonRGBA (PySurface_AsSurface (surface), x1, _y1, x2, y2, x3, y3,
		    rgba[0], rgba[1], rgba[2], rgba[3])
        == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_aatrigoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x1, x2, x3, _y1, y2, y3;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhhhO:aatrigon", &surface, &x1, &_y1, &x2,
            &y2, &x3, &y3, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (aatrigonRGBA (PySurface_AsSurface (surface), x1, _y1, x2, y2, x3, y3,
		      rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_filledtrigoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color;
    Sint16 x1, x2, x3, _y1, y2, y3;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OhhhhhhO:filled_trigon", &surface, &x1, &_y1,
            &x2, &y2, &x3, &y3, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }

    if (filledTrigonRGBA (PySurface_AsSurface (surface), x1, _y1, x2, y2,
			  x3, y3, rgba[0], rgba[1], rgba[2], rgba[3]) == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_polygoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *points, *item;
    Sint16 *vx, *vy, x, y;
    Py_ssize_t count, i;
    int ret;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOO:polygon", &surface, &points, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }
    if (!PySequence_Check (points))
    {
        PyErr_SetString (PyExc_TypeError, "points must be a sequence");
        return NULL;
    }

    count = PySequence_Size (points);
    if (count < 3)
    {
        PyErr_SetString (PyExc_ValueError,
            "points must contain more than 2 points");
        return NULL;
    }

    vx = PyMem_New (Sint16, (size_t) count);
    vy = PyMem_New (Sint16, (size_t) count);
    if (!vx || !vy)
    {
        if (vx)
            PyMem_Free (vx);
        if (vy)
            PyMem_Free (vy);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (points, i);
        if (!Sint16FromSeqIndex (item, 0, &x))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        if (!Sint16FromSeqIndex (item, 1, &y))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        Py_DECREF (item);
        vx[i] = x;
        vy[i] = x;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = polygonRGBA (PySurface_AsSurface (surface), vx, vy, (int)count,
		       rgba[0], rgba[1], rgba[2], rgba[3]);
    Py_END_ALLOW_THREADS;

    PyMem_Free (vx);
    PyMem_Free (vy);

    if (ret == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_aapolygoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *points, *item;
    Sint16 *vx, *vy, x, y;
    Py_ssize_t count, i;
    int ret;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOO:aapolygon", &surface, &points, &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }
    if (!PySequence_Check (points))
    {
        PyErr_SetString (PyExc_TypeError, "points must be a sequence");
        return NULL;
    }

    count = PySequence_Size (points);
    if (count < 3)
    {
        PyErr_SetString (PyExc_ValueError,
            "points must contain more than 2 points");
        return NULL;
    }

    vx = PyMem_New (Sint16, (size_t) count);
    vy = PyMem_New (Sint16, (size_t) count);
    if (!vx || !vy)
    {
        if (vx)
            PyMem_Free (vx);
        if (vy)
            PyMem_Free (vy);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (points, i);
        if (!Sint16FromSeqIndex (item, 0, &x))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        if (!Sint16FromSeqIndex (item, 1, &y))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        Py_DECREF (item);
        vx[i] = x;
        vy[i] = x;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = aapolygonRGBA (PySurface_AsSurface (surface), vx, vy, (int)count,
			 rgba[0], rgba[1], rgba[2], rgba[3]);
    Py_END_ALLOW_THREADS;

    PyMem_Free (vx);
    PyMem_Free (vy);

    if (ret == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_filledpolygoncolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *points, *item;
    Sint16 *vx, *vy, x, y;
    Py_ssize_t count, i;
    int ret;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOO:filled_polygon", &surface, &points,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }
    if (!PySequence_Check (points))
    {
        PyErr_SetString (PyExc_TypeError, "points must be a sequence");
        return NULL;
    }

    count = PySequence_Size (points);
    if (count < 3)
    {
        PyErr_SetString (PyExc_ValueError,
            "points must contain more than 2 points");
        return NULL;
    }

    vx = PyMem_New (Sint16, (size_t) count);
    vy = PyMem_New (Sint16, (size_t) count);
    if (!vx || !vy)
    {
        if (vx)
            PyMem_Free (vx);
        if (vy)
            PyMem_Free (vy);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (points, i);
        if (!Sint16FromSeqIndex (item, 0, &x))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        if (!Sint16FromSeqIndex (item, 1, &y))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        Py_DECREF (item);
        vx[i] = x;
        vy[i] = x;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = filledPolygonRGBA (PySurface_AsSurface (surface), vx, vy,
			     (int)count, rgba[0], rgba[1], rgba[2], rgba[3]);
    Py_END_ALLOW_THREADS;

    PyMem_Free (vx);
    PyMem_Free (vy);

    if (ret == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_gfx_texturedpolygon (PyObject *self, PyObject* args)
{
    PyObject *surface, *texture, *points, *item;
    Sint16 *vx, *vy, x, y, tdx, tdy;
    Py_ssize_t count, i;
    int ret;

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOOhh:textured_polygon", &surface, &points,
            &texture, &tdx, &tdy))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!PySurface_Check (texture))
    {
        PyErr_SetString (PyExc_TypeError, "texture must be a Surface");
        return NULL;
    }
    if (!PySequence_Check (points))
    {
        PyErr_SetString (PyExc_TypeError, "points must be a sequence");
        return NULL;
    }

    count = PySequence_Size (points);
    if (count < 3)
    {
        PyErr_SetString (PyExc_ValueError,
            "points must contain more than 2 points");
        return NULL;
    }

    vx = PyMem_New (Sint16, (size_t) count);
    vy = PyMem_New (Sint16, (size_t) count);
    if (!vx || !vy)
    {
        if (vx)
            PyMem_Free (vx);
        if (vy)
            PyMem_Free (vy);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (points, i);
        if (!Sint16FromSeqIndex (item, 0, &x))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        if (!Sint16FromSeqIndex (item, 1, &y))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        Py_DECREF (item);
        vx[i] = x;
        vy[i] = x;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = texturedPolygon (PySurface_AsSurface (surface), vx, vy, (int)count,
			   PySurface_AsSurface (texture), tdx, tdy);
    Py_END_ALLOW_THREADS;

    PyMem_Free (vx);
    PyMem_Free (vy);

    if (ret == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
_gfx_beziercolor (PyObject *self, PyObject* args)
{
    PyObject *surface, *color, *points, *item;
    Sint16 *vx, *vy, x, y;
    Py_ssize_t count, i;
    int ret, steps;
    Uint8 rgba[4];

    ASSERT_VIDEO_INIT (NULL);

    if (!PyArg_ParseTuple (args, "OOiO:bezier", &surface, &points, &steps,
            &color))
        return NULL;
    
    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (!RGBAFromObj (color, rgba))
    {
        PyErr_SetString (PyExc_TypeError, "invalid color argument");
        return NULL;
    }
    if (!PySequence_Check (points))
    {
        PyErr_SetString (PyExc_TypeError, "points must be a sequence");
        return NULL;
    }

    count = PySequence_Size (points);
    if (count < 3)
    {
        PyErr_SetString (PyExc_ValueError,
            "points must contain more than 2 points");
        return NULL;
    }

    vx = PyMem_New (Sint16, (size_t) count);
    vy = PyMem_New (Sint16, (size_t) count);
    if (!vx || !vy)
    {
        PyErr_SetString (PyExc_MemoryError, "memory allocation failed");
        if (vx)
            PyMem_Free (vx);
        if (vy)
            PyMem_Free (vy);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (points, i);
        if (!Sint16FromSeqIndex (item, 0, &x))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        if (!Sint16FromSeqIndex (item, 1, &y))
        {
            PyMem_Free (vx);
            PyMem_Free (vy);
            Py_XDECREF (item);
            return NULL;
        }
        Py_DECREF (item);
        vx[i] = x;
        vy[i] = x;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = bezierRGBA (PySurface_AsSurface (surface), vx, vy, (int)count,
		      steps, rgba[0], rgba[1], rgba[2], rgba[3]);
    Py_END_ALLOW_THREADS;

    PyMem_Free (vx);
    PyMem_Free (vy);

    if (ret == -1)
    {
        PyErr_SetString (PyExc_SDLError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

#if PY3
PyMODINIT_FUNC PyInit_gfxdraw (void)
#else
PyMODINIT_FUNC initgfxdraw (void)
#endif
{
    PyObject *mod;
    
#if PY3
    static struct PyModuleDef _module = {
        PyModuleDef_HEAD_INIT,
        "gfxdraw",
        "",
        -1,
        _gfxdraw_methods,
        NULL, NULL, NULL, NULL
    };
#endif

    /* importe needed apis; Do this first so if there is an error
       the module is not loaded.
    */
    import_pygame_base();
    if (PyErr_Occurred ()) {
	MODINIT_RETURN (NULL);
    }
    import_pygame_color();
    if (PyErr_Occurred ()) {
	MODINIT_RETURN (NULL);
    }
    import_pygame_rect();
    if (PyErr_Occurred ()) {
	MODINIT_RETURN (NULL);
    }
    import_pygame_surface();
    if (PyErr_Occurred ()) {
	MODINIT_RETURN (NULL);
    }

#if PY3
    mod = PyModule_Create (&_module);
#else
    mod = Py_InitModule3 ("gfxdraw", _gfxdraw_methods, "");
#endif

    MODINIT_RETURN (mod);
}
