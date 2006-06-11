/*
    pygame - Python Game Library
    Copyright (C) 2000-2001  Pete Shinners

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

/*
 *  pygame Surface module
 */
#define PYGAMEAPI_SURFACE_INTERNAL
#include "pygame.h"
#include "pygamedocs.h"
#include "structmember.h"


staticforward PyTypeObject PySurface_Type;
static PyObject* PySurface_New(SDL_Surface* info);
#define PySurface_Check(x) ((x)->ob_type == &PySurface_Type)
extern int pygame_AlphaBlit(SDL_Surface *src, SDL_Rect *srcrect,
                        SDL_Surface *dst, SDL_Rect *dstrect);
extern int pygame_Blit(SDL_Surface *src, SDL_Rect *srcrect,
                        SDL_Surface *dst, SDL_Rect *dstrect, int the_args);

static PyObject* surface_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static intptr_t surface_init(PySurfaceObject *self, PyObject *args, PyObject *kwds);


/* surface object methods */


static PyObject* surf_get_at(PyObject* self, PyObject* arg)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_PixelFormat* format = surf->format;
	Uint8* pixels = (Uint8*)surf->pixels;
	int x, y;
	Uint32 color;
	Uint8* pix;
	Uint8 r, g, b, a;

	if(!PyArg_ParseTuple(arg, "(ii)", &x, &y))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(x < 0 || x >= surf->w || y < 0 || y >= surf->h)
		return RAISE(PyExc_IndexError, "pixel index out of range");

	if(format->BytesPerPixel < 1 || format->BytesPerPixel > 4)
		return RAISE(PyExc_RuntimeError, "invalid color depth for surface");

	if(!PySurface_Lock(self)) return NULL;
	pixels = (Uint8*)surf->pixels;

	switch(format->BytesPerPixel)
	{
		case 1:
			color = (Uint32)*((Uint8*)pixels + y * surf->pitch + x);
			break;
		case 2:
			color = (Uint32)*((Uint16*)(pixels + y * surf->pitch) + x);
			break;
		case 3:
			pix = ((Uint8*)(pixels + y * surf->pitch) + x * 3);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			color = (pix[0]) + (pix[1]<<8) + (pix[2]<<16);
#else
			color = (pix[2]) + (pix[1]<<8) + (pix[0]<<16);
#endif
			break;
		default: /*case 4:*/
			color = *((Uint32*)(pixels + y * surf->pitch) + x);
			break;
	}
	if(!PySurface_Unlock(self)) return NULL;

	SDL_GetRGBA(color, format, &r, &g, &b, &a);
	return Py_BuildValue("(bbbb)", r, g, b, a);
}


