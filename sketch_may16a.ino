#include <SoftwareSerial.h>

// Подключение GPS-модуля к Arduino через SoftwareSerial
SoftwareSerial gpsSerial(0, 1);  // RX, TX

void setup() {
  Serial.begin(4800);
  gpsSerial.begin(4800);
}

void loop() {
  if (gpsSerial.available()) {
    char c = gpsSerial.read();
        Serial.println(c);
    // Проверяем, если получен символ начала NMEA предложения ($)
    if (c == '$') {
      String nmeaSentence = gpsSerial.readStringUntil('\n');
        Serial.println(nmeaSentence);
      // Парсим предложение
      parseNmeaSentence(nmeaSentence);
    }
  }
}

void parseNmeaSentence(String sentence) {
  // Игнорируем пустые строки и предложения, не начинающиеся с "$"
  if (sentence.length() == 0 || sentence.charAt(0) != '$') {
        Serial.println(sentence);
    return;
  }

  // Разделяем предложение на отдельные части по запятым
  int commaIndex = sentence.indexOf(',');
  String nmeaType = sentence.substring(1, commaIndex);
          Serial.println(commaIndex);
                    Serial.println(nmeaType);


  // Пример обработки разных типов NMEA предложений
  if (nmeaType == "GPGGA") {
    // Обработка предложения GPGGA (информация о положении и времени)
    // Разделяем предложение на поля
    String fields[13];
    int fieldIndex = 0;
    int startIndex = commaIndex + 1;
    int endIndex;
    while (fieldIndex < 13 && startIndex < sentence.length()) {
      endIndex = sentence.indexOf(',', startIndex);
      if (endIndex == -1) {
        endIndex = sentence.length();
      }
      fields[fieldIndex] = sentence.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
      fieldIndex++;
    }

    // Выводим значения полей
    Serial.print("Latitude: ");
    Serial.println(fields[2]);
    Serial.print("Longitude: ");
    Serial.println(fields[4]);
    // и т.д. - обработка остальных полей предложения GPGGA
  } else if (nmeaType == "GPRMC") {
    // Обработка предложения GPRMC (минимальная информация о положении и времени)
    // Аналогично обработке GPGGA
  }
  // добавьте обработку других типов NMEA предложений, если необходимо
}
