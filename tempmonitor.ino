#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// https://github.com/milesburton/Arduino-Temperature-Control-Library

            /*Seuraavassa valitaan pinni, johon anturien data tulee.
            * Pinni pitää olla ylösvedettynä +5V 4,7K vastuksella.
            * 
            * Scratchpad on anturin sisäinen 9 byten muisti, joka sisältää
            * byte 0: Temperature LSB
            * byte 1: Temperature MSB
            * byte 2: Th register or User byte 1
            * byte 3: Tl register or User byte 2
            * byte 4: Configuration register
            * byte 5: Varattu laitteen sisäisiin toimintoihin
            * byte 6: Varattu laitteen sisäisiin toimintoihin
            * byte 7: Varattu laitteen sisäisiin toimintoihin
            * byte 8: CRC
             */
            
OneWire  ds(14);

void setup(void) {
  pinMode(14,INPUT);
  Serial.begin(9600);
}

void loop(void) {
  byte i;                   /*for-looppeja varten alustettu muuttuja jo tässä*/
 // byte present = 0;       tämä sekoitti pitkän aikaa, kommentoitu pois, koska ei tarvita
 // byte type_s;            tätä tarvitaan jos olisi käytössä 18S20-antureita.
  byte data[12];            /*parhaillaan luettavan anturin data (=scratchpad)*/   
  byte addr[8];             /*parhaillaan luettavan anturin osoite*/
  float celsius;            /*lämpötilan reaalilukema*/
//  float fahrenheit;       fahrenheiteja ei Suomessa ymmärretä, eikä juuri missään muuallakaan
  
                    /*  ds.search-funktio palauttaa 0, jos antureita ei löydy,
                    *   enempää antureita ei löydy tai jotain menee pieleen.
                    *   Tällä ehdolla loop() aloitetaan alusta jos näin käy.
                    *   ds.search palauttaa 1, jos seuraava anturi (sarjanumeron
                    *   mukaan järjestyksessä on valittu luettavaksi.
                    *   Funktioon viedään addr-taulukko pointterina, ja se
                    *   kirjoittaa seuraavan anturin osoitteen taulukkoon.*/
                    
  if ( !ds.search(addr)) {
//    Serial.println("No more addresses.");
//    Serial.println();
    ds.reset_search();
                    /*  ds.reset_search() nollaa ds.search()-funktion tilan,
                    *   eli ds.search() toimii tämän jälkeen aivan kuin se
                    *   ajettaisiin ensimmäisen kerran.*/
    delay(250);
    return;
  }

//  Serial.print("ROM =");
//  for( i = 0; i < 8; i++) {
//    Serial.write(' ');
//    Serial.print(addr[i], HEX);
//  }

                      /*CRC-tarkistus, jos jotain vikaa loop() aloitetaan alusta.*/
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
//  Serial.println();
 
  // the first ROM byte indicates which chip
/*  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } kommentoitu pois, koska tiedetään, että käytössä on 18B20*/
                              
                          /*- ds.reset() käynnistää anturin uudelleen ja tarkistaa,
                          *   että kaikki on kunnossa tilan vaihtoa varten.
                          * - ds.select() kirjoittaa sen hetkisen anturin osoitteen
                          *   rekisteriin
                          * - ds.write(0x44) kirjoittaa anturin sisäisen
                          *   configuration registerin siten, että anturi tietää
                          *   mitata lämpöä ja kirjoittaa sen muistiin eli
                          *   Scratchpadiin (bitwise &)*/                             
  ds.reset();                
  ds.select(addr);
//  ds.write(0x44, 1);        // start conversion, with parasite power on at the end, 1 tarvitaan jos käyttää parasite poweria
  ds.write(0x44);

                          /*Tässä tarvitaan min 750ms viive, että anturi ehtii muuntaa
                          * lämpötilan 12-bittiseksi sanaksi. Pienemmällä tarkkuudella
                          * riittäisi pienempi viive.*/
  delay(1000);     // maybe 750ms is enough, maybe not

  
                          /*Taas ennen tilan vaihtoa pitää ottaa reset,
                          * ja ilmoittaa sen hetkisen anturin osoite.
                          * Sitten ilmoitetaan että nyt luetaan muistiin
                          * kirjoitettu Scratchpad*/
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

//  Serial.print("  Data = ");
//  Serial.print(present, HEX);
//  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read(); 
            /*tässä luetaan valitun anturin data arduinon data-taulukkoon*/
//    Serial.print(data[i], HEX);
//    Serial.print(" ");
  }
//  Serial.print(" CRC=");
//  Serial.print(OneWire::crc8(data, 8), HEX);  /*CRC check*/
//  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.

              /*Anturi tallentaa lämpötilan 2 byteen, data[1] ja data[0], eli
              * 16-bittisenä. raw pakotetaan 16-bittiseksi signed integeriksi.
              * data[1] shiftataan 8 ylöspäin, saadaan 8 ylintä bittiä ja
              * data[0] saadaan 8 alinta bittiä.*/
  int16_t raw = (data[1] << 8) | data[0];
  /*if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else { Kommentoitu pois, koska käytössä on 18B20*/
  
              /*seuraavassa voisi pudottaa resoluutiota, jos sen kokee
              * tarpeelliseksi vaihtamalla 0x60 tilalle 0x00 (9-bit)
              * 0x20 (10-bit) tai 0x40 (11-bit).*/
              
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  /*}*/
  celsius = (float)raw / 16.0;
  /*fahrenheit = celsius * 1.8 + 32.0;      ei tarvita fahrenheiteja*/
//  Serial.print("  Temperature = ");
  Serial.println(celsius);
//  Serial.print(" Celsius, ");
  /*Serial.print(fahrenheit);*/
  /*Serial.println(" Fahrenheit");*/
}
