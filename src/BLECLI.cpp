#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

//Declaration
extern void disp_val(String str);
extern void state_logo();
void ble_client_init();
void ble_client_handle();

// The remote service NAME we wish to connect to.
static const char* sens1_name = "ODB2-BLE-GATE";

//these UIDs are common to all sensors 
static BLEUUID serviceUUID("f0717496-ab77-4ebc-9705-eff9b74e5c39");
static BLEUUID    charUUID("916cc1fc-7111-48e7-a511-80b76a4df530");

static boolean doConnect1 = false; //запрос на коннект с сенсором #1
static boolean connected1 = false; //текущее состояние связи #1
static boolean doScan = false;    //запрос на сканирования BLE сети

//живыe обьекты после коннекта
static BLERemoteCharacteristic* pRemoteCharacteristic1;
static BLEAdvertisedDevice* myDevice1;

unsigned long previousMillis = 0;
unsigned long interval = 10000;  //10 sec. интервал между сканированием

//обработка события от характеристики, когда пришли данные с сервера BLE
static void notifyCallback1(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    //печатаем полученную строку
    String instr="";
    Serial.print("["+String(pData[length-5])+":"+String(pData[length-4])+"] ");
    Serial.print("Collant temperature=");
    for (int i = 0; i < length; i++) {
      //Serial.print((char)pData[i]);
      instr+=(char)pData[i];
    }
    Serial.print("size="+String(length)+" value=");
    Serial.print(instr); //контроль
    disp_val(instr);
}


//обработка события от нашего клиента1 BLE, connect-disconnect 1111111111111
class MyClientCallback1 : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }
  void onDisconnect(BLEClient* pclient) {  //сервер отпал..
    connected1 = false;
    state_logo(); //лого на дисплей
    Serial.println("Event-Disconnect1..");
    delay(100);
  }
};


//подключение к серверу ********************************************************
bool connectToServer1() {

    doConnect1 = false; //однократно !!!!!!
    Serial.println("-Start connection our device 1..");
    
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback1());//connect-disconnect events !!!!!

    // Connect to the remove BLE Server.(это блокирующий вызов)
    boolean  r_ok = pClient->connect(myDevice1/*sens1_address*/);// адрес !!!!!!!
    if(!r_ok) return false; 

    // ссылка на удаленный сервис
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);

    if (pRemoteService == nullptr) {
      Serial.print("-Failed to find our service1 UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false; //не смогли подключиться к сервису..
    }
    Serial.print("-Found our service1: ");
    Serial.println(serviceUUID.toString().c_str());

    // ссылка на удаленную характеристику..
    pRemoteCharacteristic1 = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic1 == nullptr) {
      Serial.print("-Failed to find our characteristic1 UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false; //не смогли подключиться к характеристике..
    }
    Serial.print("-Found our characteristic1: ");
    Serial.println(pRemoteCharacteristic1->getUUID().toString().c_str());

    // событие порождается сервером...
    if(pRemoteCharacteristic1->canNotify()) 
        pRemoteCharacteristic1->registerForNotify(notifyCallback1,false);//!!!!!!

    return true;
}


//----------------------------------------------------------------
// событие, когда сканер находит очередной BLE сенсор
// если это один из наших, для него дается команда на подключение 
// и сохраняется его адрес
//----------------------------------------------------------------
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  // событие вызывается для каждого сервера рекламирующего услуги..
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    
    Serial.print("Scanner: Name=");Serial.print(advertisedDevice.getName().c_str());
    Serial.print(" Address=");Serial.print(advertisedDevice.getAddress().toString().c_str());

      // проверяем имена наших сенсоров
      if(advertisedDevice.haveName() && advertisedDevice.getName()==sens1_name){
          myDevice1 = new BLEAdvertisedDevice(advertisedDevice); //class with address to connect
          doConnect1 = true; // команда: Connect
          Serial.println("  -Found..");
      } else {Serial.println();}
  } // onResult
}; // MyAdvertisedDeviceCallbacks

//--------------------------------------------------------------------------------------------------
//запускается после старта один раз в процедуре setup()
void ble_client_init(){

  BLEDevice::init("");

  //инициализируем процедуру сканирования сети  
  BLEScan* pBLEScan = BLEDevice::getScan(); //object singleton !
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);//интервал сканирования 1,3 сек
  pBLEScan->setWindow(449);//окно сканирования 449 мс
  pBLEScan->setActiveScan(true);

    doConnect1 = false;
    connected1 = false;
    doScan = true; //запрос на сканирование

    Serial.println("Go to Loop..");
}

//вызывается в цикле loop() =============================================================================
void ble_client_handle(){

  // периодически запускаем сканирование раз в [interval]
  // если есть хоть один неподключенный датчик
  unsigned long currentMillis = millis();   
  if (currentMillis - previousMillis >=interval) { //10 sec.
    if(!connected1 ){ //клиент отпал..
      doScan = true;
      Serial.println("Scanning Go..");
    }
    previousMillis = currentMillis; //10 sec.
  }

  //запрос - сканировать
  if(doScan){
    BLEDevice::getScan()->start(10/*0*/);//0-сканирует, пока принудительно не остановим.
    doScan = false; // однократно 
  }

  //получен зарос на CONNECT 1
  if (doConnect1) {
    if (connectToServer1()) { //подключение к серверу удачно
      Serial.println("We are now connected to the BLE Server1 - from loop()...");
      connected1 = true;
    } else {
      Serial.println("-Failed to connect our device 1 - from loop()...");
      connected1 = false;
    }
  }

/*
  //если на сервере включен цикл, то ничего посылать не требуется
  if (connected1) {
  // периодически посылаем команду серверу 1
    String newValue = "01 05";
    pRemoteCharacteristic1->writeValue(newValue.c_str(), newValue.length());
    delay(50);
  }
*/

}
