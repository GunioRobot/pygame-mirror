//
//  camera_mac.m
//
//  Created by Werner Laurensse on 2009-05-28.
//  Copyright (c) 2009 . All rights reserved.
//

// TODO: memory management in open, init and close

#import "camera.h"
#import <SDL.h>
#import <Cocoa/Cocoa.h>

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
        //TODO ....
        //PyErr_Format(PyExc_SystemError, "Cannot set bounds of Rect");
        //return 0;
    }
    
    // Create the GWorld
    //theErr = QTNewGWorld(&self->gWorld, k32ARGBPixelFormat, &self->boundsRect, 0, NULL, 0);
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
    theErr = SGSetDataProc(self->component, NewSGDataUPP(&mac_que_frame_old), (long) self);
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
    
    // Start recording
    theErr = SGStartRecord(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot start recording");
        return 0;
    }
    
    // Setup Decompression
    ComponentResult result;
    PixMapHandle hPixMap = NULL;
    ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)NewHandle(0);
    result = SGGetChannelSampleDescription(self->channel, (Handle)imageDesc);
    if (result != noErr) {
        NSLog(@"SGGetChannelSampleDescription() returned %ld", theErr);
        PyErr_Format(PyExc_SystemError, "Cannot set Sequence Grabber to record");
        return 0;
    }
    
    // Set up getting grabbed data into the Window
    /*hPixMap = GetGWorldPixMap(self->gWorld);
    if (hPixMap != NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot obtains the pixel map created for an offscreen graphics world");
        return 0;
    }
    GetPixBounds(hPixMap, &self->boundsRect);
    self->decompressionSequence = 0;*/

    Rect sourceRect;
    sourceRect.top = 0;
    sourceRect.left = 0;
    sourceRect.right = (**imageDesc).width;
    sourceRect.bottom = (**imageDesc).height;

    MatrixRecord scaleMatrix;
    RectMatrix(&scaleMatrix, &sourceRect, &self->boundsRect);
    
    self->size = (GetPixRowBytes(hPixMap) * (*imageDesc)->height);

    result = DecompressSequenceBeginS(&self->decompressionSequence,
                                      imageDesc,
                                      NULL, //GetPixBaseAddr(hPixMap),
                                      self->size,
                                      self->gWorld,
                                      NULL,
                                      NULL,
                                      &scaleMatrix,
                                      srcCopy,
                                      NULL,
                                      0,
                                      codecNormalQuality,
                                      bestSpeedCodec);
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
    /*
    // Start recording
    theErr = SGStartRecord(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot start recording");
        return 0;
    }*/

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

PyObject *mac_read_raw(PyCameraObject *self) {
    if (self->gWorld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }
    
    PyObject *raw = NULL;
    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gWorld);
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gWorld, &portRect);
        int pixels_wide = (portRect.right - portRect.left);
        int pixels_high = (portRect.bottom - portRect.top);
        void* pixBaseAddr = GetPixBaseAddr(pixMapHandle);
        long pixmapRowBytes = GetPixRowBytes(pixMapHandle);
        
        raw = PyString_FromStringAndSize(pixBaseAddr, pixels_wide*pixmapRowBytes);
        printf("raw string: %s, size: %d\n", PyString_AsString(raw), PyString_Size(raw));
        UnlockPixels(pixMapHandle);
    }
    return raw;
}

int mac_read_frame(PyCameraObject* self, SDL_Surface* surf) {
    mac_que_frame(self);
    //mac_camera_idle(self);
    mac_gworld_to_surface(self, surf);
    //mac_gworld_to_nsimage(self);
    return 1;
}

int mac_que_frame(PyCameraObject* self) {
    ComponentResult theErr;
    
    if (self->gWorld) {
        printf("helper: que frame 2\n");
        CodecFlags ignore;
        
        theErr = DecompressSequenceFrameS(self->decompressionSequence,
                                          GetPixBaseAddr(GetGWorldPixMap(self->gWorld)),
                                          self->size,
                                          0,
                                          &ignore,
                                          NULL);
        if (theErr != noErr) {
            PyErr_Format(PyExc_SystemError, "an error occurred when trying to decompress the sequence");
            return theErr;
        }
    }
    
    return 1;
}

int mac_gworld_to_surface(PyCameraObject* self, SDL_Surface* surf) {
    if (!surf)
        return 0;
        
    SDL_LockSurface(surf);
    
    if (self->gWorld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }

    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gWorld);
    void *pixBaseAddr;
    SDL_Surface *surf2 = NULL;
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gWorld, &portRect );
        int pixels_wide = (portRect.right - portRect.left);
        int pixels_high = (portRect.bottom - portRect.top);
        
        pixBaseAddr = GetPixBaseAddr(pixMapHandle);
        long pixmapRowBytes = GetPixRowBytes(pixMapHandle);
        
        int row = 0;
        Uint32 *dst, *src;
        row = 0;
        while(row < pixels_high) {
            dst = (Uint32*) surf->pixels + row*pixels_wide;
            src = pixBaseAddr + row*pixmapRowBytes;
            memcpy(dst, src, pixels_wide*4);
            row++;
        }
        
        UnlockPixels( pixMapHandle );    
    }
    
    SDL_UnlockSurface(surf);
    
    printf("helper: surf.pixels: %dl\n", surf->pixels);
    printf("helper: pix bassadr: %dl\n", pixBaseAddr);
    printf("helper: surf: %dl\n", surf);
    printf("=====================================================================\n");
    
    return 1;
}

