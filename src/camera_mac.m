//
//  camera_mac.m
//
//  Created by Werner Laurensse on 2009-05-28.
//  Copyright (c) 2009 . All rights reserved.
//

// TODO: memory management in open, init and close

#import "camera.h"

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
    
    printf("helper: enter movie\n");
    // Initialize movie toolbox
    theErr = EnterMovies();
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot initializes the Movie Toolbox");
        return 0;
    }
    
    printf("helper: open camera\n");
    // Open camera component
    SeqGrabComponent component = OpenComponent((Component) atoi(self->device_name));
    if (component == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot open '%s'", self->device_name);
        return 0;
    }
    self->component = component;
    
    printf("helper: done...\n");
    return 1;
}

/* Make the Camera object ready for capturing images. */
int mac_init_device(PyCameraObject* self) {
    OSErr theErr;
    
    printf("helper: init sqc\n");
    // Initialize sequence grabber component
    theErr = SGInitialize(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot initializes the Saequence Grabber component");
        return 0;
    }
    
    printf("helper: no movie\n");
    // Don't make movie
    theErr = SGSetDataRef(self->component, 0, 0, seqGrabDontMakeMovie);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot not make a movie");
        return 0;
    }
    
    printf("helper: create sq video channel \n");
    // Create sequence grabber video channel
    theErr = SGNewChannel(self->component, VideoMediaType, &self->channel);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot creates a sequence grabber channel and assigns a channel component to the channel");
        return 0;
    }
    
    printf("helper: set channel bounds \n");
    // Create sequence grabber video channel
    theErr = SGSetChannelBounds(self->component, &self->boundsRect); //TODO find out why it allways returns an error (-32766)
    if (theErr != noErr) {
        NSLog(@"boundsRect=(%d, %d, %d, %d)", self->boundsRect.top, self->boundsRect.left, self->boundsRect.bottom, self->boundsRect.right);
        NSLog(@"SGSetChannelBounds() returned %ld", theErr);
        //PyErr_Format(PyExc_SystemError, "Cannot set bounds of Rect");
        //return 0;
    }
    
    // Create the GWorld
    theErr = QTNewGWorld(&self->gWorld, k32ARGBPixelFormat, &self->boundsRect, 0, NULL, 0);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot create gWord");
        return 0;
    }
    // Lock the pixmap
    if (!LockPixels(GetPortPixMap(self->gWorld))) {
        PyErr_Format(PyExc_SystemError, "Could not lock pixels");
        return 0;
    }

    // Set GWorld
    theErr = SGSetGWorld(self->component, self->gWorld, GetMainDevice());
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Could not set gWord");
        return 0;
    }

    // Set the channel's bounds
    theErr = SGSetChannelBounds(self->channel, &self->boundsRect);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot set channel bounds");
        return 0;
    }

    // Set the channel usage to record
    theErr = SGSetChannelUsage(self->channel, seqGrabRecord);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot set channel usage to record");
        return 0;
    }

    // Set data proc
    theErr = SGSetDataProc(self->component, NewSGDataUPP(&sg_data_proc), (long) self);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot set channel usage to record");
        return 0;
    }

    // Prepare Sequence Grabber to record.
    theErr = SGPrepare(self->component, false, true);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot set Sequence Grabber to record");
        return 0;
    }
    
    ComponentResult result;

    // test
    ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)NewHandle(0);
    result = SGGetChannelSampleDescription(self->channel, (Handle)imageDesc);
    if (result != noErr) {
        NSLog(@"SGGetChannelSampleDescription() returned %ld", theErr);
        PyErr_Format(PyExc_SystemError, "Cannot set Sequence Grabber to record");
        return 0;
    }

    Rect sourceRect;
    sourceRect.top = 0;
    sourceRect.left = 0;
    sourceRect.right = (**imageDesc).width;
    sourceRect.bottom = (**imageDesc).height;

    MatrixRecord scaleMatrix;
    RectMatrix(&scaleMatrix, &sourceRect, &self->boundsRect);

    result = DecompressSequenceBegin(&self->decompressionSequence, imageDesc, self->gWorld, NULL, NULL, &scaleMatrix, srcCopy, NULL, 0, codecNormalQuality, bestSpeedCodec);
    if (result != noErr) {
        NSLog(@"DecompressionSequenceBegin() returned %ld", theErr);
        PyErr_Format(PyExc_SystemError, "Cannot set Sequence Grabber to record");
        return 0;
    }

    DisposeHandle((Handle)imageDesc);
    
    return 1;
}

/* Close the camera component, and stop the image capturing if necessary. */
int mac_close_device (PyCameraObject* self) {
    // Stop recording
   	if (self->component)
   		SGStop(self->component);

       ComponentResult theErr;

    // End decompression sequence
   	/*if (self->decompressionSequence) {
   		theErr = CDSequenceEnd(self->decompressionSequence);
   		if (theErr != noErr) {
   		    PyErr_Format(PyExc_SystemError, "Cannot end decompression sequence");
            return 0;
   		}
   		self->decompressionSequence = 0;
   	}*/

    // Close sequence grabber component
   	if (self->component) {
   		theErr = CloseComponent(self->component);
   		if (theErr != noErr) {
   			PyErr_Format(PyExc_SystemError, "Cannot close sequence grabber component");
            return 0;
   		}
   		self->component = NULL;
   	}
    
    // Dispose of GWorld
   	if (self->gWorld) {
   		DisposeGWorld(self->gWorld);
   		self->gWorld = NULL;
   	}
    return 1;
}

int mac_start_capturing(PyCameraObject* self) {
    OSErr theErr;
    
    // Start recording
    theErr = SGStartRecord(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot start recording");
        return 0;
    }

//	startTime = [NSDate timeIntervalSinceReferenceDate];
/*
    // Set up decompression sequence (camera -> GWorld)
    [self _setupDecompression];

    // Start frame timer
    frameTimer = [[NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(_sequenceGrabberIdle) userInfo:nil repeats:YES] retain];

    [self retain]; // Matches autorelease in -stop
*/
    return 1;
}

int mac_stop_capturing (PyCameraObject* self) {
    return 1;
}
/* TODO: leg uit */
int sg_data_proc(PyCameraObject* self, SGChannel channel, Ptr data, long dataLength, long *offset, long channelRefCon,
TimeValue time, short writeType, long refCon) {
    printf("helper: sg_data_proc recall fun...\n");
    
    //CSGCamera *camera = (CSGCamera *)refCon;
    ComponentResult theErr;
    
    if (self->gWorld) {
        CodecFlags ignore;
        theErr = DecompressSequenceFrameS(self->decompressionSequence, data, dataLength, 0, &ignore, NULL);
        if (theErr != noErr) {
            NSLog(@"DecompressSequenceFrameS() returned %ld", theErr);
            return theErr;
        }
    }
    
    return 1;                 
}

#if 1==0

int main(int argc, const char *argv[]) {
    printf("Hoi 0\n");
    int aantal;
    char** cameras = mac_list_cameras(&aantal);
    printf("cameras: %d\n", cameras[0]);
    
    printf("Hoi 9\n");
    ComponentInstance instance = OpenComponent((Component) atoi(cameras[0]));
    printf("Hoi 10\n");
    sleep(60);
    
    return 0;
}

# endif