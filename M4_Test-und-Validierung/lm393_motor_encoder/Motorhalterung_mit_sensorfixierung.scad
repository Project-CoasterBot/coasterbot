// Motorfixierung + seitlicher Halter für LM393-Gabellichtschranke (T-Platine)
$fn = 48;

STL = "/home/gereon/proj/coasterbot/git_repo/M4_Test-und-Validierung/Mechanikkonzept/model/coasterbot_stl/Motorfixierung.stl.stl";

// --- Sensor-Maße ---
hole_dist   = 21.5;  // Bohrabstand Mitte-Mitte
pcb_w       = 26.5;  // Breite Querbalken der T-Platine
hole_d      = 3.2;   // 3.2 = M3 durchstecken, 2.8 = M3 direkt ins Plastik schrauben

// --- Halter-Maße ---
plate_t     = 3;     // Dicke der Halteplatte
plate_d     = 12;    // wie weit die Platte seitlich absteht
hole_inset  = 6;     // Abstand Bohrmitte zur Seitenwand des Halters
standoff_h  = 3;     // Abstandshalter (Platz für Lötpins unter der Platine)
standoff_d  = 6.5;
sensor_x    = 21.8 - (pcb_w + 2)/2;  // Sensormitte entlang X; so schließt die Platte bündig mit der Schraubfläche (X=21.8) ab

module motorfixierung() {
    // verschiebt das STL in den Nullpunkt: X 0..21.8, Y 0..34, Z 0..16
    translate([-162.344, 299.817, 0]) import(STL);
}

lm_x = -14.3;
lm_y = -2.3;
lm_z = 22.9;
lm_len = 21.8;
lm_w = 6.6;
lm_h = 4.2;
lm_Tw = 12;
lm_Tlen = 8;
lm_Hlen = 4;
lm_Hh = 9;
lm_Hdist = (14-9)*0.5;

module lm393_sensor() {
    color("#3080ff") {
    rotate([-90,0,0]) {
    union() {
                cube([lm_len, lm_w, lm_h]);
        
        translate([0, lm_w/2, 0])
                cylinder(d = lm_w, h = lm_h);
        translate([lm_len, lm_w/2, 0])
                cylinder(d = lm_w, h = lm_h);
        
        translate([lm_len/2 - lm_Tw/2, lm_w, 0])
                cube([lm_Tw, lm_Tlen, lm_h*0.5]);
        
        translate([lm_len/2-lm_Hlen-lm_Hdist, 0, lm_h])
                cube([lm_Hlen, lm_w, lm_Hh]);
        
        translate([lm_len/2+lm_Hdist, 0, lm_h])
                cube([lm_Hlen, lm_w, lm_Hh]);
    }
}
}
}


// Halter an der Außenseite des Arms bei Y=0, flach auf Z=0
module lm393_halter() {
    color("#80ff30") {
        rotate([-90,0,0]){
    difference() {
        union() {
            translate([sensor_x - (pcb_w + 2)/2, -plate_d, 0])
                cube([pcb_w + 2, plate_d + 0.01, plate_t]);
            for (s = [-1, 1])
                translate([sensor_x + s * hole_dist/2, -hole_inset, 0])
                    cylinder(d = standoff_d, h = plate_t + standoff_h);
        }
        for (s = [-1, 1])
            translate([sensor_x + s * hole_dist/2, -hole_inset, -1])
                cylinder(d = hole_d, h = plate_t + standoff_h + 2);
    }
}
}
}


// --- Verbindung Halter <-> Motorfixierung ---
halter_pos = [lm_x + 3, lm_y - 6.25, lm_z - 9.25];  // unverändert, wie bisher

// Lage der Halteplatte im Raum (ergibt sich aus rotate([-90,0,0]) + halter_pos)
plate_x0 = halter_pos.x + sensor_x - (pcb_w + 2)/2;
plate_x1 = plate_x0 + pcb_w + 2;
plate_y0 = halter_pos.y;            // Außenseite der Platte
plate_z0 = halter_pos.z;            // Unterkante der Platte

mf_h      = 16;    // Höhe der Motorfixierung
steg_x0   = 3.5;   // Steg beginnt hier (Abstand zum T-Fuß des Sensors)
steg_x1   = min(plate_x1, 21.8);
ueberlapp = 3.;   // wie weit der Steg in den Arm ragt (für eine saubere Verbindung)

module verbindung() {
    color("#80ff30") {
        // Steg: von der Halteplatte bis in die Seitenwand des Arms, volle Höhe
        translate([steg_x0, plate_y0, 0])
            cube([steg_x1 - steg_x0, -plate_y0 + ueberlapp, mf_h]);

        // 45°-Stütze unter dem überstehenden Teil der Platte
        hull() {
            translate([steg_x0 - 0.01, plate_y0, 0])
                cube([0.01, plate_t, plate_z0 + 0.01]);
            translate([steg_x0 - plate_z0, plate_y0, plate_z0])
                cube([plate_z0, plate_t, 0.01]);
        }
    }
}

union() {
    motorfixierung();
    translate(halter_pos) lm393_halter();
    verbindung();
}

//translate([lm_x, lm_y, lm_z]) lm393_sensor();