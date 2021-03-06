#include "sb_dynamixel.h"

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

// unsigned char input_buffer[IN_BUF_SIZE] = {};
// unsigned int buffer_idx = 0;

// const int end_delay_us = 10; // this is a tiny delay after the Serial flush in order to make sure all data is sent before pulling the half-duplex RX/TX select line low
// const int wait_micros = 2000; // this is a delay after a read function in order to allow the Dynamixel to respond

SB_Dynamixel::SB_Dynamixel(unsigned char device_id, int curr_pos, short goal_current, int goal_position, int last_position) {
  m_device_id = device_id;
  m_curr_pos = curr_pos;
  m_goal_current = goal_current;
  m_goal_position = goal_position;
  m_last_position = last_position;
}


void SB_Dynamixel::init() {

  // begin dynamixel serial
  DYNAMIXEL_SERIAL.begin(DYNAMIXEL_BAUD);
  DYNAMIXEL_SERIAL.transmitterEnable(DYNAMIXEL_IO_SELECT);

  delay(20);
  torque_en(0); // disable torque
  delay(20);
  operating_mode(5); // operating mode continuous position

  union ushortToChar P_gain;
  P_gain.as_ushort = 400;

  union ushortToChar Velocity_Limit;
  Velocity_Limit.as_ushort = 100;

  // set device 100 position P_gain (84) to 400
  {
    unsigned char address[] = {0x54, 0x00};
    unsigned char payload[] = {P_gain.as_uchar[0], P_gain.as_uchar[1]};
    generic_write(address , payload, sizeof(payload));
  }

  //  // set device 100 position D-gain (80) to zero
  //  {
  //    unsigned char address[] = {0x50, 0x00};
  //    unsigned char payload[] = {0x00, 0x00};
  //    generic_write(100, address , payload, sizeof(payload));
  //  }
  //
  //  // set device 101 position D-gain (80) to zero
  //  {
  //    unsigned char address[] = {0x50, 0x00};
  //    unsigned char payload[] = {0x00, 0x00};
  //    generic_write(101, address , payload, sizeof(payload));
  //  }
  //
  //  // set device 102 position D-gain (80) to zero
  //  {
  //    unsigned char address[] = {0x50, 0x00};
  //    unsigned char payload[] = {0x00, 0x00};
  //    generic_write(102, address , payload, sizeof(payload));
  //  }
  //  delay(20);

  led_msg(1);
  delay(20);

  // set device status return level [0x44] to PING and READ only [value = 1]
  {
    unsigned char address[] = {0x44, 0x00};
    unsigned char payload[] = {0x01};
    generic_write(address, payload, sizeof(payload));
    delay(20);
  }
  /*//set device velocity limit(44) to 100, it works for velocity control, not sure for position control
  {
      unsigned char address[] = { 0x2C, 0x00 };
      unsigned char payload[] = { 0x64 };//Velocity_Limit.as_uchar[0], Velocity_Limit.as_uchar[1]};
      generic_write(address, payload, sizeof(payload));
      delay(20);
  }
  //set device MAX voltage limit(34) to 95 to limit the maximum velocity
  {
      unsigned char address[] = { 0x22, 0x00 };
      unsigned char payload[] = { 0x5F};
      generic_write(address, payload, sizeof(payload));
      delay(20);
  }*/
  m_last_position = read_position();
  // TESTING
  torque_en(1);
}


unsigned short SB_Dynamixel::update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size) {
  unsigned short i, j;
  unsigned short crc_table[256] = {
    0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
    0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
    0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
    0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
    0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
    0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
    0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
    0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
    0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
    0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
    0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
    0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
    0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
    0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
    0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
    0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
    0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
    0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
    0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
    0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
    0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
    0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
    0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
    0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
    0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
    0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
    0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
    0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
    0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
    0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
    0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
    0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202
  };

  for (j = 0; j < data_blk_size; j++)
  {
    i = ((unsigned short)(crc_accum >> 8) ^ data_blk_ptr[j]) & 0xFF;
    crc_accum = (crc_accum << 8) ^ crc_table[i];
  }

  return crc_accum;
}

