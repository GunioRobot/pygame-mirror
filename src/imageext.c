/*
    pygame - Python Game Library
    Copyright (C) 2000-2001  Pete Shinners
    Copyright (C) 2006 Rene Dudfield

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
 *  extended image module for pygame, note this only has
 *  the extended load and save functions, which are autmatically used
 *  by the normal pygame.image module if it is available.
 */
#include <png.h>
#include <jpeglib.h>
#include "pygame.h"
#include "pygamedocs.h"
#include <SDL_image.h>

SDL_Surface* opengltosdl(void);



static char* find_extension(char* fullname)
{
	char* dot;

	if(!fullname)
		return NULL;

	dot = strrchr(fullname, '.');
	if(!dot)
		return fullname;

	return dot+1;
}



static PyObject* image_load_ext(PyObject* self, PyObject* arg)
{
	PyObject* file, *final;
	char* name = NULL;
	SDL_Surface* surf;
	SDL_RWops *rw;
	if(!PyArg_ParseTuple(arg, "O|s", &file, &name))
		return NULL;
	if(PyString_Check(file) || PyUnicode_Check(file))
	{
		if(!PyArg_ParseTuple(arg, "s|O", &name, &file))
			return NULL;
		Py_BEGIN_ALLOW_THREADS
		surf = IMG_Load(name);
		Py_END_ALLOW_THREADS
	}
	else
	{
		if(!name && PyFile_Check(file))
			name = PyString_AsString(PyFile_Name(file));

		if(!(rw = RWopsFromPython(file)))
			return NULL;
		if(RWopsCheckPython(rw))
                {
			surf = IMG_LoadTyped_RW(rw, 1, find_extension(name));
                }
		else
		{
			Py_BEGIN_ALLOW_THREADS
			surf = IMG_LoadTyped_RW(rw, 1, find_extension(name));
			Py_END_ALLOW_THREADS
		}
	}

	if(!surf)
		return RAISE(PyExc_SDLError, IMG_GetError());

	final = PySurface_New(surf);
	if(!final)
		SDL_FreeSurface(surf);
	return final;
}







#ifdef PNG_H

int write_png(char *file_name, png_bytep *rows, int w, int h, int colortype, int bitdepth) {

    png_structp png_ptr;
    png_infop info_ptr;
    FILE *fp = NULL;
    char *doing = "open for writing";

    if (!(fp = fopen(file_name, "wb"))) goto fail;

    doing = "create png write struct";
    if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) goto fail;

    doing = "create png info struct";
    if (!(info_ptr = png_create_info_struct(png_ptr))) goto fail;
    if (setjmp(png_jmpbuf(png_ptr))) goto fail;

    doing = "init IO";
    png_init_io(png_ptr, fp);

    doing = "write header";
    png_set_IHDR(png_ptr, info_ptr, w, h, bitdepth, colortype, 
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, 
                 PNG_FILTER_TYPE_BASE);

    doing = "write info";
    png_write_info(png_ptr, info_ptr);

    doing = "write image";
    png_write_image(png_ptr, rows);

    doing = "write end";
    png_write_end(png_ptr, NULL);

    doing = "closing file";
    if(0 != fclose(fp)) goto fail;

    return 0;

    fail:
    SDL_SetError("SavePNG: could not %s", doing);

    return -1;
}




int SavePNG(SDL_Surface *surface, char *file) {

    static unsigned char** ss_rows;
    static int ss_size;
    static int ss_w, ss_h;
    SDL_Surface *ss_surface;
    SDL_Rect ss_rect;
    int r, i;
    int alpha = 0;
    int pixel_bits = 32;

    unsigned surf_flags;
    unsigned surf_alpha;

    ss_rows = 0;
    ss_size = 0;
    ss_surface = NULL;


    ss_w = surface->w;
    ss_h = surface->h;




    if(surface->format->Amask) {
        alpha = 1;
        pixel_bits = 32;
    } else {
        pixel_bits = 24;
    }


    ss_surface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, ss_w, ss_h, pixel_bits,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                           0xff0000, 0xff00, 0xff, 0x000000ff
#else
                                           0xff, 0xff00, 0xff0000, 0xff000000
#endif
                );

    if(ss_surface == NULL) {
        return -1;
    }


    surf_flags = surface->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY);
    surf_alpha = surface->format->alpha;
    if(surf_flags & SDL_SRCALPHA)
	SDL_SetAlpha(surface, 0, 255);
    if(surf_flags & SDL_SRCCOLORKEY)
	SDL_SetColorKey(surface, 0, surface->format->colorkey);


    ss_rect.x = 0;
    ss_rect.y = 0;
    ss_rect.w = ss_w;
    ss_rect.h = ss_h;
    SDL_BlitSurface(surface, &ss_rect, ss_surface, NULL);

    if(ss_size == 0) {
        ss_size = ss_h;
        ss_rows = (unsigned char**)malloc(sizeof(unsigned char*) * ss_size);
        if(ss_rows == NULL) {
            return -1;
        }
    }
    if(surf_flags & SDL_SRCALPHA)
	SDL_SetAlpha(surface, SDL_SRCALPHA, (Uint8)surf_alpha);
    if(surf_flags & SDL_SRCCOLORKEY)
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, surface->format->colorkey);


    for(i = 0; i < ss_h; i++) {
        ss_rows[i] = ((unsigned char*)ss_surface->pixels) + i * ss_surface->pitch;
    }

    if(alpha) {
        r = write_png(file, ss_rows, surface->w, surface->h, PNG_COLOR_TYPE_RGB_ALPHA, 8);
    } else {
        r = write_png(file, ss_rows, surface->w, surface->h, PNG_COLOR_TYPE_RGB, 8);
    }

    free(ss_rows);
    SDL_FreeSurface(ss_surface);
    ss_surface = NULL;

    return r;
}



