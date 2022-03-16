#define I2C_WRITE 0
#define I2C_READ 1

using namespace std;

bool i2c_init(void);
bool i2c_start(uint8_t addr);
bool i2c_start_wait(uint8_t addr);
bool i2c_rep_start(uint8_t addr);
bool i2c_write(uint8_t data);
byte i2c_read(bool last);
void i2c_stop();