static PyObject* surf_set_at(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_PixelFormat* format = surf->format;
	Uint8* pixels;
	int x, y;
	Uint32 color;
	Uint8 rgba[4];
	PyObject* rgba_obj;
	Uint8* byte_buf;

	if(!PyArg_ParseTuple(args, "(ii)O", &x, &y, &rgba_obj))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(format->BytesPerPixel < 1 || format->BytesPerPixel > 4)
		return RAISE(PyExc_RuntimeError, "invalid color depth for surface");

	if(x < surf->clip_rect.x || x >= surf->clip_rect.x + surf->clip_rect.w ||
                    y < surf->clip_rect.y || y >= surf->clip_rect.y + surf->clip_rect.h)
	{
		/*out of clip area*/
                RETURN_NONE
	}

	if(PyInt_Check(rgba_obj))
		color = (Uint32)PyInt_AsLong(rgba_obj);
	else if(RGBAFromObj(rgba_obj, rgba))
		color = SDL_MapRGBA(surf->format, rgba[0], rgba[1], rgba[2], rgba[3]);
	else
		return RAISE(PyExc_TypeError, "invalid color argument");

	if(!PySurface_Lock(self)) return NULL;
	pixels = (Uint8*)surf->pixels;

	switch(format->BytesPerPixel)
	{
		case 1:
			*((Uint8*)pixels + y * surf->pitch + x) = (Uint8)color;
			break;
		case 2:
			*((Uint16*)(pixels + y * surf->pitch) + x) = (Uint16)color;
			break;
		case 3:
			byte_buf = (Uint8*)(pixels + y * surf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                        *(byte_buf + (format->Rshift >> 3)) = rgba[0];
                        *(byte_buf + (format->Gshift >> 3)) = rgba[1];
                        *(byte_buf + (format->Bshift >> 3)) = rgba[2];
#else
                        *(byte_buf + 2 - (format->Rshift >> 3)) = rgba[0];
                        *(byte_buf + 2 - (format->Gshift >> 3)) = rgba[1];
                        *(byte_buf + 2 - (format->Bshift >> 3)) = rgba[2];
#endif
			break;
		default: /*case 4:*/
			*((Uint32*)(pixels + y * surf->pitch) + x) = color;
			break;
	}

	if(!PySurface_Unlock(self)) return NULL;
	RETURN_NONE
}


static PyObject* surf_map_rgb(PyObject* self,PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	Uint8 rgba[4];
	int color;

	if(!RGBAFromObj(args, rgba))
		return RAISE(PyExc_TypeError, "Invalid RGBA argument");
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	color = SDL_MapRGBA(surf->format, rgba[0], rgba[1], rgba[2], rgba[3]);
	return PyInt_FromLong(color);
}


static PyObject* surf_unmap_rgb(PyObject* self,PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	Uint32 col;
	Uint8 r, g, b, a;

	if(!PyArg_ParseTuple(args, "i", &col))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	SDL_GetRGBA(col,surf->format, &r, &g, &b, &a);

	return Py_BuildValue("(bbbb)", r, g, b, a);
}


static PyObject* surf_lock(PyObject* self, PyObject* args)
{
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	if(!PySurface_Lock(self))
		return NULL;

	RETURN_NONE
}


static PyObject* surf_unlock(PyObject* self, PyObject* args)
{
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	if(!PySurface_Unlock(self))
		return NULL;

	RETURN_NONE
}


static PyObject* surf_mustlock(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	return PyInt_FromLong(SDL_MUSTLOCK(surf) || ((PySurfaceObject*)self)->subsurface);
}


static PyObject* surf_get_locked(PyObject* self, PyObject* args)
{
	PySurfaceObject* surf = (PySurfaceObject*)self;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	return PyInt_FromLong(surf->surf->pixels != NULL);
}


static PyObject* surf_get_palette(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_Palette* pal = surf->format->palette;
	PyObject* list;
	int i;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(!pal)
		return RAISE(PyExc_SDLError, "Surface has no palette to get\n");

	list = PyTuple_New(pal->ncolors);
	if(!list)
		return NULL;

	for(i = 0;i < pal->ncolors;i++)
	{
		PyObject* color;
		SDL_Color* c = &pal->colors[i];

		color = Py_BuildValue("(bbb)", c->r, c->g, c->b);
		if(!color)
		{
			Py_DECREF(list);
			return NULL;
		}

		PyTuple_SET_ITEM(list, i, color);
	}

	return list;
}


static PyObject* surf_get_palette_at(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_Palette* pal = surf->format->palette;
	SDL_Color* c;
	int index;

	if(!PyArg_ParseTuple(args, "i", &index))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(!pal)
		return RAISE(PyExc_SDLError, "Surface has no palette to set\n");
	if(index >= pal->ncolors || index < 0)
		return RAISE(PyExc_IndexError, "index out of bounds");

	c = &pal->colors[index];
	return Py_BuildValue("(bbb)", c->r, c->g, c->b);
}


static PyObject* surf_set_palette(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_Palette* pal = surf->format->palette;
	SDL_Color* colors;
	PyObject* list, *item;
	int i, len;
	int r, g, b;

	if(!PyArg_ParseTuple(args, "O", &list))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	if(!PySequence_Check(list))
		return RAISE(PyExc_ValueError, "Argument must be a sequence type");

	if(!pal)
		return RAISE(PyExc_SDLError, "Surface has no palette\n");

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return RAISE(PyExc_SDLError, "cannot set palette without pygame.display initialized");

	len = min(pal->ncolors, PySequence_Length(list));

	colors = (SDL_Color*)malloc(len * sizeof(SDL_Color));
	if(!colors)
		return NULL;

	for(i = 0; i < len; i++)
	{
		item = PySequence_GetItem(list, i);

		if(!PySequence_Check(item) || PySequence_Length(item) != 3)
		{
			Py_DECREF(item);
			free((char*)colors);
			return RAISE(PyExc_TypeError, "takes a sequence of sequence of RGB");
		}
		if(!IntFromObjIndex(item, 0, &r) || !IntFromObjIndex(item, 1, &g) || !IntFromObjIndex(item, 2, &b))
			return RAISE(PyExc_TypeError, "RGB sequence must contain numeric values");

		colors[i].r = (unsigned char)r;
		colors[i].g = (unsigned char)g;
		colors[i].b = (unsigned char)b;
		Py_DECREF(item);
	}

	SDL_SetColors(surf, colors, 0, len);
	free((char*)colors);
	RETURN_NONE
}


static PyObject* surf_set_palette_at(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_Palette* pal = surf->format->palette;
	SDL_Color color;
	int index;
	Uint8 r, g, b;

	if(!PyArg_ParseTuple(args, "i(bbb)", &index, &r, &g, &b))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(!pal)
	{
		PyErr_SetString(PyExc_SDLError, "Surface is not palettized\n");
		return NULL;
	}

	if(index >= pal->ncolors || index < 0)
	{
		PyErr_SetString(PyExc_IndexError, "index out of bounds");
		return NULL;
	}

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return RAISE(PyExc_SDLError, "cannot set palette without pygame.display initialized");

	color.r = r;
	color.g = g;
	color.b = b;

	SDL_SetColors(surf, &color, index, 1);

	RETURN_NONE
}


static PyObject* surf_set_colorkey(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	Uint32 flags = 0, color = 0;
	PyObject* rgba_obj = NULL, *intobj = NULL;
	Uint8 rgba[4];
	int result, hascolor=0;

	if(!PyArg_ParseTuple(args, "|Oi", &rgba_obj, &flags))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(rgba_obj && rgba_obj!=Py_None)
	{
		if(PyNumber_Check(rgba_obj) && (intobj=PyNumber_Int(rgba_obj)))
		{
			color = (Uint32)PyInt_AsLong(intobj);
			Py_DECREF(intobj);
		}
		else if(RGBAFromObj(rgba_obj, rgba))
			color = SDL_MapRGBA(surf->format, rgba[0], rgba[1], rgba[2], rgba[3]);
		else
			return RAISE(PyExc_TypeError, "invalid color argument");
		hascolor = 1;
	}
	if(hascolor)
		flags |= SDL_SRCCOLORKEY;

	PySurface_Prep(self);
	result = SDL_SetColorKey(surf, flags, color);
	PySurface_Unprep(self);

	if(result == -1)
		return RAISE(PyExc_SDLError, SDL_GetError());

	RETURN_NONE
}


static PyObject* surf_get_colorkey(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	Uint8 r, g, b, a;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(!(surf->flags&SDL_SRCCOLORKEY))
		RETURN_NONE

	SDL_GetRGBA(surf->format->colorkey, surf->format, &r, &g, &b, &a);
	return Py_BuildValue("(bbbb)", r, g, b, a);
}


static PyObject* surf_set_alpha(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	Uint32 flags = 0;
	PyObject* alpha_obj = NULL, *intobj=NULL;
	Uint8 alpha;
	int result, alphaval=255, hasalpha=0;

	if(!PyArg_ParseTuple(args, "|Oi", &alpha_obj, &flags))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(alpha_obj && alpha_obj!=Py_None)
	{
		if(PyNumber_Check(alpha_obj) && (intobj=PyNumber_Int(alpha_obj)))
		{
			alphaval = (int)PyInt_AsLong(intobj);
			Py_DECREF(intobj);
		}
		else
			return RAISE(PyExc_TypeError, "invalid alpha argument");
		hasalpha = 1;
	}
	if(hasalpha)
		flags |= SDL_SRCALPHA;

	if(alphaval>255) alpha = 255;
	else if(alphaval<0) alpha = 0;
	else alpha = (Uint8)alphaval;

	PySurface_Prep(self);
	result = SDL_SetAlpha(surf, flags, alpha);
	PySurface_Unprep(self);

	if(result == -1)
		return RAISE(PyExc_SDLError, SDL_GetError());

	RETURN_NONE
}


static PyObject* surf_get_alpha(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(surf->flags&SDL_SRCALPHA)
		return PyInt_FromLong(surf->format->alpha);

	RETURN_NONE
}


static PyObject* surf_copy(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	PyObject* final;
	SDL_Surface* newsurf;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

        if(surf->flags & SDL_OPENGL)
        {
            return RAISE(PyExc_SDLError, "Cannot copy opengl display");
        }

	PySurface_Prep(self);
        newsurf = SDL_ConvertSurface(surf, surf->format, surf->flags);
	PySurface_Unprep(self);

	final = PySurface_New(newsurf);
	if(!final)
		SDL_FreeSurface(newsurf);
	return final;
}


static PyObject* surf_convert(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	PyObject* final;
	PyObject* argobject=NULL;
	SDL_Surface* src;
	SDL_Surface* newsurf;
	Uint32 flags=-1;

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return RAISE(PyExc_SDLError, "cannot convert without pygame.display initialized");

	if(!PyArg_ParseTuple(args, "|Oi", &argobject, &flags))
		return NULL;

        if(surf->flags & SDL_OPENGL)
        {
            return RAISE(PyExc_SDLError, "Cannot convert opengl display");
        }

	PySurface_Prep(self);
	if(argobject)
	{
		if(PySurface_Check(argobject))
		{
			src = PySurface_AsSurface(argobject);
			flags = src->flags | (surf->flags & (SDL_SRCCOLORKEY|SDL_SRCALPHA));
			newsurf = SDL_ConvertSurface(surf, src->format, flags);
		}
		else
		{
			int bpp;
			SDL_PixelFormat format;
			memcpy(&format, surf->format, sizeof(format));
			if(IntFromObj(argobject, &bpp))
			{
				int Rmask, Gmask, Bmask, Amask;
				if(flags!=-1 && flags&SDL_SRCALPHA)
				{
					switch(bpp)
					{
					case 16:
						Rmask = 0xF<<8; Gmask = 0xF<<4; Bmask = 0xF; Amask = 0xF<<12; break;
					case 32:
						Rmask = 0xFF<<16; Gmask = 0xFF<<8; Bmask = 0xFF; Amask = 0xFF<<24; break;
					default:
						return RAISE(PyExc_ValueError, "no standard masks exist for given bitdepth with alpha");
					}
				}
				else
				{
					Amask = 0;
					switch(bpp)
					{
					case 8:
						Rmask = 0xFF>>6<<5; Gmask = 0xFF>>5<<2; Bmask = 0xFF>>6; break;
					case 12:
						Rmask = 0xFF>>4<<8; Gmask = 0xFF>>4<<4; Bmask = 0xFF>>4; break;
					case 15:
						Rmask = 0xFF>>3<<10; Gmask = 0xFF>>3<<5; Bmask = 0xFF>>3; break;
					case 16:
						Rmask = 0xFF>>3<<11; Gmask = 0xFF>>2<<5; Bmask = 0xFF>>3; break;
					case 24:
					case 32:
						Rmask = 0xFF << 16; Gmask = 0xFF << 8; Bmask = 0xFF; break;
					default:
						return RAISE(PyExc_ValueError, "nonstandard bit depth given");
					}
				}
				format.Rmask = Rmask; format.Gmask = Gmask;
				format.Bmask = Bmask; format.Amask = Amask;
			}
			else if(PySequence_Check(argobject) && PySequence_Size(argobject)==4)
			{
				Uint32 mask;
				if(!UintFromObjIndex(argobject, 0, &format.Rmask) ||
							!UintFromObjIndex(argobject, 1, &format.Gmask) ||
							!UintFromObjIndex(argobject, 2, &format.Bmask) ||
							!UintFromObjIndex(argobject, 3, &format.Amask))
				{
					PySurface_Unprep(self);
					return RAISE(PyExc_ValueError, "invalid color masks given");
				}
				mask = format.Rmask|format.Gmask|format.Bmask|format.Amask;
				for(bpp=0; bpp<32; ++bpp)
					if(!(mask>>bpp)) break;
			}
			else
			{
				PySurface_Unprep(self);
				return RAISE(PyExc_ValueError, "invalid argument specifying new format to convert to");
			}
			format.BitsPerPixel = (Uint8)bpp;
			format.BytesPerPixel = (bpp+7)/8;
			if(flags == -1)
				flags = surf->flags;
			if(format.Amask)
				flags |= SDL_SRCALPHA;
			newsurf = SDL_ConvertSurface(surf, &format, flags);
		}
	}
	else
	{
		if(SDL_WasInit(SDL_INIT_VIDEO))
			newsurf = SDL_DisplayFormat(surf);
		else
			newsurf = SDL_ConvertSurface(surf, surf->format, surf->flags);
	}
	PySurface_Unprep(self);

	final = PySurface_New(newsurf);
	if(!final)
		SDL_FreeSurface(newsurf);
	return final;
}


static PyObject* surf_convert_alpha(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	PyObject* final;
	PySurfaceObject* srcsurf = NULL;
	SDL_Surface* newsurf, *src;

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return RAISE(PyExc_SDLError, "cannot convert without pygame.display initialized");

	if(!PyArg_ParseTuple(args, "|O!", &PySurface_Type, &srcsurf))
		return NULL;

	PySurface_Prep(self);
	if(srcsurf)
	{
		/*hmm, we have to figure this out, not all depths have good support for alpha*/
		src = PySurface_AsSurface(srcsurf);
		newsurf = SDL_DisplayFormatAlpha(surf);
	}
	else
		newsurf = SDL_DisplayFormatAlpha(surf);
	PySurface_Unprep(self);

	final = PySurface_New(newsurf);
	if(!final)
		SDL_FreeSurface(newsurf);
	return final;
}


static PyObject* surf_set_clip(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	PyObject* item;
	GAME_Rect *rect=NULL, temp;
        SDL_Rect sdlrect;
	int result;

        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	if(PyTuple_Size(args))
	{
		item = PyTuple_GET_ITEM(args, 0);
		if(!(item == Py_None && PyTuple_Size(args) == 1))
		{
		    rect = GameRect_FromObject(args, &temp);
		    if(!rect)
		        return RAISE(PyExc_ValueError, "invalid rectstyle object");
		    sdlrect.x = rect->x;
		    sdlrect.y = rect->y;
		    sdlrect.h = rect->h;
		    sdlrect.w = rect->w;
		}
                result = SDL_SetClipRect(surf, &sdlrect);
	}
        else
                result = SDL_SetClipRect(surf, NULL);

	if(result == -1)
		return RAISE(PyExc_SDLError, SDL_GetError());

	RETURN_NONE
}


static PyObject* surf_get_clip(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return PyRect_New(&surf->clip_rect);
}


static PyObject* surf_fill(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	GAME_Rect *rect, temp;
	PyObject* r = NULL;
	Uint32 color;
	int result;
	PyObject* rgba_obj;
	Uint8 rgba[4];
        SDL_Rect sdlrect;
	if(!PyArg_ParseTuple(args, "O|O", &rgba_obj, &r))
		return NULL;
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(PyInt_Check(rgba_obj))
		color = (Uint32)PyInt_AsLong(rgba_obj);
	else if(RGBAFromObj(rgba_obj, rgba))
		color = SDL_MapRGBA(surf->format, rgba[0], rgba[1], rgba[2], rgba[3]);
	else
		return RAISE(PyExc_TypeError, "invalid color argument");

	if(!r)
	{
		rect = &temp;
		temp.x = temp.y = 0;
		temp.w = surf->w;
		temp.h = surf->h;
	}
	else if(!(rect = GameRect_FromObject(r, &temp)))
		return RAISE(PyExc_ValueError, "invalid rectstyle object");

	/*we need a fresh copy so our Rect values don't get munged*/
	if(rect != &temp)
	{
		memcpy(&temp, rect, sizeof(temp));
		rect = &temp;
	}

	if(rect->w < 0 || rect->h < 0)
	{
		sdlrect.x = sdlrect.y = 0;
		sdlrect.w = sdlrect.h = 0;
	}
	else
	{
		sdlrect.x = rect->x;
		sdlrect.y = rect->y;
		sdlrect.w = rect->w;
		sdlrect.h = rect->h;
	
		PySurface_Prep(self);
		result = SDL_FillRect(surf, &sdlrect, color);
		PySurface_Unprep(self);
	
		if(result == -1)
			return RAISE(PyExc_SDLError, SDL_GetError());
	}
	return PyRect_New(&sdlrect);
}

/*this internal blit function is accessable through the C api*/


int PySurface_Blit(PyObject *dstobj, PyObject *srcobj, SDL_Rect *dstrect, SDL_Rect *srcrect, int the_args)
{
    SDL_Surface *src = PySurface_AsSurface(srcobj);
    SDL_Surface *dst = PySurface_AsSurface(dstobj);
    SDL_Surface *subsurface = NULL;
    int result, suboffsetx=0, suboffsety=0;
    SDL_Rect orig_clip, sub_clip;
    int didconvert = 0;

    /*passthrough blits to the real surface*/
    if(((PySurfaceObject*)dstobj)->subsurface)
    {
	    PyObject *owner;
	    struct SubSurface_Data *subdata;

	    subdata = ((PySurfaceObject*)dstobj)->subsurface;
	    owner = subdata->owner;
            subsurface = PySurface_AsSurface(owner);
	    suboffsetx = subdata->offsetx;
	    suboffsety = subdata->offsety;

	    while(((PySurfaceObject*)owner)->subsurface)
	    {
		subdata = ((PySurfaceObject*)owner)->subsurface;
    		owner = subdata->owner;
	        subsurface = PySurface_AsSurface(owner);
	    	suboffsetx += subdata->offsetx;
    	    	suboffsety += subdata->offsety;
	    }

	    SDL_GetClipRect(subsurface, &orig_clip);
	    SDL_GetClipRect(dst, &sub_clip);
	    sub_clip.x += suboffsetx;
	    sub_clip.y += suboffsety;
	    SDL_SetClipRect(subsurface, &sub_clip);
	    dstrect->x += suboffsetx;
	    dstrect->y += suboffsety;
	    dst = subsurface;
    }
    else
    {
	    PySurface_Prep(dstobj);
	    subsurface = NULL;
    }

    PySurface_Prep(srcobj);
/*    Py_BEGIN_ALLOW_THREADS */

    /*can't blit alpha to 8bit, crashes SDL*/
    if(dst->format->BytesPerPixel==1 && (src->format->Amask || src->flags&SDL_SRCALPHA))
    {
	    didconvert = 1;
	    src = SDL_DisplayFormat(src);
    }

    /*see if we should handle alpha ourselves*/
    if(dst->format->Amask && (dst->flags&SDL_SRCALPHA) &&
                !(src->format->Amask && !(src->flags&SDL_SRCALPHA)) && /*special case, SDL works*/
                (dst->format->BytesPerPixel == 2 || dst->format->BytesPerPixel==4))
    {
        result = pygame_AlphaBlit(src, srcrect, dst, dstrect);
    } else if(the_args != 0) {
        result = pygame_Blit(src, srcrect, dst, dstrect, the_args);
    } else
    {
        result = SDL_BlitSurface(src, srcrect, dst, dstrect);
    }

    if(didconvert)
	    SDL_FreeSurface(src);

/*    Py_END_ALLOW_THREADS */
    if(subsurface)
    {
	    SDL_SetClipRect(subsurface, &orig_clip);
	    dstrect->x -= suboffsetx;
	    dstrect->y -= suboffsety;
    }
    else
	PySurface_Unprep(dstobj);
    PySurface_Unprep(srcobj);

    if(result == -1)
	    RAISE(PyExc_SDLError, SDL_GetError());
    if(result == -2)
	    RAISE(PyExc_SDLError, "Surface was lost");

    return result != 0;
}



static PyObject* surf_blit(PyObject* self, PyObject* args)
{
	SDL_Surface* src, *dest = PySurface_AsSurface(self);
	GAME_Rect* src_rect, temp;
	PyObject* srcobject, *argpos, *argrect = NULL;
	int dx, dy, result;
	SDL_Rect dest_rect, sdlsrc_rect;
	int sx, sy;
        int the_args;
        the_args = 0;

	if(!PyArg_ParseTuple(args, "O!O|Oi", &PySurface_Type, &srcobject, 
                                            &argpos, &argrect, &the_args))
		return NULL;


	src = PySurface_AsSurface(srcobject);
        if(!dest || !src) return RAISE(PyExc_SDLError, "display Surface quit");

	if(dest->flags & SDL_OPENGL && !(dest->flags&(SDL_OPENGLBLIT&~SDL_OPENGL)))
		return RAISE(PyExc_SDLError, "Cannot blit to OPENGL Surfaces (OPENGLBLIT is ok)");

	if((src_rect = GameRect_FromObject(argpos, &temp)))
	{
		dx = src_rect->x;
		dy = src_rect->y;
	}
	else if(TwoIntsFromObj(argpos, &sx, &sy))
	{
		dx = sx;
		dy = sy;
	}
	else
		return RAISE(PyExc_TypeError, "invalid destination position for blit");

        


	if(argrect && argrect != Py_None)
	{
		if(!(src_rect = GameRect_FromObject(argrect, &temp)))
			return RAISE(PyExc_TypeError, "Invalid rectstyle argument");
	}
	else
	{
		temp.x = temp.y = 0;
		temp.w = src->w;
		temp.h = src->h;
		src_rect = &temp;
	}

	dest_rect.x = (short)dx;
	dest_rect.y = (short)dy;
	dest_rect.w = (unsigned short)src_rect->w;
	dest_rect.h = (unsigned short)src_rect->h;
        sdlsrc_rect.x = (short)src_rect->x;
        sdlsrc_rect.y = (short)src_rect->y;
        sdlsrc_rect.w = (unsigned short)src_rect->w;
        sdlsrc_rect.h = (unsigned short)src_rect->h;

        if(!the_args) the_args = 0;

	result = PySurface_Blit(self, srcobject, &dest_rect, &sdlsrc_rect, the_args);
	if(result != 0)
	    return NULL;

	return PyRect_New(&dest_rect);
}


static PyObject* surf_get_flags(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return PyInt_FromLong(surf->flags);
}


static PyObject* surf_get_pitch(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return PyInt_FromLong(surf->pitch);
}


static PyObject* surf_get_size(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return Py_BuildValue("(ii)", surf->w, surf->h);
}


static PyObject* surf_get_width(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return PyInt_FromLong(surf->w);
}


static PyObject* surf_get_height(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return PyInt_FromLong(surf->h);
}


static PyObject* surf_get_rect(PyObject* self, PyObject* args, PyObject* kw)
{
	PyObject *rect;
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");

	rect = PyRect_New4(0, 0, surf->w, surf->h);
	if(rect && kw)
	{
		PyObject *key, *value;
		int pos=0;
		while(PyDict_Next(kw, &pos, &key, &value))
		{
			if((PyObject_SetAttr(rect, key, value) == -1))
			{
				Py_DECREF(rect);
				return NULL;
			}
		}
	}
	return rect;
}


static PyObject* surf_get_bitsize(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	return PyInt_FromLong(surf->format->BitsPerPixel);
}


static PyObject* surf_get_bytesize(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	return PyInt_FromLong(surf->format->BytesPerPixel);
}


static PyObject* surf_get_masks(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return Py_BuildValue("(iiii)", surf->format->Rmask, surf->format->Gmask,
				surf->format->Bmask, surf->format->Amask);
}


static PyObject* surf_get_shifts(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return Py_BuildValue("(iiii)", surf->format->Rshift, surf->format->Gshift,
				surf->format->Bshift, surf->format->Ashift);
}


static PyObject* surf_get_losses(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	return Py_BuildValue("(iiii)", surf->format->Rloss, surf->format->Gloss,
				surf->format->Bloss, surf->format->Aloss);
}


static PyObject* surf_subsurface(PyObject* self, PyObject* args)
{
	SDL_Surface* surf = PySurface_AsSurface(self);
	SDL_PixelFormat* format = surf->format;
	GAME_Rect *rect, temp;
	SDL_Surface* sub;
	PyObject* subobj;
	int pixeloffset;
	char* startpixel;
	struct SubSurface_Data* data;

        if(!surf) return RAISE(PyExc_SDLError, "display Surface quit");
	if(surf->flags & SDL_OPENGL)
		return RAISE(PyExc_SDLError, "Cannot call on OPENGL Surfaces");

	if(!(rect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_ValueError, "invalid rectstyle argument");
	if(rect->x < 0 || rect-> y < 0 || rect->x + rect->w > surf->w || rect->y + rect->h > surf->h)
		return RAISE(PyExc_ValueError, "subsurface rectangle outside surface area");


	PySurface_Lock(self);

	pixeloffset = rect->x * format->BytesPerPixel + rect->y * surf->pitch;
	startpixel = ((char*)surf->pixels) + pixeloffset;

	sub = SDL_CreateRGBSurfaceFrom(startpixel, rect->w, rect->h, format->BitsPerPixel, \
				surf->pitch, format->Rmask, format->Gmask, format->Bmask, format->Amask);

	PySurface_Unlock(self);

	if(!sub)
		return RAISE(PyExc_SDLError, SDL_GetError());

	/*copy the colormap if we need it*/
	if(surf->format->BytesPerPixel == 1 && surf->format->palette)
		SDL_SetPalette(sub, SDL_LOGPAL, surf->format->palette->colors, 0, surf->format->palette->ncolors);
	if(surf->flags & SDL_SRCALPHA)
		SDL_SetAlpha(sub, surf->flags&SDL_SRCALPHA, format->alpha);
	if(surf->flags & SDL_SRCCOLORKEY)
		SDL_SetColorKey(sub, surf->flags&(SDL_SRCCOLORKEY|SDL_RLEACCEL), format->colorkey);


	data = PyMem_New(struct SubSurface_Data, 1);
	if(!data) return NULL;

	subobj = PySurface_New(sub);
	if(!subobj)
	{
		PyMem_Del(data);
		return NULL;
	}
	Py_INCREF(self);
	data->owner = self;
	data->pixeloffset = pixeloffset;
	data->offsetx = rect->x;
	data->offsety = rect->y;
	((PySurfaceObject*)subobj)->subsurface = data;

	return subobj;
}


static PyObject* surf_get_offset(PyObject* self, PyObject* args)
{
    	struct SubSurface_Data *subdata;
    	subdata = ((PySurfaceObject*)self)->subsurface;
	if(!subdata)
    	    	return Py_BuildValue("(ii)", 0, 0);
    	return Py_BuildValue("(ii)", subdata->offsetx, subdata->offsety);
}


static PyObject* surf_get_abs_offset(PyObject* self, PyObject* args)
{
    	struct SubSurface_Data *subdata;
	PyObject *owner;
	int offsetx, offsety;

    	subdata = ((PySurfaceObject*)self)->subsurface;
	if(!subdata)
    	    	return Py_BuildValue("(ii)", 0, 0);

	subdata = ((PySurfaceObject*)self)->subsurface;
	owner = subdata->owner;
	offsetx = subdata->offsetx;
	offsety = subdata->offsety;

	while(((PySurfaceObject*)owner)->subsurface)
	{
	    subdata = ((PySurfaceObject*)owner)->subsurface;
    	    owner = subdata->owner;
	    offsetx += subdata->offsetx;
    	    offsety += subdata->offsety;
	}


	return Py_BuildValue("(ii)", offsetx, offsety);
}


static PyObject* surf_get_parent(PyObject* self, PyObject* args)
{
    	struct SubSurface_Data *subdata;
    	subdata = ((PySurfaceObject*)self)->subsurface;
	if(!subdata)
    	    	RETURN_NONE

    	Py_INCREF(subdata->owner);
	return subdata->owner;
}


static PyObject* surf_get_abs_parent(PyObject* self, PyObject* args)
{
    	struct SubSurface_Data *subdata;
	PyObject *owner;

    	subdata = ((PySurfaceObject*)self)->subsurface;
	if(!subdata)
	{
	    Py_INCREF(self);
	    return self;
	}

	subdata = ((PySurfaceObject*)self)->subsurface;
	owner = subdata->owner;

	while(((PySurfaceObject*)owner)->subsurface)
	{
	    subdata = ((PySurfaceObject*)owner)->subsurface;
    	    owner = subdata->owner;
	}

	Py_INCREF(owner);
	return owner;
}




static struct PyMethodDef surface_methods[] =
{
	{"get_at",			surf_get_at,		1, DOC_SURFACEGETAT },
	{"set_at",			surf_set_at,		1, DOC_SURFACESETAT },

	{"map_rgb",			surf_map_rgb,		1, DOC_SURFACEMAPRGB },
	{"unmap_rgb",		surf_unmap_rgb, 	1, DOC_SURFACEUNMAPRGB },

	{"get_palette", 	surf_get_palette,	1, DOC_SURFACEGETPALETTE },
	{"get_palette_at",	surf_get_palette_at,1, DOC_SURFACEGETPALETTEAT },
	{"set_palette", 	surf_set_palette,	1, DOC_SURFACESETPALETTE },
	{"set_palette_at",	surf_set_palette_at,1, DOC_SURFACESETPALETTEAT },

	{"lock",			surf_lock,			1, DOC_SURFACELOCK },
	{"unlock",			surf_unlock,		1, DOC_SURFACEUNLOCK },
	{"mustlock",		surf_mustlock,		1, DOC_SURFACEMUSTLOCK },
	{"get_locked",		surf_get_locked,	1, DOC_SURFACEGETLOCKED },

	{"set_colorkey",	surf_set_colorkey,	1, DOC_SURFACESETCOLORKEY },
	{"get_colorkey",	surf_get_colorkey,	1, DOC_SURFACEGETCOLORKEY },
	{"set_alpha",		surf_set_alpha, 	1, DOC_SURFACESETALPHA },
	{"get_alpha",		surf_get_alpha, 	1, DOC_SURFACEGETALPHA },

	{"copy",		surf_copy,		1, DOC_SURFACECOPY },
	{"__copy__",		surf_copy,		1, DOC_SURFACECOPY },
	{"convert",			surf_convert,		1, DOC_SURFACECONVERT },
	{"convert_alpha",	surf_convert_alpha,	1, DOC_SURFACECONVERTALPHA },

	{"set_clip",		surf_set_clip,		1, DOC_SURFACESETCLIP },
	{"get_clip",		surf_get_clip,		1, DOC_SURFACEGETCLIP },

	{"fill",			surf_fill,			1, DOC_SURFACEFILL },
	{"blit",			surf_blit,			1, DOC_SURFACEBLIT },

	{"get_flags",		surf_get_flags, 	1, DOC_SURFACEGETFLAGS },
	{"get_size",		surf_get_size,		1, DOC_SURFACEGETSIZE },
	{"get_width",		surf_get_width, 	1, DOC_SURFACEGETWIDTH },
	{"get_height",		surf_get_height,	1, DOC_SURFACEGETHEIGHT },
	{"get_rect",		(PyCFunction)surf_get_rect, METH_KEYWORDS, DOC_SURFACEGETRECT },
	{"get_pitch",		surf_get_pitch, 	1, DOC_SURFACEGETPITCH },
	{"get_bitsize", 	surf_get_bitsize,	1, DOC_SURFACEGETBITSIZE },
	{"get_bytesize",	surf_get_bytesize,	1, DOC_SURFACEGETBYTESIZE },
	{"get_masks",		surf_get_masks, 	1, DOC_SURFACEGETMASKS },
	{"get_shifts",		surf_get_shifts,	1, DOC_SURFACEGETSHIFTS },
	{"get_losses",		surf_get_losses,	1, DOC_SURFACEGETLOSSES },

	{"subsurface",		surf_subsurface,	1, DOC_SURFACESUBSURFACE },
	{"get_offset",		surf_get_offset,	1, DOC_SURFACEGETOFFSET },
	{"get_abs_offset",	surf_get_abs_offset,	1, DOC_SURFACEGETABSOFFSET },
	{"get_parent",		surf_get_parent,	1, DOC_SURFACEGETPARENT },
	{"get_abs_parent",	surf_get_abs_parent,	1, DOC_SURFACEGETABSPARENT },

	{NULL,		NULL}
};



/* surface object internals */
static void surface_cleanup(PySurfaceObject* self)
{
	if(self->surf) 
        {
            if(!(self->surf->flags&SDL_HWSURFACE) || SDL_WasInit(SDL_INIT_VIDEO))
            {
                    /*unsafe to free hardware surfaces without video init*/
                    /*i question SDL's ability to free a locked hardware surface*/
                    SDL_FreeSurface(self->surf);
            }
            self->surf = NULL;
        }
	if(self->subsurface)
	{
		Py_XDECREF(self->subsurface->owner);
		PyMem_Del(self->subsurface);
                self->subsurface = NULL;
	}
        if(self->dependency) {
            Py_DECREF(self->dependency);
            self->dependency = NULL;
        }
}


static void surface_dealloc(PyObject* self)
{
        if(((PySurfaceObject*)self)->weakreflist)
        {
            PyObject_ClearWeakRefs(self);
        }
	surface_cleanup((PySurfaceObject*)self);
	self->ob_type->tp_free(self);
}


PyObject* surface_str(PyObject* self)
{
	char str[1024];
	SDL_Surface* surf = PySurface_AsSurface(self);
	const char* type;

	if(surf)
	{
	    type = (surf->flags&SDL_HWSURFACE)?"HW":"SW";
	    sprintf(str, "<Surface(%dx%dx%d %s)>", surf->w, surf->h, surf->format->BitsPerPixel, type);
	}
	else
	{
	    strcpy(str, "<Surface(Dead Display)>");
	}

	return PyString_FromString(str);
}

  
static PyTypeObject PySurface_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,                              /*size*/
	"pygame.Surface",               /*name*/
	sizeof(PySurfaceObject),        /*basic size*/
	0,                              /*itemsize*/
	surface_dealloc,                /*dealloc*/
	0,                              /*print*/
	NULL,		/*getattr*/
	NULL,                           /*setattr*/
	NULL,                           /*compare*/
	surface_str,			/*repr*/
	NULL,                           /*as_number*/
	NULL,                           /*as_sequence*/
	NULL,                           /*as_mapping*/
	(hashfunc)NULL, 		/*hash*/
	(ternaryfunc)NULL,		/*call*/
	(reprfunc)NULL, 		/*str*/
	0,
        0L,0L,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	DOC_PYGAMESURFACE, /* Documentation string */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PySurfaceObject, weakreflist), /* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	surface_methods,			/* tp_methods */
	0,					/* tp_members */
	0,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)surface_init,			/* tp_init */
	0,					/* tp_alloc */
	surface_new,		                /* tp_new */
};


