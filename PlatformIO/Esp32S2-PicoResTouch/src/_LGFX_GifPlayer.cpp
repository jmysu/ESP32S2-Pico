/*\
 *
 * ESP32-GifPlayer-LittleFS
 * ************************
 *
 * https://github.com/tobozo/ESP32-GifPlayer
 *
 * GIF player Demo for  ESP32S2-PICO + LovyanGFX
 *
 * This sketch will open FS gif files and queue all files found in the /gif
 * folder then play them in an endless loop.
 *
 *
 * This demo is based on Larry Bank's AnimatedGIF library:
 *   https://github.com/bitbank2/ && https://github.com/tobozo/ESP32-GifPlayer
 * 
 * Modified w/ Waveshare ESP32S2-Pico 2.8" ST7789 LCD on LittleFS
 *
 * Library dependencies (available from the Arduino Library Manager):
 *
 * - AnimatedGIF        https://github.com/bitbank2/AnimatedGIF (Apache License 2.0)
 * - LovyanGFX          https://github.com/lovyan03/LovyanGFX (FreeBSD License)
 * - SimpleList         https://github.com/spacehuhn/SimpleList (MIT License)
 *
 * MIT License
 *
 * Copyright (c) 2020 tobozo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
\*/
#include <Arduino.h>
#include <LittleFS.h>
#include <USB.h>
 
#if ARDUINO_USB_CDC_ON_BOOT //Serial used for USB CDC
 #define HWSerial Serial0
 USBCDC USBSerial;
#else
 USBCDC USBSerial;
 #define Serial USBSerial
#endif

#define TOUCH_CS 36

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <LGFX_ESP32S2_PicoResTouch2.8.hpp>

static LGFX tft;
static LGFX_Sprite sprite(&tft);

#define GIFPLAYER_FS LittleFS
#define GIFPLAYER_FS_Begin() LittleFS.begin(true)
//#define GIFPLAYER_FS SDFS
//#define GIFPLAYER_FS_Begin() GIFPLAYER_FS.begin( TFCARD_CS_PIN )

 
#include <AnimatedGIF.h>
AnimatedGIF gif;

// rule: loop GIF at least during 3s, maximum 5 times, and don't loop/animate longer than 30s per GIF
const int maxLoopIterations =     3; // stop after this amount of loops
const int maxLoopsDuration  =  6000; // ms, max cumulated time after the GIF will break loop
const int maxGifDuration    = 30000; // ms, max GIF duration

// used to center image based on GIF dimensions
static int xOffset = 0;
static int yOffset = 0;

static int totalFiles = 0; // GIF files count
static int currentFile = 0;
static int lastFile = -1;

//char GifComment[256];

static File FSGifFile; // temp gif file holder
static File GifRootFolder; // directory listing

#include <SimpleList.h>
SimpleList<String>* listFiles;

static void MyCustomDelay( unsigned long ms ) {
  delay( ms );
  //log_d("delay %d\n", ms);
}


static void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //log_d("GIFOpenFile( %s )\n", fname );
  FSGifFile = GIFPLAYER_FS.open(fname);
  if (FSGifFile) {
    *pSize = FSGifFile.size();
    return (void *)&FSGifFile;
  }
  return NULL;
}


static void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
     f->close();
}


static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
      iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
      return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}


static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  //log_d("Seek time = %d us\n", i);
  return pFile->iPos;
}


static void TFTDraw(int x, int y, int w, int h, uint16_t* lBuf )
{
  tft.pushRect( x+xOffset, y+yOffset, w, h, lBuf );
}


// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > tft.width() )
      iWidth = tft.width() ;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) {// restore to background color
    for (x=0; x<iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
          s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) { // if transparency used
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while(x < iWidth) {
      c = ucTransparent-1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) { // done, stop
          s--; // back up to treat it like transparent
        } else { // opaque
            *d++ = usPalette[c];
            iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) { // any opaque pixels?
        TFTDraw( pDraw->iX+x, y, iCount, 1, (uint16_t*)usTemp );
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
            iCount++;
        else
            s--;
      }
      if (iCount) {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  } else {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x=0; x<iWidth; x++)
      usTemp[x] = usPalette[*s++];
    TFTDraw( pDraw->iX, y, iWidth, 1, (uint16_t*)usTemp );
  }
} /* GIFDraw() */

