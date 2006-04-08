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
 *  pygame event module
 */
#define PYGAMEAPI_EVENT_INTERNAL
#include "pygame.h"
#include "pygamedocs.h"

// FIXME: The system message code is only tested on windows, so only
//          include it there for now.
#include <SDL_syswm.h>

/*this user event object is for safely passing
 *objects through the event queue.
 */

#define USEROBJECT_CHECK1 0xDEADBEEF
#define USEROBJECT_CHECK2 0xFEEDF00D

typedef struct UserEventObject
{
	struct UserEventObject* next;
	PyObject* object;
}UserEventObject;

static UserEventObject* user_event_objects = NULL;


/*must pass dictionary as this object*/
static UserEventObject* user_event_addobject(PyObject* obj)
{
	UserEventObject* userobj = PyMem_New(UserEventObject, 1);
	if(!userobj) return NULL;

	Py_INCREF(obj);
	userobj->next = user_event_objects;
	userobj->object = obj;
	user_event_objects = userobj;

	return userobj;
}

/*note, we doublecheck to make sure the pointer is in our list,
 *not just some random pointer. this will keep us safe(r).
 */
static PyObject* user_event_getobject(UserEventObject* userobj)
{
	PyObject* obj = NULL;
	if(!user_event_objects) /*fail in most common case*/
		return NULL;
	if(user_event_objects == userobj)
	{
		obj = userobj->object;
		user_event_objects = userobj->next;
	}
	else
	{
		UserEventObject* hunt = user_event_objects;
		while(hunt && hunt->next != userobj)
			hunt = hunt->next;
		if(hunt)
		{
			hunt->next = userobj->next;
			obj = userobj->object;
		}
	}
	if(obj)
		PyMem_Del(userobj);
	return obj;
}


static void user_event_cleanup(void)
{
	if(user_event_objects)
	{
		UserEventObject *hunt, *kill;
		hunt = user_event_objects;
		while(hunt)
		{
			kill = hunt;
			hunt = hunt->next;
			Py_DECREF(kill->object);
			PyMem_Del(kill);
		}
		user_event_objects = NULL;
	}
}

static int PyEvent_FillUserEvent(PyEventObject *e, SDL_Event *event) {
	UserEventObject *userobj = user_event_addobject(e->dict);
	if(!userobj)
		return -1;

	event->type = e->type;
	event->user.code = USEROBJECT_CHECK1;
	event->user.data1 = (void*)USEROBJECT_CHECK2;
	event->user.data2 = userobj;
    return 0;
}

staticforward PyTypeObject PyEvent_Type;
static PyObject* PyEvent_New(SDL_Event*);
static PyObject* PyEvent_New2(int, PyObject*);
#define PyEvent_Check(x) ((x)->ob_type == &PyEvent_Type)



static char* name_from_eventtype(int type)
{
	switch(type)
	{
	case SDL_ACTIVEEVENT:	return "ActiveEvent";
	case SDL_KEYDOWN:		return "KeyDown";
	case SDL_KEYUP: 		return "KeyUp";
	case SDL_MOUSEMOTION:	return "MouseMotion";
	case SDL_MOUSEBUTTONDOWN:return "MouseButtonDown";
	case SDL_MOUSEBUTTONUP: return "MouseButtonUp";
	case SDL_JOYAXISMOTION: return "JoyAxisMotion";
	case SDL_JOYBALLMOTION: return "JoyBallMotion";
	case SDL_JOYHATMOTION:	return "JoyHatMotion";
	case SDL_JOYBUTTONUP:	return "JoyButtonUp";
	case SDL_JOYBUTTONDOWN: return "JoyButtonDown";
	case SDL_QUIT:			return "Quit";
	case SDL_SYSWMEVENT:	return "SysWMEvent";
	case SDL_VIDEORESIZE:	return "VideoResize";
	case SDL_VIDEOEXPOSE:	return "VideoExpose";
	case SDL_NOEVENT:		return "NoEvent";
	}
	if(type >= SDL_USEREVENT && type < SDL_NUMEVENTS)
		return "UserEvent";

	return "Unknown";
}


/* Helper for adding objects to dictionaries. Check for errors with
   PyErr_Occurred() */
static void insobj(PyObject *dict, char *name, PyObject *v)
{
	if(v)
	{
		PyDict_SetItemString(dict, name, v);
		Py_DECREF(v);
	}
}

#ifdef Py_USING_UNICODE

