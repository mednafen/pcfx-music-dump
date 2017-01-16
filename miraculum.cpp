// TODO:  Add support for running on MSB-first platforms

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"

static bool UOFlowDetected = false;
static const bool EmulateBuggyCodec = true;
#include "pcfx-adpcm.inc"

typedef struct
{
 uint32 sector __attribute__((packed));
 uint16 count __attribute__((packed));
} song_pointer_t;

static const int NumSongs = 32;
song_pointer_t songs[NumSongs];

int main(int argc, char *argv[])
{
 FILE *fp;

 if(argc < 2)
 {
  printf("Usage: %s track2048.iso\n", argv[0]);
  return(-1);
 }

 if(!(fp = fopen(argv[1], "rb")))
 {
  printf("Couldn't open input file!\n");
  return(-1);
 }

 fseek(fp, 0x655da, SEEK_SET);
 fread(songs, sizeof(song_pointer_t), NumSongs, fp);

 for(int i = 0; i < NumSongs; i++)
 {
  uint8 raw_left[0x8000], raw_right[0x8000];
  int32 count = songs[i].count;
  char wavpath[256];
  SF_INFO sfi;
  SNDFILE *sf;

  printf("%08x:%04x\n", songs[i].sector, songs[i].count);

  snprintf(wavpath, 256, "miraculum-%d.wav", i);

  sfi.samplerate = ((double)1789772.727272727272 * 12 * 2) / 1365;
  sfi.channels = 2;
  sfi.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  sf = sf_open(wavpath, SFM_WRITE, &sfi);
  ResetADPCM(0);
  ResetADPCM(1);
  fseek(fp, 2048 * (songs[i].sector - 4395), SEEK_SET);
  
  while(count > 0)
  {
   fread(raw_left, 1, 0x8000, fp);
   fread(raw_right, 1, 0x8000, fp);

   //printf("%08x\n", ftell(fp)); //%02x %02x\n", raw_left[0], raw_left[1]);
   //exit(1);
   static int16 lri[0x8000 * 2][2];

   for(int i = 0; i < 0x8000 * 2; i++)
   {
    lri[i][0] = DecodeADPCMNibble(0, (raw_left[i >> 1] >> ((i & 1) * 4)) & 0xF);
    lri[i][1] = DecodeADPCMNibble(1, (raw_right[i >> 1] >> ((i & 1) * 4)) & 0xF);
   }
   sf_write_short(sf, &lri[0][0], sizeof(lri) / sizeof(lri[0][0]));
   count -= 0x8000 * 2 / 2048;
  }

  sf_close(sf);
 }

 fclose(fp);
}
