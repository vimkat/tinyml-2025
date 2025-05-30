#include <Arduino_LSM9DS1.h> // IMU
#include <Arduino_LPS22HB.h> // BARO
#include <SD.h>

File record;
String recFileName;
String buffer;
unsigned long timestamp = 0;

void setup() {
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);

  Serial.begin(9600);
  //while (!Serial); // uncomment for prod :)
  Serial.println("Started");
  buffer.reserve(1024);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  if (!SD.begin(10)) {
    Serial.println("Failed to initialize SD!");
    blink(LED_RED);
  }
  //record = SD.open("REC.txt", FILE_WRITE);
  //if (!record) {
  //  Serial.println("Failed to open file!");
  //  while (1);
  //}

	recFileName = getRecordFile();
  record = SD.open(recFileName, FILE_WRITE);

////////////////////////////////////////////////////////////////////////////////

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("A_x\tA_y\tA_z\tG_x\tG_y\tGY_z\tP\tT");
}

String getRecordFile()
{
  if (!SD.exists("index.txt")) {
    Serial.println("No index, creating...");
    File index = SD.open("index.txt", FILE_WRITE);
    index.print("0");
    index.close();
  }

  File index = SD.open("index.txt", FILE_READ);
	String buf = "";
  while(index.available()) {
    char c = index.read();
	  buf += c;
  }
  index.close();

  int nextFile = buf.toInt() + 1;
  SD.remove("index.txt");
  index = SD.open("index.txt", FILE_WRITE);
  index.print(nextFile);
  index.close();

	Serial.print("LAST FILE: ");
	Serial.println(buf);
  Serial.print("NEXT FILE: ");
  Serial.println(nextFile);

  String filename = String(nextFile);
  return "REC_" + filename + ".txt";
}

float a_x, a_y, a_z;
float g_x, g_y, g_z;

void loop() {
  timestamp = millis();
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(a_x, a_y, a_z);
  }

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(g_x, g_y, g_z);
  }

  // Print all values
  Serial.print(timestamp);
  Serial.print('\t');
  Serial.print(a_x);
  Serial.print('\t');
  Serial.print(a_y);
  Serial.print('\t');
  Serial.print(a_z);
  Serial.print('\t');
  Serial.print(g_x);
  Serial.print('\t');
  Serial.print(g_y);
  Serial.print('\t');
  Serial.print(g_z);
  Serial.print('\t');
  Serial.print(BARO.readPressure() * 10);
  Serial.print('\t');
  Serial.print(BARO.readTemperature());
  Serial.println();


  buffer += (timestamp);
  buffer += ('\t');
  buffer += (a_x);
  buffer += ('\t');
  buffer += (a_y);
  buffer += ('\t');
  buffer += (a_z);
  buffer += ('\t');
  buffer += (g_x);
  buffer += ('\t');
  buffer += (g_y);
  buffer += ('\t');
  buffer += (g_z);
  buffer += ('\t');
  buffer += (BARO.readPressure() * 10);
  buffer += ('\t');
  buffer += (BARO.readTemperature());
  buffer += "\n";

  Serial.print("BUFFER = ");
  Serial.println(buffer.length());

  // check if the SD card is available to write data without blocking
  // and if the dataBuffered data is enough for the full chunk size
  // record = SD.open(recFileName, FILE_WRITE);
  unsigned int chunkSize = record.availableForWrite();
  if (chunkSize && buffer.length() >= chunkSize) {
    // write to file and blink LED
    digitalWrite(LED_RED, HIGH);
    record.write(buffer.c_str(), chunkSize);
    digitalWrite(LED_RED, LOW);
    // remove written data from dataBuffer
    buffer.remove(0, chunkSize);
  }
  // record.close();
}

void blink(PinName pin) {
  while(1) {
    digitalWrite(pin, HIGH);
    delay(1000);
    digitalWrite(pin, LOW);
    delay(1000);
  }
}
