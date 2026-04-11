include <lib/cydintosh.scad>

$fn = 100;

// CYD Dummy (aligned to the frame)
// X+ is top of the screen.
*#up(CYD_SCREEN_D + CYD_PCB_D)
  rotate([180, 0, 0])
    cyd_dummy();

cydintosh_stand();