#endif /* end if PNG_H */









#ifdef JPEGLIB_H

int write_jpeg(char *file_name, unsigned char** image_buffer, 
                   int image_width, int image_height, int quality) {

  struct jpeg_compress_struct cinfo;

  struct jpeg_error_mgr jerr;
  FILE * outfile;
  JSAMPROW row_pointer[1];
  int row_stride;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  if ((outfile = fopen(file_name, "wb")) == NULL) {
    SDL_SetError("SaveJPEG: could not open %s", file_name);
    return -1;
  }
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = image_width;
  cinfo.image_height = image_height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  
  jpeg_set_defaults(&cinfo);

  jpeg_set_quality(&cinfo, quality, TRUE );


  jpeg_start_compress(&cinfo, TRUE);
  row_stride = image_width * 3;

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = image_buffer[cinfo.next_scanline];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(outfile);
  jpeg_destroy_compress(&cinfo);
  return 0;
}


int SaveJPEG(SDL_Surface *surface, char *file) {
    static unsigned char** ss_rows;
    static int ss_size;
    static int ss_w, ss_h;
    SDL_Surface *ss_surface;
    SDL_Rect ss_rect;
    int r, i;
    int alpha = 0;
    int pixel_bits = 32;

    ss_rows = 0;
    ss_size = 0;
    ss_surface = NULL;


    ss_w = surface->w;
    ss_h = surface->h;


    alpha = 0;
    pixel_bits = 24;

    ss_surface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, ss_w, ss_h, pixel_bits,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                           0xff0000, 0xff00, 0xff, 0x000000ff
#else
                                           0xff, 0xff00, 0xff0000, 0xff000000
#endif
                );

    if(ss_surface == NULL) {
        return -1;
    }

    ss_rect.x = 0;
    ss_rect.y = 0;
    ss_rect.w = ss_w;
    ss_rect.h = ss_h;
    SDL_BlitSurface(surface, &ss_rect, ss_surface, NULL);

    if(ss_size == 0) {
        ss_size = ss_h;
        ss_rows = (unsigned char**)malloc(sizeof(unsigned char*) * ss_size);
        if(ss_rows == NULL) {
            return -1;
        }
    }

    for(i = 0; i < ss_h; i++) {
        ss_rows[i] = ((unsigned char*)ss_surface->pixels) + i * ss_surface->pitch;
    }

    r = write_jpeg(file, ss_rows, surface->w, surface->h, 85);


    free(ss_rows);
    SDL_FreeSurface(ss_surface);
    ss_surface = NULL;

    return r;
}

#endif /* end if JPEGLIB_H */








/* NOTE XX HACK TODO FIXME: this opengltosdl is also in image.c  
 need to share it between both.
*/


