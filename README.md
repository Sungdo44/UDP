# UDP
This project integrates code from previous labs (Code 11, 12, 13) to create a weather monitoring system and transmit data to a server via the UDP protocol
# Assignment: Weather Station UDP Sender

## Objective 
* นำโค้ดตั้งค่า Hardware (Code 13) มารวมกับโค้ดอ่านค่า Sensor (Code 11-12)
* ส่งข้อมูลขึ้น Server ผ่าน **UDP Protocol** โดยใช้โมดูล EC25

## Files
* **`UDP2.ino`** : โค้ดหลัก (Hardware Setup + Modbus Read + UDP Send)
* **`README.md`** : เอกสารอธิบายงาน

## Process 
1. **Hardware Init:** ตั้งค่า Pin A1, A5, A6 ใน `setup()` เพื่อเปิดใช้งานโมดูล EC25 และปิด Neoway
2. **Read Sensor:** อ่านค่า Modbus Address 500-515 (ลม, อุณหภูมิ, PM2.5) ทุกๆ 5 นาที
3. **Send UDP:** นำค่าที่อ่านได้แปลงเป็น Hex String และส่งไปยัง Server
    * **Target IP:** `165.22.61.85`
    * **Target Port:** `5254`
    * **Command:** ใช้ AT Command `AT+QIOPEN` และ `AT+QISENDEX`

## Requirements
* บอร์ด SAMD (Arduino Zero/M0)
* Library: `ModbusMaster`
