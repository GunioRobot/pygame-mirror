import pygame
from pygame import camera

camera.init()
print camera.list_cameras()
c = camera.Camera("")
print c
c.start()
print c
