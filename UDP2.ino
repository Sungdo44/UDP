#include <Arduino.h>
#include "wiring_private.h"
#include <ModbusMaster.h>

// Serial
Uart LTEserial (&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2);
Uart modbus_iso (&sercom0, 3, 2, SERCOM_RX_PAD_3, UART_TX_PAD_2);

#define MAX485_DE 4
#define WORK_DE 5

ModbusMaster node;

uint32_t last1;
uint8_t result, qty, i;
uint16_t start_register;
String str;
String nid = "0001"; // Node ID 

// ฟังก์ชัน CMD
String CMD(String at) {
  String txt = "";
  LTEserial.println(at);
  delay(100);
  while (LTEserial.available()) {
    txt += (char)LTEserial.read();
  }
  Serial.println(txt);
  return txt;
}

// Helper Functions
void preTransmission() { digitalWrite(MAX485_DE, 1); }
void postTransmission() { digitalWrite(MAX485_DE, 0); }
void SERCOM0_Handler() { modbus_iso.IrqHandler(); }
void SERCOM3_Handler() { LTEserial.IrqHandler(); }

String GET_RSSI() {
  String msg = CMD("AT+CSQ");
  String r = msg.substring(msg.indexOf(" "), msg.indexOf(","));
  int rv = (r.toInt() * 2) - 113;
  if (rv == -113 || rv == 85) rv = -113;
  int myrssi = rv * -1;
  char rbuf[3];
  sprintf(rbuf, "%02x", myrssi);
  return String(rbuf);
}

void setup() {
  Serial.begin(9600);
  LTEserial.begin(115200);

  pinPeripheral(7, PIO_SERCOM_ALT);
  pinPeripheral(6, PIO_SERCOM_ALT);

  //ตั้งค่า Modbus 
  modbus_iso.begin(4800); 
  pinPeripheral(3, PIO_SERCOM);
  pinPeripheral(2, PIO_SERCOM);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);
  pinMode(WORK_DE, OUTPUT);
  digitalWrite(WORK_DE, LOW);

  // Hardware Setup Code 13 
  pinMode(A1, OUTPUT);    
  digitalWrite(A1, HIGH); 

  pinMode(A5, OUTPUT);    
  digitalWrite(A5, LOW);  

  pinMode(A6, OUTPUT);    
  digitalWrite(A6, HIGH); 
  
  Serial.println("Waiting for Module to Boot");
  delay(10000);  

  Serial.println("CHECKING SIGNAL");
  CMD("AT+CPIN?"); //เช็คซิมการ์ด
  CMD("AT+CSQ");   //เช็คความแรงสัญญาณ
  CMD("AT+CREG?"); //เช็คสถานะการเกาะเสา
  CMD("AT+CGATT?");//เช็คารเชื่อมต่อ GPS
  CMD("AT+COPS?");  //เช็คว่าเชื่อมต่อเครือข่ายไหนอยู๋

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("System Ready: Ultrasonic Weather Station + UDP Sending");
  delay(5000);


}

void loop() {
  if (millis() - last1 >= 300000) { 
    last1 = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(WORK_DE, HIGH);

    //อ่านค่า Sensor 
    node.begin(1, modbus_iso); 

    // ข้อมูลเริ่มที่ Address 500 (0x01F4)
    // 500 = Wind Speed
    // 501 = Winf Force
    // 502 = Wind Direction (0-7)
    // 503 = Wind Direction (360)
    // 504 = Humidity
    // 505 = Temp 
    // 506 = Noise
    // 507 = PM2.5
    // 508 = PM10
    // 509 = Pressure
    // 510 = Hi 16ibt lux
    // 511 = Lo 16bit lux
    // 512 = Light value
    // 513 = Rainfall
    // 514 = Compass
    // 515 = Radiation
    start_register = 0x01F4; // Address 500
    qty = 16; 
    
    str = "";
    str = nid;
    str += "03";       
    str += GET_RSSI(); 

    result = node.readHoldingRegisters(start_register, qty);
    
    if (result == node.ku8MBSuccess) {
      Serial.println("\n--- Sensor Read Success ---");
      for (i = 0; i < qty; i++) {
        char buf[5];
        sprintf(buf, "%04x", node.getResponseBuffer(i));
        str += String(buf);
      }
      Serial.print("Data: "); Serial.println(str);

      //ส่งข้อมูล UDP
      
      // เปิดเน็ต 
      CMD("AT+QIACT=1"); 
      
    
      //โครงสร้าง: AT+QIOPEN=context,socket_id,"UDP","IP",PORT,0,0
      //ใช้ตัวแปรจากไฟล์ secrets มาต่อ String
      String connectCmd = "AT+QIOPEN=1,0,\"UDP\",\"" + String(SECRET_SERVER_IP) + "\"," + String(SECRET_SERVER_PORT) + ",0,0";
      CMD(connectCmd);

      // ส่งข้อมูล 
      // โครงสร้าง: AT+QISENDEX=socket_id,"HEX_STRING"
      String sendCmd = "AT+QISENDEX=0,\"" + str + "\"";
      CMD(sendCmd);
      
      // ยืนยันการส่ง
      CMD("AT+QISEND=0,0"); 

      // ปิด Socket
      CMD("AT+QICLOSE=0");
      CMD("AT+QIDEACT=1"); 
      
      Serial.println(">> Sending Packet Finished");

    } else {
      Serial.print("Modbus Error: ");
      Serial.println(result);
    }

    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(WORK_DE, LOW);
  }
}
