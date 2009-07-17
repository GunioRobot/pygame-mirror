//
//  camera_mac.m
//
//  Created by Werner Laurensse on 2009-05-28.
//  Copyright (c) 2009 . All rights reserved.
//

// TODO: memory management in open, init and close

#import "camera.h"
#import <SDL.h>

/* 
 * return: an array of the available cameras ids.
 * num_cameras: number of cameras in array.
 */
char** mac_list_cameras(int* num_cameras) {
    char** cameras;
    char* camera;
    
    // The ComponentDescription allows you to search for Components that can be used to capture
    // images, and thus can be used as camera.
    ComponentDescription cameraDescription;
    memset(&cameraDescription, 0, sizeof(ComponentDescription));
    cameraDescription.componentType = SeqGrabComponentType;
    
    // Count the number of cameras on the system, and allocate an array for the cameras
    *num_cameras = (int) CountComponents(&cameraDescription);
    cameras = (char **) malloc(sizeof(char*) * *num_cameras);
    
    // Try to find a camera, and add the camera's 'number' as a string to the array of cameras 
    Component cameraComponent = FindNextComponent(0, &cameraDescription);
    short num = 0;
    while(cameraComponent != NULL) {
        camera = malloc(sizeof(char) * 50); //TODO: find a better way to do this...
        sprintf(camera, "%d", cameraComponent);
        cameras[num] = camera;
        cameraComponent = FindNextComponent(cameraComponent, &cameraDescription);
        num++;
    }
    return cameras;
}

/* Open a Camera component. */
int mac_open_device (PyCameraObject* self) {
    OSErr theErr;

    // Initialize movie toolbox
    theErr = EnterMovies();
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot initializes the Movie Toolbox");
        return 0;
    }
    
    // Open camera component
    SeqGrabComponent component = OpenComponent((Component) atoi(self->device_name));
    if (component == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot open '%s'", self->device_name);
        return 0;
    }
    self->component = component;
    
    return 1;
}

/* Make the Camera object ready for capturing images. */
int mac_init_device(PyCameraObject* self) {
    OSErr theErr;
    OSType pixelFormat = k32RGBAPixelFormat;
    
    int rowlength = self->boundsRect.right * self->bytes;
	
	theErr = SGInitialize(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot initialize sequence grabber component");
        return 0;
    }
    
	
    theErr = SGSetDataRef(self->component, 0, 0, seqGrabDontMakeMovie);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot set the sequence grabber destination data reference for a record operation");
        return 0;
    }
        
    theErr = SGNewChannel(self->component, VideoMediaType, &self->channel);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot creates a sequence grabber channel and assigns a channel component to the channel");
        return 0;
    }
    
    theErr = SGSetChannelBounds(self->channel, &self->boundsRect);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot specifie a channel's display boundary rectangle");
        return 0;
    }
	
	/*
	theErr = SGSetFrameRate (vc, fps);
    if(theErr != noErr){
        PyErr_Format(PyExc_SystemError,
        "Cannot set the frame rate of the sequence grabber");
        return 0;
    }
    */
      
    theErr = SGSetChannelUsage(self->channel, seqGrabPreview);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot specifie how a channel is to be used by the sequence grabber componen");
        return 0;
    }
    
	theErr = SGSetChannelPlayFlags(self->channel, channelPlayAllData);
	if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot adjust the speed and quality with which the sequence grabber displays data from a channel");
        return 0;
	};
	
    self->pixels.length = self->boundsRect.right * self->boundsRect.bottom * self->bytes;
	self->pixels.start = (unsigned char*) malloc(self->pixels.length);
	
	theErr = QTNewGWorldFromPtr(&self->gworld,
								pixelFormat,
                                &self->boundsRect, 
                                NULL, 
                                NULL, 
                                0, 
                                self->pixels.start, 
                                rowlength);
        
	if (theErr != noErr) {
	    PyErr_Format(PyExc_SystemError,
	    "Cannot wrap a graphics world and pixel map structure around an existing block of memory containing an image, "
	    "failed to run QTNewGWorldFromPtr");
		free(self->pixels.start);
		self->pixels.start = NULL;
        self->pixels.length = 0;
		return 0;
	}  
	
    if (self->gworld == NULL) {
		PyErr_Format(PyExc_SystemError,
		"Cannot wrap a graphics world and pixel map structure around an existing block of memory containing an image, "
		"gworld is NULL");
		free(self->pixels.start);
		self->pixels.start = NULL;
        self->pixels.length = 0;
		return 0;
	}
	
    theErr = SGSetGWorld(self->component, (CGrafPtr)self->gworld, NULL);
	if (theErr != noErr) {
		PyErr_Format(PyExc_SystemError,
		"Cannot establishe the graphics port and device for a sequence grabber component");
        free(self->pixels.start);
		self->pixels.start = NULL;
        self->pixels.length = 0;
		return 0;
	}
	    
    return 1;
}

