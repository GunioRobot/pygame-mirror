#################################### IMPORTS ###################################

import unittest, pygame, test_utils

from test_utils import unordered_equality, test_not_implemented

from pygame import sprite
    
################################# MODULE LEVEL #################################

class SpriteModuleTest( unittest.TestCase ):
    pass

######################### SPRITECOLLIDE FUNCTIONS TEST #########################

class spritecollide_Test( unittest.TestCase ):
    def setUp(self):
        self.ag = sprite.AbstractGroup()
        self.ag2 = sprite.AbstractGroup()
        self.s1 = sprite.Sprite(self.ag)
        self.s2 = sprite.Sprite(self.ag2)
        self.s3 = sprite.Sprite(self.ag2)

        self.s1.image = pygame.Surface((50,10), pygame.SRCALPHA, 32)
        self.s2.image = pygame.Surface((10,10), pygame.SRCALPHA, 32)
        self.s3.image = pygame.Surface((10,10), pygame.SRCALPHA, 32)

        self.s1.rect = self.s1.image.get_rect()
        self.s2.rect = self.s2.image.get_rect()
        self.s3.rect = self.s3.image.get_rect()
        self.s2.rect.move_ip(40, 0)
        self.s3.rect.move_ip(100, 100)

    def test_spritecollide__works_if_collided_cb_is_None(self):
    # Test that sprites collide without collided function.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, collided = None
            ),
            [self.s2]
        )

    def test_spritecollide__works_if_collided_cb_not_passed(self):        
    # Should also work when collided function isn't passed at all.
        self.assertEqual(sprite.spritecollide (
            self.s1, self.ag2, dokill = False),
            [self.s2]
        )
    
    def test_spritecollide__collided_must_be_a_callable(self):        
        # Need to pass a callable.
        self.assertRaises ( 
            TypeError, 
            sprite.spritecollide, self.s1, self.ag2, dokill = False, collided = 1
        )

    def test_spritecollide__collided_defaults_to_collide_rect(self):        
        # collide_rect should behave the same as default.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, collided = sprite.collide_rect
            ),
            [self.s2]
        )

    def test_collide_rect_ratio__ratio_of_one_like_default(self):
        # collide_rect_ratio should behave the same as default at a 1.0 ratio.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, 
                collided = sprite.collide_rect_ratio(1.0)
            ),
            [self.s2]
        )
    
    def test_collide_rect_ratio__collides_all_at_ratio_of_twenty(self):        
        # collide_rect_ratio should collide all at a 20.0 ratio.
        self.assert_ (
            unordered_equality (
                sprite.spritecollide (
                    self.s1, self.ag2, dokill = False, 
                    collided = sprite.collide_rect_ratio(20.0)
                ),
                [self.s2, self.s3]
            )
        )

    def test_collide_circle__no_radius_set(self):
        # collide_circle with no radius set.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, collided = sprite.collide_circle
            ),
            [self.s2]
        )

    def test_collide_circle_ratio__no_radius_and_ratio_of_one(self):        
        # collide_circle_ratio with no radius set, at a 1.0 ratio.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, 
                collided = sprite.collide_circle_ratio(1.0)
            ),
            [self.s2]
        )
    
    def test_collide_circle_ratio__no_radius_and_ratio_of_twenty(self):
        # collide_circle_ratio with no radius set, at a 20.0 ratio.
        self.assert_ ( 
            unordered_equality (
                sprite.spritecollide (
                    self.s1, self.ag2, dokill = False, 
                    collided = sprite.collide_circle_ratio(20.0)
                ),
                [self.s2, self.s3]
            )
        )
    
    def test_collide_circle__with_radii_set(self):
        # collide_circle with a radius set.
        
        self.s1.radius = 50
        self.s2.radius = 10
        self.s3.radius = 400

        self.assert_ ( 
            unordered_equality (
                sprite.spritecollide (
                    self.s1, self.ag2, dokill = False, 
                    collided = sprite.collide_circle
                ),
                [self.s2, self.s3]
            )
        )

    def test_collide_circle_ratio__with_radii_set(self):
        self.s1.radius = 50
        self.s2.radius = 10
        self.s3.radius = 400

        # collide_circle_ratio with a radius set.
        self.assert_ ( 
            unordered_equality (
                sprite.spritecollide (
                    self.s1, self.ag2, dokill = False, 
                    collided = sprite.collide_circle_ratio(0.5)
                ),
                [self.s2, self.s3]
            )
        )
                
    def test_collide_mask(self):        
        # make some fully opaque sprites that will collide with masks.
        self.s1.image.fill((255,255,255,255))
        self.s2.image.fill((255,255,255,255))
        self.s3.image.fill((255,255,255,255))

        # masks should be autogenerated from image if they don't exist.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, 
                collided = sprite.collide_mask
            ),
            [self.s2]
        )
        
        self.s1.mask = pygame.mask.from_surface(self.s1.image)
        self.s2.mask = pygame.mask.from_surface(self.s2.image)
        self.s3.mask = pygame.mask.from_surface(self.s3.image)

        # with set masks.
        self.assertEqual (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, 
                collided = sprite.collide_mask
            ),
            [self.s2]
        )

    def test_collide_mask(self):
        # make some sprites that are fully transparent, so they won't collide.
        self.s1.image.fill((255,255,255,0))
        self.s2.image.fill((255,255,255,0))
        self.s3.image.fill((255,255,255,0))

        self.s1.mask = pygame.mask.from_surface(self.s1.image, 255)
        self.s2.mask = pygame.mask.from_surface(self.s2.image, 255)
        self.s3.mask = pygame.mask.from_surface(self.s3.image, 255)

        self.assertFalse (
            sprite.spritecollide (
                self.s1, self.ag2, dokill = False, collided = sprite.collide_mask
            )
        )