static PyObject* our_unichr(long uni)
{
	static PyObject* bltin_unichr = NULL;

	if (bltin_unichr == NULL) {
		PyObject* bltins;

		bltins = PyImport_ImportModule("__builtin__");
		bltin_unichr = PyObject_GetAttrString(bltins, "unichr");

		Py_DECREF(bltins);
	}

	return PyEval_CallFunction(bltin_unichr, "(l)", uni);
}

static PyObject* our_empty_ustr(void)
{
	static PyObject* empty_ustr = NULL;

	if (empty_ustr == NULL) {
		PyObject* bltins;
		PyObject* bltin_unicode;

		bltins = PyImport_ImportModule("__builtin__");
		bltin_unicode = PyObject_GetAttrString(bltins, "unicode");
		empty_ustr = PyEval_CallFunction(bltin_unicode, "(s)", "");

		Py_DECREF(bltin_unicode);
		Py_DECREF(bltins);
	}

	Py_INCREF(empty_ustr);

	return empty_ustr;
}

#else

static PyObject* our_unichr(long uni)
{
	return PyInt_FromLong(uni);
}

static PyObject* our_empty_ustr(void)
{
	return PyInt_FromLong(0);
}

#endif /* Py_USING_UNICODE */

static PyObject* dict_from_event(SDL_Event* event)
{
	PyObject *dict=NULL, *tuple, *obj;
	int hx, hy;

	/*check if it is an event the user posted*/
	if(event->user.code == USEROBJECT_CHECK1 && event->user.data1 == (void*)USEROBJECT_CHECK2)
	{
		dict = user_event_getobject((UserEventObject*)event->user.data2);
		if(dict)
			return dict;
	}

	if(!(dict = PyDict_New()))
		return NULL;
	switch(event->type)
	{
	case SDL_ACTIVEEVENT:
		insobj(dict, "gain", PyInt_FromLong(event->active.gain));
		insobj(dict, "state", PyInt_FromLong(event->active.state));
		break;
	case SDL_KEYDOWN:
		if(event->key.keysym.unicode)
			insobj(dict, "unicode", our_unichr(event->key.keysym.unicode));
		else
			insobj(dict, "unicode", our_empty_ustr());
	case SDL_KEYUP:
		insobj(dict, "key", PyInt_FromLong(event->key.keysym.sym));
		insobj(dict, "mod", PyInt_FromLong(event->key.keysym.mod));
		break;
	case SDL_MOUSEMOTION:
		obj = Py_BuildValue("(ii)", event->motion.x, event->motion.y);
		insobj(dict, "pos", obj);
		obj = Py_BuildValue("(ii)", event->motion.xrel, event->motion.yrel);
		insobj(dict, "rel", obj);
		if((tuple = PyTuple_New(3)))
		{
			PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong((event->motion.state&SDL_BUTTON(1)) != 0));
			PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((event->motion.state&SDL_BUTTON(2)) != 0));
			PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((event->motion.state&SDL_BUTTON(3)) != 0));
			insobj(dict, "buttons", tuple);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		obj = Py_BuildValue("(ii)", event->button.x, event->button.y);
		insobj(dict, "pos", obj);
		insobj(dict, "button", PyInt_FromLong(event->button.button));
		break;
	case SDL_JOYAXISMOTION:
		insobj(dict, "joy", PyInt_FromLong(event->jaxis.which));
		insobj(dict, "axis", PyInt_FromLong(event->jaxis.axis));
		insobj(dict, "value", PyFloat_FromDouble(event->jaxis.value/32767.0));
		break;
	case SDL_JOYBALLMOTION:
		insobj(dict, "joy", PyInt_FromLong(event->jball.which));
		insobj(dict, "ball", PyInt_FromLong(event->jball.ball));
		obj = Py_BuildValue("(ii)", event->jball.xrel, event->jball.yrel);
		insobj(dict, "rel", obj);
		break;
	case SDL_JOYHATMOTION:
		insobj(dict, "joy", PyInt_FromLong(event->jhat.which));
		insobj(dict, "hat", PyInt_FromLong(event->jhat.hat));
		hx = hy = 0;
		if(event->jhat.value&SDL_HAT_UP) hy = 1;
		else if(event->jhat.value&SDL_HAT_DOWN) hy = -1;
		if(event->jhat.value&SDL_HAT_RIGHT) hx = 1;
		else if(event->jhat.value&SDL_HAT_LEFT) hx = -1;
		insobj(dict, "value", Py_BuildValue("(ii)", hx, hy));
		break;
	case SDL_JOYBUTTONUP:
	case SDL_JOYBUTTONDOWN:
		insobj(dict, "joy", PyInt_FromLong(event->jbutton.which));
		insobj(dict, "button", PyInt_FromLong(event->jbutton.button));
		break;
	case SDL_VIDEORESIZE:
		obj = Py_BuildValue("(ii)", event->resize.w, event->resize.h);
		insobj(dict, "size", obj);
		insobj(dict, "w", PyInt_FromLong(event->resize.w));
		insobj(dict, "h", PyInt_FromLong(event->resize.h));
		break;
	case SDL_SYSWMEVENT:
	        #ifdef WIN32
		insobj(dict, "hwnd", PyInt_FromLong((long)(event-> syswm.msg->hwnd)));
		insobj(dict, "msg", PyInt_FromLong(event-> syswm.msg->msg));
		insobj(dict, "wparam", PyInt_FromLong(event-> syswm.msg->wParam));
		insobj(dict, "lparam", PyInt_FromLong(event-> syswm.msg->lParam));
		#endif
                /*
                 * Make the event
                 */
