#include <Arduino.h>
 
#include <WiFi.h>
#include <esp32-hal-log.h>

#ifdef ARDUINO_ARCH_ESP32
 #include "esp_chip_info.h"
 esp_chip_info_t chip_info;
 #ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
    #if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
    #include "esp32/rom/rtc.h"
    #elif CONFIG_IDF_TARGET_ESP32S2
     #include "esp32s2/rom/rtc.h"
     #include "USB.h"
     extern USBCDC USBSerial;
     #define Serial USBSerial
     //extern "C" {
     // #include <esp_spiram.h>
     // #include <esp_himem.h>
     // }
    #elif CONFIG_IDF_TARGET_ESP32C3
    #include "esp32c3/rom/rtc.h"
    #elif CONFIG_IDF_TARGET_ESP32S3
    #include "esp32s3/rom/rtc.h"
    #else 
    #error Target CONFIG_IDF_TARGET is not supported
    #endif
    #else // ESP32 Before IDF 4.0
    #include "rom/rtc.h"
 #endif
#else
  #error ESP32ChipID works only on ESP32 
#endif


/**
 * \brief verbose_print_reset_reason()
 * \param reason
 */ 
void verbose_print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1  : Serial.println ("Vbat power on reset");break;
    case 3  : Serial.println ("Software reset digital core");break;
    case 4  : Serial.println ("Legacy watch dog reset digital core");break;
    case 5  : Serial.println ("Deep Sleep reset digital core");break;
    case 6  : Serial.println ("Reset by SLC module, reset digital core");break;
    case 7  : Serial.println ("Timer Group0 Watch dog reset digital core");break;
    case 8  : Serial.println ("Timer Group1 Watch dog reset digital core");break;
    case 9  : Serial.println ("RTC Watch dog Reset digital core");break;
    case 10 : Serial.println ("Instrusion tested to reset CPU");break;
    case 11 : Serial.println ("Time Group reset CPU");break;
    case 12 : Serial.println ("Software reset CPU");break;
    case 13 : Serial.println ("RTC Watch dog Reset CPU");break;
    case 14 : Serial.println ("for APP CPU, reseted by PRO CPU");break;
    case 15 : Serial.println ("Reset when the vdd voltage is not stable");break;
    case 16 : Serial.println ("RTC Watch dog reset digital core and rtc module");break;
    default : Serial.println ("NO_MEAN");
  }
}

/**
 * @brief getESP32ChipID()
 * 
 * @return uint32_t id, 3 bytes 
 * 
 */
