import time, pygame
from pygame import camera

camera.init()
l = camera.list_cameras()
print l
c = camera.Camera(l[0])
print c
c.start()
time.sleep(5)
print c
