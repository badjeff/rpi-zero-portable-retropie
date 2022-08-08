#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit function
#include <inttypes.h>  // uint8_t, etc
#include <time.h>      // Needed for struct timespec
#include <linux/i2c-dev.h> // I2C bus definitions

#define NANO_SECOND_MULTIPLIER  1000000  // 1 millisecond = 1,000,000 Nanoseconds
#define ADS1115_CONVERSIONDELAY (2 * NANO_SECOND_MULTIPLIER)

void write_ADS1115(int fd, int channel) {
  // set config register and start conversion
  // AIN0 and GND, 4.096v, 860s/s
  // Refer to page 19 area of spec sheet
  uint8_t writeBuf[3];
  //--------------------------------------------
  writeBuf[0] = 1; // config register is 1
  //--------------------------------------------
  writeBuf[1] = 0b11000011; // 0xC3 single shot off
  // Bit 15
  //   flag bit for single shot not used here
  // Bits 14-12 input selection:
  //   100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
  // Bits 11-9 Amp gain.
  //   Default to 010 here 001 P19
  // Bit 8 Operational mode of the ADS1115.
  //   0 : Continuous conversion mode
  //   1 : Power-down single-shot mode (default)
  switch (channel) {
    case (0): writeBuf[1] |= 0b01000000; break;
    case (1): writeBuf[1] |= 0b01010000; break;
    case (2): writeBuf[1] |= 0b01100000; break;
    case (3): writeBuf[1] |= 0b01110000; break;
  }
  //--------------------------------------------
  writeBuf[2] = 0b11100101; // bits 7-0  0x85
  // Bits 7-5
  //   data rate 100 for 128SPS (>=15ms conversion delay)
  //             101 fpr 250SPS (>=8ms conversion delay)
  //             110 for 475SPS (>=2ms conversion delay)
  //             111 for 860SPS (>=2ms conversion delay)
  // Bits 4-0
  //   comparator functions see spec sheet.
  //--------------------------------------------
  // begin conversion
  if (write(fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    // exit (1);
  }
}

int16_t read_ADS1115(int fd, int channel) {
  write_ADS1115(fd, channel);
  nanosleep((const struct timespec[]){{0, ADS1115_CONVERSIONDELAY}}, NULL);

  // set config register to 0 for read
  uint8_t readBuf[2];
  readBuf[0] = 0;
  if (write(fd, readBuf, 1) != 1) {
    perror("Write register select");
    // exit(-1);
  }
  // read conversion register
  if (read(fd, readBuf, 2) != 2) {
    perror("Read conversion");
    // exit(-1);
  }
  // could also multiply by 256 then add readBuf[1]
  int16_t val = readBuf[0] << 8 | readBuf[1];
  // with +- LSB sometimes generates very low neg number.
  if (val < 0) val = 0;
  return val;
}

int main() {

  // open device on /dev/i2c-1 the default on Raspberry Pi B
  int fd;
  if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
    printf("Error: Couldn't open device! %d\n", fd);
    // exit (1);
  }

  // connect to ADS1115 as i2c slave
  int ads_address = 0x48;
  if (ioctl(fd, I2C_SLAVE, ads_address) < 0) {
    printf("Error: Couldn't find device on address!\n");
    // exit (1);
  }

  const float VPS = 4.096 / 32768.0; // volts per step
  int val;

  val = read_ADS1115(fd, 0);
  printf("channel 0 0x%02x | %5d | %4.3f volts\n", val, val, val * VPS);

  val = read_ADS1115(fd, 1);
  printf("channel 1 0x%02x | %5d | %4.3f volts\n", val, val, val * VPS);

  val = read_ADS1115(fd, 2);
  printf("channel 2 0x%02x | %5d | %4.3f volts\n", val, val, val * VPS);

  val = read_ADS1115(fd, 3);
  printf("channel 3 0x%02x | %5d | %4.3f volts\n", val, val, val * VPS);

  close(fd);

  return 0;
}
