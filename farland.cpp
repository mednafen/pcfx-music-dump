// for f in farland-*.wav; do sox $f -r 48000 temp.wav rate -v -s && opusenc --bitrate 128 temp.wav ${f%.wav}.opus; done
//
// TODO:  Add support for running on MSB-first platforms

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>
#include <map>

#include "types.h"

static bool UOFlowDetected = false;
static const bool EmulateBuggyCodec = true;
#include "pcfx-adpcm.inc"

typedef struct
{
 uint32 sector __attribute__((packed));
 uint16 length __attribute__((packed));
 uint8 dummy[0x14 - 2 - 4] __attribute__((packed));
} song_pointer_t;

static const int NumSongs = 0xD;
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

 fseek(fp, 0xBEFCC, SEEK_SET);
 fread(songs, sizeof(song_pointer_t), NumSongs, fp);

 for(int i = 0; i < NumSongs; i++)
   songs[i].sector = songs[i].sector - 4395; //3 * 75; //4395; // - 3 * 75;

 std::map<uint64, bool> RepeatMurder;

 for(int i = 0; i < NumSongs; i++)
 {
  uint8 raw_mono[2048];
  int32 count;
  char wavpath[256];
  SF_INFO sfi;
  SNDFILE *sf;

  count = songs[i].length;

  printf("%d, %08x:%04x\n", i, songs[i].sector, count);
  snprintf(wavpath, 256, "farland-%d.wav", i);

  if(RepeatMurder[songs[i].sector | ((uint64)songs[i].length << 32)])
  {
   printf(" Skipping(Repeat)\n");
   continue;
  }

  RepeatMurder[songs[i].sector | ((uint64)songs[i].length << 32)] = true;

  sfi.samplerate = ((double)1789772.727272727272 * 12 * 2) / 1365;
  sfi.channels = 1;
  sfi.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  sf = sf_open(wavpath, SFM_WRITE, &sfi);
  ResetADPCM(0);
  ResetADPCM(1);
  UOFlowDetected = false;

  fseek(fp, 2048 * songs[i].sector, SEEK_SET);
  
  while(count > 0)
  {
   fread(raw_mono, 1, 2048, fp);

   for(int i = 0; i < 2048 * 2; i++)
   {
    int16 lri;
    lri = DecodeADPCMNibble(0, (raw_mono[i >> 1] >> ((i & 1) * 4)) & 0xF);

    sf_write_short(sf, &lri, 1);
   }

   count--;
  }

  if(UOFlowDetected)
   printf("Under/Over-flow detected.\n");

  sf_close(sf);
 }

 fclose(fp);
}