#if (defined(unix) || defined(__unix__) || defined(_AIX) || defined(__OpenBSD__)) && \
    (!defined(DISABLE_X11) && !defined(__CYGWIN32__) && !defined(ENABLE_NANOX) && \
     !defined(__QNXNTO__))

                //printf("asdf :%d:", event->syswm.msg->event.xevent.type);
                insobj(dict, 
                       "event",
                       PyString_FromStringAndSize((char*)&(event->syswm.msg->event.xevent),
                       sizeof(XEvent)));
                #endif

		break;
/* SDL_VIDEOEXPOSE and SDL_QUIT have no attributes */
	}
	if(event->type >= SDL_USEREVENT && event->type < SDL_NUMEVENTS)
	{
		insobj(dict, "code", PyInt_FromLong(event->user.code));
	}

	return dict;
}




/* event object internals */

static void event_dealloc(PyObject* self)
{
	PyEventObject* e = (PyEventObject*)self;
	Py_XDECREF(e->dict);
	PyObject_DEL(self);
}


static PyObject *event_getattr(PyObject *self, char *name)
{
	PyEventObject* e = (PyEventObject*)self;
	PyObject* item;

	if(!strcmp(name, "type"))
		return PyInt_FromLong(e->type);

	if(!strcmp(name, "dict"))
	{
		Py_INCREF(e->dict);
		return e->dict;
	}

	item = PyDict_GetItemString(e->dict, name);
	if(item)
		Py_INCREF(item);
	else
		RAISE(PyExc_AttributeError, "event member not defined");
	return item;
}


PyObject* event_str(PyObject* self)
{
	PyEventObject* e = (PyEventObject*)self;
	char str[1024];
	PyObject *strobj;

	strobj = PyObject_Str(e->dict);
	sprintf(str, "<Event(%d-%s %s)>", e->type, name_from_eventtype(e->type),
				PyString_AsString(strobj));

	Py_DECREF(strobj);
	return PyString_FromString(str);
}


static int event_nonzero(PyEventObject *self)
{
	return self->type != SDL_NOEVENT;
}

static PyNumberMethods event_as_number = {
	(binaryfunc)NULL,		/*add*/
	(binaryfunc)NULL,		/*subtract*/
	(binaryfunc)NULL,		/*multiply*/
	(binaryfunc)NULL,		/*divide*/
	(binaryfunc)NULL,		/*remainder*/
	(binaryfunc)NULL,		/*divmod*/
	(ternaryfunc)NULL,		/*power*/
	(unaryfunc)NULL,		/*negative*/
	(unaryfunc)NULL,		/*pos*/
	(unaryfunc)NULL,		/*abs*/
	(inquiry)event_nonzero,	/*nonzero*/
	(unaryfunc)NULL,		/*invert*/
	(binaryfunc)NULL,		/*lshift*/
	(binaryfunc)NULL,		/*rshift*/
	(binaryfunc)NULL,		/*and*/
	(binaryfunc)NULL,		/*xor*/
	(binaryfunc)NULL,		/*or*/
	(coercion)NULL,			/*coerce*/
	(unaryfunc)NULL,		/*int*/
	(unaryfunc)NULL,		/*long*/
	(unaryfunc)NULL,		/*float*/
	(unaryfunc)NULL,		/*oct*/
	(unaryfunc)NULL,		/*hex*/
};


