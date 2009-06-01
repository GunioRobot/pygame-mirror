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
    
    printf("Hoi 1\n");
    
    // The ComponentDescription allows you to search for Components that can be used to capture
    // images, and thus can be used as camera.
    ComponentDescription cameraDescription;
    memset(&cameraDescription, 0, sizeof(ComponentDescription));
    cameraDescription.componentType = SeqGrabComponentType;
    
    printf("Hoi 2\n");
    
    // Count the number of cameras on the system, and allocate an array for the cameras
    *num_cameras = (int) CountComponents(&cameraDescription);
    cameras = (char **) malloc(sizeof(char*) * *num_cameras);
    
    printf("Hoi 3\n");
    
    // Try to find a camera, and add the camera's 'number' as a string to the array of cameras 
    Component cameraComponent = FindNextComponent(0, &cameraDescription);
    printf("Hoi 4\n");
    short num = 0;
    while(cameraComponent != NULL) {
        printf("Hoi 5\n");
        cameraComponent = FindNextComponent(cameraComponent, &cameraDescription);
        printf("Hoi 6\n");
        //camera = itoa(camera, cameraComponent, );
        camera = malloc(sizeof(char) * 50);
        sprintf(camera, "%d", cameraComponent);
        printf("Hoi 7\n");
        cameras[num] = camera;
        num++;
    }
    printf("Hoi 8\n");
    printf("cameras: %d\n", cameras[0]);
    return cameras;
}

int mac_open_device (PyCameraObject* self) {
    printf("open\n");
    ComponentInstance instance = OpenComponent((Component) atoi(self->device_name));
    if (instance == NULL) {
        PyErr_Format(PyExc_SystemError, "Cannot open '%s'", self->device_name);
        return NULL;
    }
    return 1;
}

int mac_init_device(PyCameraObject* self) {
    return 0;
}

int mac_close_device (PyCameraObject* self) {
    //CloseComponent()
    return 0;
}

int mac_start_capturing(PyCameraObject* self) {
    return 0;
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