#include "OBDuinoTFTLCD.h"

#include <Adafruit_TFTLCD.h>
#include <SPI.h> 

#if (sclk == 0)
  Adafruit_TFTLCD tft(cs, cd, wr, rd, rst);
#endif

#if (sclk > 0)
  ST7735 tft = ST7735(cs, dc, mosi, sclk, rst); 
#endif

void progmemPrint(const char *str);
void progmemPrintln(const char *str);
//--------------------------------------------------------------------------------

OBDuinoLCD::OBDuinoLCD(void)
{
  SetCursor(0, 0);
  ColorMode = 0;
  BackGroundColor = CL_BLACK;
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::InitOBDuinoLCD(void)
{
    SPI.setClockDivider(TFTInitSPISpeed);

#ifdef UseSoftwareReset
    pinMode(rstpin, OUTPUT);

    digitalWrite(rstpin, HIGH);
#endif
    
    //  tft.initR();               // initialize a ST7735R chip
    tft.reset();
    uint16_t identifier = tft.readID();
    if (identifier == 0x9325) {
      progmemPrintln(PSTR("Found ILI9325 LCD driver"));
    } else if(identifier == 0x9328) {
      progmemPrintln(PSTR("Found ILI9328 LCD driver"));
    } else if(identifier == 0x7575) {
      progmemPrintln(PSTR("Found HX8347G LCD driver"));
    } else {
      progmemPrint(PSTR("Unknown LCD driver chip: "));
      Serial.println(identifier, HEX);
      return;
    }

    tft.begin(identifier);
//  tft.writecommand(ST7735_DISPON);

#if TFTInitSPISpeed != TFTDataSPISpeed
    SPI.setClockDivider(TFTDataSPISpeed);
#endif

    // Flip upside down. I have the screen mounted oddly
    tft.setRotation(2);
    tft.setTextColor(CL_WHITE, BackGroundColor);
    tft.setTextSize(2);
    tft.fillScreen(BackGroundColor);
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::ReinitOBDuinoLCD(void)
{
  InitOBDuinoLCD();
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::LCDInitChar(void)
{
  //nothing
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::SetCursor(byte Position, byte Row)
{
  tft_row = Row;
  tft_position = Position;
}
//--------------------------------------------------------------------------------

// Print a character on the first row of the screen. Above everything else.
void OBDuinoLCD::PrintWarningChar(char c)
{
  if (tft_position >= LCD_COLS)
    return;

  tft.drawChar(WarningPosition(tft_position, tft_row), c, CL_MAIN, BackGroundColor, 2);
  tft_position++;
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::PrintWarning(char *string)
{
  while (string[0] != 0)
  {
    PrintWarningChar(string[0]);
    string++;
  }  
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::PrintWarning_P(const char *string)
{
  char c;
  while ((c = pgm_read_byte(string++)))
    PrintWarningChar(c);
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::ClearPrintWarning_P(const char *string)
{
  ClearWarning();
  PrintWarning_P(string);
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::ClearWarning(void)
{
    tft.setCursor(0,0);
//  for (tft_row = 0; tft_row < 2; tft_row++)
    for (tft_position = 0; tft_position < LCD_COLS;) {
      PrintWarningChar(0x00);
    }

    tft_row = 0;
    tft_position = 0;
}
//--------------------------------------------------------------------------------

static const uint16_t BarColors[4+8] PROGMEM = {
                                           CL_LIGHT_GREEN, CL_GREEN, CL_YELLOW, CL_RED,
                                           CL_RED, CL_RED, CL_ORANGE, CL_YELLOW, CL_GREEN, CL_GREEN, CL_YELLOW, CL_RED
                                         }; 
//--------------------------------------------------------------------------------

void OBDuinoLCD::LCDBar(byte Position, uint16_t Value, uint16_t MaxValue, char *string)
{
  uint16_t Color;
  byte i;
  byte Left;
  byte Top;
  byte Length = strlen(string);
  
  if (Position & 0x02) //Horizontal
  {
    Left = 6;
    Top  = 111 + 9 * (Position & 0x01);
  
    // Draw bar
    for (i = 0; i < 16; i++)
    {
      Color = BackGroundColor;
    
      if (Value > MaxValue / 16 * i)
        Color = pgm_read_word(BarColors + byte(i / 4));

      Left += tft.print('\n');
    }
  
    // Draw text
    Left += 6;
  
    Left += tft.print(string);
//    for (i = 0; i < Length; i++)
//      Left += tft.redrawChar(Left, Top, string[i], CL_MAIN, BackGroundColor);
  }
  else //Vertical
  {
    Left = 154 * (Position & 0x01);
    
    // Draw bar
    for (i = 0; i < 15; i++)
    {
      Color = BackGroundColor;
    
      if (Value > MaxValue / 16 * i)
        Color = pgm_read_word(BarColors + ((Position & 0x04) ? (4 + byte(i / 2)) : byte(i / 4)));

      tft.print('\n');
    }  

    // Draw text
    Left = (Position & 0x08) ? (tft.width() - Length * 6) : 0;
  
    tft.print(string);
//    for (i = 0; i < Length + 2; i++)
//      Left += tft.redrawChar(Left, 0, (i < Length) ? string[i] : 0x00, CL_MAIN, BackGroundColor);
  }
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::LCDNum(byte Position, char *string)
{ 
  byte Size     =   Position & 0x07;
//  byte Top      = ((Position & 0xF0) >> 4) * 9 + 9 + 2;

  char *output_str = string;
  
  if (Size == 6)
  {
    output_str = string + 20;
    sprintf_P(output_str, PSTR("%7s"), string);
  }
  
//  byte Length = strlen(output_str);
  
//  byte Left = 20 -                           // default
              ((Size == 4) ? 6 : 0);         // for 2-3rd rows (small font)

  tft.print(string);
//  for (byte i = 0; i < Length; i++)
//    Left += tft.redrawChar(Left, Top, output_str[i], CL_MAIN, BackGroundColor, Size);
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::LCDTime(char *string)
{
  byte Length = strlen(string);
  byte Left = 60;

  tft.print(string);
//  for (byte i = 0; i < Length; i++)
//    Left += tft.redrawChar(Left, 0, string[i], CL_MAIN, BackGroundColor);
}
//--------------------------------------------------------------------------------

void OBDuinoLCD::LCDClearBottom(void)
{
  tft.fillRect(6, 107, 148, 3, BackGroundColor);
}
//--------------------------------------------------------------------------------


void OBDuinoLCD::SwitchDayNightMode(void)
{
  ColorMode++;

  if (ColorMode % 2 == 1)
  {
//    tft.writecommand(ST7735_INVON);

    return;
  }

//  tft.writecommand(ST7735_INVOFF);

  BackGroundColor = CL_WHITE - BackGroundColor;
  tft.fillScreen(BackGroundColor); 
}
//--------------------------------------------------------------------------------

Adafruit_TFTLCD* OBDuinoLCD::getTFT(void)
{
    return &tft;
}

void progmemPrint(const char *str) {
  char c;
  while ( (c = pgm_read_byte(str++)) ) Serial.print(c);
}

// Same as above, with trailing newline
void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}
