// for f in zeroigar-*.wav; do sox $f -r 48000 temp.wav rate -v -s && opusenc --bitrate 170 temp.wav ${f%.wav}.opus; done
//
// Updated March 14, 2015 to correct totally wrong decoding algorithm.
//
// TODO:  Add support for running on MSB-first platforms

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#include "types.h"

static bool UOFlowDetected = false;
static bool EmulateBuggyCodec;
#include "pcfx-adpcm.inc"

// Each entry 16 bytes
typedef struct
{
 uint32 num;
 uint32 length; // Maybe
 uint32 loop_point; // Maybe
 uint32 loop_length; // Maybe
} song_spec_t;

static const int NumSongs = 31;
static const int NumSongSPS = 47;

uint32 songs_sectors[NumSongSPS];
song_spec_t songs_spec[NumSongs];

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

 fseek(fp, 0x12480, SEEK_SET);
 fread(songs_sectors, sizeof(uint32), NumSongSPS, fp);

 fseek(fp, 0x1253C, SEEK_SET); //0x1953C, SEEK_SET);
 fread(songs_spec, sizeof(song_spec_t), NumSongs, fp);

 for(int i = 0; i < NumSongs; i++)
 {
  char wavpath[256];
  SF_INFO sfi;
  SNDFILE *sf;
  uint32 sector = songs_sectors[songs_spec[i].num];
  int32 count = songs_spec[i].length / 4; // * 2;
  uint32 loop_counter;
  uint32 loop_times;

  printf("%d, %08x:%08x %08x:%08x\n", i, sector, songs_spec[i].length, songs_sectors[songs_spec[i].loop_point], songs_spec[i].loop_length);

  if(songs_sectors[songs_spec[i].num] == songs_sectors[songs_spec[i].loop_point] &&
     songs_spec[i].length == songs_spec[i].loop_length)
  {
   loop_times = 1;
   puts("Short loop");
  }
  else
   loop_times = 2;

  snprintf(wavpath, 256, "zeroigar-%d.wav", i);

  sfi.samplerate = round(((double)1789772.727272727272 * 12 * 2) / 1365);
  sfi.channels = 2;
  sfi.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  sf = sf_open(wavpath, SFM_WRITE, &sfi);

  if(i == 10)
   EmulateBuggyCodec = true;
  else
   EmulateBuggyCodec = false;

  UOFlowDetected = false;
  ResetADPCM(0);
  ResetADPCM(1);
  fseek(fp, 2048 * (sector - 1982), SEEK_SET);
  
  loop_counter = 0;
  while(count > 0)
  {
   uint16 *raw;
   int16 *outbuffer;

   raw = (uint16 *)malloc(count * sizeof(uint16) * 2);
   outbuffer = (int16 *)malloc(count * sizeof(int16) * 4 * 2);

   fread(raw, 1, count * sizeof(uint16) * 2, fp);

   for(int zoo = 0; zoo < count; zoo++)
   {
    for(int kaka = 0; kaka < 4; kaka++)
    {
     outbuffer[0 + zoo * 8 + kaka * 2] = DecodeADPCMNibble(0, (raw[0 + zoo * 2] >> ((kaka & 3) * 4)) & 0xF);
     outbuffer[1 + zoo * 8 + kaka * 2] = DecodeADPCMNibble(1, (raw[1 + zoo * 2] >> ((kaka & 3) * 4)) & 0xF);
    }
   }
   sf_write_short(sf, outbuffer, count * 4 * 2);  
   count = 0;
   free(raw);
   free(outbuffer);

   if(loop_counter < loop_times)
   {
    loop_counter++;
    //fseek(fp, 2048 * (sector - 1982) + ((songs_spec[i].length / 2) & ~ 1), SEEK_SET);
    fseek(fp, 2048 * (songs_sectors[songs_spec[i].loop_point] - 1982), SEEK_SET);
    count = songs_spec[i].loop_length / 4;
    ResetADPCM(0);
    ResetADPCM(1);
   }
  }
  if(UOFlowDetected)
   printf("Under/Over-flow detected.\n");

  sf_close(sf);
 }

 fclose(fp);
}
