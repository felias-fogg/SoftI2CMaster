// -*- c++ -*-
/* I2CShell
 * This is a simple program acting as a shell to interact with I2C devices, using the
 * syntax from Bus Pirate. The difference to the Bus Pirate is that we only start
 * executing after everything has been typed in. Further, a line has to be closed by ']', i.e.
 * the stop condition. If it doesn't, we will add it.
 * You can use the Arduino serial monitor or a terminal window to interact.
 *
 * V 0.9 (08-FEB-18)
 * - first full version working with SoftI2CMaster
 */

#define HELPSTRING "Commands:\r\n" \
                   "H       - help                        S      - scan I2C bus\r\n" \
                   "L       - list macros                 <dig>= - define macro\r\n" \
                   "Ctrl-P  - retrieve last input & edit  P      - print last exec trace\r\n" \
                   "[ ...   - I2C command\r\n" \
                   "I2C command syntax:\r\n" \
                   "[       - (repeated) start condition  {       - start, polling for ACK\r\n" \
                   "]       - stop condition              r       - read byte\r\n" \
                   "<num>   - address or write byte       :<num>  - repeat previous <num> times\r\n" \
                   "&       - microsecond pause           (<dig>) - macro call\r\n" \
                   "<dig> is a single digit between 0 and 9.\r\n" \
                   "<num> is any number between 0 and 255 in decimal, hex, octal or binary\r\n" \
                   "using the usual notation, e.g., 0xFF, 0o77, 0b11\r\n" \
                   "<num>? denotes a 7-bit I2C read address, <num>! a write address,\r\n" \
                   "i.e., <num>? = 2x<num>+1 and <num>! = 2x<num>.\r\n"

#define VERSION "0.9"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// constants for the I2C interface
#define I2C_TIMEOUT 20
#define I2C_PULLUP 1
#define SDA_PORT PORTC
#define SDA_PIN 4 // = A4
#define SCL_PORT PORTC
#define SCL_PIN 5 // = A5


#include <SoftI2CMaster.h>


// constants for this program
#define LINELEN 80
#define MAXCMDS 255
#define MACROS 5 // note: each macro needs LINELEN space!

// error codes
#define EXECOVF_ERR -1
#define REPZERO_ERR -2
#define ILLCH_ERR -3
#define NUMTL_ERR -4
#define ILLMAC_ERR -5
#define WRGMACNUM_ERR -6
#define DBLREP_ERR -7
#define STRTREP_ERR -8
#define NOADDR_ERR -9
#define UNKNWN_ERR -10

// exec type (cmds & results)
typedef enum { START, REPSTART, WSTART, WREPSTART,
	       WRITE, WRITE_ACK, WRITE_NAK,
	       READ_ACK, READ_NAK, PAUSE, STOP, EOC, LOOP } exec_t;

// token types
typedef enum { EOL_TOK, START_TOK, WSTART_TOK, STOP_TOK,  NUM_TOK, READ_TOK,
	       WAIT_TOK, REPEAT_TOK, MAC_TOK, UNDEF_TOK } token_t;

// global variables
char line[LINELEN*2+1] = { '\0' };
exec_t cmds[MAXCMDS] = { EOC };
byte vals[MAXCMDS];
char macroline[MACROS][LINELEN+1];
char illegal_char;
long illegal_num;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("\n\n\rI2C Shell Version " VERSION));
  if (!i2c_init()) {
    Serial.println(F("Sorry! I2C bus is locked up!"));
    Serial.println(F("Giving up!"));
    while (1);
  }
  for (byte i=0; i < MACROS; i++) macroline[i][0] = '\0';
}

void loop()
{
  int lineres, errpos;
  Serial.print(F("I2C>"));
  readLine(line, 0);
  if (strlen(line) == 1) 
    switch (toupper(line[0])) {
    case 'H':
      help();
      return;
    case 'L':
      list();
      return;
    case 'S':
      scan();
      return;
    case 'P':
      report(cmds, vals);
      return;
    case '\x10':
      Serial.print(line);
      readLine(line, strlen(line));
      break;
    }
  lineres = parseLine(line, cmds, vals, errpos);
  if (lineres == 0) {
    execute(cmds, vals);
    report(cmds, vals);
  } else if (lineres > 0) {
    if (lineres-1 < MACROS) storeMacro(lineres-1, line);
    else {
      Serial.println(F("Macro index in definition too large:"));
      Serial.println(lineres-1);
    }
  } else {
    for (int i=0; i < errpos+3; i++) Serial.print(" ");
    Serial.println("^");
    switch(lineres) {
    case EXECOVF_ERR: Serial.print(F("Too many commands"));
      break;
    case REPZERO_ERR: Serial.print(F("Repeat counter of 0 is not allowed"));
      break;
    case ILLCH_ERR: Serial.print(F("Unrecognized character: '"));
      Serial.print(illegal_char);
      Serial.print(F("'"));
      break;
    case NUMTL_ERR: Serial.print(F("Number too large: "));
      Serial.print(illegal_num);
      break;
    case ILLMAC_ERR: Serial.print(F("Ill-formed macro call"));
      break;
    case WRGMACNUM_ERR: Serial.print(F("Macro number too large"));
      break;
    case DBLREP_ERR: Serial.print(F("Double repetition is not allwed"));
      break;
    case STRTREP_ERR: Serial.print(F("Start repetition is not allowed"));
      break;
    case NOADDR_ERR: Serial.print(F("No address after start condition"));
      break;
    default: Serial.print(F("Unrecogized parsing error"));
      break;
    }
    Serial.println();
    cmds[0] = EOC;
  }
}

