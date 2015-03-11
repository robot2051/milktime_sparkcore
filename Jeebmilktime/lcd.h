#ifndef LCD_H_INCLUDED
#define LCD_H_INCLUDED
#include <string>
#include "application.h"
/* include guards */

//All function lists here
void printLCD(String text);
void serCommand();
void backlightOff();
void backlightOn();
void clearLCD();
void goTo(int position);
void selectLineTwo();
void selectLineOne();

#endif
