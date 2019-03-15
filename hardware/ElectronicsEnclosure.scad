// Holes
// 188mm /  20mm

// Enclosure should be 200mm wide x 50mm deep x n-high
 
// Servo Control Board 85mmx35mm
// Power Supply 115mm x 65mm


/*

1x https://www.kitronik.co.uk/4345-clear-perspex-sheet-3mm-x-600mm-x-400mm.html
1x https://www.kitronik.co.uk/4363-yellow-opaque-perspex-sheet-3mm-x-600mm-x-400mm.html
1x https://www.kitronik.co.uk/4362-blue-opaque-perspex-sheet-3mm-x-600mm-x-400mm.html

*/


$fn=50;


baseWidth = 200;
baseHeight = 50;
frontHeight = 125;

tabsize = 40;
tabspacing = 80;
tabdepth = 3;

coolingSlotSize = 3;

3mmHole = 3.5;

//psu();
//nodeMcu();
//servoBoard();
//powerRail();

assembly();
//cutlist();

module cutlist()
{
	back();
	translate([0, 190, 0])
		front();
	translate([0, -95, 0])
		base();
	translate([0, 95, 0])
		top();
	for (i=[0,1])
	mirror([i, 0, 0])
	translate([-135, 0, 0])
		side();
}

module assembly()
{
	translate([0, 125/2, 0]) {
		linear_extrude(3)
			back();
		
		%translate([0, 0, 50-3])
		linear_extrude(3)
			front();

		color("grey")
		translate([0, 26, 3])
		rotate([0, 0, 0])
			psu();

		color("green")
		translate([-80, 30, 45])
		rotate([0, 0, 90])
		linear_extrude(1)
			nodeMcu();
		
		color("red")
		translate([0, -30, 5])
		//linear_extrude(1)
		rotate([0, 0, 0])
			servoBoard();
		
		color("black")
		translate([0, -50, 45])
			powerRail();
	}

	t = 0.7;

	color("blue", t)
	translate([0, 3, 25])
	rotate([90, 0, 0])
	linear_extrude(3)
		base();

	color("blue", t)
	translate([0, frontHeight, 25])
	rotate([90, 0, 0])
	linear_extrude(3)
		top();
	
	color("blue", t)
	for (i=[0,1])
	mirror([i, 0, 0])
	translate([-baseWidth/2, frontHeight/2, baseHeight/2])
	rotate([0, 90, 0])
	linear_extrude(3)
		side();
}

module side()
{
	difference() {
		square([baseHeight, frontHeight-2*tabdepth], center=true);
		
		// Tabs
		for (i=[0,1], j=[-tabspacing/2, tabspacing/2])
		mirror([i, 0, 0])		
		translate([-baseHeight/2+tabdepth/2, j, 0])
			square([tabdepth, tabsize], center=true);
		
		// Cooling
		for (i = [-5, -3, -2, -1])
		translate([0, i*10, 0])
			square([baseHeight-18, coolingSlotSize], center=true);
		
		// Nut traps
		for (i=[0,1])
		mirror([0, i, 0])
		translate([baseHeight/2 - 15/2, 40, 0]) {
			square([15, 3mmHole], center=true);
			translate([-4, 0, 0])
				square([2.5, 6], center=true);
		}
	}
}

module back() {
	
	difference() {
		square([baseWidth+6, frontHeight], center=true);
		
		backFrontTabs();
		
		// screw fastening holes
		for (i=[0,1], j=[0,1])
		mirror([i, 0, 0])
		mirror([0, j, 0])
		translate([baseWidth/2 - 3/2, 40, 0])
			circle(d=3mmHole);
		
		// psu mount holes
		translate([0, 26, 0])
		rotate([0, 0, 0])
			psuMounts();

		// zip tie mounts for servo board
		for(i=[0,1])
		mirror([i, 0, 0])
		translate([-45, -30, 0])
			square([3, 8], center=true);
		
		// Servo wire exits
		for(i=[0,1])
		mirror([i, 0, 0])
		translate([-45, -frontHeight/2, 0]) {
			square([20, 20], center=true);
			translate([0, 10, 0])
				circle(d=20);
		}
			
		// Power wire exits
		for(i=[0,1])
		mirror([i, 0, 0])
		translate([-80, frontHeight/2, 0]) {
			square([10, 10], center=true);
			translate([0, -5, 0])
				circle(d=10);
		}
		
		// Wall hangers?
		for (i=[0,1])
		mirror([i, 0, 0])
		translate([-70, 25, 0]) {
			square([7, 20], center=true);
			translate([0, 20/2, 0])
				circle(d=7);
			translate([0, -20/2, 0])
				circle(d=15);
		}
	}
}