static PyObject* PySurface_New(SDL_Surface* s)
{
	PySurfaceObject* self;
	if(!s) return RAISE(PyExc_SDLError, SDL_GetError());

	self = (PySurfaceObject *)PySurface_Type.tp_new(&PySurface_Type, NULL, NULL);
        
	if(self)
	{
		self->surf = s;
	}
	return (PyObject*)self;
}

static PyObject* surface_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySurfaceObject *self;
    self = (PySurfaceObject *)type->tp_alloc(type, 0);
    if (self)
    {
        self->surf = NULL;
        self->subsurface = NULL;
        self->weakreflist = NULL;
    }
    return (PyObject *)self;
}

static intptr_t surface_init(PySurfaceObject *self, PyObject *args, PyObject *kwds)
{
	Uint32 flags = 0;
	int width, height;
	PyObject *depth=NULL, *masks=NULL;
	int bpp;
	Uint32 Rmask, Gmask, Bmask, Amask;
	SDL_Surface* surface;
	SDL_PixelFormat default_format;

	if(!PyArg_ParseTuple(args, "(ii)|iOO", &width, &height, &flags, &depth, &masks))
		return -1;
        
        if(width < 0 || height < 0)
        {
            RAISE(PyExc_SDLError, "Invalid resolution for Surface");
            return -1;
        }        
        
        surface_cleanup(self);
        
	if(depth && masks) /*all info supplied, most errorchecking needed*/
	{
		if(PySurface_Check(depth))
			return (intptr_t)RAISE(PyExc_ValueError, "cannot pass surface for depth and color masks");
		if(!IntFromObj(depth, &bpp))
			return (intptr_t)RAISE(PyExc_ValueError, "invalid bits per pixel depth argument");
		if(!PySequence_Check(masks) || PySequence_Length(masks)!=4)
			return (intptr_t)RAISE(PyExc_ValueError, "masks argument must be sequence of four numbers");
		if(!UintFromObjIndex(masks, 0, &Rmask) || !UintFromObjIndex(masks, 1, &Gmask) ||
					!UintFromObjIndex(masks, 2, &Bmask) || !UintFromObjIndex(masks, 3, &Amask))
			return (intptr_t)RAISE(PyExc_ValueError, "invalid mask values in masks sequence");
	}
	else if(depth && PyNumber_Check(depth))/*use default masks*/
	{
		if(!IntFromObj(depth, &bpp))
			return (intptr_t)RAISE(PyExc_ValueError, "invalid bits per pixel depth argument");
		if(flags & SDL_SRCALPHA)
		{
			switch(bpp)
			{
			case 16:
				Rmask = 0xF<<8; Gmask = 0xF<<4; Bmask = 0xF; Amask = 0xF<<12; break;
			case 32:
				Rmask = 0xFF<<16; Gmask = 0xFF<<8; Bmask = 0xFF; Amask = 0xFF<<24; break;
			default:
				return (intptr_t)RAISE(PyExc_ValueError, "no standard masks exist for given bitdepth with alpha");
			}
		}
		else
		{
			Amask = 0;
			switch(bpp)
			{
			case 8:
				Rmask = 0xFF>>6<<5; Gmask = 0xFF>>5<<2; Bmask = 0xFF>>6; break;
			case 12:
				Rmask = 0xFF>>4<<8; Gmask = 0xFF>>4<<4; Bmask = 0xFF>>4; break;
			case 15:
				Rmask = 0xFF>>3<<10; Gmask = 0xFF>>3<<5; Bmask = 0xFF>>3; break;
			case 16:
				Rmask = 0xFF>>3<<11; Gmask = 0xFF>>2<<5; Bmask = 0xFF>>3; break;
			case 24:
			case 32:
				Rmask = 0xFF<<16; Gmask = 0xFF<<8; Bmask = 0xFF; break;
			default:
				return (intptr_t)RAISE(PyExc_ValueError, "nonstandard bit depth given");
			}
		}
	}
	else /*no depth or surface*/
	{
		SDL_PixelFormat* pix;
		if(depth && PySurface_Check(depth))
			pix = ((PySurfaceObject*)depth)->surf->format;
		else if(SDL_GetVideoSurface())
			pix = SDL_GetVideoSurface()->format;
		else if(SDL_WasInit(SDL_INIT_VIDEO))
			pix = SDL_GetVideoInfo()->vfmt;
		else
		{
			pix = &default_format;
			pix->BitsPerPixel = 32; pix->Amask = 0;
			pix->Rmask = 0xFF0000; pix->Gmask = 0xFF00; pix->Bmask = 0xFF;
		}
		bpp = pix->BitsPerPixel;
		Rmask = pix->Rmask;
		Gmask = pix->Gmask;
		Bmask = pix->Bmask;
		Amask = pix->Amask;
	}
	surface = SDL_CreateRGBSurface(flags, width, height, bpp, Rmask, Gmask, Bmask, Amask);
        if(!surface)
        {
            RAISE(PyExc_SDLError, SDL_GetError());
            return -1;
        }
        
	if(surface)
	{
		self->surf = surface;
		self->subsurface = NULL;
	}

        return 0;
}