static PyTypeObject PyEvent_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,						/*size*/
	"Event",				/*name*/
	sizeof(PyEventObject),	/*basic size*/
	0,						/*itemsize*/
	event_dealloc,			/*dealloc*/
	0,						/*print*/
	event_getattr,			/*getattr*/
	NULL,					/*setattr*/
	NULL,					/*compare*/
	event_str,				/*repr*/
	&event_as_number,		/*as_number*/
	NULL,					/*as_sequence*/
	NULL,					/*as_mapping*/
	(hashfunc)NULL, 		/*hash*/
	(ternaryfunc)NULL,		/*call*/
	(reprfunc)NULL, 		/*str*/
	0L,0L,0L,0L,
	DOC_PYGAMEEVENTEVENT /* Documentation string */
};



static PyObject* PyEvent_New(SDL_Event* event)
{
	PyEventObject* e;
	e = PyObject_NEW(PyEventObject, &PyEvent_Type);
	if(!e) return NULL;

	if(event)
	{
		e->type = event->type;
		e->dict = dict_from_event(event);
	}
	else
	{
		e->type = SDL_NOEVENT;
		e->dict = PyDict_New();
	}
	return (PyObject*)e;
}

static PyObject* PyEvent_New2(int type, PyObject* dict)
{
	PyEventObject* e;
	e = PyObject_NEW(PyEventObject, &PyEvent_Type);
	if(e)
	{
		e->type = type;
		if(!dict)
			dict = PyDict_New();
		else
			Py_INCREF(dict);
		e->dict = dict;
	}
	return (PyObject*)e;
}



/* event module functions */


static PyObject* Event(PyObject* self, PyObject* arg, PyObject* keywords)
{
	PyObject* dict = NULL;
	PyObject* event;
	int type;
	if(!PyArg_ParseTuple(arg, "i|O!", &type, &PyDict_Type, &dict))
		return NULL;

	if(!dict)
		dict = PyDict_New();
	else
		Py_INCREF(dict);

	if(keywords)
	{
		PyObject *key, *value;
		int pos  = 0;
		while(PyDict_Next(keywords, &pos, &key, &value))
			PyDict_SetItem(dict, key, value);
	}

	event = PyEvent_New2(type, dict);

	Py_DECREF(dict);
	return event;
}


static PyObject* event_name(PyObject* self, PyObject* arg)
{
	int type;

	if(!PyArg_ParseTuple(arg, "i", &type))
		return NULL;

	return PyString_FromString(name_from_eventtype(type));
}


static PyObject* set_grab(PyObject* self, PyObject* arg)
{
	int doit;
	if(!PyArg_ParseTuple(arg, "i", &doit))
		return NULL;
	VIDEO_INIT_CHECK();

	if(doit)
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);

	RETURN_NONE;
}


static PyObject* get_grab(PyObject* self, PyObject* arg)
{
	int mode;

	if(!PyArg_ParseTuple(arg, ""))
		return NULL;
	VIDEO_INIT_CHECK();

	mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);

	return PyInt_FromLong(mode == SDL_GRAB_ON);
}


static PyObject* pygame_pump(PyObject* self, PyObject* args)
{
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	SDL_PumpEvents();

	RETURN_NONE
}


static PyObject* pygame_wait(PyObject* self, PyObject* args)
{
	SDL_Event event;
	int status;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	Py_BEGIN_ALLOW_THREADS
	status = SDL_WaitEvent(&event);
	Py_END_ALLOW_THREADS

	if(!status)
		return RAISE(PyExc_SDLError, SDL_GetError());

	return PyEvent_New(&event);
}


static PyObject* pygame_poll(PyObject* self, PyObject* args)
{
	SDL_Event event;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	VIDEO_INIT_CHECK();

	if(SDL_PollEvent(&event))
		return PyEvent_New(&event);
	return PyEvent_New(NULL);
}


