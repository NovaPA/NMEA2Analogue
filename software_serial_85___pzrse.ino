#include <SoftwareSerial.h>

/*
 * [PARSER]
 * Parser code starts here
 *
*/

typedef enum NMPARSE_MESSAGE_TYPE {
  NMEA_MESSAGE_TYPE_UNKNOWN,
  NMEA_MESSAGE_TYPE_VHW
} NMPARSE_MESSAGE_TYPE;

typedef struct NMPARSE_MESSAGE {
  NMPARSE_MESSAGE_TYPE type;
  float speed_water_knots;
  float speed_water_kilometers;
} NMPARSE_MESSAGE;

/**
 * Get start position and length of NMEA message stored in the buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 *
 * @return true on success, false on error
 */
bool nmparse_peek(const char *buffer, int buflen, int *nmeastart, int *nmealen)
{
  if (!nmeastart || !nmealen)
  {
    return false;
  }

  *nmeastart = 0;
  *nmealen = 0;

  bool has_start = false;
  for (int i = 0; i < buflen; i++)
  {
    if (buffer[i] == '$')
    {
      *nmeastart = i;
      has_start = true;
    }
  }

  if (has_start)
  {
     int spos = *nmeastart;
    for (int i = spos; i < buflen - spos; i++)
    {
      if (buffer[i] == '\n')
      {
        if (i < 5)
        {
          return false;
        }
        *nmealen = i + 1;
        return true;
      }
    }
  }

  return false;
}

/**
 * Get type of NMEA message stored in the buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 *
 * @return NMEA message type
 */
NMPARSE_MESSAGE_TYPE nmparse_get_type(const char *buffer, int buflen)
{
  if (!buffer || buflen < 5)
  {
    return NMEA_MESSAGE_TYPE_UNKNOWN;
  }

  for (int i = 0; i < buflen; i++)
  {
    if (buffer[i] == '$')
    {
      if (!strncmp(buffer + i + 1, "--VHW", 5))
      {
        return NMEA_MESSAGE_TYPE_VHW;
      }
    }
  }

  return NMEA_MESSAGE_TYPE_UNKNOWN;
}

/**
 * Parse NMEA message
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 * @param[inout] result - parse result
 *
 * @return NMEA message type
 */
void nmparse_parse_core(const char *buffer, int buflen, NMPARSE_MESSAGE *result)
{
  int comma_num = 0;
  char *param_start = NULL;
  char *param_end = NULL;
  switch (result->type)
  {
    case NMEA_MESSAGE_TYPE_VHW:
    for (int i = 0; i < buflen; i++)
    {
      if (buffer[i] == ',')
      {
        comma_num++;
        if (comma_num == 5)
        {
          result->speed_water_knots = atof(buffer + i + 1);
        }
        else if (comma_num == 7)
        {
          result->speed_water_kilometers = atof(buffer + i + 1);
          return;
        }
      }
    }
    break;
    default:
    break;
  }
}

/**
 * Try to parse NMEA message from buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 * @param[out] result - parse result
 *
 * @return true on success, false on error
 */
bool nmparse_try_parse(const char *buffer, int buflen, NMPARSE_MESSAGE *result)
{
  if (!buffer || !result)
  {
    return false;
  }

  memset(result, 0, sizeof(NMPARSE_MESSAGE));

  int pos;
  int len;
  if (nmparse_peek(buffer, buflen, &pos, &len))
  {
    buffer += pos;
    result->type = nmparse_get_type(buffer, len);
    if (result->type != NMEA_MESSAGE_TYPE_UNKNOWN)
    {
      nmparse_parse_core(buffer, len, result);
    }
    return true;
  }

  return false;
}

/*
 * [MAIN]
 * MCU code starts here
 *
*/

#define SERIAL0_BAUD 4800
#define SERIAL0_CONF SERIAL_8N1

#define SERIAL1_BAUD 4800
#define SERIAL1_CONF SERIAL_8N1

#define NMEA_MAX_MSG_LENGTH 82
#define SERIAL_BUFFER_SIZE 64

static int s_s0_collected = 0;
static char s_s0_rx[(NMEA_MAX_MSG_LENGTH * 2) + (SERIAL_BUFFER_SIZE + 1)];

static int s_s1_collected = 0;
static char s_s1_rx[(NMEA_MAX_MSG_LENGTH * 2) + (SERIAL_BUFFER_SIZE + 1)];


SoftwareSerial mySerial(3, 4); // RX, TX

float vout;
int anaout;

void setup()
{
    memset(s_s0_rx, 0, sizeof(s_s0_rx));
  memset(s_s1_rx, 0, sizeof(s_s1_rx));
  
  mySerial.begin(4800);
  mySerial.println("Hello, world?");
  pinMode (1, OUTPUT);
}

void loop() 
{

    int avail = mySerial.available();
  NMPARSE_MESSAGE result;

  if (avail)
  {
    if ((s_s0_collected + avail) > (sizeof(s_s0_rx)))
    {
      //Buffer overflow. This should never happen during normal operation, so just force reset the buffer
      s_s0_collected = 0;
    }

    s_s0_collected += mySerial.readBytes(s_s0_rx + s_s0_collected, avail);

    if (nmparse_try_parse(s_s0_rx, s_s0_collected, &result))
    {
      if (result.type == NMEA_MESSAGE_TYPE_VHW)
      {
        // OUTPUT:
        //result.speed_water_knots
        //result.speed_kilometers
      }

      memset(s_s0_rx, 0, sizeof(s_s0_rx));
      s_s0_collected = 0;
    }

  }
  
             if (s_s0_collected>0 and s_s0_collected<255) { 
              vout = s_s0_collected*0.0875+0.360-s_s0_collected*s_s0_collected*0.0008;
              anaout = 255/5*vout;
              analogWrite(1, anaout);};

      mySerial.print("knots discreets: ");
    mySerial.println(anaout);

  }