static PyMethodDef surface_builtins[] =
{
	{ NULL, NULL }
};


PYGAME_EXPORT
void initsurface(void)
{
	PyObject *module, *dict, *apiobj, *lockmodule;
	static void* c_api[PYGAMEAPI_SURFACE_NUMSLOTS];

        if (PyType_Ready(&PySurface_Type) < 0)
            return;
        
    /* create the module */
	module = Py_InitModule3("surface", surface_builtins, DOC_PYGAMESURFACE);
	dict = PyModule_GetDict(module);

	PyDict_SetItemString(dict, "SurfaceType", (PyObject *)&PySurface_Type);
	PyDict_SetItemString(dict, "Surface", (PyObject *)&PySurface_Type);

	/* export the c api */
	c_api[0] = &PySurface_Type;
	c_api[1] = PySurface_New;
	c_api[2] = PySurface_Blit;
	apiobj = PyCObject_FromVoidPtr(c_api, NULL);
	PyDict_SetItemString(dict, PYGAMEAPI_LOCAL_ENTRY, apiobj);
	Py_DECREF(apiobj);
        Py_INCREF(PySurface_Type.tp_dict);
        PyDict_SetItemString(dict, "_dict", PySurface_Type.tp_dict);
        
	/*imported needed apis*/
	import_pygame_base();
	import_pygame_rect();

	/*import the surflock module manually*/
	lockmodule = PyImport_ImportModule("pygame.surflock");
	if(lockmodule != NULL)
	{
		PyObject *dict = PyModule_GetDict(lockmodule);
		PyObject *c_api = PyDict_GetItemString(dict, PYGAMEAPI_LOCAL_ENTRY);
		if(PyCObject_Check(c_api))
		{
			int i; void** localptr = (void*)PyCObject_AsVoidPtr(c_api);
			for(i = 0; i < PYGAMEAPI_SURFLOCK_NUMSLOTS; ++i)
				PyGAME_C_API[i + PYGAMEAPI_SURFLOCK_FIRSTSLOT] = localptr[i];
		}
		Py_DECREF(lockmodule);
	}
}