void storeMacro(int macnum, char *line)
{
  strncpy(macroline[macnum], &(line[2]), LINELEN+1);
}


void list()
{
  for (byte i=0; i < MACROS; i++) {
    Serial.print(i);
    Serial.print(F("="));
    Serial.println(macroline[i]);
  }
}
    

void help() {
  Serial.println(F("\nI2C Shell Version " VERSION));
  Serial.println(F(HELPSTRING));
}



// read one line until a CR/LF is entered or line limit is reached
bool readLine(char *buf, int i)
{
  char next = '\0';

  while (next != '\r' && next != '\n') {
    while (!Serial.available());
    next = Serial.read();
    if (next == '\x1b') { 
      while (!Serial.available());
      Serial.read();
      while (!Serial.available());
      Serial.read();
      next = '\0';
    }
    if (next >= '\0' || next < '\x7F') Serial.write(next);
    switch (next) {
    case '\r':
    case '\n':
      buf[i] = 0;
      break;
    case '\x7F':
    case '\b':
      if (i > 0) {
	i--;
	Serial.print(F("\b \b"));
      }
      break;
    case '\x10': // ^P
      if (i == 0) {
	Serial.print(buf);
	while (i < LINELEN && buf[i]) i++;
      }
      break;
    default:
      if (i == LINELEN)
	buf[i] = 0;
      else if (next >= ' ')
	buf[i++] = next;
      break;
    }
  }
  Serial.print("\r\n");
  delay(50);
  while (Serial.available() && (Serial.peek() == '\r' || Serial.peek() == '\n')) Serial.read();
  return true;
}

int parseLine(char *buf, exec_t *cmds, byte *vals, int &lineix)
{
  bool busreleased = true;
  int execix = 0;
  int result;

  lineix = 0;
  result = parseLineHelper(buf, cmds, vals, lineix, execix, busreleased, false);

  if (result >= 0) {
    execix = 0;
    while (cmds[execix] != EOC) {
      if (cmds[execix] == READ_ACK) 
	if (stopFollows(cmds, execix+1))
	  cmds[execix] = READ_NAK;
      execix++;
    }
  }
  
  return result;
}

bool stopFollows(exec_t *cmds, int index)
{
  while (cmds[index] == LOOP || cmds[index] == PAUSE) index++;
  if (cmds[index] == READ_ACK) return false;
  return true;
}
  

