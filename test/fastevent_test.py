#################################### IMPORTS ###################################

import test_utils
import test.unittest as unittest

import pygame
import pygame.event as event
import pygame.fastevent as fastevent

from event_test import race_condition_notification

################################################################################

class FasteventModuleTest(unittest.TestCase):
    def setUp(self):
        pygame.display.init()
        fastevent.init()
        event.clear()
        self.assert_(not event.get())

    def tearDown(self):
        pygame.display.quit()
    
    def test_get(self):
        # __doc__ (as of 2008-08-02) for pygame.fastevent.get:
    
          # pygame.fastevent.get() -> list of Events
          # get all events from the queue

        for _ in range(1, 11):
            event.post(event.Event(pygame.USEREVENT))
        
        self.assertEquals (
            [e.type for e in fastevent.get()], [pygame.USEREVENT] * 10,
            race_condition_notification
        )

    def todo_test_init(self):
        # __doc__ (as of 2008-08-02) for pygame.fastevent.init:
    
          # pygame.fastevent.init() -> None
          # initialize pygame.fastevent.

        self.fail()

    def test_poll(self):
    
        # __doc__ (as of 2008-08-02) for pygame.fastevent.poll:
    
          # pygame.fastevent.poll() -> Event
          # get an available event
          # 
          # Returns next event on queue. If there is no event waiting on the
          # queue, this will return an event with type NOEVENT.
        
        self.assertEquals (
            fastevent.poll().type, pygame.NOEVENT, race_condition_notification
        )
    
    def test_post(self):
    
        # __doc__ (as of 2008-08-02) for pygame.fastevent.post:
    
          # pygame.fastevent.post(Event) -> None
          # place an event on the queue
          # 
          # This will post your own event objects onto the event queue.
          # You can past any event type you want, but some care must be
          # taken. For example, if you post a MOUSEBUTTONDOWN event to the
          # queue, it is likely any code receiving the event will expect
          # the standard MOUSEBUTTONDOWN attributes to be available, like
          # 'pos' and 'button'.
          # 
          # Because pygame.fastevent.post() may have to wait for the queue
          # to empty, you can get into a dead lock if you try to append an
          # event on to a full queue from the thread that processes events.
          # For that reason I do not recommend using this function in the
          # main thread of an SDL program.
    
        for _ in range(1, 11):
            fastevent.post(event.Event(pygame.USEREVENT))
        
        self.assertEquals (
            [e.type for e in event.get()], [pygame.USEREVENT] * 10,
            race_condition_notification
        )
    
    def todo_test_pump(self):
    
        # __doc__ (as of 2008-08-02) for pygame.fastevent.pump:
    
          # pygame.fastevent.pump() -> None
          # update the internal messages
          # 
          # For each frame of your game, you will need to make some sort
          # of call to the event queue. This ensures your program can internally
          # interact with the rest of the operating system. If you are not using
          # other event functions in your game, you should call pump() to allow
          # pygame to handle internal actions.
          # 
          # There are important things that must be dealt with internally in the
          # event queue. The main window may need to be repainted. Certain joysticks
          # must be polled for their values. If you fail to make a call to the event
          # queue for too long, the system may decide your program has locked up.
    
        self.fail() 
    
    def test_wait(self):
    
        # __doc__ (as of 2008-08-02) for pygame.fastevent.wait:
    
          # pygame.fastevent.wait() -> Event
          # wait for an event
          # 
          # Returns the current event on the queue. If there are no messages
          # waiting on the queue, this will not return until one is
          # available. Sometimes it is important to use this wait to get
          # events from the queue, it will allow your application to idle
          # when the user isn't doing anything with it.
        
        event.post(pygame.event.Event(1))
        self.assertEquals(fastevent.wait().type, 1, race_condition_notification)
    
################################################################################

if __name__ == '__main__':
    unittest.main()