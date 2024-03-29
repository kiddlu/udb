chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         min  mid  max
set params=0x3A 0xC0 0xFC

for /f "delims=" %%f in ('checksum.exe 0x6e 0x08 0x24 0x07 %params%') do (set checksum=%%f)

udb.exe shell himax-w 7 0x08 0x24 0x07 %params% %checksum%

echo "注意！AE参数需要重新开关流才生效"

pause


::static const struct
::{
::    unsigned int time;
::    unsigned char reg_expo_high;
::    unsigned char reg_expo_low;
::    unsigned char reg_pulse;
::} expo_table[] = {
::    { 01, 0x00, 0x50, 0x05 },
::    { 02, 0x00, 0xa0, 0x0a },
::    { 03, 0x01, 0x00, 0x10 },
::    { 04, 0x01, 0x50, 0x15 },
::    { 05, 0x01, 0xa0, 0x1a },
::    { 06, 0x02, 0x00, 0x20 },
::    { 07, 0x02, 0x50, 0x25 },
::    { 08, 0x02, 0xa0, 0x2a },
::    { 09, 0x03, 0x00, 0x30 },
::    { 10, 0x03, 0x50, 0x35 },
::    { 11, 0x03, 0xa0, 0x3a },
::    { 12, 0x04, 0x00, 0x40 },
::    { 13, 0x04, 0x50, 0x45 },
::    { 14, 0x04, 0xa0, 0x4a },
::    { 15, 0x05, 0x00, 0x50 },
::    { 16, 0x05, 0x50, 0x55 },
::    { 17, 0x05, 0xa0, 0x5a },
::    { 18, 0x06, 0x00, 0x60 },
::    { 19, 0x06, 0x50, 0x65 },
::    { 20, 0x06, 0xb0, 0x6b },
::    { 21, 0x07, 0x00, 0x70 },
::    { 22, 0x07, 0x50, 0x75 },
::    { 23, 0x07, 0xb0, 0x7b },
::    { 24, 0x08, 0x00, 0x80 },
::    { 25, 0x08, 0x50, 0x85 },
::    { 26, 0x08, 0xb0, 0x8b },
::    { 27, 0x09, 0x00, 0x90 },
::    { 28, 0x09, 0x50, 0x95 },
::    { 29, 0x09, 0xb0, 0x9b },
::    { 30, 0x0a, 0x00, 0xa0 },
::    { 31, 0x0a, 0x50, 0xa5 },
::    { 32, 0x0a, 0xb0, 0xab },
::    { 33, 0x0b, 0x00, 0xb0 },
::    { 34, 0x0b, 0x50, 0xb5 },
::    { 35, 0x0b, 0xb0, 0xbb },
::    { 36, 0x0c, 0x00, 0xc0 },
::    { 37, 0x0c, 0x50, 0xc5 },
::    { 38, 0x0c, 0xb0, 0xcb },
::    { 39, 0x0d, 0x00, 0xd0 },
::    { 40, 0x0d, 0x60, 0xd6 },
::    { 41, 0x0d, 0xb0, 0xdb },
::    { 42, 0x0e, 0x00, 0xe0 },
::    { 43, 0x0e, 0x60, 0xe6 },
::    //A6130323
::};
::
::static const struct
::{
::    unsigned int time;
::    unsigned char reg_expo_high;
::    unsigned char reg_expo_low;
::    unsigned char reg_pulse;
::} slave_mode_expo_table[] = {
::    {  1, 0x00, 0x50, 0x05},
::    {  2, 0x00, 0xA0, 0x0A},
::    {  3, 0x00, 0xF0, 0x0F},
::    {  4, 0x01, 0x40, 0x14},
::    {  5, 0x01, 0x90, 0x19},
::    {  6, 0x01, 0xE0, 0x1E},
::    {  7, 0x02, 0x30, 0x23},
::    {  8, 0x02, 0x80, 0x28},
::    {  9, 0x02, 0xD0, 0x2D},
::    { 10, 0x03, 0x30, 0x33},
::    { 11, 0x03, 0x80, 0x38},
::    { 12, 0x03, 0xD0, 0x3D},
::    { 13, 0x04, 0x10, 0x41},
::    { 14, 0x04, 0x60, 0x46},
::    { 15, 0x04, 0xB0, 0x4B},
::    { 16, 0x05, 0x00, 0x50},
::    { 17, 0x05, 0x50, 0x55},
::    { 18, 0x05, 0xA0, 0x5A},
::    { 19, 0x05, 0xF0, 0x5F},
::    { 20, 0x06, 0x40, 0x64},
::    { 21, 0x06, 0x90, 0x69},
::    { 22, 0x06, 0xE0, 0x6E},
::    { 23, 0x07, 0x30, 0x73},
::    { 24, 0x07, 0x80, 0x78},
::    { 25, 0x07, 0xD0, 0x7D},
::    { 26, 0x08, 0x20, 0x82},
::    { 27, 0x08, 0x70, 0x87},
::    { 28, 0x08, 0xC0, 0x8C},
::    { 29, 0x09, 0x10, 0x91},
::    { 30, 0x09, 0x60, 0x96},
::    { 31, 0x09, 0xB0, 0x9B},
::    { 32, 0x0A, 0x00, 0xA0},
::    { 33, 0x0A, 0x50, 0xA5},
::    { 34, 0x0A, 0xA0, 0xAA},
::    { 35, 0x0A, 0xF0, 0xAF},
::    { 36, 0x0B, 0x40, 0xB4},
::    { 37, 0x0B, 0x90, 0xB9},
::    { 38, 0x0B, 0xE0, 0xBE},
::    { 39, 0x0C, 0x30, 0xC3},
::    { 40, 0x0C, 0x80, 0xC8},
::    { 41, 0x0C, 0xD0, 0xCD},
::    { 42, 0x0D, 0x30, 0xD3},
::    { 43, 0x0D, 0x90, 0xD9},
::};