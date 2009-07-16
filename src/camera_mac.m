//
//  camera_mac.m
//
//  Created by Werner Laurensse on 2009-05-28.
//  Copyright (c) 2009 . All rights reserved.
//

// TODO: memory management in open, init and close

#import "camera.h"
#import <SDL.h>
//#import <Cocoa/Cocoa.h>

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
	int width = self->boundsRect.right;
	int height = self->boundsRect.bottom;
	
    self->bytes = 4;
	int rowlength= width*self->bytes;
	
	OSType pixelFormat = k32RGBAPixelFormat;
	
	OSErr result;
	
	result = SGInitialize(self->component);                                      //
    if(result!=noErr){
         fprintf(stdout, "could not initialize SG\n");
    }
    
	
    result = SGSetDataRef(self->component, 0, 0, seqGrabDontMakeMovie);          //
        if (result != noErr){
             fprintf(stdout, "dataref failed\n");
        }
        
    result = SGNewChannel(self->component, VideoMediaType, &self->channel);		            //
    if(result!=noErr){
         //fprintf(stdout, "could not make new SG channnel\n");
		 return false;
    }

	//result = SGSettingsDialog (self->component, self->channel ,0 ,NULL ,seqGrabSettingsPreviewOnly,NULL,0);
    //if(result!=noErr){
    //     fprintf(stdout, "could not get settings from dialog\n");
    //}
    
    result = SGSetChannelBounds(self->channel, &self->boundsRect);
    if(result!=noErr){
         fprintf(stdout, "could not set SG ChannelBounds\n");
    }
	
	/*result = SGSetFrameRate (vc, fps);
    if(result!=noErr){
         fprintf(stdout, "could not set SG FrameRate\n");
    }*/
      
    result = SGSetChannelUsage(self->channel, seqGrabPreview);
    if(result!=noErr){
         fprintf(stdout, "could not set SG ChannelUsage\n");
    }
    
	result = SGSetChannelPlayFlags(self->channel, channelPlayAllData);
	if(result!=noErr){
         fprintf(stdout, "could not set SG AllData\n");
	};
	
    self->buffers.length = self->boundsRect.right * self->boundsRect.bottom * self->bytes;
	self->buffers.start = (unsigned char*) malloc(self->buffers.length);
	
	result = QTNewGWorldFromPtr (&self->gworld,
									pixelFormat,
                                    &self->boundsRect, 
                                    NULL, 
                                    NULL, 
                                    0, 
                                    self->buffers.start, 
                                    rowlength);
        
	if (result!= noErr)
  	{
		fprintf(stdout, "%d error at QTNewGWorldFromPtr\n", result);
		//delete []buffer;
		self->buffers.start = NULL;
		return false;
	}  
	
    if (self->gworld == NULL)
	{
		fprintf(stdout, "Could not allocate off screen\n");
		//delete []buffer;
		self->buffers.start = NULL;
		return false;
	}
	
    result = SGSetGWorld(self->component, (CGrafPtr)self->gworld, NULL);
	if (result != noErr) {
		fprintf(stdout, "Could not set SGSetGWorld\n");
		//delete []buffer;
		self->buffers.start = NULL;
		return false;
	}

    result = SGPrepare(self->component, TRUE, FALSE);
    if (result != noErr) {
            fprintf(stderr, "SGPrepare Preview failed\n");
	}
	
	result = SGStartPreview(self->component);
    if (result != noErr) {
            fprintf(stderr, "SGStartPreview failed\n");
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
   	if (self->gworld) {
   		DisposeGWorld(self->gworld);
   		self->gworld = NULL;
   	}
    return 1;
}

int mac_stop_capturing (PyCameraObject* self) {
    return 1;
}

PyObject *mac_read_raw(PyCameraObject *self) {
    if (self->gworld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }
    
    PyObject *raw = NULL;
    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gworld);
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gworld, &portRect);
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
    //mac_que_frame(self);
    //mac_camera_idle(self);
    //mac_gworld_to_nsimage(self);
    //mac_gworld_to_surface(self, surf);
    mac_get_frame(self, surf);
    //mac_gworld_to_nsimage(self);
    return 1;
}

int mac_get_frame(PyCameraObject* self, SDL_Surface* surf) {
    int width = self->boundsRect.right;
	int height = self->boundsRect.bottom;
	
	int rowlength= width*self->bytes;
	
    OSErr result = SGIdle(self->component);
    if (result != noErr) {
        fprintf(stderr, "SGIdle failed\n");
		return 0;
	}
	
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
    
    //surf->format->BytesPerPixel = 4;
	
	memcpy(surf->pixels, self->buffers.start, self->buffers.length);
    SDL_UnlockSurface(surf);
    
    return 1;
}

int mac_gworld_to_surface(PyCameraObject* self, SDL_Surface* surf) {
    if (!surf)
        return 0;
        
    SDL_LockSurface(surf);
    
    if (self->gworld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }

    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gworld);
    void *pixBaseAddr;
    SDL_Surface *surf2 = NULL;
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gworld, &portRect );
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

/*
int mac_gworld_to_nsimage(PyCameraObject* self) {
    if (self->gworld == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot set convert gworld to surface because gworls is 0");
        return 0;
    }

    PixMapHandle pixMapHandle = GetGWorldPixMap(self->gworld);
    if (LockPixels(pixMapHandle)) {
        Rect portRect;
        GetPortBounds(self->gworld, &portRect);
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

        //void *pixBaseAddr = GetPixBaseAddr(pixMapHandle);
        void *pixBaseAddr = self->buffer;
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
        [data writeToFile: @"/Users/abe/test.png" atomically: NO];
        
        

        [image autorelease];   
    }
    return 1;
}
*/

int _copy_gworld_to_surface(PyCameraObject* self, SDL_Surface* surf) {
    
    return 1;
}

int mac_camera_idle(PyCameraObject* self) {
    printf("####################################### IDLE #######################################\n");
    printf("helper: idle 1\n");
    OSErr theErr;

    theErr = SGIdle(self->component);
    if (theErr != noErr) {
        PyErr_Format(PyExc_SystemError, "Cannot put component into idle status");
        return 0;
    }
    return 1;
}
