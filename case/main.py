#! /usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import division
import os
import sys

from solid import *
from solid.utils import *

SEGMENTS = 200
screw_hole_diameter_3 = 2.85
cyl_6_2 = cylinder(d = 6.15, h = 10) + cylinder(d = 3, h = 20)

magnet_holes = (cyl_6_2
    + translate([0, 0, 0])(cyl_6_2)
    + translate([28, 38, 0])(cyl_6_2)
    + translate([28, 0, 0])(cyl_6_2)
    + translate([0, 38, 0])(cyl_6_2)
)

mount_plate = (cube([40, 50, 3.1])
    - translate([6, 6, -5])(magnet_holes)
)

case_bottom = (cube([40, 60, 3.1])
    - translate([6, 11, -5])(magnet_holes)
    + translate([24, 4, 0])(cylinder(d = 8, h = 5))
    - translate([24, 4, -1])(cylinder(d = screw_hole_diameter_3, h = 20))
    - translate([0, 0, 0])
)

full = mount_plate + translate([0, -5, 4])(case_bottom)

full = case_bottom

if __name__ == '__main__':
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.curdir
    file_out = os.path.join(out_dir, 'roller-blinds-controller-case.scad')

    print("%(__file__)s: SCAD file written to: \n%(file_out)s" % vars())

    # Adding the file_header argument as shown allows you to change
    # the detail of arcs by changing the SEGMENTS variable.  This can
    # be expensive when making lots of small curves, but is otherwise
    # useful.
    scad_render_to_file(full, file_out, file_header='$fn = %s;' % SEGMENTS)
