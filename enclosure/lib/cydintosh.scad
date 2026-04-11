include <BOSL2/std.scad>

// CYD (ESP32-2432S028) Dimensions
CYD_PCB_L = 86.0; // Long axis (X)
CYD_PCB_W = 50.0; // Short axis (Y)
CYD_PCB_D = 1.6;
CYD_PCB_SIZE = [CYD_PCB_L, CYD_PCB_W, CYD_PCB_D];

CYD_SCREEN_L = 69.4; // Long axis (X)
CYD_SCREEN_W = 50.0; // Short axis (Y)
CYD_SCREEN_D = 4.5;
CYD_SCREEN_SIZE = [CYD_SCREEN_L, CYD_SCREEN_W, CYD_SCREEN_D];

// Screen bezel dimensions (physical border around visible area)
CYD_SCREEN_BEZEL_TOP = 2.5; // mm
CYD_SCREEN_BEZEL_BOTTOM = 8.5; // mm
CYD_SCREEN_BEZEL_SIDE = 2.5; // mm (left and right)

// Visible area dimensions (pure LCD active area, no tolerance)
CYD_SCREEN_VISIBLE_SIZE_L = CYD_SCREEN_L - CYD_SCREEN_BEZEL_TOP - CYD_SCREEN_BEZEL_BOTTOM; // 58.4mm
CYD_SCREEN_VISIBLE_SIZE_W = CYD_SCREEN_W - CYD_SCREEN_BEZEL_SIDE * 2; // 45.0mm

// Center offset for aligning visible area to cutout center
// Positive = toward bottom (X+ direction)
CYD_SCREEN_VISIBLE_OFFSET_L = (CYD_SCREEN_BEZEL_BOTTOM - CYD_SCREEN_BEZEL_TOP) / 2; // 3.0mm

// Utility module: Position children at LCD visible area center
module pos_lcd_visible_center() {
  right(CYD_SCREEN_VISIBLE_OFFSET_L)
    children();
}

CYD_HOLE_D = 3.5;
CYD_HOLE_OFFSET = 3.8; // From corner center

module cyd_dummy() {
  color("yellow")
    cube(CYD_PCB_SIZE, anchor=BOTTOM);

  // Screen
  color("black")
    up(CYD_PCB_D)
      cube(CYD_SCREEN_SIZE, anchor=BOTTOM);

  // Mounting Holes
  color("white")
    xflip_copy(offset=(CYD_PCB_L / 2 - CYD_HOLE_OFFSET))
      yflip_copy(offset=(CYD_PCB_W / 2 - CYD_HOLE_OFFSET))
        cylinder(h=CYD_PCB_D + 0.2, d=CYD_HOLE_D, center=true);
}

// Enclosure for Cydintosh
FRAME_L = CYD_PCB_L + 1.0;
FRAME_W = CYD_PCB_W + 5.0;
FRAME_D = CYD_SCREEN_D + 2;
FRAME_ROUNDING = 2.0;

STAND_L = 39.0;
STAND_ANGLE = 70.0;
CABLE_HOLE_WALL = 1.8; // Bottom thickness for the cable hole

SCREW_HOLE_D = 1.7; // For M2 self-tapping

module cydintosh_icon() {
  // resize([8, 0, 0], auto=true)
  //   import("apple.svg", center=true, convexity=1);
}

module cydintosh_frame_2d() {
  rect([FRAME_L, FRAME_W], rounding=FRAME_ROUNDING);
}

module cydintosh_stand_front_2d() {
  rect([STAND_L, FRAME_W], rounding=FRAME_ROUNDING);
}

module cydintosh_stand_body(l = STAND_L) {
  difference() {
    rotate([0, 90, 0])
      // cylinder(d=FRAME_W, h=l, center=true);
      down(l / 2)
        linear_extrude(height=l)
          squircle([FRAME_W, FRAME_W], squareness=0.8);

    // Cut the bottom half to create a half-cylinder (Kamaboko) shape
    down(FRAME_W / 2)
      cube([l + 1, FRAME_W + 1, FRAME_W], anchor=CENTER);
  }
}