################################################################################

class AbstractGroupTest( unittest.TestCase ):
    def test_AbstractGroup__has( self ):
        " See if AbstractGroup.has() works as expected. "

        ag = sprite.AbstractGroup()
        ag2 = sprite.AbstractGroup()
        s1 = sprite.Sprite(ag)
        s2 = sprite.Sprite(ag)
        s3 = sprite.Sprite(ag2)
        s4 = sprite.Sprite(ag2)
    
        self.assertEqual(True, s1 in ag )
    
        self.assertEqual(True, ag.has(s1) )
    
        self.assertEqual(True, ag.has([s1, s2]) )
    
        # see if one of them not being in there.
        self.assertNotEqual(True, ag.has([s1, s2, s3]) )
    
        # see if a second AbstractGroup works.
        self.assertEqual(True, ag2.has(s3) )

################################################################################

# A base class to share tests between similar classes

class LayeredGroup:
    def test_LayeredGroup__get_layer_of_sprite(self):
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        self.LG.add(spr, layer=666)
        self.assert_(len(self.LG._spritelist)==1)
        self.assert_(self.LG.get_layer_of_sprite(spr)==666)
        self.assert_(self.LG.get_layer_of_sprite(spr)==self.LG._spritelayers[spr])
        
        
    def test_LayeredGroup__add(self):
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        self.LG.add(spr)
        self.assert_(len(self.LG._spritelist)==1)
        self.assert_(self.LG.get_layer_of_sprite(spr)==self.LG._default_layer)
        
    def test_LayeredGroup__add__sprite_with_layer_attribute(self):
        #test_add_sprite_with_layer_attribute
        
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        spr._layer = 100
        self.LG.add(spr)
        self.assert_(len(self.LG._spritelist)==1)
        self.assert_(self.LG.get_layer_of_sprite(spr)==100)
        
    def test_LayeredGroup__add__passing_layer_keyword(self):
        # test_add_sprite_passing_layer
        
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        self.LG.add(spr, layer=100)
        self.assert_(len(self.LG._spritelist)==1)
        self.assert_(self.LG.get_layer_of_sprite(spr)==100)
        
    def test_LayeredGroup__add__overriding_sprite_layer_attr(self):
        # test_add_sprite_overriding_layer_attr
        
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        spr._layer = 100
        self.LG.add(spr, layer=200)
        self.assert_(len(self.LG._spritelist)==1)
        self.assert_(self.LG.get_layer_of_sprite(spr)==200)
        
    def test_LayeredGroup__add__adding_sprite_on_init(self):
        # test_add_sprite_init
        
        spr = self.sprite()
        lrg2 = sprite.LayeredUpdates(spr)
        self.assert_(len(lrg2._spritelist)==1)
        self.assert_(lrg2._spritelayers[spr]==lrg2._default_layer)
        
    def test_LayeredGroup__add__sprite_init_layer_attr(self):
        # test_add_sprite_init_layer_attr
        
        spr = self.sprite()
        spr._layer = 20
        lrg2 = sprite.LayeredUpdates(spr)
        self.assert_(len(lrg2._spritelist)==1)
        self.assert_(lrg2._spritelayers[spr]==20)
        
    def test_LayeredGroup__add__sprite_init_passing_layer(self):
        # test_add_sprite_init_passing_layer
        
        spr = self.sprite()
        lrg2 = sprite.LayeredUpdates(spr, layer=33)
        self.assert_(len(lrg2._spritelist)==1)
        self.assert_(lrg2._spritelayers[spr]==33)
        
    def test_LayeredGroup__add__sprite_init_overiding_layer(self):
        # test_add_sprite_init_overiding_layer
        
        spr = self.sprite()
        spr._layer = 55
        lrg2 = sprite.LayeredUpdates(spr, layer=33)
        self.assert_(len(lrg2._spritelist)==1)
        self.assert_(lrg2._spritelayers[spr]==33)
        
    def test_LayeredGroup__add__spritelist(self):
        # test_add_spritelist
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
        self.LG.add(sprites)
        self.assert_(len(self.LG._spritelist)==10)
        for i in range(10):
            self.assert_(self.LG.get_layer_of_sprite(sprites[i])==self.LG._default_layer)
        
    def test_LayeredGroup__add__spritelist_with_layer_attr(self):
        # test_add_spritelist_with_layer_attr
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
            sprites[-1]._layer = i
        self.LG.add(sprites)
        self.assert_(len(self.LG._spritelist)==10)
        for i in range(10):
            self.assert_(self.LG.get_layer_of_sprite(sprites[i])==i)
        
    def test_LayeredGroup__add__spritelist_passing_layer(self):
        # test_add_spritelist_passing_layer
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
        self.LG.add(sprites, layer=33)
        self.assert_(len(self.LG._spritelist)==10)
        for i in range(10):
            self.assert_(self.LG.get_layer_of_sprite(sprites[i])==33)
        
    def test_LayeredGroup__add__spritelist_overriding_layer(self):
        # test_add_spritelist_overriding_layer
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
            sprites[-1].layer = i
        self.LG.add(sprites, layer=33)
        self.assert_(len(self.LG._spritelist)==10)
        for i in range(10):
            self.assert_(self.LG.get_layer_of_sprite(sprites[i])==33)
            
    def test_LayeredGroup__add__spritelist_init(self):
        # test_add_spritelist_init

        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
        lrg2 = sprite.LayeredUpdates(sprites)
        self.assert_(len(lrg2._spritelist)==10)
        for i in range(10):
            self.assert_(lrg2.get_layer_of_sprite(sprites[i])==self.LG._default_layer)
        
    def test_LayeredGroup__remove__sprite(self):
        # test_remove_sprite
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
            sprites[-1].rect = 0
        self.LG.add(sprites)
        self.assert_(len(self.LG._spritelist)==10)
        for i in range(10):
            self.LG.remove(sprites[i])
        self.assert_(len(self.LG._spritelist)==0)
        
    def test_LayeredGroup__sprites(self):
        # test_sprites
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            sprites.append(self.sprite())
            sprites[-1]._layer = 10-i
        self.LG.add(sprites)
        self.assert_(len(self.LG._spritelist)==10)
        for idx,spr in enumerate(self.LG.sprites()):
            self.assert_(spr == sprites[9-idx])
        
    def test_LayeredGroup__layers(self):
        # test_layers
        
        self.assert_(len(self.LG._spritelist)==0)
        sprites = []
        for i in range(10):
            for j in range(5):
                sprites.append(self.sprite())
                sprites[-1]._layer = i
        self.LG.add(sprites)
        lays = self.LG.layers()
        for i in range(10):
            self.assert_(lays[i] == i)
            
    def test_LayeredGroup__add__layers_are_correct(self):  #TODO
        # test_layers2

        self.assert_(len(self.LG)==0)
        layers = [1,4,6,8,3,6,2,6,4,5,6,1,0,9,7,6,54,8,2,43,6,1]
        for lay in layers:
            self.LG.add(self.sprite(), layer=lay)
        layers.sort()
        for idx, spr in enumerate(self.LG.sprites()):
            self.assert_(self.LG.get_layer_of_sprite(spr)==layers[idx])

    def test_LayeredGroup__change_layer(self):
        # test_change_layer
        
        self.assert_(len(self.LG._spritelist)==0)
        spr = self.sprite()
        self.LG.add(spr, layer=99)
        self.assert_(self.LG._spritelayers[spr] == 99)
        self.LG.change_layer(spr, 44)
        self.assert_(self.LG._spritelayers[spr] == 44)
        
        spr2 = self.sprite()
        spr2.layer = 55
        self.LG.add(spr2)
        self.LG.change_layer(spr2, 77)
        self.assert_(spr2.layer == 77)
        
    def test_LayeredGroup__get_top_layer(self):
        # test_get_top_layer
        
        layers = [1,5,2,8,4,5,3,88,23,0]
        for i in layers:
            self.LG.add(self.sprite(), layer=i)
        self.assert_(self.LG.get_top_layer()==max(layers))
        self.assert_(self.LG.get_top_layer()==max(self.LG._spritelayers.values()))
        self.assert_(self.LG.get_top_layer()==self.LG._spritelayers[self.LG._spritelist[-1]])
            
    def test_LayeredGroup__get_bottom_layer(self):
        # test_get_bottom_layer
        
        layers = [1,5,2,8,4,5,3,88,23,0]
        for i in layers:
            self.LG.add(self.sprite(), layer=i)
        self.assert_(self.LG.get_bottom_layer()==min(layers))
        self.assert_(self.LG.get_bottom_layer()==min(self.LG._spritelayers.values()))
        self.assert_(self.LG.get_bottom_layer()==self.LG._spritelayers[self.LG._spritelist[0]])
            
    def test_LayeredGroup__move_to_front(self):
        # test_move_to_front
        
        layers = [1,5,2,8,4,5,3,88,23,0]
        for i in layers:
            self.LG.add(self.sprite(), layer=i)
        spr = self.sprite()
        self.LG.add(spr, layer=3)
        self.assert_(spr != self.LG._spritelist[-1]) 
        self.LG.move_to_front(spr)
        self.assert_(spr == self.LG._spritelist[-1]) 
        
    def test_LayeredGroup__move_to_back(self):
        # test_move_to_back
        
        layers = [1,5,2,8,4,5,3,88,23,0]
        for i in layers:
            self.LG.add(self.sprite(), layer=i)
        spr = self.sprite()
        self.LG.add(spr, layer=55)
        self.assert_(spr != self.LG._spritelist[0]) 
        self.LG.move_to_back(spr)
        self.assert_(spr == self.LG._spritelist[0]) 
        
    def test_LayeredGroup__get_top_sprite(self):
        # test_get_top_sprite
        
        layers = [1,5,2,8,4,5,3,88,23,0]
        for i in layers:
            self.LG.add(self.sprite(), layer=i)
        self.assert_(self.LG.get_layer_of_sprite(self.LG.get_top_sprite())== self.LG.get_top_layer())
        
    def test_LayeredGroup__get_sprites_from_layer(self):
        # test_get_sprites_from_layer
        
        self.assert_(len(self.LG)==0)
        sprites = {}
        layers = [1,4,5,6,3,7,8,2,1,3,4,5,6,7,8,9,1,2,3,4,5,6,7,8,9,0,1,6,5,4,3,2]
        for lay in layers:
            spr = self.sprite()
            spr._layer = lay
            self.LG.add(spr)
            if not sprites.has_key(lay):
                sprites[lay] = []
            sprites[lay].append(spr)
            
        for lay in self.LG.layers():
            for spr in self.LG.get_sprites_from_layer(lay):
                self.assert_(spr in sprites[lay])
                sprites[lay].remove(spr)
                if len(sprites[lay]) == 0:
                    del sprites[lay]
        self.assert_(len(sprites.values())==0)
        
    def test_LayeredGroup__switch_layer(self):
        # test_switch_layer

        self.assert_(len(self.LG)==0)
        sprites1 = []
        sprites2 = []
        layers = [3,2,3,2,3,3,2,2,3,2,3,2,3,2,3,2,3,3,2,2,3,2,3]
        for lay in layers:
            spr = self.sprite()
            spr._layer = lay
            self.LG.add(spr)
            if lay==2:
                sprites1.append(spr)
            else:
                sprites2.append(spr)
                
        for spr in sprites1:
            self.assert_(spr in self.LG.get_sprites_from_layer(2))
        for spr in sprites2:
            self.assert_(spr in self.LG.get_sprites_from_layer(3))
        self.assert_(len(self.LG)==len(sprites1)+len(sprites2))

        self.LG.switch_layer(2,3)

        for spr in sprites1:
            self.assert_(spr in self.LG.get_sprites_from_layer(3))
        for spr in sprites2:
            self.assert_(spr in self.LG.get_sprites_from_layer(2))
        self.assert_(len(self.LG)==len(sprites1)+len(sprites2))


