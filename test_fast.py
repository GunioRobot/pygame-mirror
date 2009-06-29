import time
import pygame
import pygame.camera

if __name__ == '__main__':
    pygame.init()
    pygame.camera.init()
    
    l = pygame.camera.list_cameras()
    for c in l:
        print c
    
    c = pygame.camera.Camera(l[0])
    print c
    
    c.start()
    print 'camera started...'
    time.sleep(5)
    c.stop()
    print 'camera stopped...'
    time.sleep(3)
    
    print c