void SB_Dynamixel::generic_write(unsigned char address[2], unsigned char payload[], unsigned short n_payload) {

  const unsigned char write_template[] = {0xFF, 0xFF, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};

  union ushortToChar n_length;
  n_length.as_ushort = (n_payload + 5);
  unsigned short total_length = n_payload + 12;
  unsigned char curr_msg[total_length];

  //union ushortToChar the_address.as_uchar[] = address[];

  curr_msg[0] = write_template[0]; //header
  curr_msg[1] = write_template[1]; //header
  curr_msg[2] = write_template[2]; //header
  curr_msg[3] = write_template[3]; //reserved
  curr_msg[4] = m_device_id;       //ID
  curr_msg[5] = n_length.as_uchar[0]; //length LSB
  curr_msg[6] = n_length.as_uchar[1]; //length MSB
  curr_msg[7] = write_template[7]; //write
  curr_msg[8] = address[0]; //address 0 (LSB)
  curr_msg[9] = address[1]; //address 1 (MSB)

  unsigned int curr_msg_idx = 10;

  for (unsigned int payload_idx = 0; payload_idx < n_payload; payload_idx++) {
    curr_msg[curr_msg_idx] = payload[payload_idx];
    curr_msg_idx++;
  }

  union ushortToChar theCRC;
  theCRC.as_ushort = update_crc( 0 , curr_msg, (total_length - 2));

  curr_msg[curr_msg_idx] = theCRC.as_uchar[0]; //CRC 0 (MSB)
  curr_msg[curr_msg_idx + 1] = theCRC.as_uchar[1]; //CRC 1 (MSB)

  for ( int idx = 0; idx < sizeof(curr_msg); idx++) {
    DYNAMIXEL_SERIAL.write(curr_msg[idx]);
  }

  DYNAMIXEL_SERIAL.flush(); // block until the output buffer is empty
  delayMicroseconds(end_delay_us);

}

void SB_Dynamixel::led_msg(unsigned int led_cmd) {
  unsigned char address[] = {0x41, 0x00};
  unsigned char payload[] = {led_cmd};
  generic_write(address , payload, sizeof(payload));
}

void SB_Dynamixel::operating_mode(unsigned int mode) {
  unsigned char address[] = {0x0B, 0x00};
  unsigned char payload[] = {mode};
  generic_write(address , payload, sizeof(payload));
}

void SB_Dynamixel::position_goal() {
  union intToChar goal_position;
  goal_position.as_int = m_goal_position;
  unsigned char address[] = {0x74, 0x00};
  unsigned char payload[] = {goal_position.as_uchar[0], goal_position.as_uchar[1], goal_position.as_uchar[2], goal_position.as_uchar[3]};
  generic_write(address , payload, sizeof(payload));
  m_last_position = m_goal_position;
}

void SB_Dynamixel::current_goal() {
  union ushortToChar goal_current;
  goal_current.as_ushort = m_goal_current;
  unsigned char address[] = {0x66, 0x00};
  unsigned char payload[] = {goal_current.as_uchar[0], goal_current.as_uchar[1]};
  generic_write(address , payload, sizeof(payload));
}

void SB_Dynamixel::torque_en(unsigned char desired_status) {
  unsigned char address[] = {0x40, 0x00};
  unsigned char payload[] = {desired_status};
  generic_write(address , payload, sizeof(payload));
}

int SB_Dynamixel::read_current() {
  const unsigned char read_current_template[] = {0xFF, 0xFF, 0xFD, 0x00, 0x00, 0x07, 0x00, 0x02, 0x7E, 0x00, 0x02, 0x00, 0x00, 0x00};
  union shortToChar curr_current;

  unsigned char curr_msg[sizeof(read_current_template)];
  curr_msg[0] = read_current_template[0]; //header
  curr_msg[1] = read_current_template[1]; //header
  curr_msg[2] = read_current_template[2]; //header
  curr_msg[3] = read_current_template[3]; //reserved
  curr_msg[4] = m_device_id;       //ID
  curr_msg[5] = read_current_template[5]; //length LSB
  curr_msg[6] = read_current_template[6]; //length MSB
  curr_msg[7] = read_current_template[7]; //read
  curr_msg[8] = read_current_template[8]; //address 0 (present current address LSB)
  curr_msg[9] = read_current_template[9]; //address 1 (present current address MSB)
  curr_msg[10] = read_current_template[10]; //data length 0 (LSB)
  curr_msg[11] = read_current_template[11]; //data length 1 (MSB)

  union ushortToChar theCRC;
  theCRC.as_ushort = update_crc( 0 , curr_msg, sizeof(read_current_template) - 2);

  curr_msg[12] = theCRC.as_uchar[0]; //CRC 0 (MSB)
  curr_msg[13] = theCRC.as_uchar[1]; //CRC 1 (MSB)

  //  Dynamixel_Serial2_Write(curr_msg, sizeof(curr_msg));

  for ( int idx = 0; idx < sizeof(curr_msg); idx++) {
    DYNAMIXEL_SERIAL.write(curr_msg[idx]);
  }

  // I don't really like how this is set up but it seems to work:
  unsigned long curr_micros = micros();
  while (micros() < (curr_micros + wait_micros)) {
    while (DYNAMIXEL_SERIAL.available()) {
      input_buffer[buffer_idx] = DYNAMIXEL_SERIAL.read();
      buffer_idx++;
      if (buffer_idx == IN_BUF_SIZE) {
        buffer_idx = 0;
        // Serial.println("buffer reset");
      }
    }
  }

  if ( (input_buffer[2] == 0xFD) &&
       (input_buffer[1] == 0xFF) &&
       (input_buffer[0] == 0xFF) ) {

    //parse the current message
    curr_current.as_uchar[0] = input_buffer[9];
    curr_current.as_uchar[1] = input_buffer[10];
  }

  //reset the buffer index
  buffer_idx = 0;

  return curr_current.as_short;
}

