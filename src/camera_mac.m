//
//  camera_mac.m
//
//  Created by Werner Laurensse on 2009-05-28.
//  Copyright (c) 2009 . All rights reserved.
//

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
    
    printf("helper: init sqc\n");
    // Initialize sequence grabber component
    theErr = SGInitialize(component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot initializes the Saequence Grabber component");
        return 0;
    }
    
    printf("helper: no movie\n");
    // Don't make movie
    theErr = SGSetDataRef(component, 0, 0, seqGrabDontMakeMovie);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot not make a movie");
        return 0;
    }
    
    printf("helper: create sq video channel \n");
    // Create sequence grabber video channel
    theErr = SGNewChannel(component, VideoMediaType, &self->channel);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot creates a sequence grabber channel and assigns a channel component to the channel");
        return 0;
    }
    
    printf("helper: done...\n");
    return 1;
}

/* Make the Camera object ready for capturing images. */
int mac_init_device(PyCameraObject* self) {
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
   			NSLog(@"CDSequenceEnd() returned %ld", theErr);
   		}
   		self->decompressionSequence = 0;
   	}

    // Close sequence grabber component
   	if (self->component) {
   		theErr = CloseComponent(self->component);
   		if (theErr != noErr) {
   			NSLog(@"CloseComponent() returned %ld", theErr);
   		}
   		self->component = NULL;
   	}
    
    // Dispose of GWorld
   	if (self->gWorld) {
   		DisposeGWorld(self->gWorld);
   		self->gWorld = NULL;
   	} */
    return 0;
}

int mac_start_capturing(PyCameraObject* self) {
    return 1;
}

int mac_stop_capturing (PyCameraObject* self) {
    return 0;
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