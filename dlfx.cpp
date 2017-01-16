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
const bool EmulateBuggyCodec = true;
#include "pcfx-adpcm.inc"

typedef struct
{
 uint32 sector __attribute__((packed));
} song_pointer_t;

static const int NumSongs = 36;
song_pointer_t songs[NumSongs];

int main(int argc, char *argv[])
{
 FILE *fp;

 if(argc < 2)
 {
  printf("Usage: %s derlangrisser.bin\n", argv[0]);
  return(-1);
 }

 if(!(fp = fopen(argv[1], "rb")))
 {
  printf("Couldn't open input file!\n");
  return(-1);
 }

 if(fseek(fp, 0x2A7D6E0, SEEK_SET) == -1)
 {
  printf("%m\n");
  return(-1);
 }

 if(fread(songs, sizeof(song_pointer_t), NumSongs, fp) != (size_t)NumSongs)
 {
  printf("%m\n");
  return(-1);
 }

 for(int i = 0; i < NumSongs; i++)
   songs[i].sector = (songs[i].sector >> 11) + 0x4ae0 - 225;

 for(int i = 0; i < NumSongs; i++)
 {
  uint8 raw_mono[2048];
  int32 count; // = songs[i + 1].sector - songs[i].sector;
  char wavpath[256];
  SF_INFO sfi;
  SNDFILE *sf;

  if(i == (NumSongs - 1)) // Last song is a dummy song in Der Langrisser FX
   break;
  else
   count = songs[i + 1].sector - songs[i].sector;

  printf("%08x:%04x\n", songs[i].sector, count);

  snprintf(wavpath, 256, "dlfx-%d.wav", i);

  sfi.samplerate = ((double)1789772.727272727272 * 12 * 2) / 1365;
  sfi.channels = 1;
  sfi.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  if(!(sf = sf_open(wavpath, SFM_WRITE, &sfi)))
  {
   printf("Error opening output file \"%s\".\n", wavpath);
   return(-1);
  }
  ResetADPCM(0);
  ResetADPCM(1);

  if(fseek(fp, 2352 * songs[i].sector + 12 + 3 + 1, SEEK_SET) == -1)
  {
   printf("%m\n");
   return(-1);
  }

  while(count > 0)
  {
   int16 out_buffer[2048 * 2];

   if(fread(raw_mono, 1, 2048, fp) != 2048)
   {
    printf("%m\n");
    return(-1);
   }

   for(int i = 0; i < 2048 * 2; i++)
   {
    out_buffer[i] = DecodeADPCMNibble(0, (raw_mono[i >> 1] >> ((i & 1) * 4)) & 0xF);
   }

   if(sf_write_short(sf, out_buffer, 2048 * 2) != 2048 * 2)
   {
    puts(sf_strerror(sf));
    return(-1);
   }

   count--;
   fseek(fp, 2352 - 2048, SEEK_CUR);
  }

  sf_close(sf);
 }

 fclose(fp);

 puts("DONE.");
}
