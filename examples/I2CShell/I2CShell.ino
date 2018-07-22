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
 * V 1.0 (10-Feb-18)
 * - numerous bug fixes
 * - dynamic changes of pullups and clock frequency
 * - storing macros in EEPROM
 * V 1.1 (11-Feb-18)
 * - line editing added
 * - documentation completed
 * V 1.2 (12-Feb)
 * - changed the implementation of kill-rest-of-line
 * - added line number for T-command
 * - allowed now for 700 kHz as the maximum bus clock; works perfect, but
 *   you definitely need low pullup resistors, the internal ones won't 
 *   work at this speed.
 * - some cleanup for the T command
 */

#define VERSION "1.3"
#define USEEEPROM 1
#define I2C_HARDWARE 1


#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <EEPROM.h>

#define HELPSTRING "Commands:\r\n" \
                   "H       - help                        S       - scan I2C bus\r\n" \
                   "T       - print last exec trace       T<num>  - print 20 cmds starting at <num>\r\n"\
                   "L       - list macros                 L<dig>  - list <dig> macro\r\n" \
                   "P       - show status of pullups      P<dig>  - enable/disable(1/0) pullups\r\n" \
                   "F       - show current I2C frequency  F<num>  - set I2C frequency in kHz\r\n" \
                   "<dig>=  - define macro                [ ...   - I2C interaction\r\n" \
                   "I2C interaction syntax:\r\n" \
                   "[       - (repeated) start condition  {       - start, polling for ACK\r\n" \
                   "]       - stop condition              r       - read byte\r\n" \
                   "<num>   - address or write byte       :<num>  - repeat previous <num> times\r\n" \
                   "&       - microsecond pause           %       - millisecond pause\r\n" \
                   "(<dig>) - macro call\r\n" \
                   "Editing commands:\r\n"\
                   "^A, UP  - Go to start of line         ^B,LEFT - go one char to back\r\n"\
                   "^D      - delete current char         ^E,DOWN - go to end of line\r\n"\
		   "^F,RIGHT- go one char forward         ^H, DEL - delete last character\r\n"\
                   "^K      - kill rest of line           ^P      - retrieve previous line\r\n"\
                   "$<dig>  - insert macro into line\r\n"\
                   "<dig> is a single digit between 0 and 9. <num> is any number in decimal, hex,\r\n"\
                   "octal or binary using the usual notation, e.g., 0xFF, 0o77, 0b11.\r\n" \
                   "<num>? denotes a 7-bit I2C read address, <num>! a write address." \


// constants for the I2C interface
#define I2C_TIMEOUT 100
#define I2C_PULLUP 1
#define SDA_PORT PORTC
#define SDA_PIN 4 // = A4
#define SCL_PORT PORTC
#define SCL_PIN 5 // = A5


#include <SoftI2CMaster.h>

// constants for this program
#define LINELEN 80
#define MAXCMDS 300
#define MACROS 5 // note: each macro needs LINELEN space

// magic code for EEPROM
#define MAGICKEY 0xA15A372FUL

// start of saving area for macros
#define EEMACSTART 4

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
	       READ_ACK, READ_NAK, PAUSE, LPAUSE, STOP, EOC, LOOP } exec_t;

// token types
typedef enum { EOL_TOK, START_TOK, WSTART_TOK, STOP_TOK,  NUM_TOK, READ_TOK,
	       WAIT_TOK, LWAIT_TOK, REPEAT_TOK, MAC_TOK, UNDEF_TOK } token_t;

// global variables
char line[LINELEN*2+1] = { '\0' };
exec_t cmds[MAXCMDS] = { EOC };
byte vals[MAXCMDS];
char macroline[MACROS][LINELEN+1];
char illegal_char;
long illegal_num;
int i2cfreq = (I2C_FASTMODE ? 400 : (I2C_SLOWMODE ? 25 : 100));
bool i2cpullups = (I2C_PULLUP != 0);