module backFrontTabs()
{
	for (i=[0,1])
	mirror([0, i, 0])
	for (x = [-tabspacing, 0, tabspacing])
	translate([x, -frontHeight/2+tabdepth/2, 0])
		square([tabsize, tabdepth], center=true);

	for (i=[0,1])
	mirror([i, 0, 0])
	translate([-baseWidth/2+tabdepth/2, 0, 0])
		square([tabdepth, tabsize], center=true);

}

module front() {
	difference() {
		square([200, 125], center=true);
		
		backFrontTabs();
		
		translate([-80, 30, 0])
		rotate([0, 0, 90])
			nodeMcuMounts();
		
		// about message mounts
		translate([58, 10, 0])
		for (i=[0,1], j=[0,1])
		mirror([i, 0, 0])
		mirror([0, j, 0])
		translate([60/2, 80/2, 0])
			circle(d=3mmHole);
		
		translate([-93, -55, 0])
		scale([0.25, 0.25, 0.25])
		import("MakerspaceLogo.dxf");

	}
}


module base() {
	difference() {
		square([baseWidth, baseHeight], center=true);
		
		baseTopTabs();

		// Holes to mount to top of clock
		for (i=[0,1], j=[0,1])
		mirror([i, 0, 0])
		mirror([0, j, 0])
		translate([188/2, 20/2])
			circle(d=3mmHole);
		
		// power distribution board clip holes
		for (i=[0,1])
		mirror([i, 0, 0])
		translate([-100/2, 8, 0])
			square([3, 8], center=true);
	}
}

module top() 
{
	difference() {
		square([baseWidth, baseHeight], center=true);
	
		baseTopTabs();
		
		for (i = [-9:9])
		translate([i*10, 0, 0])
			square([coolingSlotSize, baseHeight-18], center=true);
	}
}

module baseTopTabs()
{
	for (i=[0,1])
	mirror([0, i, 0])
	for (x = [-tabspacing/2, tabspacing/2])
	translate([x, -baseHeight/2+tabdepth/2, 0])
		square([tabsize, tabdepth], center=true);
}

module powerRail()
{
	linear_extrude(1) // 20
		square([100, 10], center=true);
}

module servoBoard()
{
	linear_extrude(1) //35
		square([85, 36], center=true);
}

module nodeMcu(justcuts=false)
{
	render()
	difference() {
		if (!justcuts)
		hull()
		for (i=[0,1], j=[0,1])
		mirror([i, 0, 0])
		mirror([0, j, 0])
		translate([48.5/2-2, 25.75/2-2, 0])
			circle(d=4);
		
		nodeMcuMounts();
	}
}

module nodeMcuMounts()
{
	for (i=[0,1], j=[0,1])
	mirror([i, 0, 0])
	mirror([0, j, 0])
	translate([43.5/2, 20.5/2, 0])
		circle(d=3mmHole);
}

module psu()
{
	linear_extrude(6.5)
	difference() {
		roundedRect(113, 65, 5);
		
		psuMounts();
	}
	
	linear_extrude(32)
		roundedRect(90, 65, 2.5);
}

module psuMounts() {
	for (i=[0,1], j=[0,1])
	mirror([i, 0, 0])
	mirror([0, j, 0])
	translate([100/2, 50/2, 0])
		circle(d=4.5);
}

module roundedRect(width, height, radius)
{
	hull()
	for (i=[0,1], j=[0,1])
	mirror([i, 0, 0])
	mirror([0, j, 0])
	translate([width/2-radius, height/2-radius, 0])
		circle(r=radius);
}