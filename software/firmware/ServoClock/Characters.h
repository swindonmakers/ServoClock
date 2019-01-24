#ifndef CHARACTERS_H
#define CHARACTERS_H

/* Servo Layout
        0 
      1   2
        3
      4   5
        6
*/

struct CLOCKDIGIT {
    // Which character this is - eg "a", "1"
    char digit;
    // Servo positions
    // Each bit represents the position of servos
    // in order 0123456x with "x" being spare
    char positions;
};

#define N_DIGITS 11

CLOCKDIGIT CDSPACE { ' ', 0b00000000 };

CLOCKDIGIT CD0 { '0', 0b11101110 };
CLOCKDIGIT CD1 { '1', 0b00100100 };
CLOCKDIGIT CD2 { '2', 0b10111010 };
CLOCKDIGIT CD3 { '3', 0b10110110 };
CLOCKDIGIT CD4 { '4', 0b01110100 };
CLOCKDIGIT CD5 { '5', 0b11010110 };
CLOCKDIGIT CD6 { '6', 0b11011110 };
CLOCKDIGIT CD7 { '7', 0b10100100 }; 
CLOCKDIGIT CD8 { '8', 0b11111110 };
CLOCKDIGIT CD9 { '9', 0b11110110 };


CLOCKDIGIT allDigits[N_DIGITS] = 
{ 
  CDSPACE,
  CD0,
  CD1,
  CD2,
  CD3,
  CD4,
  CD5,
  CD6,
  CD7,
  CD8,
  CD9
} ;

#endif