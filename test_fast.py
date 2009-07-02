import time
import pygame
import pygame.camera
from pygame.locals import *

if __name__ == '__main__':
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
        snapshot = c.get_image(snapshot)
        print 'get image stop'
        display.blit(snapshot, (0, 0))
    c.stop()
    

def stuff():
    c.start()
    print 'camera started...'
    time.sleep(2)
    print '======================================='
    snapshot = c.get_image(snapshot)
    print snapshot
    display.blit(snapshot, (0,0))
    print '======================================='
    time.sleep(2)
    c.stop()
    print 'camera stopped...'
    time.sleep(2)
    
    print c
