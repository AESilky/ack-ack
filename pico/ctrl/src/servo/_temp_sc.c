


void LobotSerialServoMove(HardwareSerial& SerialX, uint8_t id, int16_t position, uint16_t time) {
    uint8_t buf[10];
    if (position < 0)
        position = 0;
    if (position > 1000)
        position = 1000;
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 7;
    buf[4] = BS_MOVE_TIME_WRITE;
    buf[5] = GET_LOW_BYTE(position);
    buf[6] = GET_HIGH_BYTE(position);
    buf[7] = GET_LOW_BYTE(time);
    buf[8] = GET_HIGH_BYTE(time);
    buf[9] = LobotCheckSum(buf);
    SerialX.write(buf, 10);
}

void LobotSerialServoStopMove(HardwareSerial& SerialX, uint8_t id) {
    uint8_t buf[6];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = BS_MOVE_STOP;
    buf[5] = LobotCheckSum(buf);
    SerialX.write(buf, 6);
}

void LobotSerialServoSetID(HardwareSerial& SerialX, uint8_t oldID, uint8_t newID) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = oldID;
    buf[3] = 4;
    buf[4] = BS_ID_WRITE;
    buf[5] = newID;
    buf[6] = LobotCheckSum(buf);
    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO ID WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

}

void LobotSerialServoSetMode(HardwareSerial& SerialX, uint8_t id, uint8_t Mode, int16_t Speed) {
    uint8_t buf[10];

    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 7;
    buf[4] = BS_OR_MOTOR_MODE_WRITE;
    buf[5] = Mode;
    buf[6] = 0;
    buf[7] = GET_LOW_BYTE((uint16_t)Speed);
    buf[8] = GET_HIGH_BYTE((uint16_t)Speed);
    buf[9] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO Set Mode");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    SerialX.write(buf, 10);
}

void LobotSerialServoLoad(HardwareSerial& SerialX, uint8_t id) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = BS_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 1;
    buf[6] = LobotCheckSum(buf);

    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO LOAD WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

}

void LobotSerialServoUnload(HardwareSerial& SerialX, uint8_t id) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 4;
    buf[4] = BS_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 0;
    buf[6] = LobotCheckSum(buf);

    SerialX.write(buf, 7);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO LOAD WRITE");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif
}


int LobotSerialServoReceiveHandle(HardwareSerial& SerialX, uint8_t* ret) {
    bool frameStarted = false;
    bool receiveFinished = false;
    uint8_t frameCount = 0;
    uint8_t dataCount = 0;
    uint8_t dataLength = 2;
    uint8_t rxBuf;
    uint8_t recvBuf[32];
    uint8_t i;

    while (SerialX.available()) {
        rxBuf = SerialX.read();
        delayMicroseconds(100);
        if (!frameStarted) {
            if (rxBuf == BS_FRAME_HEADER) {
                frameCount++;
                if (frameCount == 2) {
                    frameCount = 0;
                    frameStarted = true;
                    dataCount = 1;
                }
            }
            else {
                frameStarted = false;
                dataCount = 0;
                frameCount = 0;
            }
        }
        if (frameStarted) {
            recvBuf[dataCount] = (uint8_t)rxBuf;
            if (dataCount == 3) {
                dataLength = recvBuf[dataCount];
                if (dataLength < 3 || dataCount > 7) {
                    dataLength = 2;
                    frameStarted = false;
                }
            }
            dataCount++;
            if (dataCount == dataLength + 3) {

#ifdef LOBOT_DEBUG
                Serial.print("RECEIVE DATA:");
                for (i = 0; i < dataCount; i++) {
                    Serial.print(recvBuf[i], HEX);
                    Serial.print(":");
                }
                Serial.println(" ");
#endif

                if (LobotCheckSum(recvBuf) == recvBuf[dataCount - 1]) {

#ifdef LOBOT_DEBUG
                    Serial.println("Check SUM OK!!");
                    Serial.println("");
#endif

                    frameStarted = false;
                    memcpy(ret, recvBuf + 4, dataLength);
                    return 1;
                }
                return -1;
            }
        }
    }
}