/* Start Capturing */
int mac_start_capturing(PyCameraObject* self) {
    OSErr theErr;
    
    theErr = SGPrepare(self->component, true, false);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot istruct a sequence grabber to get ready to begin a preview or record operation");
        free(self->pixels.start);
		self->pixels.start = NULL;
        self->pixels.length = 0;
        return 0;
	}
	
	theErr = SGStartPreview(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError,
        "Cannot instruct the sequence grabber to begin processing data from its channels");
        free(self->pixels.start);
		self->pixels.start = NULL;
        self->pixels.length = 0;
        return 0;
	}
	
    return 1;
}

/* Close the camera component, and stop the image capturing if necessary. */
int mac_close_device (PyCameraObject* self) {
    ComponentResult theErr;
    
    // Stop recording
   	if (self->component)
   		SGStop(self->component);

    // Close sequence grabber component
   	if (self->component) {
   		theErr = CloseComponent(self->component);
   		if (theErr != noErr) {
   			PyErr_Format(PyExc_SystemError,
   			"Cannot close sequence grabber component");
            return 0;
   		}
   		self->component = NULL;
   	}
    
    // Dispose of GWorld
   	if (self->gworld) {
   		DisposeGWorld(self->gworld);
   		self->gworld = NULL;
   	}
   	// Dispose of pixels buffer
    free(self->pixels.start);
    self->pixels.start = NULL;
    self->pixels.length = 0;
    return 1;
}

/* Stop capturing. */
int mac_stop_capturing (PyCameraObject* self) {
    OSErr theErr = SGStop(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Could not stop the sequence grabber with previewing");
        return 0;
    }
    return 1;
}

/* Read a frame, and put the raw data into a python string. */
PyObject *mac_read_raw(PyCameraObject *self) {
    if (self->gworld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }
    
    if (mac_camera_idle(self) == 0) {
        return 0;
    }
    
    PyObject *raw = NULL;
    PixMapHandle pixmap_handle = GetGWorldPixMap(self->gworld);
    LockPixels(pixmap_handle);
    raw = PyString_FromStringAndSize(self->pixels.start, self->pixels.length);
    UnlockPixels(pixmap_handle);
    return raw;
}

/* Read a frame from the camera and copy it to a surface. */
int mac_read_frame(PyCameraObject* self, SDL_Surface* surf) {
    if (mac_camera_idle(self) != 0) {
        return mac_copy_gworld_to_surface(self, surf);
    } else {
        return 0;
    }
}

/* Put the camera in idle mode. */
int mac_camera_idle(PyCameraObject* self) {
    OSErr theErr = SGIdle(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "SGIdle failed");
        return 0;
    }
    
    return 1;
}

/* Copy the data from a gworld into an SDL_Surface.
 * If nesesary it addjust the masks of the surface to fit the rgb layout of the gworld. */
int mac_copy_gworld_to_surface(PyCameraObject* self, SDL_Surface* surf) {
    SDL_LockSurface(surf);
    /*
    surf->format->Rmask = 0xff000000;
    surf->format->Gmask = 0x00ff0000;
    surf->format->Bmask = 0x0000ff00;
    surf->format->Amask = 0x000000ff;
    */
    
    /*
    surf->format->Bshift = 0xff000000;
    surf->format->Gshift = 0x00ff0000;
    surf->format->Rshift = 0x0000ff00;
    surf->format->Ashift = 0x000000ff;
    */
	
	memcpy(surf->pixels, self->pixels.start, self->pixels.length);
    SDL_UnlockSurface(surf);
    
    return 1;
}