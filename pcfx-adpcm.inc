static const int StepSizes[49] =
{
 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50,
 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157,
 173, 190,  209, 230, 253, 279, 307, 337, 371, 408, 449,
 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
};

static const int StepIndexDeltas[16] =
{
 -1, -1, -1, -1, 2, 4, 6, 8,
 -1, -1, -1, -1, 2, 4, 6, 8
};

static int32 ADPCMPredictor[2];
static int32 StepSizeIndex[2];


static void ResetADPCM(int ch)
{
 ADPCMPredictor[ch] = 0;
 StepSizeIndex[ch] = 0;
}


static inline int32 DecodeADPCMNibble(int ch, uint8 nibble)
{
 int32 ret;
 int32 delta = 0;
 assert(nibble < 0x10);

 int32 BaseStepSize = StepSizes[StepSizeIndex[ch]];

 if(EmulateBuggyCodec)
 {
  if(BaseStepSize == 1552)
   BaseStepSize = 1522;

  delta = BaseStepSize * ((nibble & 0x7) + 1) * 2;
 }
 else
  delta = BaseStepSize * ((nibble & 0x7) + 1);

 if(nibble & 0x8)
  delta = -delta;

 StepSizeIndex[ch] += StepIndexDeltas[nibble];

 if(StepSizeIndex[ch] < 0)
  StepSizeIndex[ch] = 0;

 if(StepSizeIndex[ch] > 48)
  StepSizeIndex[ch] = 48;

 ADPCMPredictor[ch] += delta;

 if(ADPCMPredictor[ch] > 0x3FFF) { ADPCMPredictor[ch] = 0x3FFF; UOFlowDetected = true; /*printf("Overflow: %d\n", ch);*/ }
 if(ADPCMPredictor[ch] < -0x4000) { ADPCMPredictor[ch] = -0x4000; UOFlowDetected = true; /*printf("Underflow: %d\n", ch);*/ }

 if(EmulateBuggyCodec)
  ret = ADPCMPredictor[ch];
 else
  ret = ADPCMPredictor[ch] << 1;

 return(ret);
}

