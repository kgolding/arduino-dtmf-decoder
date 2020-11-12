#define DEFAULT_THRESHOLD 2500
#define DEFAULT_N 128
#define DEFAULT_SAMPLING_RATE 8926

// The library toggles digital pin 4 in the sampling loop which allows
// measurement of the actual sampling frequency.
// If you call .sample() continuously like this:
// while(1)dtmf.sample(sensorPin);
// you can put a frequency counter on pin 4 to determine what the
// sampling frequency is on your Arduino. Note that the frequency
// counter will show half the actual rate. My meter showed 4.463kHz
// so the sampling rate is 8926Hz
//
// SOURCE: https://forum.arduino.cc/index.php/topic,121540.0.html

#include "src/DTMF/DTMF.h"

int sensorPin = A0;
int led = 13;


// NOTE that N MUST NOT exceed 160
// This is the number of samples which are taken in a call to
// .sample. The smaller the value of N the wider the bandwidth.
// For example, with N=128 at a sample rate of 8926Hz the tone
// detection bandwidth will be 8926/128 = 70Hz. If you make N
// smaller, the bandwidth increases which makes it harder to detect
// the tones because some of the energy of one tone can cross into
// an adjacent (in frequency) tone. But a larger value of N also means
// that it takes longer to collect the samples.
// A value of 64 works just fine, as does 128.
// NOTE that the value of N does NOT have to be a power of 2.
int n = DEFAULT_N;
// sampling rate in Hz
int sampling_rate = DEFAULT_SAMPLING_RATE;

#include <EEPROM.h>

DTMF dtmf = DTMF(DEFAULT_N, DEFAULT_SAMPLING_RATE);

#define EEPROM_THRESHOLD 0
#define EEPROM_N 2
#define EEPROM_SAMPLING_RATE 4

// Settings from EEPROM
int threshold = DEFAULT_SAMPLING_RATE;

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);

  Serial.println("DTMF Decoder V0.9 - Kevin Golding");
  Serial.println("Send H <CR> for help");

  EEPROM.get(EEPROM_THRESHOLD, threshold);
  EEPROM.get(EEPROM_N, n);
  EEPROM.get(EEPROM_SAMPLING_RATE, sampling_rate);
  if (isnan(threshold) || isnan(n) || isnan(sampling_rate)) {
    Serial.println("Error reading EEPROM, using default values");
    threshold = DEFAULT_THRESHOLD;
    n = DEFAULT_N;
    sampling_rate = DEFAULT_SAMPLING_RATE;
    EEPROM.put(EEPROM_THRESHOLD, threshold);
    EEPROM.put(EEPROM_N, n);
    EEPROM.put(EEPROM_SAMPLING_RATE, sampling_rate);
  }

  // Instantiate the dtmf library with the number of samples to be taken
  // and the sampling rate.
  dtmf = DTMF((float)n, (float)sampling_rate);

}

void printValues() {
  Serial.print("Threshold: ");
  Serial.println(threshold);

  Serial.print("N: ");
  Serial.println(n);

  Serial.print("Sampling rate: ");
  Serial.println(sampling_rate);

}

int nochar_count = 0;
float d_mags[8];

void(* ResetFunc) (void) = 0; // Restart!