static PyObject* event_clear(PyObject* self, PyObject* args)
{
	SDL_Event event;
	int mask = 0;
	int loop, num;
	PyObject* type;
	int val;

	if(PyTuple_Size(args) != 0 && PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "get requires 0 or 1 argument");

	VIDEO_INIT_CHECK();

	if(PyTuple_Size(args) == 0)
		mask = SDL_ALLEVENTS;
	else
	{
		type = PyTuple_GET_ITEM(args, 0);
		if(PySequence_Check(type))
		{
			num = PySequence_Size(type);
			for(loop=0; loop<num; ++loop)
			{
				if(!IntFromObjIndex(type, loop, &val))
					return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
				mask |= SDL_EVENTMASK(val);
			}
		}
		else if(IntFromObj(type, &val))
			mask = SDL_EVENTMASK(val);
		else
			return RAISE(PyExc_TypeError, "get type must be numeric or a sequence");
	}

	SDL_PumpEvents();

	while(SDL_PeepEvents(&event, 1, SDL_GETEVENT, mask) == 1)
	{}

	RETURN_NONE;
}


static PyObject* event_get(PyObject* self, PyObject* args)
{
	SDL_Event event;
	int mask = 0;
	int loop, num;
	PyObject* type, *list, *e;
	int val;

	if(PyTuple_Size(args) != 0 && PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "get requires 0 or 1 argument");

	VIDEO_INIT_CHECK();

	if(PyTuple_Size(args) == 0)
		mask = SDL_ALLEVENTS;
	else
	{
		type = PyTuple_GET_ITEM(args, 0);
		if(PySequence_Check(type))
		{
			num = PySequence_Size(type);
			for(loop=0; loop<num; ++loop)
			{
				if(!IntFromObjIndex(type, loop, &val))
					return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
				mask |= SDL_EVENTMASK(val);
			}
		}
		else if(IntFromObj(type, &val))
			mask = SDL_EVENTMASK(val);
		else
			return RAISE(PyExc_TypeError, "get type must be numeric or a sequence");
	}

	list = PyList_New(0);
	if(!list)
		return NULL;

	SDL_PumpEvents();

	while(SDL_PeepEvents(&event, 1, SDL_GETEVENT, mask) == 1)
	{
		e = PyEvent_New(&event);
		if(!e)
		{
			Py_DECREF(list);
			return NULL;
		}

		PyList_Append(list, e);
		Py_DECREF(e);
	}

	return list;
}


static PyObject* event_peek(PyObject* self, PyObject* args)
{
	SDL_Event event;
	int result;
	int mask = 0;
	int loop, num, noargs=0;
	PyObject* type;
	int val;

	if(PyTuple_Size(args) != 0 && PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "peek requires 0 or 1 argument");

	VIDEO_INIT_CHECK();

	if(PyTuple_Size(args) == 0)
	{
		mask = SDL_ALLEVENTS;
		noargs=1;
	}
	else
	{
		type = PyTuple_GET_ITEM(args, 0);
		if(PySequence_Check(type))
		{
			num = PySequence_Size(type);
			for(loop=0; loop<num; ++loop)
			{
				if(!IntFromObjIndex(type, loop, &val))
					return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
				mask |= SDL_EVENTMASK(val);
			}
		}
		else if(IntFromObj(type, &val))
			mask = SDL_EVENTMASK(val);
		else
			return RAISE(PyExc_TypeError, "peek type must be numeric or a sequence");
	}

	SDL_PumpEvents();
	result = SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, mask);

	if(noargs)
		return PyEvent_New(&event);
	return PyInt_FromLong(result == 1);
}


static PyObject* event_post(PyObject* self, PyObject* args)
{
	PyEventObject* e;
	SDL_Event event;

	if(!PyArg_ParseTuple(args, "O!", &PyEvent_Type, &e))
		return NULL;

	VIDEO_INIT_CHECK();

    if (PyEvent_FillUserEvent(e, &event))
		return NULL;

	if(SDL_PushEvent(&event) == -1)
		return RAISE(PyExc_SDLError, "Event queue full");

	RETURN_NONE
}


static PyObject* set_allowed(PyObject* self, PyObject* args)
{
	int loop, num;
	PyObject* type;
	int val;

	if(PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "set_allowed requires 1 argument");

	VIDEO_INIT_CHECK();

	type = PyTuple_GET_ITEM(args, 0);
	if(PySequence_Check(type))
	{
		num = PySequence_Length(type);
		for(loop=0; loop<num; ++loop)
		{
			if(!IntFromObjIndex(type, loop, &val))
				return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
			SDL_EventState((Uint8)val, SDL_ENABLE);
		}
	}
	else if(type == Py_None)
		SDL_EventState((Uint8)0xFF, SDL_IGNORE);
	else if(IntFromObj(type, &val))
		SDL_EventState((Uint8)val, SDL_ENABLE);
	else
		return RAISE(PyExc_TypeError, "type must be numeric or a sequence");

	RETURN_NONE
}