int parseLineHelper(char *buf, exec_t *cmds, byte *vals, int &lineix, int &execix, bool &busreleased, bool recursive)
{
  token_t token = UNDEF_TOK, last = UNDEF_TOK, beforelast = UNDEF_TOK;
  long value = 0; 
  int macindex = 0;
  int maclineix,  maccall, repeat;
  exec_t cmd;

  if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '=') { // macro def
    macindex = buf[0] - '0' + 1;
    lineix = 2;
  }
  
  while (true) {
    beforelast = last;
    last = token;
    token = nextToken(buf, lineix, value);
    switch(token) {
    case EOL_TOK:
      if (recursive) return 0;
      if (!busreleased) {
	if (execix > MAXCMDS-2) return EXECOVF_ERR; 
	cmds[execix++] = STOP;
      } else {
	if (execix > MAXCMDS-1) return EXECOVF_ERR;
      }
      cmds[execix++] = EOC;
      return macindex;
    case START_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      if (busreleased) cmds[execix++] = START;
      else cmds[execix++] = REPSTART;
      busreleased = false;
      break;
    case WSTART_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      if (busreleased) cmds[execix++] = WSTART;
      else cmds[execix++] = WREPSTART;
      busreleased = false;
      break;
    case STOP_TOK:
      if (execix > MAXCMDS-2) return EXECOVF_ERR; // reserve one for EOC
      cmds[execix++] = STOP;
      busreleased = true;
      break;
    case NUM_TOK:
      if (value < 0 || value > 255) {
	illegal_num = value;
	return NUMTL_ERR;
      }
      if (execix > 0) {
	if (cmds[execix-1] == LOOP && vals[execix-1] == 0) {
	  if (value == 0) {
	    return REPZERO_ERR;
	  } else {
	    vals[execix-1] = value;
	    value = -1;
	  }
	}
      }
      if (value >= 0) {
	if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
	cmds[execix] = WRITE;
	vals[execix++] = value;
      }
      break;
    case READ_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      cmds[execix++] = READ_ACK;
      break;
    case WAIT_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      cmds[execix++] = PAUSE;
      break;
    case REPEAT_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      if (execix > 0) {
	if (cmds[execix-1] == LOOP || beforelast == REPEAT_TOK) return DBLREP_ERR;
	if (cmds[execix-1] == START) return STRTREP_ERR;
	if (cmds[execix-1] == REPSTART) return STRTREP_ERR;
      }
      cmds[execix] = LOOP;
      vals[execix++] = 0;
      break;
    case MAC_TOK:
      maclineix = 0;
      if (value >= MACROS) return WRGMACNUM_ERR;
      maccall = parseLineHelper(macroline[value], cmds, vals, maclineix, execix, busreleased, true);
      if (maccall < 0) return maccall;
      break;
    case UNDEF_TOK:
      return ILLCH_ERR;
      break;
    default:
      return UNKNWN_ERR;
      break;
    }
    if (execix >= 2 && cmds[execix-1] == LOOP && vals[execix-1] > 0 &&
	(cmds[execix-2] == READ_ACK ||  cmds[execix-2] == WRITE)) {
      // unfold read and write commands!
      repeat = vals[execix-1];
      cmd = cmds[execix-2];
      value = vals[execix-2];
      execix--;
      if (execix + repeat - 1 > MAXCMDS-3) return EXECOVF_ERR;
      for (byte i=0; i<repeat-1; i++) {
	cmds[execix] = cmd;
	vals[execix++] = value;
      }
    }
    if (execix >= 2 && (cmds[execix-2] == START || cmds[execix-2] == REPSTART) &&
	cmds[execix-1] != WRITE)
      return NOADDR_ERR;
  }
}

token_t nextToken(char *buf, int &i, long &value)
{
  char nextch;
  token_t token;
  value = 0;
  while (buf[i] == ' ' || buf[i] == '\t' || buf[i] == ',') i++;
  nextch = toupper(buf[i]);
  if (nextch >= '0' && nextch <= '9') token = NUM_TOK;
  else {
    switch(nextch) {
    case '\0': token = EOL_TOK; break;
    case '[': token = START_TOK; break;
    case '{': token = WSTART_TOK; break;
    case ']': token = STOP_TOK; break;
    case 'R': token = READ_TOK; break;
    case '&': token = WAIT_TOK; break;
    case ':': token = REPEAT_TOK; break;
    case '(': token = MAC_TOK; break;
    default: token = UNDEF_TOK; illegal_char = buf[i]; break;
    }
  }
  if (token == NUM_TOK) value = parseNum(buf, i);
  else if (token == MAC_TOK) value = parseMac(buf, i);
  else i++;
  return token;
}

long parseNum(char *buf, int &i)
{
  long result = 0;
  byte base = 10;

  if (buf[i] == '0') {
    switch (toupper(buf[i+1])) {
    case 'X':
      base = 16;
      i += 2;
      break;
    case 'O':
      base = 8;
      i += 2;
      break;
    case 'B':
      base = 2;
      i += 2;
      break;
    }
  }
  while (conv2digit(buf[i]) < base) 
    result = result*base + conv2digit(buf[i++]);
  if (buf[i] == '?' || buf[i] == '!') 
    result = (result<<1) + ((buf[i++] == '?') ? 1 : 0);
  return result;
}

byte conv2digit(char c)
{
  if ('0' <= c && c <= '9') return c - '0';
  else if ('A' <= toupper(c) && toupper(c) <= 'F') return toupper(c) - 'A' + 10;
  else return 255;
}

int parseMac(char *buf, int &i)
{
  if (buf[i+1] < '0' || buf[i+1] > '9' || buf[i+2] != ')') return ILLMAC_ERR;
  i += 3;
  return (buf[i-2] - '0');
}


