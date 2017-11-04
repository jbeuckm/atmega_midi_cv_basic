translate([0,0,2]) rotate([180]) EuroPanel("MIDI", 7);

module EuroPanel(label, hp=8) {
    
  font = ".SF Compact Display:style=Bold";
  labelFontSize = 5.8;

  sideOffset = 4;
  panelWidth = hp*5.08;

  translate([panelWidth,0,0]) scale([-1,1,1]) Support();
  
  
  difference() {
    cube([hp*5.08, 128.5, 2]);
      
    translate([2,115,1])  {
        linear_extrude(height=2, convexity=4) text(label, size=8, font=font);
    }
    translate([7.5,3]) MountingHole();
    translate([panelWidth - 7.5, 128.5 - 3]) MountingHole();

    translate ([sideOffset, 117, 0]) union() {
        translate([0,-87.750,0]) MIDIHole();
        translate([0,-71.39,0]) LEDHole();
        translate([0,-59.97,0]) JackHole();
        translate([0,-44.7,0]) JackHole();
        translate([0,-29.43,0]) JackHole();
        translate([0,-14.17,0]) JackHole();
    }
    translate([18,10,1]) {
      translate([0,90])   {
          linear_extrude(height=2, convexity=4) text("gate", size=labelFontSize, font=font);
      }
      translate([0,75])   {
          linear_extrude(height=2, convexity=4) text("pch", size=labelFontSize, font=font);
      }
      translate([0,60])   {
          linear_extrude(height=2, convexity=4) text("tone", size=labelFontSize, font=font);
      }
      translate([0,45])   {
          linear_extrude(height=2, convexity=4) text("velo", size=labelFontSize, font=font);
      }
  }

  }
/*
    color("green", .3)
        translate([panelWidth - sideOffset, 14.25,2])
            cube([1,100,50]);
            */
}

module KnobHole() { translate([12.5,0,-.5]) cylinder(h=3, d=8, $fn=12); }

module JackHole() { translate([6,0,-.5]) cylinder(h=3, d=6.3, $fn=12); }

module LEDHole() { translate([10,0,-.5]) cylinder(h=3, d=5.3, $fn=12); }

module MIDIHole() { translate([10,0,-.5]) cylinder(h=3, d=18.5, $fn=24); }

module Support() {
    translate([0,10,0])
    rotate([-90,0,0])
  linear_extrude(108.5)
    polygon([[0,0], [0,2], [1.5,5], [3,5], [3,0]]);
}


module MountingHole() {
  hull() {
    translate([-2,0,-.5]) cylinder(h=3, r=1.6, $fn=12);
    translate([2,0,-.5]) cylinder(h=3, r=1.6, $fn=12);
  }
}

echo(version=version());