int SB_Dynamixel::read_position() {
  const unsigned char read_position_template[] = {0xFF, 0xFF, 0xFD, 0x00, 0x00, 0x07, 0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x00, 0x00};
  union intToChar curr_position;
  int curr_pos;

  unsigned char curr_msg[sizeof(read_position_template)];
  curr_msg[0] = read_position_template[0]; //header
  curr_msg[1] = read_position_template[1]; //header
  curr_msg[2] = read_position_template[2]; //header
  curr_msg[3] = read_position_template[3]; //reserved
  curr_msg[4] = m_device_id;       //ID
  curr_msg[5] = read_position_template[5]; //length LSB
  curr_msg[6] = read_position_template[6]; //length MSB
  curr_msg[7] = read_position_template[7]; //read
  curr_msg[8] = read_position_template[8]; //address 0 (present position address LSB)
  curr_msg[9] = read_position_template[9]; //address 1 (present position address MSB)
  curr_msg[10] = read_position_template[10]; //data length 0 (LSB)
  curr_msg[11] = read_position_template[11]; //data length 1 (MSB)

  union ushortToChar theCRC;
  theCRC.as_ushort = update_crc( 0 , curr_msg, sizeof(read_position_template) - 2);

  curr_msg[12] = theCRC.as_uchar[0]; //CRC 0 (MSB)
  curr_msg[13] = theCRC.as_uchar[1]; //CRC 1 (MSB)

  for ( int idx = 0; idx < sizeof(curr_msg); idx++) {
    DYNAMIXEL_SERIAL.write(curr_msg[idx]);
    //Serial.print(curr_msg[idx], HEX);
    //Serial.print(" ");
  }

  // I don't really like how this is set up but it seems to work:

  unsigned long curr_micros = micros();
  while (micros() < (curr_micros + wait_micros)) {
    while (DYNAMIXEL_SERIAL.available()) {
      input_buffer[buffer_idx] = DYNAMIXEL_SERIAL.read();
      buffer_idx++;
      if (buffer_idx == IN_BUF_SIZE) {
        buffer_idx = 0;
        // Serial.println("buffer reset");
      }
    }
  }


  if ( (input_buffer[2] == 0xFD) &&
       (input_buffer[1] == 0xFF) &&
       (input_buffer[0] == 0xFF) ) {

    //parse the current message
    curr_position.as_uchar[0] = input_buffer[9];
    curr_position.as_uchar[1] = input_buffer[10];
    curr_position.as_uchar[2] = input_buffer[11];
    curr_position.as_uchar[3] = input_buffer[12];
  }

  //reset the buffer index
  buffer_idx = 0;

  curr_pos = curr_position.as_int;
  return curr_pos;
}

/* 
  The next four functions are for quickly set and get variables 
  stored in mcu, not the values set in dynamixels.
*/
void SB_Dynamixel::set_goal_position(int goal_pos) {
  m_goal_position = goal_pos;
}
void SB_Dynamixel::set_goal_current(short goal_current) {
  m_goal_current = goal_current;
}

int SB_Dynamixel::get_goal_position() {
  return m_goal_position;
}

short SB_Dynamixel::get_goal_current() {
  return m_goal_current;
}

double SB_Dynamixel::sind(double x) { return sin(x * M_PI / 180); }
double SB_Dynamixel::cosd(double x) { return cos(x * M_PI / 180); }

double SB_Dynamixel::posIntToDeg(int posInInt, int zeroOffset) {
  const double conversionFactor = 0.087890625;
  double posInDeg = double(posInInt - zeroOffset) * conversionFactor;
  return posInDeg;
}

int SB_Dynamixel::posDegToInt(double posInDeg, int zeroOffset) {
  const double conversionFactor = 11.37777778;
  int posInInt = int(posInDeg * conversionFactor) + zeroOffset;
  return posInInt;
}
