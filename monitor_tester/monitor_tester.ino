/* 
 *  This code sends some test packets through the serial port
 *  to test the communication and the monitor
 */

/*********************************
 * ARDUINO-MONITOR MESSAGE FORMAT:
 *********************************
 *
 ********* INFO-MESSAGE:**********
 * Size: 25 Bytes
 * TABBBBBBBBCCCCCCCCDDDEFG\n
 *
 *     
 *      FIELD NAME        DIM
 * T    MessageType       1B
 * A    RoadID            1B
 * B    Manufacturer      8B
 * C    Model             8B
 * D    Orientation       3B
 * E    Priority          1B
 * F    RequestedAction   1B   // The action the car wants to do
 * G    CurrentAction     1B   // The action the car is doing to
 *                                cooperate with the network
 * 
 * FIELD              VALUE       DESCRIPTION
 * MessageType:       I           info message (this message)
 *                    L           frequency spectrum message from leftmost receiver
 *                    F           frequency spectrum message from front receiver
 *                    R           frequency spectrum message from rightmost receiver
 * RoadID:            M           my road
 *                    L           road to my left
 *                    A           road ahead
 *                    R           road to my right
 * Manufacturer:      Alfa⎵⎵⎵⎵
 *                    Fiat⎵⎵⎵⎵
 *                    Tesla⎵⎵⎵
 * Model:             Giulia⎵⎵
 *                    500⎵⎵⎵⎵⎵
 *                    Model S⎵
 * Orientation:       [0..360]    degrees clockwise
 * Priority:          N           regular car
 *                    Y           priority car (ambulance, police)
 * RequestedAction,
 * CurrentAction:     S           Stay still
 *                    L           turn left
 *                    A           go straight ahead
 *                    R           turn right
 * 
 *** FREQUENCY-SPECTRUM-MESSAGE ***
 * Size: 263-775 Bytes (Assuming FFT_N = 128 = number of bins)
 * TM;A,A,...,A,A\n
 *    |_________|
 *       128    
 *     
 * A,A,...,A,A   The sensor data relative to the IR receiver
 * 
 *        FIELD NAME          DIM
 * T      MessageType         1B
 * M      MaximumFrequency    1-5B  // the end of the frequency range of the last bin
 * A      BinFreqIntensity    1-5B
 * ,      BinsSeparator       1B    // separates each pair of bins
 * ;      headerSeparator     1B    // separates the header from the frequency data
 * 
 * Each bin represents the intensity of a frequency range. The
 * frequencies go from 0 to sampling_frequency / 2. Bin i represents //TODO: check the end of the range
 * the range [i * sampling_frequency / 128, (i+1) * sampling_frequency / 128]
 * 
 * The intensity goes from 0 to 65535, it is sent as a decimal string, so it may be sent with a variable 
 * number of Bytes: from 1 Byte (eg. '6') up to 5 Bytes (eg. '65535')
 */

uint16_t bins1[64];
uint16_t bins2[64];
uint16_t bins3[64];

//Serial.write("IMAlfa    Giulia    0NLS\n");

void sendPartialInfoMessage(char type, uint16_t roadId, uint16_t orientation) {
  Serial.write(type);
  Serial.write('I');
  Serial.write(roadId);
  Serial.write("                "); // 16 spaces
  if (orientation < 10) Serial.print(F("  "));
  else if (orientation < 100) Serial.write(' ');
  Serial.print(orientation);
  Serial.write('\n');
}

/*
 * Sends an info-message to the serial port
 * Manufacturer and model should be already padded
 */
void sendInfoMessage(uint8_t roadId, String manufacturer, String model,
                     uint16_t orientation, char priority, char requestedAction, char currentAction) {

  Serial.write('I');
  Serial.write(roadId);
  Serial.print(manufacturer);
  Serial.print(model);
  if (orientation < 10) Serial.print(F("  "));
  else if (orientation < 100) Serial.write(' ');
  Serial.print(orientation);
  Serial.write(priority);
  Serial.write(requestedAction);
  Serial.write(currentAction);
  Serial.write('\n');
}

void sendFrequencyMessage(char type, uint16_t fft_bins[]) {
  Serial.write(type);
  Serial.print(10000);
  Serial.write(';');
  for(uint8_t i = 0; i < 63; i++) {
    Serial.print(fft_bins[i]);
    Serial.write(',');
  }
  Serial.print(fft_bins[63]);
  Serial.write('\n');
}



void setup() {
  Serial.begin(230400);

  for (uint16_t i = 0; i < 64; i++) {
    bins1[i] = i;
    bins2[i] = i;
    bins3[i] = i;
  }
}

void loop() {
  delay(3000); // 2 seconds
  
//  unsigned long startTime = micros();


  
  /**********
   * The car just arrived, it thinks it is alone (its sensors see nothing)
   **********/
  Serial.write("IMAlfa    Giulia    0NLS\n");
  sendInfoMessage('I', 'M', "Alfa    ", "Giulia  ", 0, 'N', 'L', 'S');
  sendFrequencyMessage('L', bins1);
  sendFrequencyMessage('F', bins2);
  sendFrequencyMessage('R', bins3);

  delay(2000); // 2 seconds
  
  /**********
   * The car sees 3 cars, it has not joined the network
   **********/

  sendInfoMessage('M', "Alfa    ", "Giulia  ", 0, 'N', 'L', 'S');
  sendPartialInfoMessage('L', 90);
  sendPartialInfoMessage('A', 180);
  sendPartialInfoMessage('R', 270);

  delay(2000); // 2 seconds
  
  /**********
   * The car sees 2 cars and joined the network
   **********/
  sendInfoMessage('M', "Alfa    ", "Giulia  ",   0, 'N', 'L', 'S');
  sendInfoMessage('L', "Fiat    ", "500     ",  90, 'N', 'A', 'S');
  sendInfoMessage('R', "Tesla   ", "Model S ", 270, 'N', 'L', 'L');

  

//  unsigned long time = micros() - startTime;
//  Serial.write("STATS: Time passed is (us): ");
//  Serial.println(time); 
  
}

/************
 * TESTS
 ************
 * Time to send 8 Info-messages and 3 frequency-spectrum-messages: 30,812 ms
 * Time to send a single frequency-spectrum-message: 7,496 ms 
 * (slightly old message format)
 * 
 * 'Serial.write("IMAlfa    Giulia    0NLS\n");' 500 times takes 559604 us
 * sendInfoMessage(...)                                    takes 562504 us
 */
