# TODO

## Map Rendering

- [x] Use own container for Appearance
- [x] Use object sprite id to get sprite texture
- [x] Use sprite patterns
- [x] Implement animation
- [ ] Handle stackable appearances
- [x] Handle appearances with elevation
- [ ] BUG: Ground (and bottom stackpos items) should not be affected by tile elevation when rendered (both normally and as item preview)
- [ ] BUG: ESC should clear selections
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?

## Editing functionality

- [x] Implement multi-select
- [x] Delete selected items
- [ ] Move selected items
- [ ] topItem **not** selected: PRESS selects.
- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.

### GUI

- [ ] Replace Dear ImGui with QT

## General

- [x] Refactor the TimeMeasure class into TimePoint or similar
- [ ] System for custom keybindings
