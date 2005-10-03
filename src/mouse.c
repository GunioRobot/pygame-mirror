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
 *  pygame mouse module
 */
#include "pygame.h"
#include "pygamedocs.h"


/* mouse module functions */

static PyObject* mouse_set_pos(PyObject* self, PyObject* args)
{
	int x, y;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "invalid position argument for set_pos");

	VIDEO_INIT_CHECK();

	SDL_WarpMouse((Uint16)x, (Uint16)y);

	RETURN_NONE
}


static PyObject* mouse_get_pos(PyObject* self, PyObject* args)
{
	int x, y;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	VIDEO_INIT_CHECK();

	SDL_GetMouseState(&x, &y);
	return Py_BuildValue("(ii)", x, y);
}


static PyObject* mouse_get_rel(PyObject* self, PyObject* args)
{
	int x, y;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	SDL_GetRelativeMouseState(&x, &y);
	return Py_BuildValue("(ii)", x, y);
}


static PyObject* mouse_get_pressed(PyObject* self, PyObject* args)
{
	PyObject* tuple;
	int state;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	state = SDL_GetMouseState(NULL, NULL);
	if(!(tuple = PyTuple_New(3)))
		return NULL;

	PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong((state&SDL_BUTTON(1)) != 0));
	PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((state&SDL_BUTTON(2)) != 0));
	PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((state&SDL_BUTTON(3)) != 0));

	return tuple;
}


static PyObject* mouse_set_visible(PyObject* self, PyObject* args)
{
	int toggle;

	if(!PyArg_ParseTuple(args, "i", &toggle))
		return NULL;
	VIDEO_INIT_CHECK();

	toggle = SDL_ShowCursor(toggle);
	return PyInt_FromLong(toggle);
}


static PyObject* mouse_get_focused(PyObject* self, PyObject* args)
{
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	return PyInt_FromLong((SDL_GetAppState()&SDL_APPMOUSEFOCUS) != 0);
}


static PyObject* mouse_set_cursor(PyObject* self, PyObject* args)
{
	int w, h, spotx, spoty;
	PyObject *xormask, *andmask;
	Uint8 *xordata=NULL, *anddata=NULL;
	int xorsize, andsize, loop;
	int val;
	SDL_Cursor *lastcursor, *cursor = NULL;

	if(!PyArg_ParseTuple(args, "(ii)(ii)OO", &w, &h, &spotx, &spoty, &xormask, &andmask))
		return NULL;

	VIDEO_INIT_CHECK();

	if(!PySequence_Check(xormask) || !PySequence_Check(andmask))
		return RAISE(PyExc_TypeError, "xormask and andmask must be sequences");

	if(w % 8)
		return RAISE(PyExc_ValueError, "Cursor width must be divisible by 8.");

	xorsize = PySequence_Length(xormask);
	andsize = PySequence_Length(andmask);

	if(xorsize != w*h/8 || andsize != w*h/8)
		return RAISE(PyExc_ValueError, "bitmasks must be sized width*height/8");

	xordata = (Uint8*)malloc(xorsize);
	anddata = (Uint8*)malloc(andsize);

	for(loop = 0; loop < xorsize; ++loop)
	{
		if(!IntFromObjIndex(xormask, loop, &val))
			goto interror;
		xordata[loop] = (Uint8)val;
		if(!IntFromObjIndex(andmask, loop, &val))
			goto interror;
		anddata[loop] = (Uint8)val;
	}

	cursor = SDL_CreateCursor(xordata, anddata, w, h, spotx, spoty);
	free(xordata);
	free(anddata);
	xordata = NULL;
	anddata = NULL;

	if(!cursor)
		return RAISE(PyExc_SDLError, SDL_GetError());

	lastcursor = SDL_GetCursor();
	SDL_SetCursor(cursor);
	SDL_FreeCursor(lastcursor);

	RETURN_NONE;

interror:
	if(xordata) free(xordata);
	if(anddata) free(anddata);
	return RAISE(PyExc_TypeError, "Invalid number in mask array");
}


static PyObject* mouse_get_cursor(PyObject* self, PyObject* args)
{
	SDL_Cursor *cursor = NULL;
	PyObject* xordata, *anddata;
	int size, loop, w, h, spotx, spoty;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	cursor = SDL_GetCursor();
	if(!cursor)
		return RAISE(PyExc_SDLError, SDL_GetError());

	w = cursor->area.w;
	h = cursor->area.h;
	spotx = cursor->hot_x;
	spoty = cursor->hot_y;

	size = cursor->area.w * cursor->area.h / 8;
	xordata = PyTuple_New(size);
	if(!xordata)
		return NULL;
	anddata = PyTuple_New(size);
	if(!anddata)
	{
		Py_DECREF(anddata);
		return NULL;
	}

	for(loop = 0; loop < size; ++loop)
	{
		PyTuple_SET_ITEM(xordata, loop, PyInt_FromLong(cursor->data[loop]));
		PyTuple_SET_ITEM(anddata, loop, PyInt_FromLong(cursor->mask[loop]));
	}

	return Py_BuildValue("((ii)(ii)OO)", w, h, spotx, spoty, xordata, anddata);
}



static PyMethodDef mouse_builtins[] =
{
	{ "set_pos", mouse_set_pos, 1, DOC_PYGAMEMOUSESETPOS },
	{ "get_pos", mouse_get_pos, 1, DOC_PYGAMEMOUSEGETPOS },
	{ "get_rel", mouse_get_rel, 1, DOC_PYGAMEMOUSEGETREL },
	{ "get_pressed", mouse_get_pressed, 1, DOC_PYGAMEMOUSEGETPRESSED },
	{ "set_visible", mouse_set_visible, 1, DOC_PYGAMEMOUSESETVISIBLE },
	{ "get_focused", mouse_get_focused, 1, DOC_PYGAMEMOUSEGETFOCUSED },
	{ "set_cursor", mouse_set_cursor, 1, DOC_PYGAMEMOUSESETCURSOR },
	{ "get_cursor", mouse_get_cursor, 1, DOC_PYGAMEMOUSEGETCURSOR },

	{ NULL, NULL }
};


PYGAME_EXPORT
void initmouse(void)
{
	PyObject *module, *dict;

    /* create the module */
	module = Py_InitModule3("mouse", mouse_builtins, DOC_PYGAMEMOUSE);
	dict = PyModule_GetDict(module);

	/*imported needed apis*/
	import_pygame_base();
}

