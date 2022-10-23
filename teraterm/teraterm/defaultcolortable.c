static const BYTE DefaultColorTable[256][3] = {
  {  0,  0,  0}, {255,  0,  0}, {  0,255,  0}, {255,255,  0}, {  0,  0,255}, {255,  0,255}, {  0,255,255}, {255,255,255},  //   0 -   7
  {128,128,128}, {128,  0,  0}, {  0,128,  0}, {128,128,  0}, {  0,  0,128}, {128,  0,128}, {  0,128,128}, {192,192,192},  //   8 -  15
  {  0,  0,  0}, {  0,  0, 95}, {  0,  0,135}, {  0,  0,175}, {  0,  0,215}, {  0,  0,255}, {  0, 95,  0}, {  0, 95, 95},  //  16 -  23
  {  0, 95,135}, {  0, 95,175}, {  0, 95,215}, {  0, 95,255}, {  0,135,  0}, {  0,135, 95}, {  0,135,135}, {  0,135,175},  //  24 -  31
  {  0,135,215}, {  0,135,255}, {  0,175,  0}, {  0,175, 95}, {  0,175,135}, {  0,175,175}, {  0,175,215}, {  0,175,255},  //  32 -  39
  {  0,215,  0}, {  0,215, 95}, {  0,215,135}, {  0,215,175}, {  0,215,215}, {  0,215,255}, {  0,255,  0}, {  0,255, 95},  //  40 -  47
  {  0,255,135}, {  0,255,175}, {  0,255,215}, {  0,255,255}, { 95,  0,  0}, { 95,  0, 95}, { 95,  0,135}, { 95,  0,175},  //  48 -  55
  { 95,  0,215}, { 95,  0,255}, { 95, 95,  0}, { 95, 95, 95}, { 95, 95,135}, { 95, 95,175}, { 95, 95,215}, { 95, 95,255},  //  56 -  63
  { 95,135,  0}, { 95,135, 95}, { 95,135,135}, { 95,135,175}, { 95,135,215}, { 95,135,255}, { 95,175,  0}, { 95,175, 95},  //  64 -  71
  { 95,175,135}, { 95,175,175}, { 95,175,215}, { 95,175,255}, { 95,215,  0}, { 95,215, 95}, { 95,215,135}, { 95,215,175},  //  72 -  79
  { 95,215,215}, { 95,215,255}, { 95,255,  0}, { 95,255, 95}, { 95,255,135}, { 95,255,175}, { 95,255,215}, { 95,255,255},  //  80 -  87
  {135,  0,  0}, {135,  0, 95}, {135,  0,135}, {135,  0,175}, {135,  0,215}, {135,  0,255}, {135, 95,  0}, {135, 95, 95},  //  88 -  95
  {135, 95,135}, {135, 95,175}, {135, 95,215}, {135, 95,255}, {135,135,  0}, {135,135, 95}, {135,135,135}, {135,135,175},  //  96 - 103
  {135,135,215}, {135,135,255}, {135,175,  0}, {135,175, 95}, {135,175,135}, {135,175,175}, {135,175,215}, {135,175,255},  // 104 - 111
  {135,215,  0}, {135,215, 95}, {135,215,135}, {135,215,175}, {135,215,215}, {135,215,255}, {135,255,  0}, {135,255, 95},  // 112 - 119
  {135,255,135}, {135,255,175}, {135,255,215}, {135,255,255}, {175,  0,  0}, {175,  0, 95}, {175,  0,135}, {175,  0,175},  // 120 - 127
  {175,  0,215}, {175,  0,255}, {175, 95,  0}, {175, 95, 95}, {175, 95,135}, {175, 95,175}, {175, 95,215}, {175, 95,255},  // 128 - 135
  {175,135,  0}, {175,135, 95}, {175,135,135}, {175,135,175}, {175,135,215}, {175,135,255}, {175,175,  0}, {175,175, 95},  // 136 - 143
  {175,175,135}, {175,175,175}, {175,175,215}, {175,175,255}, {175,215,  0}, {175,215, 95}, {175,215,135}, {175,215,175},  // 144 - 151
  {175,215,215}, {175,215,255}, {175,255,  0}, {175,255, 95}, {175,255,135}, {175,255,175}, {175,255,215}, {175,255,255},  // 152 - 159
  {215,  0,  0}, {215,  0, 95}, {215,  0,135}, {215,  0,175}, {215,  0,215}, {215,  0,255}, {215, 95,  0}, {215, 95, 95},  // 160 - 167
  {215, 95,135}, {215, 95,175}, {215, 95,215}, {215, 95,255}, {215,135,  0}, {215,135, 95}, {215,135,135}, {215,135,175},  // 168 - 175
  {215,135,215}, {215,135,255}, {215,175,  0}, {215,175, 95}, {215,175,135}, {215,175,175}, {215,175,215}, {215,175,255},  // 176 - 183
  {215,215,  0}, {215,215, 95}, {215,215,135}, {215,215,175}, {215,215,215}, {215,215,255}, {215,255,  0}, {215,255, 95},  // 184 - 191
  {215,255,135}, {215,255,175}, {215,255,215}, {215,255,255}, {255,  0,  0}, {255,  0, 95}, {255,  0,135}, {255,  0,175},  // 192 - 199
  {255,  0,215}, {255,  0,255}, {255, 95,  0}, {255, 95, 95}, {255, 95,135}, {255, 95,175}, {255, 95,215}, {255, 95,255},  // 200 - 207
  {255,135,  0}, {255,135, 95}, {255,135,135}, {255,135,175}, {255,135,215}, {255,135,255}, {255,175,  0}, {255,175, 95},  // 208 - 215
  {255,175,135}, {255,175,175}, {255,175,215}, {255,175,255}, {255,215,  0}, {255,215, 95}, {255,215,135}, {255,215,175},  // 216 - 223
  {255,215,215}, {255,215,255}, {255,255,  0}, {255,255, 95}, {255,255,135}, {255,255,175}, {255,255,215}, {255,255,255},  // 224 - 231
  {  8,  8,  8}, { 18, 18, 18}, { 28, 28, 28}, { 38, 38, 38}, { 48, 48, 48}, { 58, 58, 58}, { 68, 68, 68}, { 78, 78, 78},  // 232 - 239
  { 88, 88, 88}, { 98, 98, 98}, {108,108,108}, {118,118,118}, {128,128,128}, {138,138,138}, {148,148,148}, {158,158,158},  // 240 - 247
  {168,168,168}, {178,178,178}, {188,188,188}, {198,198,198}, {208,208,208}, {218,218,218}, {228,228,228}, {238,238,238}   // 248 - 255
};