int fps=0;
int gifPlay( char* gifPath )
{ // 0=infinite

  gif.begin(BIG_ENDIAN_PIXELS);

  if( ! gif.open( gifPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw ) ) {
    log_n("Could not open gif %s", gifPath );
    return maxLoopsDuration;
  }

  int frameDelay = 0; // store delay for the last frame
  int then = 0; // store overall delay
  bool showcomment = false;

  // center the GIF !!
  int w = gif.getCanvasWidth();
  int h = gif.getCanvasHeight();
  xOffset = ( tft.width()  - w )  /2;
  yOffset = ( tft.height() - h ) /2;
  yOffset +=8; //jmysu, Offset 8 lines for showing filaname!

  if( lastFile != currentFile ) {
    log_n("Playing %s [%d,%d] with offset [%d,%d]", gifPath, w, h, xOffset, yOffset );
    lastFile = currentFile;
    showcomment = true;
  }

  bool isMoreFrames =true;
  unsigned long ulMs;
  while (isMoreFrames) {
    //if( showcomment )
      //if (gif.getComment(GifComment))
      //  log_n("GIF Comment: %s", GifComment);
    ulMs = millis();
    isMoreFrames = gif.playFrame(true, &frameDelay);
    fps = 1000/ (millis()-ulMs);

    then += frameDelay;
    if( then > maxGifDuration ) { // avoid being trapped in infinite GIF's
      //log_w("Broke the GIF loop, max duration exceeded");
      break;
    }

  }

  gif.close();

  return then;
}


int getGifInventory( const char* basePath )
{
  int amount = 0;
  GifRootFolder = GIFPLAYER_FS.open(basePath);
  if(!GifRootFolder){
    log_n("Failed to open directory");
    return 0;
  }

  if(!GifRootFolder.isDirectory()){
    log_n("Not a directory");
    return 0;
  }

  File file = GifRootFolder.openNextFile();

  tft.setTextColor( TFT_WHITE, TFT_BLACK );
  tft.setTextSize( 2 );

  int textPosX = tft.width()/2 - 16;
  int textPosY = tft.height()/2 - 10;

  tft.drawString("GIF Files:", textPosX-40, textPosY-20 );

  while( file ) {
    if(!file.isDirectory()) {
      //GifFiles.push_back( file.name() );
      listFiles->add(file.name());
      amount++;
      tft.drawString(String(amount), textPosX+40, textPosY-20 );
      file.close();
      delay(100);
    }
    file = GifRootFolder.openNextFile();
  }
  GifRootFolder.close();
  log_n("Found %d GIF files", amount);
  return amount;
}




void setup()
{
  //USBCDC debug output
  USBSerial.begin(115200);
  USBSerial.setDebugOutput(true);
  USB.begin();
  delay(2000);
  //Loading LittleFS
  if(LittleFS.begin()){
    log_w("LittleFS Mounted!");
    Serial.printf("Used/Total:%lu/%luKB\n", LittleFS.usedBytes()/1024L, LittleFS.totalBytes()/1024L);
    }
  //Checking SD  
  //setupSD();
    
  // create new list
  listFiles = new SimpleList<String>();

  tft.init();
  tft.setRotation(3);
  if (tft.width() < tft.height()) {
    tft.setRotation(tft.getRotation() ^ 1);
    }


  int attempts = 0;
  int maxAttempts = 50;
  int delayBetweenAttempts = 1000;
  bool isblinked = false;

  tft.setTextDatum( MC_DATUM );

  while(! GIFPLAYER_FS_Begin() ) {
    log_n("FS mount failed! (attempt %d of %d)", attempts, maxAttempts );
    isblinked = !isblinked;
    attempts++;
    if( isblinked ) {
      tft.setTextColor( TFT_WHITE, TFT_BLACK );
    } else {
      tft.setTextColor( TFT_BLACK, TFT_WHITE );
    }
    tft.drawString( "Mounting FS", tft.width()/2, tft.height()/2 );

    if( attempts > maxAttempts ) {
        log_n("Giving up");
        tft.setBrightness(0);
        while (1);
        }
    delay( delayBetweenAttempts );

    GIFPLAYER_FS_Begin();
  }

  log_n("FS mounted!");

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  totalFiles = getGifInventory( "/gif" ); // scan the GIF folder
  tft.setTextDatum( TL_DATUM );

  sprite.createSprite(320, 8);
  sprite.fillScreen(TFT_BLACK);
  sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
}



void loop()
{
  //tft.clear();
  tft.init(); //for some gif will reverse the position!
  tft.setRotation(3);

  //const char * fileName = GifFiles[currentFile++%totalFiles].c_str();
  String sFile = String("/gif/")+listFiles->get(currentFile++ % totalFiles); //Play file no more than totlaFiles
  const char * fileName = sFile.c_str();

  int loops = maxLoopIterations; // max loops
  int durationControl = maxLoopsDuration; // force break loop after xxx ms

  while(loops>0 && durationControl > 0 ) {
    if (loops==maxLoopIterations) { //show file name
      log_d("Playing file:%s", fileName);
      //tft.drawString(String(fileName), 0,0);
      char cBuf[32];
      sprintf(cBuf, "%s:%dfps", fileName, fps);
            
      sprite.clear();
      sprite.drawString(String(cBuf), 0, 0);
      sprite.pushSprite(0,0);
      }
    durationControl -= gifPlay( (char*)fileName );
     
    gif.reset();
    loops--;
    }
    
}

