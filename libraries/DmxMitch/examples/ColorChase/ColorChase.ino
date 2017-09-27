#include <DmxMitch.h>


/* To use DmxMitch, you will need the following line. Arduino will
** auto-insert it if you select Sketch > Import Library > DmxMitch. */

void setup() {
  /* The most common pin for DMX output is pin 3, which DmxMitch
** uses by default. If you need to change that, do it here. */
  DmxMitch.usePin(3);
  DmxMitch.write(1, 255);// This is optional (Puts Master channel on my parcan to full)
  /* DMX devices typically need to receive a complete set of channels
** even if you only need to adjust the first channel. You can
** easily change the number of channels sent here.  */
  DmxMitch.maxChannel(4);
}

void loop() {
  int brightness;
  /* Simple loop to Chase Colors */
   DmxMitch.write(2, 255);   // turn the channel full on (255 is the chanel level)
  delay(1000);              // wait for a second
   DmxMitch.write(2, 0);    // turn the channel low ( Off)
  
    DmxMitch.write(3, 255);   // turn the channel full on (255 is the chanel level)
  delay(1000);              // wait for a second
    DmxMitch.write(3, 0);    // turn the channel low ( Off)
 
    DmxMitch.write(4, 255);   // turn the channel full on (255 is the chanel level)
  delay(1000);              // wait for a second
    DmxMitch.write(4, 0);    // turn the channel low ( Off)

 
  }