uint32_t getESP32ChipID24()
{
  uint32_t i24 = 0;
    for (int i=0; i<17; i=i+8) {
      i24 |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
  return i24;    
}
 
void printFlashPartions()
{
  size_t ul;
  esp_partition_iterator_t _mypartiterator;
  const esp_partition_t *_mypart;
  ul = spi_flash_get_chip_size(); Serial.print("Flash chip size: "); Serial.println(ul);

  Serial.println("Partiton table:");
  _mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (_mypartiterator) {
    do {
      _mypart = esp_partition_get(_mypartiterator);
      printf("%x - %x - %x - %x - %s - %i\r\n", _mypart->type, _mypart->subtype, _mypart->address, _mypart->size, _mypart->label, _mypart->encrypted);
      } while (_mypartiterator = esp_partition_next(_mypartiterator));
    }
  esp_partition_iterator_release(_mypartiterator);
 
  _mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (_mypartiterator) {
    do {
      _mypart = esp_partition_get(_mypartiterator);
      printf("%x - %x - %x - %x - %s - %i\r\n", _mypart->type, _mypart->subtype, _mypart->address, _mypart->size, _mypart->label, _mypart->encrypted);
      } while (_mypartiterator = esp_partition_next(_mypartiterator));
    }
  esp_partition_iterator_release(_mypartiterator);
}

void printFlashPartionsEasy()
{
  size_t ul;
  esp_partition_iterator_t _mypartiterator;
  const esp_partition_t *_mypart;
  //ul = spi_flash_get_chip_size(); Serial.print("Flashsize(KB): "); Serial.println(ul/1024);

  Serial.println("-----Flash--Size---(Code)-@Addr--");
  _mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (_mypartiterator) {
    do {
      _mypart = esp_partition_get(_mypartiterator);
      char cBuf[40];
      sprintf(cBuf, "%10s:%5luKB (%02X-%02X)@%06X\r\n", _mypart->label, _mypart->size/1024L, _mypart->type, _mypart->subtype, _mypart->address );
      Serial.printf(cBuf);
      } while (_mypartiterator = esp_partition_next(_mypartiterator));
    }
  esp_partition_iterator_release(_mypartiterator);
 
  Serial.println("-----Flash--Size---(Data)-@Addr--");
  _mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (_mypartiterator) {
    do {
      _mypart = esp_partition_get(_mypartiterator);
      char cBuf[40];
      sprintf(cBuf, "%10s:%5luKB (%02X-%02X)@%06X\r\n", _mypart->label, _mypart->size/1024L, _mypart->type, _mypart->subtype, _mypart->address  );
      Serial.printf(cBuf);
      } while (_mypartiterator = esp_partition_next(_mypartiterator));
    }
  esp_partition_iterator_release(_mypartiterator);
}

/**
 * \brief printESP32info()
 * \param nill
 */  
void printESP32info() {

    log_i("Reset reason:");
    verbose_print_reset_reason(rtc_get_reset_reason(0));

    esp_chip_info(&chip_info);
    Serial.print("\nChip Info:\n---------------------------------\n");
    //Serial.println("Hardware info");
    //Serial.println("-----------------");
    switch (chip_info.model) {
        case CHIP_ESP32://  = 1, //!< ESP32
            Serial.print("ESP32");
            break;
        case CHIP_ESP32S2:// = 2, //!< ESP32-S2
            Serial.print("ESP32S2");
            break;
        case CHIP_ESP32S3:// = 9, //!< ESP32-S3
            Serial.print("ESP32S3");
            break;
        case CHIP_ESP32C3:// = 5, //!< ESP32-C3
            Serial.print("ESP32C3");
            break;
        case CHIP_ESP32H2:// = 6, //!< ESP32-H2
            Serial.print("ESP32H2");
            break;
    }
    Serial.print("@");Serial.print(getCpuFrequencyMhz());Serial.println("MHz");
    Serial.printf("%d cores Wifi%s%s\n", chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
     (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    Serial.printf("Silicon revision: %d\n", chip_info.revision);

    //Get 32bit chip id from reversed MAC address
    uint32_t ChipID32 = getESP32ChipID24();
    Serial.printf("ChipID %08X\n", ChipID32);
    Serial.printf("MAC ");
    Serial.println(WiFi.macAddress());

    //Serial.printf("ChipRevision %d, CPU Freq %dMHz\n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
    //Serial.printf("Flash: %dKB, Speed: %dMHz\n", ESP.getFlashChipSize()/1024L, ESP.getFlashChipSpeed()/1000000L);
    //Firmware=================================================================
    Serial.print("ESP SDK: ");
    Serial.println(ESP.getSdkVersion());
    Serial.print("ESP32 Arduino: ");
    Serial.print(ESP_ARDUINO_VERSION_MAJOR);
    Serial.print(ESP_ARDUINO_VERSION_MINOR);
    Serial.println(ESP_ARDUINO_VERSION_PATCH);
    
    //Serial.println("---------------------------");
    Serial.printf("Heap: %d/%dKB ", ESP.getFreeHeap()/1024L, ESP.getHeapSize()/1024L);
    Serial.printf("SPIRAM: %d/%dKB\n", ESP.getFreePsram()/1024L, ESP.getPsramSize()/1024L);
    Serial.printf("%dKB %s flash ", spi_flash_get_chip_size()/1024L,
     (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external");
    Serial.printf("@%dMHz\n", ESP.getFlashChipSpeed()/1000000L);

    float fSketch    = ESP.getSketchSize()/1024.0;
    float fPartition = ESP.getFreeSketchSpace()/1024.0;
    Serial.printf("Sketch: %4.0f/%4.0fKB(%3.1f%%)\n", fSketch, fPartition, fSketch*100/fPartition);
    
    printFlashPartionsEasy();
    Serial.println("---------------------------------");

   
    log_d("");
}