########################## LAYERED RENDER GROUP TESTS ##########################

class LayeredUpdates_SpriteTest(LayeredGroup, unittest.TestCase):
    sprite = sprite.Sprite

    def setUp(self):
        self.LG = sprite.LayeredUpdates()

class LayeredUpdates_DirtySpriteTest(LayeredGroup, unittest.TestCase):
    sprite = sprite.DirtySprite

    def setUp(self):
        self.LG = sprite.LayeredUpdates()

class LayeredDirty_DirtySpriteTest(LayeredGroup, unittest.TestCase):
    sprite = sprite.DirtySprite

    def setUp(self):
        self.LG = sprite.LayeredDirty()


############################### SPRITE BASE CLASS ##############################
#
# tests common between sprite classes

class Sprite:
    def setUp(self):
        self.groups = []
        for Group in self.Groups:
            self.groups.append(Group())
        
        self.sprite = self.Sprite()
    
    def test_Sprite____init____added_to_groups_passed(self):
        self.sprite = self.Sprite(self.groups)

        self.assert_(unordered_equality(
            self.sprite.groups(),
            self.groups
        ))

    def test_Sprite__add(self):
        self.sprite.add(self.groups)

        self.assert_(unordered_equality(
            self.sprite.groups(),
            self.groups
        ))

    def test_Sprite__add_internal(self):
        self.assert_(test_not_implemented())
    
    def test_Sprite__alive(self):
        self.assert_(
            not self.sprite.alive(),
            "Sprite should not be alive if in no groups"
        )

        self.sprite.add(self.groups)
        self.assert_(self.sprite.alive())

    def test_Sprite__groups(self):
        for i, g in enumerate(self.groups):
            self.sprite.add(g)

            groups = self.sprite.groups()
            self.assert_( unordered_equality (
                    groups,
                    self.groups[:i+1],
            ))
    
    def test_Sprite__kill(self):
        self.sprite.add(self.groups)
        
        self.assert_(self.sprite.alive())
        self.sprite.kill()
        
        self.assert_(not self.sprite.groups() and not self.sprite.alive() )        
    
    def test_Sprite__remove(self):
        self.sprite.add(self.groups)
        self.sprite.remove(self.groups)
        self.assert_(not self.sprite.groups())

    def test_Sprite__remove_internal(self):
        self.assert_(test_not_implemented())

    def test_Sprite__update(self):
        # Doc string for pygame.sprite.Sprite.update:
    
          # method to control sprite behavior
          # Sprite.update(*args):
          # 
          # The default implementation of this method does nothing; it's just a
          # convenient "hook" that you can override. This method is called by
          # Group.update() with whatever arguments you give it.
          # 
          # There is no need to use this method if not using the convenience
          # method by the same name in the Group class.
          # 
    
        self.assert_(test_not_implemented())

############################## SPRITE CLASS TESTS ##############################

class SpriteTest(Sprite, unittest.TestCase):
    Sprite = sprite.Sprite

    Groups = [ sprite.Group, 
               sprite.LayeredUpdates,
               sprite.RenderUpdates, 
               sprite.OrderedUpdates, ]

class DirtySpriteTest(Sprite, unittest.TestCase):
    Sprite = sprite.DirtySprite
    
    Groups = [ sprite.Group,
               sprite.LayeredUpdates,
               sprite.RenderUpdates,
               sprite.OrderedUpdates,
               sprite.LayeredDirty, ]

################################################################################

if __name__ == '__main__':
    test_utils.get_fail_incomplete_tests_option()
    unittest.main()