void execute(exec_t *cmds, byte *vals)
{
  int i = 0;
  bool ack;
  while (true) {
    switch(cmds[i]) {
    case START:
      i++;
      if (cmds[i] == WRITE) {
	ack = i2c_start(vals[i]);
	cmds[i++] = (ack ? WRITE_ACK : WRITE_NAK);
      }
      break;
    case REPSTART:
      i++;
      if (cmds[i] == WRITE) {
	ack = i2c_rep_start(vals[i]);
	cmds[i++] = (ack ? WRITE_ACK : WRITE_NAK);
      }
      break;
    case WSTART:
      i++;
      if (cmds[i] == WRITE) {
	vals[i-1] = 0;
	ack = false;
	while (vals[i-1] < 255 && !ack) {
	  ack = i2c_start(vals[i]);
	  if (!ack) i2c_stop();
	  vals[i-1]++;
	}
	vals[i-1]--;
	cmds[i++] = (ack ? WRITE_ACK : WRITE_NAK);
      }
      break;
    case WREPSTART:
      i++;
      if (cmds[i] == WRITE) {
	vals[i-1] = 0;
	ack = false;
	while (vals[i-1] < 255 && !ack) {
	  ack = i2c_rep_start(vals[i]);
	  if (!ack) i2c_stop();
	  vals[i-1]++;
	}
	vals[i-1]--;
	cmds[i++] = (ack ? WRITE_ACK : WRITE_NAK);
      }
      break;
    case WRITE:
      ack = i2c_write(vals[i]);
      cmds[i++] = (ack ? WRITE_ACK : WRITE_NAK);
      break;
    case READ_ACK: 
      vals[i++] = i2c_read(false);
      break;
    case READ_NAK: 
      vals[i++] = i2c_read(true);
      break;
    case PAUSE:
      i++;
      if (cmds[i] == LOOP) delayMicroseconds(vals[i++]);
      else delayMicroseconds(1);
      break;
    case STOP: 
      i++;
      if (cmds[i] == LOOP) {
	for (byte i=0; i < vals[i]; i++) {
	  i2c_stop();
	  delayMicroseconds(1);
	}
	i++;
      } else i2c_stop(); 
      break;
    case LOOP:
      break;
    case EOC:
      delay(200); // decouple from console output!
      return;
    }
  }
}

void report(exec_t *cmds, byte *vals)
{
 int i = 0;
 bool ack, nostart;
  byte readval;
  while (true) {
    switch(cmds[i]) {
    case START:
      Serial.print(F("Start cond.: "));
      break;
    case REPSTART:
      Serial.print(F("Rep. start cond.: "));
      break;
    case WSTART:
      Serial.print(F("Start cond. ("));
      Serial.print(vals[i]);
      Serial.print(F(" NAKs): "));
      break;
    case WREPSTART:
      Serial.print(F("Rep. start cond. ("));
      Serial.print(vals[i]);
      Serial.print(F(" NAKs): "));
      break;
    case WRITE_ACK:
    case WRITE_NAK:
      nostart = false;
      if (i == 0 || (cmds[i-1] != START && cmds[i-1] != REPSTART && cmds[i-1] != WSTART && cmds[i-1] != WREPSTART)) {
	Serial.print(F("Write byte: "));
	nostart = true;
      }
      Serial.print(F("0x"));
      if (vals[i] < 0x10) Serial.print(0);
      Serial.print(vals[i], HEX);
      if (!nostart) {
	Serial.print(F(" (0x"));
	if ((vals[i]>>1) < 0x10) Serial.print(0);
	Serial.print((vals[i]>>1), HEX);
	Serial.print(((vals[i])%2 == 0 ? "!" : "?"));
	Serial.print(F(")"));
      }
      Serial.println(((cmds[i] == WRITE_ACK ? " + ACK" : " + NAK")));
      break;
    case READ_ACK: 
    case READ_NAK: 
      Serial.print(F("Read byte: "));
      if (vals[i] < 0x10) Serial.print(0);
      Serial.print(vals[i], HEX);
      Serial.println(((cmds[i] == READ_ACK ? " + ACK" : " + NAK")));
      break;
    case PAUSE:
      Serial.print(F("Microsecond delay"));
      if (cmds[i+1] == LOOP) {
	Serial.print(F(": "));
	Serial.print(vals[i+1]);
	Serial.print(F(" repetitions"));
      }
      Serial.println();
    case STOP: 
      Serial.print(F("Stop condition"));
      if (cmds[i+1] == LOOP) {
	Serial.print(F(": "));
	Serial.print(vals[i+1]);
	Serial.print(F(" repetitions"));
      }
      Serial.println();
      break;
    case EOC:
      return;
      break;
    }
    i++;
  }
}

void scan()
{
  Serial.println(F("Scanning ...\n"));
  Serial.println(F(" 8-bit 7-bit addr"));
  for (int addr = 0; addr < 256; addr = addr+1) {
    if (i2c_start(addr)) {
      if (addr%2 == 0) i2c_write(0);
      else i2c_read(true);
      i2c_stop();
      Serial.print(F(" 0x"));
      if (addr <= 0xF) Serial.print(0);
      Serial.print(addr,HEX);
      Serial.print(F("  0x"));
      if (addr>>1 <= 0xF) Serial.print(0);
      Serial.print((addr>>1),HEX);
      Serial.println((addr%2 == 0 ? "!" : "?"));
      delay(200); // 
    } else i2c_stop();
  }  
}
