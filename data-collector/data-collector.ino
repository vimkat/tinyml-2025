#include <Arduino_LSM9DS1.h> // IMU
#include <Arduino_LPS22HB.h> // BARO

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("A_x\tA_y\tA_z\tG_x\tG_y\tGY_z\tP\tT");
}

float a_x, a_y, a_z;
float g_x, g_y, g_z;

void loop() {

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(a_x, a_y, a_z);
  }

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(g_x, g_y, g_z);
  }

  // Print all values
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
}