static PyObject* set_blocked(PyObject* self, PyObject* args)
{
	int loop, num;
	PyObject* type;
	int val;

	if(PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "set_blocked requires 1 argument");

	VIDEO_INIT_CHECK();

	type = PyTuple_GET_ITEM(args, 0);
	if(PySequence_Check(type))
	{
		num = PySequence_Length(type);
		for(loop=0; loop<num; ++loop)
		{
			if(!IntFromObjIndex(type, loop, &val))
				return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
			SDL_EventState((Uint8)val, SDL_IGNORE);
		}
	}
	else if(type == Py_None)
		SDL_EventState((Uint8)0, SDL_IGNORE);
	else if(IntFromObj(type, &val))
		SDL_EventState((Uint8)val, SDL_IGNORE);
	else
		return RAISE(PyExc_TypeError, "type must be numeric or a sequence");

	RETURN_NONE
}


static PyObject* get_blocked(PyObject* self, PyObject* args)
{
	int loop, num;
	PyObject* type;
	int val;
	int isblocked = 0;

	if(PyTuple_Size(args) != 1)
		return RAISE(PyExc_ValueError, "set_blocked requires 1 argument");

	VIDEO_INIT_CHECK();

	type = PyTuple_GET_ITEM(args, 0);
	if(PySequence_Check(type))
	{
		num = PySequence_Length(type);
		for(loop=0; loop<num; ++loop)
		{
			if(!IntFromObjIndex(type, loop, &val))
				return RAISE(PyExc_TypeError, "type sequence must contain valid event types");
			isblocked |= SDL_EventState((Uint8)val, SDL_QUERY) == SDL_IGNORE;
		}
	}
	else if(IntFromObj(type, &val))
		isblocked = SDL_EventState((Uint8)val, SDL_QUERY) == SDL_IGNORE;
	else
		return RAISE(PyExc_TypeError, "type must be numeric or a sequence");

	return PyInt_FromLong(isblocked);
}



static PyMethodDef event_builtins[] =
{
	{ "Event", (PyCFunction)Event, 3, DOC_PYGAMEEVENTEVENT },
	{ "event_name", event_name, 1, DOC_PYGAMEEVENTEVENTNAME },

	{ "set_grab", set_grab, 1, DOC_PYGAMEEVENTSETGRAB },
	{ "get_grab", get_grab, 1, DOC_PYGAMEEVENTGETGRAB },

	{ "pump", pygame_pump, 1, DOC_PYGAMEEVENTPUMP },
	{ "wait", pygame_wait, 1, DOC_PYGAMEEVENTWAIT },
	{ "poll", pygame_poll, 1, DOC_PYGAMEEVENTPOLL },
	{ "clear", event_clear, 1, DOC_PYGAMEEVENTCLEAR },
	{ "get", event_get, 1, DOC_PYGAMEEVENTGET },
	{ "peek", event_peek, 1, DOC_PYGAMEEVENTPEEK },
	{ "post", event_post, 1, DOC_PYGAMEEVENTPOST },

	{ "set_allowed", set_allowed, 1, DOC_PYGAMEEVENTSETALLOWED },
	{ "set_blocked", set_blocked, 1, DOC_PYGAMEEVENTSETBLOCKED },
	{ "get_blocked", get_blocked, 1, DOC_PYGAMEEVENTGETBLOCKED },

	{ NULL, NULL }
};


PYGAME_EXPORT
void initevent(void)
{
	PyObject *module, *dict, *apiobj;
	static void* c_api[PYGAMEAPI_EVENT_NUMSLOTS];

	PyType_Init(PyEvent_Type);


    /* create the module */
	module = Py_InitModule3("event", event_builtins, DOC_PYGAMEEVENT);
	dict = PyModule_GetDict(module);

	PyDict_SetItemString(dict, "EventType", (PyObject *)&PyEvent_Type);

	/* export the c api */
	c_api[0] = &PyEvent_Type;
	c_api[1] = PyEvent_New;
	c_api[2] = PyEvent_New2;
	c_api[3] = PyEvent_FillUserEvent;
	apiobj = PyCObject_FromVoidPtr(c_api, NULL);
	PyDict_SetItemString(dict, PYGAMEAPI_LOCAL_ENTRY, apiobj);
	Py_DECREF(apiobj);

	/*imported needed apis*/
	import_pygame_base();
	PyGame_RegisterQuit(user_event_cleanup);
}