/* ---------------------------- Main program ------------------------*/
void setup()
{
  Serial.begin(115200);
  Serial.println(F("\n\n\rI2C Shell Version " VERSION));
  if (!i2c_init()) {
    Serial.println(F("I2C bus is locked up or there are no pullups!"));
  }
  for (byte i=0; i < MACROS; i++) macroline[i][0] = '\0';
#if USEEEPROM
  getMacrosEEPROM();
  for (byte i=0; i < MACROS; i++) macroline[i][LINELEN] = '\0';
#endif
}

void loop()
{
  int lineres, errpos;
  Serial.print(F("I2C>"));
  readLine(line);
  switch (toupper(line[0])) {
  case 'H':
    help();
    return;
  case 'L':
    list(line[1]);
    return;
  case 'S':
    scan();
    return;
  case 'T':
    report(line, cmds, vals);
    return;
  case 'P':
    pullups(line[1]);
    return;
  case 'F':
    frequency(line);
    return;
  }
  lineres = parseLine(line, cmds, vals, errpos);
  if (lineres == 0) {
    execute(cmds, vals);
    report("", cmds, vals);
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

/* ---------------------------- Read line & editing ------------------------*/



// read one line until a CR/LF is entered, do not accept more than LINELEN chars
// implement a few basic line editing commands
bool readLine(char *buf)
{
  char next = '\0';
  int i = 0, fill = 0, j;

  while (next != '\r' && next != '\n') {
    while (!Serial.available());
    next = Serial.read();
    if (next == '\x1b') { // filter out ESC-sequences
      while (!Serial.available());
      next = Serial.read();
      if (next == '[') { 
	while (!Serial.available());
	next = 0;
	switch(Serial.read()) {
	case 'A': next = 'A'-0x40;
	  break;
	case 'B': next = 'E'-0x40;
	  break;
	case 'C': next = 'F'-0x40;
	  break;
	case 'D': next = 'B'-0x40;
	}
      } 
    }
    if ('0' <= next && next < '0' + MACROS && i > 0)
      if (buf[i-1] == '$') {
	insertMacro(line, i, fill, next-'0');
	continue;
      }
    switch (next) {
    case '\x7F':
    case '\b':
      if (i > 0) {
	i--;
	Serial.write('\b');
	deleteChar(buf, i, fill);
      }
      break;
    case 'A'-0x40: // ^A - start of line
      moveCursor(line, i, fill, 0);
      break;
    case 'B'-0x40: // ^B - one char backwards
      if (i > 0) moveCursor(line, i, fill, i-1);
      break;
    case 'D'-0x40: // ^D - delete current char
      deleteChar(buf, i, fill);
      break;
    case 'E'-0x40: // ^E - end of line
      moveCursor(line, i, fill, fill);
      break;
    case 'F'-0x40: // ^F - one character forward
      if (i < fill) moveCursor(line, i, fill, i+1);
      break;
    case 'K'-0x40: // ^K - kill rest of line
      killRest(buf, i, fill);
      break;
    case 'P'-0x40: // ^P - previous line
      if (i == 0) {
	Serial.print(buf);
	while (i < LINELEN && buf[i]) i++;
	fill = i;
      }
      break;
    default:
      if (next >= ' ' && next < '\x7F' && fill < LINELEN) {
	insertChar(buf, i, fill, next);
      }
      break;
    }
  }
  buf[fill] = '\0'; 
  Serial.print("\r\n");
  delay(50);
  while (Serial.available() && (Serial.peek() == '\r' || Serial.peek() == '\n')) Serial.read();
  return true;
}

void moveCursor(char *line, int &cursor, int &fill, int target)
{
  if (target < 0 || target > fill) return; 
  if (target < cursor) {
    while (target < cursor) { 
      Serial.write('\b');
      cursor--;
    }
  } else {
    while (target > cursor) {
      Serial.write(line[cursor]);
      cursor++;
    }
  }
}


void deleteChar(char *line, int &cursor, int &fill)
{
  int i;
  for (i = cursor; i < fill-1; i++) line[i] = line[i+1];
  line[fill-1] = ' ';
  i = cursor;
  moveCursor(line, cursor, fill, fill);
  moveCursor(line, cursor, fill, i);
  fill--;
}


void killRest(char *line, int &cursor, int &fill)
{
  int i;
  for (i = cursor; i <= fill-1; i++) line[i] = ' ';
  i = cursor;
  moveCursor(line, cursor, fill, fill);
  moveCursor(line, cursor, fill, i);
  fill = i;
}

void insertChar(char *line, int &cursor, int &fill, char newch)
{
  int i;
  if (fill >= LINELEN) return;
  for (i = fill; i >= cursor; i--) line[i+1] = line[i];
  line[cursor] = newch;
  fill++;
  i = cursor;
  moveCursor(line, cursor, fill, fill);
  moveCursor(line, cursor, fill, i+1);
}

void insertMacro(char *line, int &cursor, int &fill, int macid)
{
  int i=0;
  moveCursor(line, cursor, fill, cursor-1);
  deleteChar(line, cursor, fill);
  while (macroline[macid][i] != '\0' && i < LINELEN) 
    insertChar(line, cursor, fill, macroline[macid][i++]);
}

/* ---------------------------- Parsing input line ------------------------*/


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
    case LWAIT_TOK:
      if (execix > MAXCMDS-3) return EXECOVF_ERR; // reserve one for EOC, one for STOP
      cmds[execix++] = LPAUSE;
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
    case '%': token = LWAIT_TOK; break;
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


/* ---------------------------- Execution & reporting ------------------------*/


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
    case LPAUSE:
      i++;
      if (cmds[i] == LOOP) delay(vals[i++]);
      else delay(1);
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

void report(char *line, exec_t *cmds, byte *vals)
{
 int i = 0;
 bool ack, nostart;
 byte readval;
 int startline = 0;
 int stopline = 0;
 long val;
 int ix = 1;
 token_t token;
 if (strlen(line) > 1) {
   token = nextToken(line, ix, val);
   if (token = NUM_TOK) {
     startline = val;
     stopline = startline + 19;
   }
 }
 while (true) {
   if (i >= startline && (i <= stopline || stopline == 0)) {
     switch(cmds[i]) {
     case START:
       Serial.print(F("Start: "));
       break;
     case REPSTART:
       Serial.print(F("Rep. start: "));
       break;
     case WSTART:
       Serial.print(F("Start ("));
       Serial.print(vals[i]);
       Serial.print(F(" NAKs): "));
       break;
     case WREPSTART:
       Serial.print(F("Rep. start ("));
       Serial.print(vals[i]);
       Serial.print(F(" NAKs): "));
       break;
     case WRITE_ACK:
     case WRITE_NAK:
       nostart = false;
       if (i == 0 || (cmds[i-1] != START && cmds[i-1] != REPSTART && cmds[i-1] != WSTART && cmds[i-1] != WREPSTART)) {
	 Serial.print(F("Write: "));
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
       Serial.print(F("Read: 0x"));
       if (vals[i] < 0x10) Serial.print(0);
       Serial.print(vals[i], HEX);
       Serial.println(((cmds[i] == READ_ACK ? " + ACK" : " + NAK")));
       break;
     case LPAUSE:
     case PAUSE:
       if (cmds[i] == PAUSE)
	 Serial.print(F("msec delay"));
       else
	 Serial.print(F("\u03BC" "sec delay"));
       if (cmds[i+1] == LOOP) {
	 Serial.print(F(": "));
	 Serial.print(vals[i+1]);
	 Serial.print(F(" repetitions"));
       }
       Serial.println();
       break;
     case STOP: 
       Serial.print(F("Stop"));
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
   }
   if (cmds[i++] == EOC) return;
 }
}

/* ---------------------------- Shell commands ------------------------*/


void storeMacro(int macnum, char *line)
{
  strncpy(macroline[macnum], &(line[2]), LINELEN+1);
#if USEEEPROM
  putMacrosEEPROM();
#endif
}


void list(char index)
{
  byte i = index - '0';
  if (i >= 0 && i < MACROS) {
    Serial.print(i);
    Serial.print(F("="));
    Serial.println(macroline[i]);
  } else {
    for (i=0; i < MACROS; i++) {
      Serial.print(i);
      Serial.print(F("="));
      Serial.println(macroline[i]);
    }
  }
}
    

void help() {
  Serial.println(F("\nI2C Shell Version " VERSION));
  Serial.println(F(HELPSTRING));
}



void scan()
{
  Serial.println(F("Scanning ..."));
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

void pullups(char arg)
{
  if (arg == '\0') {
    Serial.print(F("Pullups are currently "));
    if (i2cpullups) Serial.println(F("enabled"));
    else Serial.println(F("disabled"));
    return;
  }
#if I2C_HARDWARE
  if (arg == '1') { // enable pullups
    digitalWrite(SDA, 1);
    digitalWrite(SCL, 1);
    Serial.println(F("Pullups enabled"));
  } else if (arg == '0') { // disable pullups
    digitalWrite(SDA, 0);
    digitalWrite(SCL, 0);
    Serial.println(F("Pullups disabled"));
  } else {
    Serial.println(F("Specify 0 or 1 with P command or append nothing"));
  }
#else
  Serial.println(F("Set I2C_HARDWARE constant to 1 in sketch"));
  Serial.println(F("when you want to change pullups dynamically"));
#endif
}

void frequency(char *line)
{
  token_t token;
  long value;
  int ix = 1;
  int bitrate;
  
  token = nextToken(line, ix, value);
  if (token != NUM_TOK) {
    Serial.print(F("I2C clock frequency is "));
    Serial.print(i2cfreq);
    Serial.println(F(" kHz"));
    return;
  }
#if I2C_HARDWARE
  if (value < 1 || value > 700) {
    Serial.println(F("Cannot set I2C clock frequency lower than 1 or higher than 700 kHz"));
    return;
  }
  i2cfreq = value;
  bitrate = (I2C_CPUFREQ/(value*1000UL)-16)/2;
  if (bitrate < 3) {
    Serial.println(F("Requested frequency is to high!"));
    return;
  }
  Serial.print(F("I2C clock frequency set to "));
  Serial.print(i2cfreq);
  Serial.println(F(" kHz"));
  if (bitrate <= 255) {
    TWSR = 0;
    TWBR = bitrate;
    return;
  }
  bitrate = bitrate/4;
  if (bitrate <= 255) {
    TWSR = (1<<TWPS0);
    TWBR = bitrate;
    return;
  }
  bitrate = bitrate/4;
  if (bitrate <= 255) {
    bitrate = (I2C_CPUFREQ/(value*1000UL)-16)/32;
    TWSR = (1<<TWPS1);
    TWBR = bitrate;
  }
  bitrate = bitrate/4;
  TWSR = (1<<TWPS1)|(1<<TWPS0);
  TWBR = bitrate;
  return;
#else
  Serial.println(F("Set I2C_HARDWARE constant to 1 in sketch"));
  Serial.println(F("when you want to change the frequency dynamically"));
#endif
}

/* ---------------------------- Macros in EEPROM ------------------------*/


void getMacrosEEPROM()
{
  unsigned long key;
  EEPROM.get(0, key);
  if (key == MAGICKEY) 
    EEPROM.get(EEMACSTART, macroline);
}

void putMacrosEEPROM()
{
  unsigned long key = MAGICKEY;
  EEPROM.put(0, key);
  EEPROM.put(EEMACSTART, macroline);
}