SDL_Surface* opengltosdl(void)
{
        /*we need to get ahold of the pyopengl glReadPixels function*/
        /*we use pyopengl's so we don't need to link with opengl at compiletime*/
        PyObject *pyopengl, *readpixels = NULL;
        int typeflag=0, formatflag=0;
        SDL_Surface *surf;
        Uint32 rmask, gmask, bmask;
        int i;
        unsigned char *pixels;
        PyObject *data;

        surf = SDL_GetVideoSurface();

        pyopengl = PyImport_ImportModule("OpenGL.GL");
        if(pyopengl)
        {
                PyObject* dict = PyModule_GetDict(pyopengl);
                if(dict)
                {
                        PyObject *o;
                        o = PyDict_GetItemString(dict, "GL_RGB");
                        if(!o) {Py_DECREF(pyopengl); return NULL;}
                        formatflag = PyInt_AsLong(o);
                        o = PyDict_GetItemString(dict, "GL_UNSIGNED_BYTE");
                        if(!o) {Py_DECREF(pyopengl); return NULL;}
                        typeflag = PyInt_AsLong(o);
                        readpixels = PyDict_GetItemString(dict, "glReadPixels");
                        if(!readpixels) {Py_DECREF(pyopengl); return NULL;}
                }
                Py_DECREF(pyopengl);
        }
        else
        {
            RAISE(PyExc_ImportError, "Cannot import PyOpenGL");
            return NULL;
        }

        data = PyObject_CallFunction(readpixels, "iiiiii",
                                0, 0, surf->w, surf->h, formatflag, typeflag);
        if(!data)
        {
                RAISE(PyExc_SDLError, "glReadPixels returned NULL");
                return NULL;
        }
        pixels = (unsigned char*)PyString_AsString(data);

        if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
        {
            rmask=0x000000FF; gmask=0x0000FF00; bmask=0x00FF0000;
        }
        else
        {
            rmask=0x00FF0000; gmask=0x0000FF00; bmask=0x000000FF;
        }
        surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surf->w, surf->h, 24,
                    rmask, gmask, bmask, 0);
        if(!surf)
        {
                Py_DECREF(data);
                RAISE(PyExc_SDLError, SDL_GetError());
                return NULL;
        }

        for(i=0; i<surf->h; ++i)
                memcpy(((char *) surf->pixels) + surf->pitch * i, pixels + 3*surf->w * (surf->h-i-1), surf->w*3);

        Py_DECREF(data);
        return surf;
}



static PyObject* image_save_ext(PyObject* self, PyObject* arg)
{

	PyObject* surfobj, *file;
	SDL_Surface *surf;
	SDL_Surface *temp = NULL;
	int result;

	if(!PyArg_ParseTuple(arg, "O!O", &PySurface_Type, &surfobj, &file))
		return NULL;
	surf = PySurface_AsSurface(surfobj);

	if(surf->flags & SDL_OPENGL)
	{
                temp = surf = opengltosdl();
                if(!surf)
                    return NULL;
	}
	else
		PySurface_Prep(surfobj);



	if(PyString_Check(file) || PyUnicode_Check(file))
	{
		int namelen;
		char* name;
		if(!PyArg_ParseTuple(arg, "O|s", &file, &name))
			return NULL;
                namelen = strlen(name);
		Py_BEGIN_ALLOW_THREADS
                if((namelen > 3) && (
                          ((name[namelen-1]=='g' || name[namelen-1]=='G') &&
                          (name[namelen-2]=='e' || name[namelen-2]=='E')) ||
                          ((name[namelen-1]=='g' || name[namelen-1]=='G') &&
                           (name[namelen-2]=='p' || name[namelen-2]=='P'))
                          )

                          ) {
#ifdef JPEGLIB_H
		    result = SaveJPEG(surf, name);
#else
                    return RAISE(PyExc_SDLError, "No support for jpg compiled in.");
#endif

                }
                else if(name[namelen-1]=='g' || name[namelen-1]=='G') {
#ifdef PNG_H
		    result = SavePNG(surf, name);
#else
                    return RAISE(PyExc_SDLError, "No support for png compiled in.");
#endif
                }

                else {
                    result = -1;
                }
		Py_END_ALLOW_THREADS
                /*else if(name[namelen-1]=='g' || name[namelen-1]=='G')
		    result = SavePNG(surf, name);
                else
                    result = SaveTGA(surf, name, 1);
                */
	}
	else
	{
		return NULL;
	}

	if(temp)
		SDL_FreeSurface(temp);
	else
		PySurface_Unprep(surfobj);

	if(result == -1)
		return RAISE(PyExc_SDLError, SDL_GetError());


	RETURN_NONE


}





static PyMethodDef image_builtins[] =
{
	{ "load_extended", image_load_ext, 1, DOC_PYGAMEIMAGE },
	{ "save_extended", image_save_ext, 1, DOC_PYGAMEIMAGE },

	{ NULL, NULL }
};



PYGAME_EXPORT
void initimageext(void)
{
    /* create the module */
	Py_InitModule3("imageext", image_builtins, NULL);

	/*imported needed apis*/
	import_pygame_base();
	import_pygame_surface();
	import_pygame_rwobject();
}