int mac_gworld_to_nsimage(PyCameraObject* self) {
    if (self->gWorld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }

    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gWorld);
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gWorld, &portRect );
        int pixels_wide = (portRect.right - portRect.left);
        int pixels_high = (portRect.bottom - portRect.top);
        
        int bps = 8;
        int spp = 4;
        bool has_alpha = true;
        
        printf("nsimage 1\n");
        
        NSBitmapImageRep *frameBitmap = [[[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL
                          pixelsWide:pixels_wide
                          pixelsHigh:pixels_high
                       bitsPerSample:bps
                     samplesPerPixel:spp
                            hasAlpha:has_alpha
                            isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace
                         bytesPerRow:0
                        bitsPerPixel:0] autorelease];

        CGColorSpaceRef dst_colorspaceref = CGColorSpaceCreateDeviceRGB();

        CGImageAlphaInfo dst_alphainfo = has_alpha ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;

        CGContextRef dst_contextref = CGBitmapContextCreate( [frameBitmap bitmapData],
                                                             pixels_wide,
                                                             pixels_high,
                                                             bps,
                                                             [frameBitmap bytesPerRow],
                                                             dst_colorspaceref,
                                                             dst_alphainfo );

        void *pixBaseAddr = GetPixBaseAddr(pixMapHandle);
        long pixmapRowBytes = GetPixRowBytes(pixMapHandle);

        CGDataProviderRef dataproviderref = CGDataProviderCreateWithData( NULL, pixBaseAddr, pixmapRowBytes * pixels_high, NULL );

        int src_bps = 8;
        int src_spp = 4;
        bool src_has_alpha = true;

        CGColorSpaceRef src_colorspaceref = CGColorSpaceCreateDeviceRGB();

        CGImageAlphaInfo src_alphainfo = src_has_alpha ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNone;

        printf("nsimage 2\n");
        
        CGImageRef src_imageref = CGImageCreate( pixels_wide,
                                                 pixels_high,
                                                 src_bps,
                                                 src_bps * src_spp,
                                                 pixmapRowBytes,
                                                 src_colorspaceref,
                                                 src_alphainfo,
                                                 dataproviderref,
                                                 NULL,
                                                 NO, // shouldInterpolate
                                                 kCGRenderingIntentDefault );

        CGRect rect = CGRectMake( 0, 0, pixels_wide, pixels_high );

        CGContextDrawImage( dst_contextref, rect, src_imageref );

        CGImageRelease( src_imageref );
        CGColorSpaceRelease( src_colorspaceref );
        CGDataProviderRelease( dataproviderref );
        CGContextRelease( dst_contextref );
        CGColorSpaceRelease( dst_colorspaceref );

        UnlockPixels( pixMapHandle );

        NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(pixels_wide, pixels_high)];
        [image addRepresentation:frameBitmap];
        
        printf("nsimage 3\n");
        
        // save image as png
        NSData *data;
        data = [frameBitmap representationUsingType: NSPNGFileType
                     properties: nil];
        [data writeToFile: @"/Users/abe/test.png"
              atomically: NO];
        
        

        [image autorelease];   
    }
    return 1;
}

int _copy_gworld_to_surface(PyCameraObject* self, SDL_Surface* surf) {
    
    return 1;
}


/* TODO: leg uit */
pascal int mac_que_frame_old(PyCameraObject* self, SGChannel channel, Ptr data, long dataLength, long *offset, long channelRefCon,
TimeValue time, short writeType, long refCon) {
    ComponentResult theErr;
    
    if (self->gWorld) {
        printf("helper: que frame 2\n");
        CodecFlags ignore;
        
        theErr = DecompressSequenceFrameS(self->decompressionSequence,
                                          GetPixBaseAddr(GetGWorldPixMap(self->gWorld)),
                                          self->size,
                                          0,
                                          &ignore,
                                          NULL);
        if (theErr != noErr) {
            PyErr_Format(PyExc_SystemError, "an error occurred when trying to decompress the sequence");
            return theErr;
        }
    }
    
    return 1;
}

int mac_camera_idle(PyCameraObject* self) {
    printf("helper: idle 1\n");
    OSErr theErr;

    theErr = SGIdle(self->component);
    printf("helper: idle 2\n");
    if (theErr != noErr) {
        NSLog(@"helper: idle 3, error: %@\n", theErr);
        PyErr_Format(PyExc_SystemError, "Cannot put component into idle status");
        printf("helper: idle 4\n");
        return 0;
    }
    printf("helper: idle 5\n");
    return 1;
}
