/*
  Konami.h - Library for entering the Konami
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/

#ifndef Konami_h
#define Konami_h

#include "Arduino.h"

/* sequence puzzle config */
int btnSeq[9] = {4,4,0,0,1,3,1,3,2}; // button sequence
int btnSeqIdx = 0; // button sequence index
const int btnSeqSz = sizeof(btnSeq)/sizeof(*btnSeq); // button sequence size
int checkSequence(int guess){
  if (guess == btnSeq[btnSeqIdx]) btnSeqIdx++%btnSeqSz;
  else btnSeqIdx = 0;
  return btnSeqIdx;
}

#endif