module cydintosh_stand(enable_ldr_hole = false) {
  color("#d0d0d0")
    difference() {
      union() {
        // Main Frame (X+ is top)
        difference() {
          // Main body
          linear_extrude(height=FRAME_D)
            cydintosh_frame_2d();

          // Screen cutout
          // space for LCD 
          up(FRAME_D - CYD_SCREEN_D)
            linear_extrude(height=FRAME_D + 0.1)
              rect([CYD_SCREEN_L + 0.15, CYD_SCREEN_W + 0.15]);

          // macintosh-style front screen cutout
          // center to LCD visible area
          pos_lcd_visible_center()
            hull() {
              up(FRAME_D - CYD_SCREEN_D)
                linear_extrude(height=0.01)
                  squircle(size=[CYD_SCREEN_VISIBLE_SIZE_L + 1.5, CYD_SCREEN_VISIBLE_SIZE_W + 1.0], squareness=0.85);

              down(0.1)
                linear_extrude(height=0.01)
                  rect([CYD_SCREEN_L - 5, CYD_SCREEN_W - 2], rounding=1.5);
            }
          ;
        }

        // Bridge block to fill 1mm gap between frame and stand (X=10, 5mm overlap on frame side, 4mm on stand side)
        translate([-FRAME_L / 2, 0, 0])
          linear_extrude(height=FRAME_D)
            rect([10, FRAME_W], anchor=CENTER);

        // Stand Part (Offset 1mm from -X edge of the frame)
        difference() {
          translate([-(FRAME_L / 2 + 1.0), 0, 0]) {
            hull() {
              // Front Plate
              left(STAND_L / 2)
                linear_extrude(height=0.1)
                  cydintosh_stand_front_2d();

              // Body (Half-Cylinder)
              left(STAND_L / 2)
                up(FRAME_D)
                  cydintosh_stand_body(l=STAND_L);
            }
          }

          // Cutting Cube for Stand Angle
          translate([-(FRAME_L / 2 + 1.0 + STAND_L), 0, 0])
            rotate([0, 90 - STAND_ANGLE, 0])
              cube([100, FRAME_W + 10, 200], anchor=BOTTOM + RIGHT);
        }
      }

      // Screen cutout (flex)
      translate([-(CYD_SCREEN_L) / 2 + 0.01, -(CYD_SCREEN_W / 2) + 5, 1])
        cube([1.2, 9.1, FRAME_D], anchor=BOTTOM + RIGHT + FWD);

      // Subtractions: Mounting holes, cable hole, and other cutouts
      up(1.0)
        xflip_copy(offset=(CYD_PCB_L / 2 - CYD_HOLE_OFFSET))
          yflip_copy(offset=(CYD_PCB_W / 2 - CYD_HOLE_OFFSET))
            cylinder(h=FRAME_D + 10, d=SCREW_HOLE_D);

      // Space for cable (type-c) with uniform bottom thickness
      type_c_y_offset_from_center = -1.7;
      translate([CABLE_HOLE_WALL, type_c_y_offset_from_center, FRAME_D])
        hull() {
          // Top block (at Z = FRAME_D)
          translate([-(FRAME_L / 2 + STAND_L) + ( (FRAME_W / 2) * tan(90 - STAND_ANGLE)), 0, 0])
            cube([STAND_L, 13.5, FRAME_W / 2], anchor=BOTTOM + LEFT);

          // Bottom thin block (at Z = CABLE_HOLE_WALL)
          translate([-(FRAME_L / 2 + STAND_L) + (CABLE_HOLE_WALL * tan(90 - STAND_ANGLE)), 0, 0])
            cube([STAND_L, 13.5, 0.01], anchor=BOTTOM + LEFT);
        }

      // Space for LDR
      translate([(CYD_PCB_L) / 2 - 2, -(CYD_PCB_W / 2) + 8, FRAME_D + 0.01])
        cube([4.3, 6.3, 1.5], anchor=TOP + RIGHT + FWD);

      // Hole for LDR
      if (enable_ldr_hole) {
        translate([(CYD_PCB_L) / 2 - 4.3, -(CYD_PCB_W / 2) + 11.3, -0.01])
          linear_extrude(height=FRAME_D + 1)
            rect([3, 4], rounding=1);
      }

      // Dwel (Macintosh Classic)
      left(CYD_SCREEN_L / 2 + 8)
        hull() {
          up(0.8)
            linear_extrude(height=0.01)
              rect([1.4, FRAME_W + 1]);
          down(0.01)
            linear_extrude(height=0.01)
              rect([1.7, FRAME_W + 1]);
        }
      ;

      // Macintosh-style smooth edge treatment
      right(-(1 + STAND_L) / 2) // centering to the whole body
        difference() {
          linear_extrude(height=2)
            rect([FRAME_L + STAND_L + 1 + 1, FRAME_W + 1], rounding=FRAME_ROUNDING);

          hull() {
            // front
            up(2)
              linear_extrude(height=0.01)
                rect([(FRAME_L + STAND_L + 1), FRAME_W]);

            // front
            linear_extrude(height=0.01)
              rect([FRAME_L + STAND_L + 1 - 5, FRAME_W - 2], rounding=FRAME_ROUNDING);
          }
          ;
        }
      ;
    }
  ;

  // Logo (Multi-color print required)
  translate([-(FRAME_L / 2 + 20), 0, -0.01]) {
    zrot(90)
      mirror([0, 1, 0]) {
        color("black")
          right(5)
            linear_extrude(height=0.5)
              resize([34, 0, 0], auto=true)
                import("cydintosh.svg", center=true, convexity=1);

        // Icon
        back((8 - 5) / 2)
          left(19) {
            logo_r = 1;
            color("green")
              linear_extrude(height=0.5)
                rect([8, 3], anchor=BOTTOM, rounding=[logo_r, logo_r, 0, 0]);
            color("red")
              linear_extrude(height=0.5)
                rect([8, 5], anchor=TOP, rounding=[0, 0, logo_r, logo_r]);
            color("black")
              back(( (8 - 5) ))
                linear_extrude(height=0.5)
                  shell2d(thickness=0.4, or=0, ir=0)
                    rect([8, 8], anchor=TOP, rounding=logo_r);
          }
        ;
      }
  }
  ;
}