void handleSerialInput() {
  // Syntax <char><space><value>\n
  static int i = 0;
  static char cmd = ' ';
  static int value;
  enum serialState {
    IDLE,
    CMD,
    VALUE,
  };
  static enum serialState state = IDLE;

  if (Serial.available() > 0) {
    char c = Serial.read();

    // Check for an end of line
    if (c == '\n' || c == '\r') {
      if (cmd > ' ') {
        //        Serial.print("CMD: ");
        //        Serial.print(cmd);
        //        Serial.print(", Value: ");
        //        Serial.print(value);
        //        Serial.print(": ");

        switch (cmd) {
          case 'H':
          case 'h':
            Serial.println("Commands:\n");
            Serial.println("\tH<CR>\t\tDisplay help");
            Serial.println("\t?<CR>\t\tDisplay current settings");
            Serial.println("\tT 1000<CR>\tSet threshold to 1000");
            Serial.println("\tN 128<CR>\tSet 'n' aka block size");
            Serial.println("\tS 8926<CR>\tSet sampling rate");
            Serial.println("\tR 999<CR>\tFacotry reset to defaults");
            Serial.println("\nThe space between the command character and the value is optional.");
            Serial.println("\nSettngs are stored in persistent memory.");
            Serial.println("\nOutput:");
            Serial.println("When a DTMF digit is detected a new line starting with 'DTMF: '\nfollowed by the single digit and <CR> is sent.");
            Serial.println("\tExample: \tDTMF: 5");
            break;

          case 'R':
            if (value == 999) {
              threshold = DEFAULT_THRESHOLD;
              n = DEFAULT_N;
              sampling_rate = DEFAULT_SAMPLING_RATE;
              EEPROM.put(EEPROM_THRESHOLD, threshold);
              EEPROM.put(EEPROM_N, n);
              EEPROM.put(EEPROM_SAMPLING_RATE, sampling_rate);
              Serial.println("Factory reset to default values");
            } else {
              Serial.println("Send 'D 999<CR>' to reset to defaults.");
            }
            break;

          case '?':
            printValues();
            break;

          case 'T': // Threshold
            Serial.println("OK");
            threshold = value;
            EEPROM.put(EEPROM_THRESHOLD, threshold);
            break;

          case 'N': // N
            Serial.println("OK");
            n = value;
            EEPROM.put(EEPROM_N, n);
            dtmf = DTMF((float)n, (float)sampling_rate);
            break;

          case 'S': // Sampling rate
            Serial.println("OK");
            sampling_rate = value;
            EEPROM.put(EEPROM_SAMPLING_RATE, sampling_rate);
            dtmf = DTMF((float)n, (float)sampling_rate);
            break;

          default:
            Serial.println("Unknown command");
        }
      }
      // Reset vars waiting for next command
      state = IDLE;
      i = 0;
      cmd = ' ';
      value = 0;
    } else {
      switch (state) {
        case IDLE:
          cmd = c;
          state = VALUE;
          break;

        case VALUE:
          if (c == ' ') {
            // Ignore spaces
          } else if (c >= '0' && c <= '9') {
            value = value * 10;
            value += c - '0';
//          } else {
//            Serial.println("Ignoring invalid character, expected a digit");
          }
      }
    }
  }
}


void loop()
{
  char thischar;

  // This reads N samples from sensorpin (must be an analog input)
  // and stores them in an array within the library. Use while(1)
  // to determine the actual sampling frequency as described in the
  // comment at the top of this file
  /* while(1) */dtmf.sample(sensorPin);

  // The first argument is the address of a user-supplied array
  // of 8 floats in which the function will return the magnitudes
  // of the eight tones.
  // The second argument is the value read by the ADC when there
  // is no signal present. A voltage divider with precisely equal
  // resistors will presumably give a value of 511 or 512.
  // My divider gives a value of 506.
  // If you aren't sure what to use, set this to 512
  dtmf.detect(d_mags, 506);

  // detect the button
  // If it is recognized, returns one of 0123456789ABCD*#
  // If unrecognized, returns binary zero

  // Pass it the magnitude array used when calling .sample
  // and specify a magnitude which is used as the threshold
  // for determining whether a tone is present or not
  //
  // If N=64 magnitude needs to be around 1200
  // If N=128 the magnitude can be set to 1800
  // but you will need to play with it to get the right value
  thischar = dtmf.button(d_mags, (float)threshold);
  if (thischar) {
    Serial.print("DTMF: ");
    Serial.println(thischar);
    nochar_count = 0;
    // Print the magnitudes for debugging
    // #define DEBUG_PRINT
#ifdef DEBUG_PRINT
    for (int i = 0; i < 8; i++) {
      Serial.print("  ");
      Serial.print(d_mags[i]);
    }
    Serial.println("");
#endif
  } else {
    // print a newline
    if (++nochar_count == 50) {
      Serial.println("");
    }
    // don't let it wrap around
    if (nochar_count > 30000)nochar_count = 51;
  }

  handleSerialInput();
}
