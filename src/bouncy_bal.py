#!/usr/bin/python
import time, random
import pygame
import pygame.camera
from pygame.locals import *

pygame.init()
pygame.camera.init()

class VideoCapturePlayer(object):
   size = ( 640, 480 )
   bsize = 50
   def __init__(self, **argd):
       self.__dict__.update(**argd)
       super(VideoCapturePlayer, self).__init__(**argd)
       self.display = pygame.display.set_mode( self.size )
       self.clist = pygame.camera.list_cameras()
       self.camera = pygame.camera.Camera(self.clist[0], self.size, "RGB")
       self.camera.start()
       self.clock = pygame.time.Clock()
       self.snapshot = pygame.surface.Surface(self.size, 0, 24)
       self.snapshoto = pygame.surface.Surface(self.size, 0, 24)
       self.background = pygame.surface.Surface(self.size, 0, 24)
       self.backgroundo = pygame.surface.Surface(self.size, 0, 24)
       self.thresh = pygame.surface.Surface(self.size, 0, 24)
       self.laplace = pygame.surface.Surface(self.size, 0, 24)
       self.weird = pygame.surface.Surface(self.size, 0, 24)
       self.bubble = pygame.surface.Surface((50,50), 0, 24)
       self.bubble.set_colorkey((0,0,0))
       self.bubble.fill((0,0,0))
       pygame.draw.circle(self.bubble,(0,0,255), (self.bsize/2,self.bsize/2), self.bsize/2, 0)
       self.bmask = pygame.mask.from_surface(self.bubble)
       self.thresh.set_colorkey((255,0,0))
       self.coord = (self.size[0]/2,self.size[1]/2)
       self.yv = 5
       self.xv = 5
       random.seed()

      
   def calibrate(self):
       self.backgroundo = self.camera.get_image(self.backgroundo)
       self.background = pygame.transform.flip(self.backgroundo, True, False)
       self.display.blit(self.background, (0,0))
       pygame.display.flip()

   def get_and_flip(self):
       self.snapshoto = self.camera.get_image(self.snapshoto)
       self.snapshot = pygame.transform.flip(self.snapshoto, True, False)
       pygame.transform.threshold(self.thresh, self.snapshot, (255,0,0), (40,40,40), (255,0,0), True, self.background)
       mask = pygame.mask.from_surface(self.thresh)
       mask.invert() 
       self.display.fill((50,255,128))
       self.display.blit(self.thresh, (0,0))
       overlap = mask.overlap_area(self.bmask, self.coord)
       self.yv += 1
       self.display.blit(self.bubble, self.coord)
       if not overlap:
           self.coord = (self.coord[0]+self.xv, self.coord[1]+self.yv)
           if self.coord[0] > self.size[0]-self.bsize:
               self.xv = (-self.xv)+1
               self.coord = (self.size[0]-self.bsize, self.coord[1])
           if self.coord[0] < 0:
               self.xv = (-self.xv)-1
               self.coord = (0, self.coord[1])      
           if self.coord[1] > self.size[1]-self.bsize:
               self.yv = (-self.yv)+1
               self.coord = (self.coord[0], self.size[1]-self.bsize)
           if self.coord[1] < 0:
               self.yv = (-self.yv)-1
               self.coord = (self.coord[0], 0)
       else:
           hit = mask.overlap(self.bmask, self.coord)
           pygame.draw.circle(self.display,(200,200,255), hit, overlap/(self.bsize/2), 0)
           hity = self.coord[1]+self.bsize/2-hit[1]
           hitx = self.coord[0]+self.bsize/2-hit[0]
           self.yv = (self.yv/2)+(hity/2)
           self.xv = (self.xv/2)+(hitx/2)
#           print (hitx, hity), (self.xv, self.yv)
           if mask.overlap_area(self.bmask, self.coord):
               self.coord = (self.coord[0]+hitx, self.coord[1]+hity)
            
       pygame.display.flip()

   def main(self):
       going = True
       while going:
           events = pygame.event.get()
           for e in events:
               if e.type == KEYDOWN:
                   pygame.time.wait(2000)
                   #pygame.time.wait(2)
                   self.calibrate()
                   going = False
           self.calibrate()
       going = True
       while going:
           events = pygame.event.get()
           for e in events:
               if e.type == QUIT or (e.type == KEYDOWN and e.key == K_ESCAPE):
                   going = False

           self.get_and_flip()
           self.clock.tick()
#           print self.clock.get_fps()

VideoCapturePlayer().main()