int LobotSerialServoReadPosition(HardwareSerial& SerialX, uint8_t id) {
    int count = 10000;
    int ret;
    uint8_t buf[6];

    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = BS_POS_READ;
    buf[5] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO Pos READ");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    while (SerialX.available())
        SerialX.read();

    SerialX.write(buf, 6);

    while (!SerialX.available()) {
        count -= 1;
        if (count < 0)
            return -1;
    }

    if (LobotSerialServoReceiveHandle(SerialX, buf) > 0)
        ret = BYTE_TO_HW(buf[2], buf[1]);
    else
        ret = -1;

#ifdef LOBOT_DEBUG
    Serial.println(ret);
#endif
    return ret;
}
int LobotSerialServoReadVin(HardwareSerial& SerialX, uint8_t id) {
    int count = 10000;
    int ret;
    uint8_t buf[6];

    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = id;
    buf[3] = 3;
    buf[4] = BS_VIN_READ;
    buf[5] = LobotCheckSum(buf);

#ifdef LOBOT_DEBUG
    Serial.println("LOBOT SERVO VIN READ");
    int debug_value_i = 0;
    for (debug_value_i = 0; debug_value_i < buf[3] + 3; debug_value_i++) {
        Serial.print(buf[debug_value_i], HEX);
        Serial.print(":");
    }
    Serial.println(" ");
#endif

    while (SerialX.available())
        SerialX.read();

    SerialX.write(buf, 6);

    while (!SerialX.available()) {
        count -= 1;
        if (count < 0)
            return -2048;
    }

    if (LobotSerialServoReceiveHandle(SerialX, buf) > 0)
        ret = (int16_t)BYTE_TO_HW(buf[2], buf[1]);
    else
        ret = -2048;

#ifdef LOBOT_DEBUG
    Serial.println(ret);
#endif
    return ret;
}



void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(2, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);

    delay(1000);
}

#define ID1   1
#define ID2   2

#define KEY1  2
#define KEY2  3
void loop() {
  // put your main code here, to run repeatedly:
    static bool run = false;
    static char step = 0;
    static char mode = 0;
    static int pos[4] = { 100,200,300,400 };
    static int pos1[4] = { 100,200,300,400 };
    uint16_t temp;
    while (1) {
        if (mode == 0) {
            if (run) {
                LobotSerialServoMove(Serial, ID1, pos[step], 500);
                LobotSerialServoMove(Serial, ID2, pos1[step++], 500);
                if (step == 4) {
                    step = 0;
                    run = false;
                }
                delay(1000);
            }
            if (!digitalRead(KEY2)) {
                delay(10);
                if (!digitalRead(KEY2)) {
                    run = true;
                    step = 0;
                    delay(500);
                }
            }
            if (!digitalRead(KEY1)) {
                delay(10);
                if (!digitalRead(KEY1)) {
                    LobotSerialServoUnload(Serial, ID1);
                    LobotSerialServoUnload(Serial, ID2);
                    mode = 1;
                    step = 0;
                    delay(500);
                }
            }
        }
        if (mode == 1) {
            if (!digitalRead(KEY2)) {
                delay(10);
                if (!digitalRead(KEY2)) {
                    pos[step] = LobotSerialServoReadPosition(Serial, ID1);
                    pos1[step++] = LobotSerialServoReadPosition(Serial, ID2);
                    if (step == 4)
                        step = 0;
                    delay(500);
                }
            }
            if (!digitalRead(KEY1)) {
                delay(10);
                if (!digitalRead(KEY1)) {
                    temp = LobotSerialServoReadPosition(Serial, ID1);
                    LobotSerialServoMove(Serial, ID1, temp, 200);
                    temp = LobotSerialServoReadPosition(Serial, ID2);
                    LobotSerialServoMove(Serial, ID2, temp, 200);
                    mode = 0;
                    delay(500);
                }
            }
        }
    }
}
