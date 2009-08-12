import time
import pygame
import pygame.camera
from pygame.locals import *

# test camera loop
def test1():
    pygame.init()
    pygame.camera.init()
    
    size = (640, 480)
    depth = 24
    format = 'RGB'
    #format = 'YUV'
    #format = 'HSV'
    display = pygame.display.set_mode(size, 0, depth)
    snapshot = pygame.surface.Surface(size, 0, display)
    cameras = pygame.camera.list_cameras()
    c = pygame.camera.Camera(cameras[0], size, format)
    clock = pygame.time.Clock()
    c.start()
    
    going = True
    while going:
        time_passed = clock.tick(50)
        events = pygame.event.get()
        for e in events:
            if e.type == pygame.QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                going = False
        
        snapshot_tmp = c.get_image() #snapshot)
        #snapshot = pygame.transform.flip(snapshot_tmp, True, False)
        display.blit(snapshot, (0, 0))
        pygame.display.flip()
        #raw = c.get_raw()

    c.stop()

# get raw string
def test2():
    pygame.init()
    pygame.camera.init()
    
    size = (640, 480)
    display = pygame.display.set_mode(size, 0)
    snapshot = pygame.surface.Surface(size, 0, display)
    cameras = pygame.camera.list_cameras()
    c = pygame.camera.Camera(cameras[0])
    clock = pygame.time.Clock()
    c.start()
    
    print "=======================>>>"
    s = c.get_raw()
    print s
    
    time.sleep(2)
    
    c.stop()

# main
if __name__ == '__main__':
    test1()
    #test2()
    
