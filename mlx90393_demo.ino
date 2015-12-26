#include <Wire.h>
#include <string.h>
int chip_0 = 0x0C;
int chip_1 = 0x0D;
int chip_2 = 0x0E;
int chip_3 = 0x0F;

/*Single Measurement  command 0011 zyxt*/
unsigned int Command_RT(int address) {
    unsigned int c;
    Wire.beginTransmission(address);
    Wire.write(0XF0);
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    while(Wire.available()) {
        c = Wire.read();
    }
    return c;
}

/*Single Measurement  command 0011 zyxt*/
unsigned int Command_SM(int address, int key) {
    unsigned int c;
    Wire.beginTransmission(address);
    Wire.write(key);
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    while(Wire.available()) {
        c = Wire.read();
    }
    return c;
}

/*Read Measurement command 0100 zyxt read back 1 (status) + 2*[zyxt]*/
unsigned int Command_RM(int address, int key, unsigned int *value_array, int read_len) {
    unsigned int c;
    Wire.beginTransmission(address);
    Wire.write(key);
    Wire.endTransmission();
    Wire.requestFrom(address, read_len + 1);
    while(Wire.available()) {
        c = Wire.read();
        for(int i = 0;  i < read_len; i++)
        {
            value_array[i] = Wire.read();
        }
    }
    return c;
}

/*Read Register command 0101 0000 A[5:0] << 2 read back 1 (status) + 2 (data)*/
unsigned int Command_RR(int address, int register_address, unsigned int *value_array) {
    unsigned int c;
    int value_array_len = (sizeof(value_array));
    Wire.beginTransmission(address);
    Wire.write(0x50);
    Wire.write(register_address << 2);
    Wire.endTransmission();
    Wire.requestFrom(address, value_array_len + 1);
    while(Wire.available()) {
        c = Wire.read();
        for(int i = 0;  i < value_array_len; i++)
        {
            value_array[i] = Wire.read();
        }
    }
    return c;
}

/*Write Register command 0110 0000 D[15:8] D[7:0] A[5:0] << 2 1 (status)*/
unsigned int Command_WR(int address, int AH, int AL, int register_address) {
    unsigned int c;
    Wire.beginTransmission(address);
    Wire.write(0x60);
    Wire.write(AH);
    Wire.write(AL);
    Wire.write(register_address << 2);
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    while(Wire.available()) {
        c = Wire.read();
    }
    return c;
}

float temperature(int address) {

    unsigned int value_array[2];

    Command_SM(address, 0x31);
    Command_RM(address, 0x41, value_array, 2);

    return ((value_array[0] << 8) + value_array[1] - 46244)/45.2 + 25;
}


void test_temperature() {
    int chips[4];
    chips[0] = chip_0;chips[1] = chip_1;chips[2] = chip_2;chips[3] = chip_3;
    for(size_t i = 0; i < 4; i++)
    {
        Serial.println(temperature(chips[i]));
    }
}

void get_gain_res(int address, unsigned int *gain, unsigned int *res) {
    unsigned int value_array[2];
    Command_RR(address, 0x00, value_array);//GAIN_SEL 0x00 6 5 4
    *gain = (value_array[1] & 0x70) >> 4;

    Command_RR(address, 0x02, value_array);//RES_XYZ 0x02 10 9 8 7 6 5
    *res = (value_array[1] & 0x60) >> 5;
}

void get_gauss(int address, float *x, float *y, float *z) {
    unsigned int gain, res;
    unsigned int read_back[8];

    int t;

    Command_WR(address, 0x00, 0x5C, 0x00);//
    /*delay(10);*/
    Command_WR(address, 0x02, 0xB4, 0x02);//0000 0010 1011 0100
    /*delay(10);*/
    get_gain_res(address, &gain, &res);
    Command_SM(address, 0x3F);//0011 zyxt
    delay(10);

    Command_RM(address, 0x4F, read_back, 8);//0100 zyxt
    /*Serial.println("begin");
    for(size_t i = 0; i < 8; i++)
    {
        Serial.println(read_back[i]);
    }
    Serial.println("end");*/
    t = (read_back[2] << 8) + read_back[3];
    *x = t * 0.537;

    t = (read_back[4] << 8) + read_back[5];
    *y = t * 0.537;

    t = (read_back[6] << 8) + read_back[7];
    *z = t * 0.979;

}

void test_gain_res() {
    unsigned int gain;
    unsigned int res;
    int chips[4];
    chips[0] = chip_0;chips[1] = chip_1;chips[2] = chip_2;chips[3] = chip_3;

    for(size_t i = 0; i < 4 ; i++)
    {
        Command_WR(chips[i], 0x00, 0x5C, 0x00);//
        Command_WR(chips[i], 0x02, 0xB4, 0x02);//0000 0010 1011 0100
        get_gain_res(chips[i], &gain, &res);
        Serial.println(gain);
        Serial.println(res);
    }
}

void test_register() {
    unsigned int value_array[2];
    Serial.println(Command_RR(chip_0, 0x00, value_array));
    for(size_t i = 0;  i < 2; i++)
    {
        Serial.println(value_array[i]);
    }

}

void setup() {

    Serial.begin(115200);
    Wire.begin();
}

void loop() {
    float x, y, z;
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;
    get_gauss(chip_0, &x, &y, &z);
    get_gauss(chip_1, &x1, &y1, &z1);
    get_gauss(chip_2, &x2, &y2, &z2);
    Serial.println('b' + String(x) + '%' + String(y) + '%' + String(z) +'%' + String(x1) + '%' + String(y1) + '%' + String(z1) +'%' + String(x2) + '%' + String(y2) + '%' + String(z2) +'e');
}
