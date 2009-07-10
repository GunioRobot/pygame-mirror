import time
import pygame
import pygame.camera
from pygame.locals import *

# test camera loop
def test1():
    pygame.init()
    pygame.camera.init()
    
    size = (640, 480)
    display = pygame.display.set_mode(size, 0)
    snapshot = pygame.surface.Surface(size, 0, display)
    cameras = pygame.camera.list_cameras()
    c = pygame.camera.Camera(cameras[0])
    clock = pygame.time.Clock()
    c.start()
    
    going = True
    while going:
        time_passed = clock.tick(50)
        events = pygame.event.get()
        for e in events:
            if e.type == pygame.QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                going = False
    
        print 'get image begin'
        snapshot = c.get_image() #snapshot)
        print 'get image stop'
        display.blit(snapshot, (0, 0))
        pygame.display.flip()

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
    
    print "=======================>"
    s = c.get_raw()
    print s[1]
    
    time.sleep(2)
    
    c.stop()

# main
if __name__ == '__main__':
    test1()
    #test2()
    
