# Appearance

## FrameGroup

### SpriteInfo

A **sprite pattern** is a collection of sprites. The sprite is usually (always?) chosen by indexing the pattern using a position {x, y, z}.

- bounding_box_per_direction
  One bounding box specifies the sprite offset within its square.

  - x=3 means the sprite X starts 3 pixels from the start of the box.
  - width=28 then means that the bounding box in x is [3, 31].

- bounding_square
  The bounding square is something like min(bbox_width, bbox_height) (not correct).
  That is, it is the smallest square that can contain the sprite pixels.

- pattern_width
  Width of the sprite pattern.

- pattern_height
  Height of the sprite pattern.

- pattern_depth
  Depth of the sprite pattern.
