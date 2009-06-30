import time
import pygame
import pygame.camera

if __name__ == '__main__':
    pygame.init()
    pygame.camera.init()
    
    l = pygame.camera.list_cameras()
    for c in l:
        print c
    
    size = (640, 480)
    display = pygame.display.set_mode(size, 0)
    snapshot = pygame.surface.Surface(size, 0, display)
    c = pygame.camera.Camera(l[0])
    print c
    
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
