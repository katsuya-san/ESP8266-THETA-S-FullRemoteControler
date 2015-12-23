/******************************************************************************
*
*                     RICOH THETA S Remote Control Software 
*                              Full Control Edition
*                                      for
*                     Switch Science ESP-WROOM-02 Dev.board
* 
* Author  : Katsuya Yamamoto
* Version : see `sThetaRemoteVersion' value
* 
* It shows the minimum configuration of hardware below
*  - ESP-WROOM-02 Dev.board : https://www.switch-science.com/catalog/2500/
*  - I2C LCD Display        : https://www.switch-science.com/catalog/1405/
*  - 5Way Tactil Switch     : https://www.switch-science.com/catalog/979/
* 
* Pin connection
* 
*                  ESP-WROOM-02 Pin            I2C LCD Display
*                       3V3    ------------------    VCC
*                     IO  4    ------------------    SDA
*                     IO  5    ------------------    SCL
*                       GND    ------------------    GND
* 
*                  ESP-WROOM-02 Pin            5Way Tactil Switch
*                       3V3    ------------------    + (VCC)
*                     IO 14    ------------------    U (Up)
*                     IO  0    ------------------    C (Center)
*                     IO 12    ------------------    L (Left)
*                     IO 13    ------------------    D (Down)
*                     IO 16    ------------------    R (Right)
*                       GND    ------------------    - (GND)
* 
******************************************************************************/

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>  // Add JSON Library  https://github.com/bblanchon/ArduinoJson

const char sThetaRemoteVersion[] = "v01.03";    //Last Update 2015-12-23 : corresponding to "ESP8266Arduino Core version 2.0.0"
                                                // see http://esp8266.github.io/Arduino/versions/2.0.0/
//---  Pin definition ---
const int SW_C_Pin  = 0;

#define   SW_U_PIN    14
#define   SW_L_PIN    12
#define   SW_D_PIN    13
#define   SW_R_PIN    16      //can't use interrupt

const int I2cSdaPin = 4;
const int I2cSclPin = 5;

//--- LCD ---
#include <Wire.h>
#define I2C_ADDR 0x3e

uint8_t   cmd_clr[] = {0x01};
uint8_t   cmd_cr[] = {0xc0};

//--- 4way(U,D,L,R)　Switch Read  ---
#define   KEY_ID_UP             0
#define   KEY_ID_DOWN           1
#define   KEY_ID_LEFT           2
#define   KEY_ID_RIGHT          3
#define   KEY_ID_MUN            4

#define   PUSH_DETECT_LOCK_CNT  10
#define   LONG_PRESS_SS_1EV     20
int     aiSwPinNo[KEY_ID_MUN]     = { SW_U_PIN, SW_D_PIN, SW_L_PIN, SW_R_PIN };
int     aiSwLockCnt[KEY_ID_MUN]   = {        0,        0,        0,        0 };
int     aiSwLongPress[KEY_ID_MUN] = {        0,        0,        0,        0 };
int     aiSwStat[KEY_ID_MUN]      = {     HIGH,     HIGH,     HIGH,     HIGH };

//--- EEPROM ---
#define   EEPROM_SIZE           512
#define   EEPROM_THETA_SSID_TOP 0
#define   EEPROM_BOOTMODE_TOP   128

//--- Wi-Fi Connect ---
#define   THETA_SSID_BYTE_LEN   19
#define   THETA_PASS_OFFSET     7
#define   THETA_PASS_BYTE_LEN   8

char ssid[THETA_SSID_BYTE_LEN+1] = "THETAXS00000000.OSC";   //This initial value is not used.
char password[THETA_PASS_BYTE_LEN+1] = "00000000";          //This initial value is not used.

#define   WIFI_CONNECT_THETA    0
#define   WIFI_SCAN_THETA       1
int iConnectOrScan = WIFI_CONNECT_THETA;

#define   NEAR_RSSI_THRESHOLD   -45   //[db]

//--- HTTP  ---
// Use WiFiClient class to create TCP connections
WiFiClient client;

const char sHost[] = "192.168.1.1";
const int iHttpPort = 80;

#define   HTTP_TIMEOUT_DISABLE  0 // never times out during transfer. 
#define   HTTP_TIMEOUT_NORMAL   1
#define   HTTP_TIMEOUT_STATE    2
#define   HTTP_TIMEOUT_STARTCAP 5
#define   HTTP_TIMEOUT_STOPCAP  70
#define   HTTP_TIMEOUT_CMDSTAT  2

//--- THETA API URLs ---
const char  sUrlInfo[]        = "/osc/info" ;
const char  sUrlState[]       = "/osc/state" ;
const char  sUrlChkForUp[]    = "/osc/checkForUpdates" ;
const char  sUrlCmdExe[]      = "/osc/commands/execute" ;
const char  sUrlCmdStat[]     = "/osc/commands/status" ;

//--- THETA API param lists ---

#define   LIST_MAXSTRLEN_CAPMODE  7
#define   LIST_NUM_CAPMODE        2
char  sList_CaptureMode[LIST_NUM_CAPMODE][LIST_MAXSTRLEN_CAPMODE] = {
                "image", 
                "_video"
    };

#define   LIST_MAXSTRLEN_EXPPRG   2
#define   LIST_NUM_EXPPRG         4
char  sList_ExpProg[LIST_NUM_EXPPRG][LIST_MAXSTRLEN_EXPPRG] = {
                "2",  // AUTO : Normal program
                "4",  // SS   : Shutter priority
                "9",  // ISO  : ISO priority
                "1"   // MANU : Manual
    };

#define   LIST_MAXSTRLEN_EV       5
#define   LIST_NUM_EV             13
char  sList_Ev[LIST_NUM_EV][LIST_MAXSTRLEN_EV] = {
                "-2.0",
                "-1.7",
                "-1.3",
                "-1.0",
                "-0.7",
                "-0.3",
                "0.0",
                "0.3",
                "0.7",
                "1.0",
                "1.3",
                "1.7",
                "2.0"
    };

#define   LIST_MAXSTRLEN_ISO      5
#define   LIST_NUM_ISO            13
char  sList_ISO[LIST_NUM_ISO][LIST_MAXSTRLEN_ISO] = {
                "100",
                "125",
                "160",
                "200",
                "250",
                "320",
                "400",
                "500",
                "640",
                "800",
                "1000",
                "1250",
                "1600"
    };

#define   LIST_MAXSTRLEN_SS       11
#define   LIST_NUM_SS_SSMODE      30
#define   LIST_NUM_SS             55
char  sList_SS[LIST_NUM_SS][LIST_MAXSTRLEN_SS] = {
                "0.00015625", "0.0002", "0.00025",
                "0.0003125", "0.0004", "0.0005",
                "0.000625", "0.0008", "0.001",
                "0.00125", "0.0015625", "0.002",
                "0.0025", "0.003125", "0.004",
                "0.005", "0.00625", "0.008",
                "0.01", "0.0125", "0.01666666",
                "0.02", "0.025", "0.03333333",
                "0.04", "0.05", "0.06666666",
                "0.07692307", "0.1", "0.125",
                "0.16666666", "0.2", "0.25",
                "0.33333333", "0.4", "0.5",
                "0.625", "0.76923076", "1",
                "1.3", "1.6", "2",
                "2.5", "3.2", "4", 
                "5", "6", "8", 
                "10", "13", "15", 
                "20","25", "30", 
                "60"
    };
char  sList_SS_Disp[LIST_NUM_SS][LIST_MAXSTRLEN_SS] = {
                "6400", "5000", "4000",
                "3200", "2500", "2000",
                "1600", "1250", "1000",
                "800 ", "640 ", "500 ",
                "400 ", "320 ", "250 ",
                "200 ", "160 ", "125 ",
                "100 ", "80  ", "60  ",
                "50  ", "40  ", "30  ",
                "25  ", "20  ", "15  ",
                "13  ", "10  ", "8   ",
                "6   ", "5   ", "4   ",
                "3   ", "2.5 ", "2   ",
                "1.6 ", "1.3 ", "1\"  ",
                "1.3\"","1.6\"","2\"  ",
                "2.5\"","3.2\"","4\"  ", 
                "5\"  ","6\"  ","8\"  ", 
                "10\" ","13\" ","15\" ", 
                "20\" ","25\" ","30\" ", 
                "60\" "
    };

#define   LIST_MAXSTRLEN_WB       22
#define   LIST_NUM_WB             10
char  sList_WB[LIST_NUM_WB][LIST_MAXSTRLEN_WB] = {
                "auto",
                "daylight",
                "shade",
                "cloudy-daylight",
                "incandescent",
                "_warmWhiteFluorescent",
                "_dayLightFluorescent",
                "_dayWhiteFluorescent",
                "fluorescent", 
                "_bulbFluorescent"
    };
char  sList_WB_Disp[LIST_NUM_WB][5] = {
                "auto",
                "dayl",
                "shad",
                "clou",
                "lb 1",
                "lb 2",
                "fl D",
                "fl N",
                "fl W",
                "fl L"
    };


#define   LIST_MAXSTRLEN_OPT      16
#define   LIST_NUM_OPT            4
char  sList_Opt[LIST_NUM_OPT][LIST_MAXSTRLEN_OPT] = {
                "off",
                "DR Comp", 
                "Noise Reduction",
                "hdr"
    };
char  sList_Opt_Disp[LIST_NUM_OPT][4] = {
                "Off",
                "DR ", 
                "NR ",
                "HDR"
    };


//--- Internal Info  ---
#define   TAKE_PIC_STAT_DONE  0
#define   TAKE_PIC_STAT_BUSY  1

#define   INT_EXP_OFF         0
#define   INT_EXP_ON          1

#define   PUSH_CMD_CNT_INTEXP 3

#define   INT_EXP_STAT_STOP   0
#define   INT_EXP_STAT_RUN    1

#define   MOVE_STAT_STOP      0
#define   MOVE_STAT_REC       1

#define   STATUS_CHK_CYCLE    20
#define   CHK_BUSY_END        0     //BUSY
#define   CHK_CAPMODE         0     //NO BUSY
#define   CHK_MOVESTAT        10    //NO BUSY

int     iRelease          = 0;
int     iTakePicStat      = TAKE_PIC_STAT_DONE;
int     iIntExpStat       = INT_EXP_STAT_STOP;
int     iIntExpOnOff      = INT_EXP_OFF;          //For expansion
int     iMoveStat         = MOVE_STAT_STOP;
int     iRecordedTime     = 0;
int     iRecordableTime   = 0;
int     iStatChkCnt       = 0;
String  strTakePicLastId  = "0";
String  strSSID           = "SID_0000";
String  strCaptureMode    = "";
String  strExpProg        = "2";

//--- Dysplay ---
#define   DISP_TITLE_ERROR    -1
#define   DISP_TITLE_AUTO     0
#define   DISP_TITLE_SS       1
#define   DISP_TITLE_ISO      2
#define   DISP_TITLE_MANU     3
#define   DISP_TITLE_MOVE     4
#define   DISP_TITLE_VOL      5
#define   DISP_TITLE_BOOT     6

#define   DISP_MODE_EXP       0
#define   DISP_MODE_VOL       1
#define   DISP_MODE_BOOT      2

#define   CURSOR_POS_NUM      5
#define   CURSOR_POS_MIN      0
#define   CURSOR_POS_MAX      (CURSOR_POS_NUM - 1)

#define   INT_EXP_MIN_SEC     8
#define   INT_EXP_MAX_SEC     3600

#define   INT_EXP_MIN_NUM     0
#define   INT_EXP_MAX_NUM     9999

#define   BEEP_VOL_MIN        0
#define   BEEP_VOL_MAX        100

int     iDispMode         = DISP_MODE_EXP;
int     iExpCursor        = 0;                    // 0-4 (5 Pos)
int     iCurEv            = 6;
int     iCurISO           = 0;
int     iCurSS            = 14;
int     iCurWB            = 0;
int     iCurOpt           = 0;
int     iIntExpSec        = INT_EXP_MIN_SEC;       // 8-3600
int     iIntExpNum        = INT_EXP_MIN_NUM;       // 0:NoLimit or  2-9999
int     iBeepVol          = BEEP_VOL_MAX;          // 0-100; 

int     iIntExpSecCursorUpdate = 0;
int     iIntExpSecCursor       = 0;                // 0-3 (4 Pos)

#define INTEXPSEC_BLINKCYCLE     10
int     iIntExpSecBlinkCycle   = 0;
int     iIntExpSecBlink        = 0;

//--- BOOT MODE ---
//EEPROM_BOOTMODE_TOP
#define   BOOT_MODE_AUTO      DISP_TITLE_AUTO
#define   BOOT_MODE_SS        DISP_TITLE_SS
#define   BOOT_MODE_ISO       DISP_TITLE_ISO
#define   BOOT_MODE_MANU      DISP_TITLE_MANU
#define   BOOT_MODE_MOVE      DISP_TITLE_MOVE
#define   BOOT_MODE_NUM       5

int           iBootModeUpdate = 0;
unsigned char ucBootMode      = BOOT_MODE_AUTO;
unsigned char ucBootMode_tmp  = BOOT_MODE_AUTO;


//===============================
// User define function prototype
//===============================
void    INT_ReleaseSw(void);

void    KeyPolling(void);
void    KeyCtrl_Exp(void);

void    LCD_Command(uint8_t *cmd, size_t len);
void    LCD_write(uint8_t *cmd, size_t len);
void    LCD_Init(void);
void    DispConnectTheta(void);
void    DispSearchTheta(void);
int     GetDispTitleNo(void);
void    DispExp(void);

void    ReadBootMode(void);
void    SaveBootMode(void);
void    SendBootMode(void);

int     ConnectTHETA(void);
int     SearchAndEnterTHETA(void);
int     CheckThetaSsid( const char* pcSsid );
void    SaveThetaSsid(char* pcSsid);
void    ReadThetaSsid(char* pcSsid);
void    SetThetaSsidToPassword(char* pcSsid, char* pcPass);

String  SimpleHttpProtocol(const char* sPostGet, char* sUrl, char* sHost, int iPort, String strData, unsigned int uiTimeoutSec);

int     ThetaAPI_Get_Info(void);
int     ThetaAPI_Post_State(void);
int     ThetaAPI_Post_startSession(void);
int     ThetaAPI_Post_takePicture(void);
int     ThetaAPI_Post__startCapture(void);
int     ThetaAPI_Post__stopCapture(void);
int     ThetaAPI_Post_getOptions(void);
int     ThetaAPI_Post_commnads_status(void);

int     ThetaAPI_Res_setOptions(String strJson);
int     ThetaAPI_Post_setOptions_captureMode(char* psCapMode);
int     ThetaAPI_Post_setOptions_exposureProgram(char* psExpProg);
int     ThetaAPI_Post_setOptions_exposureCompensation(void);
int     ThetaAPI_Post_setOptions_iso(void);
int     ThetaAPI_Post_setOptions_shutterSpeed(void);
int     ThetaAPI_Post_setOptions_whiteBalance(void);
int     ThetaAPI_Post_setOptions__filter(void);
int     ThetaAPI_Post_setOptions__captureInterval(void);
int     ThetaAPI_Post_setOptions__captureNumber(void);
int     ThetaAPI_Post_setOptions__shutterVolume(void);



//===============================
// setup
//===============================
void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("");
  Serial.println("");
  Serial.println("-----------------------------------------");
  Serial.println("  RICOH THETA S Remote Control Software  ");
  Serial.println("           Full Control Edition          ");
  Serial.println("                   for                   ");
  Serial.println("  Switch Science ESP-WROOM-02 Dev.board  ");
  Serial.println("             FW Ver : " + String(sThetaRemoteVersion) );
  Serial.println("-----------------------------------------");
  Serial.println("");

  //I2C LCD setup
  LCD_Init();

  
  // initialize the pushbutton pin as an input:
  pinMode(SW_C_Pin, INPUT);
  attachInterrupt( SW_C_Pin, INT_ReleaseSw, FALLING);

  pinMode(aiSwPinNo[KEY_ID_UP   ], INPUT);
  pinMode(aiSwPinNo[KEY_ID_DOWN ], INPUT);
  pinMode(aiSwPinNo[KEY_ID_LEFT ], INPUT);
  pinMode(aiSwPinNo[KEY_ID_RIGHT], INPUT);

  // Define EEPROM SIZE
  EEPROM.begin(EEPROM_SIZE);    //Max 4096Byte

  //Read TEHTA SSID from EEPROM & check
  char SsidBuff[THETA_SSID_BYTE_LEN+1];
  ReadThetaSsid(SsidBuff);  
  if ( CheckThetaSsid( (const char*)SsidBuff ) == 1 ) {
    memcpy( ssid, SsidBuff, THETA_SSID_BYTE_LEN );
    SetThetaSsidToPassword(SsidBuff, password);
    Serial.println("");
    Serial.print("EEPROM SSID=");
    Serial.println(ssid);
    Serial.print("       PASS=");
    Serial.println(password);
  } else {
    iConnectOrScan = WIFI_SCAN_THETA;
  }

  //Read BootMode from EEPROM & check
  ReadBootMode();
  
}

//===============================
// main loop
//===============================
void loop() {
  
  if ( WiFi.status() != WL_CONNECTED ) {
    
    if (iConnectOrScan == WIFI_CONNECT_THETA) {
      
      DispConnectTheta();
      
      iConnectOrScan = ConnectTHETA();
      if ( iConnectOrScan == WIFI_CONNECT_THETA ) {
        delay(50);
        ThetaAPI_Get_Info();
        delay(50);
        ThetaAPI_Post_State();
        if ( strSSID.equals("SID_0000") ) {
          ThetaAPI_Post_startSession();
          if ( (iIntExpStat == INT_EXP_STAT_STOP) && (iMoveStat == MOVE_STAT_STOP) ) {
            SendBootMode();
          }
        }
        iRelease = 0;
      }
    } else {
      
      DispSearchTheta();
      
      if( SearchAndEnterTHETA() == 1 ) {
        ReadThetaSsid(ssid);  
        SetThetaSsidToPassword(ssid, password);
        iConnectOrScan = WIFI_CONNECT_THETA;
      } else {
        iConnectOrScan = WIFI_SCAN_THETA;
      }
    }
  } else {
    
    KeyPolling();
    if ( (iIntExpStat == INT_EXP_STAT_STOP) && (iMoveStat == MOVE_STAT_STOP) ) {
      KeyCtrl_Exp();
    }
    
    if ( iRelease == 1 ) {
      //ThetaAPI_Post_getOptions();             //A reaction becomes dull, so it's eliminated.
      
      if ( strCaptureMode.equals("image") )  {
        if ( iIntExpOnOff == INT_EXP_OFF ) {
          ThetaAPI_Post_takePicture();
        } else {
          if ( iDispMode == DISP_MODE_VOL ) {
            ThetaAPI_Post_takePicture();
          } else {
            if ( iIntExpStat == INT_EXP_STAT_STOP ) {
              ThetaAPI_Post__startCapture();
              iIntExpStat = INT_EXP_STAT_RUN;
            } else {
              ThetaAPI_Post__stopCapture();
              iIntExpStat = INT_EXP_STAT_STOP;
            }
          }
        }
      } else if ( strCaptureMode.equals("_video") ) {
        ThetaAPI_Post_State();
        if ( iMoveStat == MOVE_STAT_STOP ) {
          iMoveStat = MOVE_STAT_REC;
          ThetaAPI_Post__startCapture();
        } else {
          iMoveStat = MOVE_STAT_STOP;
          ThetaAPI_Post__stopCapture();
        }
      } else {
        Serial.println("captureMode failed : [" + strCaptureMode + "]");
      }
      
      delay(1000);                              //To avoid release of the fast cycle.
      iRelease = 0;
    } else {
      
      iStatChkCnt++;
      if (iStatChkCnt >= STATUS_CHK_CYCLE){
        iStatChkCnt=0;
      }
      if (iTakePicStat == TAKE_PIC_STAT_BUSY) {
        if ( iStatChkCnt == CHK_BUSY_END ) {
          ThetaAPI_Post_commnads_status();
        } else {
          delay(10);
        }
      } else {
        if ( iStatChkCnt == CHK_CAPMODE ) {
          ThetaAPI_Post_getOptions();           //When doing just before release, it's unnecessary.
        } else if ( iStatChkCnt == CHK_MOVESTAT ) {
          ThetaAPI_Post_State();
        } else {
          delay(10);
        }
      }
    }
    
    DispExp();
  }
}

//===============================
// User Define function
//===============================
//-------------------------------------------
// Interrupt handler
//-------------------------------------------
void INT_ReleaseSw(void)
{
  if ( iExpCursor == 4 ) {
    iIntExpSecCursorUpdate = 1;
  } else if ( iDispMode == DISP_MODE_BOOT ) {
    iBootModeUpdate = 1;
  } else {
    iRelease =1;
  }
}

//-------------------------------------------
// key Control
//-------------------------------------------
void    KeyPolling(void)
{
  for ( int iCnt=0; iCnt<KEY_ID_MUN; iCnt++ ) {
    if (digitalRead(aiSwPinNo[iCnt]) == LOW) {
      aiSwLongPress[iCnt]++;
      if (aiSwLockCnt[iCnt]==0) {
        aiSwLockCnt[iCnt] = PUSH_DETECT_LOCK_CNT;
        aiSwStat[iCnt] = LOW;
      } else {
        if ( aiSwLockCnt[iCnt] > 0 ) {
          aiSwLockCnt[iCnt]--;
        }
        aiSwStat[iCnt] = HIGH;
      }
    } else {
      aiSwLongPress[iCnt] = 0;
      if ( aiSwLockCnt[iCnt] > 0 ) {
        aiSwLockCnt[iCnt]--;
      }
      aiSwStat[iCnt] = HIGH;
    }
  }
  return;
}

void    KeyCtrl_Exp(void)
{
  int iTitleNo;

  //---- Update Interval Sec Cursor ----
  if ( iIntExpSecCursorUpdate == 1 ) {
    iIntExpSecCursorUpdate = 0;
    iIntExpSecCursor++;
    if ( iIntExpSecCursor > 4 ) {
      iIntExpSecCursor = 0;
    }
  }
  //---- Update Boot Mode ----
  if ( iBootModeUpdate == 1 ) {
    iBootModeUpdate = 0;
    SaveBootMode();
  }
  
  iTitleNo = GetDispTitleNo();

  //---- Change Parameter ----
  if (aiSwStat[KEY_ID_UP] == LOW) {
    Serial.println(" Key : Up   , LongCnt=" + String(aiSwLongPress[KEY_ID_UP]) );

    if ( (DISP_TITLE_AUTO <= iTitleNo) && (iTitleNo <= DISP_TITLE_MANU) ) {
      if ( iExpCursor == 0 ) {
        if     (  (iTitleNo == DISP_TITLE_AUTO)  ||
                  (iTitleNo == DISP_TITLE_SS  )  ||
                  (iTitleNo == DISP_TITLE_ISO )  ){
          //EV
          iCurEv++;
          if ( iCurEv >= LIST_NUM_EV ) {
            iCurEv = LIST_NUM_EV - 1;
          }
          ThetaAPI_Post_setOptions_exposureCompensation();
        } else {//iTitleNo == DISP_TITLE_MANU
          //SS
          if ( aiSwLongPress[KEY_ID_UP] < LONG_PRESS_SS_1EV ) {
            iCurSS++;
          } else {
            iCurSS+=3;
          }
          if ( iCurSS >= LIST_NUM_SS ) {
            iCurSS = LIST_NUM_SS - 1;
          }
          ThetaAPI_Post_setOptions_shutterSpeed();
        }
      } else if ( iExpCursor == 1 ) {   // 
        if        (iTitleNo == DISP_TITLE_AUTO) {
          //Opt
          iCurOpt++;
          if ( iCurOpt >= LIST_NUM_OPT ) {
            iCurOpt = LIST_NUM_OPT - 1;
          }
          ThetaAPI_Post_setOptions__filter();
        } else if (iTitleNo == DISP_TITLE_SS  ) {
          //SS
          if ( aiSwLongPress[KEY_ID_UP] < LONG_PRESS_SS_1EV ) {
            iCurSS++;
          } else {
            iCurSS+=3;
          }
          if ( iCurSS >= LIST_NUM_SS_SSMODE ) {
            iCurSS = LIST_NUM_SS_SSMODE - 1;
          }
          ThetaAPI_Post_setOptions_shutterSpeed();
        } else { //iTitleNo == DISP_TITLE_ISO || iTitleNo == DISP_TITLE_MANU
          //ISO
          iCurISO++;
          if ( iCurISO >= LIST_NUM_ISO ) {
            iCurISO = LIST_NUM_ISO - 1;
          }
          ThetaAPI_Post_setOptions_iso();
        }
      } else if ( iExpCursor == 2 ) {   // WB
        iCurWB++;
        if ( iCurWB >= LIST_NUM_WB ) {
          iCurWB = LIST_NUM_WB - 1;
        }
        ThetaAPI_Post_setOptions_whiteBalance();
      } else if ( iExpCursor == 3 ) {   // Interval On/Off
        iIntExpOnOff = INT_EXP_ON;
      } else if ( iExpCursor == 4 ) {   // Interval [sec]
        if        ( iIntExpSecCursor == 0 ) {
          iIntExpSec++;
        } else if ( iIntExpSecCursor == 1 ) {
          iIntExpSec+=10;
        } else if ( iIntExpSecCursor == 2 ) {
          iIntExpSec+=60;
        } else {  //iIntExpSecCursor == 3
          iIntExpSec+=600;
        }
        if ( iIntExpSec > INT_EXP_MAX_SEC ) {
          iIntExpSec = INT_EXP_MAX_SEC;
        }
        ThetaAPI_Post_setOptions__captureInterval();
      } else{
        // NOP
      }
    } else if ( iTitleNo == DISP_TITLE_MOVE ) {
      //NOP
    } else if ( iTitleNo == DISP_TITLE_VOL ) {
      //VOL
      iBeepVol+=10;
      if ( iBeepVol > BEEP_VOL_MAX ) {
        iBeepVol = BEEP_VOL_MAX;
      }
      ThetaAPI_Post_setOptions__shutterVolume();
    } else if ( iTitleNo == DISP_TITLE_BOOT ) {
      //BOOT MODE
      ucBootMode_tmp++;
      if ( ucBootMode_tmp > BOOT_MODE_MOVE ) {
        ucBootMode_tmp = BOOT_MODE_AUTO;
      }
    } else {
      // NOP 
    }
    
  }
  if (aiSwStat[KEY_ID_DOWN] == LOW) {
    Serial.println(" Key : Down , LongCnt=" + String(aiSwLongPress[KEY_ID_DOWN]) );
    
    if ( (DISP_TITLE_AUTO <= iTitleNo) && (iTitleNo <= DISP_TITLE_MANU) ) {
      if ( iExpCursor == 0 ) {
        if     (  (iTitleNo == DISP_TITLE_AUTO)  ||
                  (iTitleNo == DISP_TITLE_SS  )  ||
                  (iTitleNo == DISP_TITLE_ISO )  ){
          //EV
          iCurEv--;
          if ( iCurEv < 0 ) {
            iCurEv = 0;
          }
          ThetaAPI_Post_setOptions_exposureCompensation();
        } else {//iTitleNo == DISP_TITLE_MANU
          //SS
          if ( aiSwLongPress[KEY_ID_DOWN] < LONG_PRESS_SS_1EV ) {
            iCurSS--;
          } else {
            iCurSS-=3;
          }
          if ( iCurSS < 0 ) {
            iCurSS = 0;
          }
          ThetaAPI_Post_setOptions_shutterSpeed();
        }
      } else if ( iExpCursor == 1 ) {   // 
        if        (iTitleNo == DISP_TITLE_AUTO) {
          //Opt
          iCurOpt--;
          if ( iCurOpt < 0 ) {
            iCurOpt = 0;
          }
          ThetaAPI_Post_setOptions__filter();
        } else if (iTitleNo == DISP_TITLE_SS  ) {
          //SS
          if ( aiSwLongPress[KEY_ID_DOWN] < LONG_PRESS_SS_1EV ) {
            iCurSS--;
          } else {
            iCurSS-=3;
          }
          if ( iCurSS < 0 ) {
            iCurSS = 0;
          }
          ThetaAPI_Post_setOptions_shutterSpeed();
        } else { //iTitleNo == DISP_TITLE_ISO || iTitleNo == DISP_TITLE_MANU
          //ISO
          iCurISO--;
          if ( iCurISO < 0 ) {
            iCurISO = 0;
          }
          ThetaAPI_Post_setOptions_iso();
        }
      } else if ( iExpCursor == 2 ) {   // WB
        iCurWB--;
        if ( iCurWB < 0 ) {
          iCurWB = 0;
        }
        ThetaAPI_Post_setOptions_whiteBalance();
      } else if ( iExpCursor == 3 ) {   // Interval On/Off
        iIntExpOnOff = INT_EXP_OFF;
      } else if ( iExpCursor == 4 ) {   // Interval [sec]
        if        ( iIntExpSecCursor == 0 ) {
          iIntExpSec--;
        } else if ( iIntExpSecCursor == 1 ) {
          iIntExpSec-=10;
        } else if ( iIntExpSecCursor == 2 ) {
          iIntExpSec-=60;
        } else {  //iIntExpSecCursor == 3
          iIntExpSec-=600;
        }
        if ( iIntExpSec < INT_EXP_MIN_SEC ) {
          iIntExpSec = INT_EXP_MIN_SEC;
        }
        ThetaAPI_Post_setOptions__captureInterval();
      } else{
        // NOP
      }
    } else if ( iTitleNo == DISP_TITLE_MOVE ) {
      //NOP
    } else if ( iTitleNo == DISP_TITLE_VOL ) {
      //VOL
      iBeepVol-=10;
      if ( iBeepVol < BEEP_VOL_MIN ) {
        iBeepVol = BEEP_VOL_MIN;
      }
      ThetaAPI_Post_setOptions__shutterVolume();
    } else if ( iTitleNo == DISP_TITLE_BOOT ) {
      //BOOT MODE
      ucBootMode_tmp--;
      if ( ucBootMode_tmp == 0xFF ) {
        ucBootMode_tmp = BOOT_MODE_MOVE;
      }
    } else {
      // NOP 
    }
    
  }
  
  //---- Change Display ----
  if (aiSwStat[KEY_ID_LEFT] == LOW) {
    Serial.println(" Key : Left , LongCnt=" + String(aiSwLongPress[KEY_ID_LEFT]) );
    
    if ( (DISP_TITLE_AUTO <= iTitleNo) && (iTitleNo <= DISP_TITLE_MANU) ) {
      iExpCursor--;
      if ( iExpCursor < CURSOR_POS_MIN ) {
        iExpCursor = CURSOR_POS_MAX;
        iIntExpSecCursor = 0;
        if        ( iTitleNo == DISP_TITLE_AUTO   ) {
          iExpCursor = CURSOR_POS_MIN;
          iDispMode = DISP_MODE_BOOT;
        } else if ( iTitleNo == DISP_TITLE_SS   ) {
          strExpProg        = String(sList_ExpProg[DISP_TITLE_AUTO]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_AUTO] ); //to "AUTO"
        } else if ( iTitleNo == DISP_TITLE_ISO  ) {
          strExpProg        = String(sList_ExpProg[DISP_TITLE_SS]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_SS] );   //to "SS"
        } else {  //iTitleNo == DISP_TITLE_MANU
          strExpProg        = String(sList_ExpProg[DISP_TITLE_ISO]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_ISO] );  //to "ISO"
        }
      }
    } else if ( iTitleNo == DISP_TITLE_MOVE ) {
      iExpCursor = CURSOR_POS_MAX;
      iIntExpSecCursor = 0;
      strCaptureMode    = String(sList_CaptureMode[0]);
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                   //to "image"
      strExpProg        = String(sList_ExpProg[DISP_TITLE_MANU]);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_MANU] );     //to "MANU"
    } else if ( iTitleNo == DISP_TITLE_VOL ) {
      iExpCursor = CURSOR_POS_MIN;
      iDispMode = DISP_MODE_EXP;
      strCaptureMode    = String(sList_CaptureMode[1]);
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[1] );                   //to "_video"
    } else if ( iTitleNo == DISP_TITLE_BOOT ) {
      iExpCursor = CURSOR_POS_MIN;
      iDispMode = DISP_MODE_VOL;
    } else {
      // NOP 
    }
    
  }
  if (aiSwStat[KEY_ID_RIGHT]== LOW) {
    Serial.println(" Key : Right, LongCnt=" + String(aiSwLongPress[KEY_ID_RIGHT]) );
    
    if ( (DISP_TITLE_AUTO <= iTitleNo) && (iTitleNo <= DISP_TITLE_MANU) ) {
      iExpCursor++;
      if ( iExpCursor == CURSOR_POS_MAX ) {
        iIntExpSecCursor = 0;
      }
      if ( iExpCursor > CURSOR_POS_MAX ) {
        iExpCursor = CURSOR_POS_MIN;
        
        if        ( iTitleNo == DISP_TITLE_AUTO ) {
          strExpProg        = String(sList_ExpProg[DISP_TITLE_SS]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_SS] );   //to "SS"
        } else if ( iTitleNo == DISP_TITLE_SS   ) {
          strExpProg        = String(sList_ExpProg[DISP_TITLE_ISO]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_ISO] );  //to "ISO"
        } else if ( iTitleNo == DISP_TITLE_ISO  ) {
          strExpProg        = String(sList_ExpProg[DISP_TITLE_MANU]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_MANU] ); //to "MANU"
        } else {  //iTitleNo == DISP_TITLE_MANU
          strExpProg        = String(sList_ExpProg[DISP_TITLE_AUTO]);
          ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_AUTO] ); //to "AUTO"
          strCaptureMode    = String(sList_CaptureMode[1]);
          ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[1] );               //to "_video"
        }
      }
    } else if ( iTitleNo == DISP_TITLE_MOVE ) {
      strCaptureMode    = String(sList_CaptureMode[0]);
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                   //to "image"
      iExpCursor = CURSOR_POS_MIN;
      iDispMode = DISP_MODE_VOL;
    } else if ( iTitleNo == DISP_TITLE_VOL ) {
      iExpCursor = CURSOR_POS_MIN;
      iDispMode = DISP_MODE_BOOT;
    } else if ( iTitleNo == DISP_TITLE_BOOT ) {
      iExpCursor = CURSOR_POS_MIN;
      iDispMode = DISP_MODE_EXP;
    } else {
      // NOP 
    }
    
  }
  
  
  return ;
}


//-------------------------------------------
// Dysplay functions
//-------------------------------------------
void LCD_Command(uint8_t *cmd, size_t len)
{
  size_t i;
  for (i=0; i<len; i++) {
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0x00);
    Wire.write(cmd[i]);
    Wire.endTransmission();
    delayMicroseconds(27);    // 26.3us
  }

  return;
}
void LCD_write(uint8_t *cmd, size_t len)
{
  size_t i;
  for (i=0; i<len; i++) {
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(0x40);
    Wire.write(cmd[i]);
    Wire.endTransmission();
    delayMicroseconds(27);    // 26.3us
  }

  return;
}
void LCD_Init(void)
{
  Wire.begin(I2cSdaPin, I2cSclPin);
  delay(40);
  //uint8_t cmd_init[] = {0x38, 0x39, 0x14, 0x70, 0x56, 0x6c, 0x38, 0x0d, 0x01};  // Cursor & Blink On
  //uint8_t cmd_init[] = {0x38, 0x39, 0x14, 0x70, 0x56, 0x6c, 0x38, 0x0C, 0x01};  // Cursor & Blink Off
  uint8_t cmd_init[] = {0x38, 0x39, 0x14, 0x72, 0x56, 0x6c, 0x38, 0x0C, 0x01};  // Cursor & Blink Off
  LCD_Command(cmd_init, sizeof(cmd_init));
  delayMicroseconds(1080);  // 1.08ms

  return;
}

void DispConnectTheta(void)
{
  char      sLine1[9] = { "Wait for"};
  char      sLine2[9] = { " connect"};
  uint8_t   cmd_TopPos[]    = {0x80};
  
  //LCD_Command(cmd_clr, sizeof(cmd_clr));
  LCD_Command(cmd_TopPos, sizeof(cmd_TopPos));
  delayMicroseconds(1080);  // 1.08ms
  
  LCD_write( (uint8_t*)&sLine1, 8);
  LCD_Command(cmd_cr, sizeof(cmd_cr));
  LCD_write( (uint8_t*)&sLine2, 8);
  
  return;  
}
void DispSearchTheta(void)
{
  char      sLine1[9] = { " Search "};
  char      sLine2[9] = { "THETA S "};
  uint8_t   cmd_TopPos[]    = {0x80};
  
  //LCD_Command(cmd_clr, sizeof(cmd_clr));
  LCD_Command(cmd_TopPos, sizeof(cmd_TopPos));
  delayMicroseconds(1080);  // 1.08ms
  
  LCD_write( (uint8_t*)&sLine1, 8);
  LCD_Command(cmd_cr, sizeof(cmd_cr));
  LCD_write( (uint8_t*)&sLine2, 8);
  
  return;  
}

int   GetDispTitleNo(void)
{
  int iDispTitleNo=0;
  
  if ( iDispMode == DISP_MODE_EXP ) {
    if ( strCaptureMode.equals("image") ) {
      if        ( strExpProg.equals("2") ) {      // AUTO
        iDispTitleNo = DISP_TITLE_AUTO;
      } else if ( strExpProg.equals("4") ) {      // SS
        iDispTitleNo = DISP_TITLE_SS;
      } else if ( strExpProg.equals("9") ) {      //ISO
        iDispTitleNo = DISP_TITLE_ISO;
      } else if ( strExpProg.equals("1") ) {      //MANU
        iDispTitleNo = DISP_TITLE_MANU;
      } else {
        iDispTitleNo = DISP_TITLE_ERROR;
      }
    } else if (strCaptureMode.equals("_video")) {
      iDispTitleNo = DISP_TITLE_MOVE;
    } else {
      iDispTitleNo = DISP_TITLE_ERROR;
    }
  } else if ( iDispMode == DISP_MODE_VOL ) {
    iDispTitleNo = DISP_TITLE_VOL;
  } else if ( iDispMode == DISP_MODE_BOOT ) {
    iDispTitleNo = DISP_TITLE_BOOT;
  } else {
    iDispTitleNo = DISP_TITLE_ERROR;
  }

  return iDispTitleNo;
}

void  DispExp(void)
{  
  char      sTitle[8][9]    = { "AUTO    ",
                                "Tv      ",
                                "ISO     ",
                                "MANU    ",
                                "MOVE    ",
                                "SET VOL ",
                                "SET BOOT",
                                "        "};
  char      sBusy[2]        = {"!"};
  char      sIntOn[2]       = {"I"};
  char      sRec[4]         = {"REC"};
  char      sIntDisp[2][9]  = {"Int Off ", "Int On  "};
  char      sBootModeBuf[5] = {"----"};
  uint8_t   cmd_TopPos[]    = {0x80};
  uint8_t   cmd_BusyPos[]   = {0x85};
  uint8_t   cmd_IntPos[]    = {0x87};

    
  //LCD_Command(cmd_clr, sizeof(cmd_clr));
  LCD_Command(cmd_TopPos, sizeof(cmd_TopPos));
  delayMicroseconds(1080);  // 1.08ms

  //---- Line 1 Disp ----
  int iTitleNo = GetDispTitleNo();
  if ( 0<=iTitleNo && iTitleNo<=6 ){
    LCD_write( (uint8_t*)sTitle[iTitleNo], strlen(sTitle[iTitleNo]) );
  } else {
    //error!
    LCD_write( (uint8_t*)sTitle[7], strlen(sTitle[7]) );
  }
  
  if ( iDispMode == DISP_MODE_EXP ) {
    //-- Busy ---
    LCD_Command(cmd_BusyPos, sizeof(cmd_BusyPos));
    delayMicroseconds(1080);  // 1.08ms
    if ( (iTakePicStat == TAKE_PIC_STAT_BUSY) || (iIntExpStat == INT_EXP_STAT_RUN) ) {
      LCD_write( (uint8_t*)&sBusy, 1);
    } else if( iMoveStat == MOVE_STAT_REC ) {
      LCD_write( (uint8_t*)&sRec, 3);
    }
    
    //-- Interval On/Off ---
    if ( iTitleNo != DISP_TITLE_MOVE ) {
      LCD_Command(cmd_IntPos, sizeof(cmd_IntPos));
      delayMicroseconds(1080);  // 1.08ms
      if ( iIntExpOnOff == INT_EXP_ON ) {
        LCD_write( (uint8_t*)&sIntOn, 1);
      }
    }
  }

  //---- CR ----
  LCD_Command(cmd_cr, sizeof(cmd_cr));

  //---- Line 2 Disp ----
  char sLine2Buf[16];
  
  switch( iTitleNo ) {
    case DISP_TITLE_AUTO:
    case DISP_TITLE_SS:
    case DISP_TITLE_ISO:
    case DISP_TITLE_MANU:
      if ( iExpCursor == 0 ) {
        if (  ( iTitleNo == DISP_TITLE_AUTO ) ||     // AUTO
              ( iTitleNo == DISP_TITLE_SS   ) ||     // SS
              ( iTitleNo == DISP_TITLE_ISO  ) ){     // ISO
          if ( sList_Ev[iCurEv][0] == '-' ) {
            sprintf(sLine2Buf, "EV %s  \0", sList_Ev[iCurEv]);                        // "EV  ****"
          } else {
            sprintf(sLine2Buf, "EV  %s \0", sList_Ev[iCurEv]);
          }
        } else if ( iTitleNo == DISP_TITLE_MANU ) {  //MANU
          sprintf(sLine2Buf, "Tv  %s    \0", sList_SS_Disp[iCurSS]);                  // "SS  ****"
        } else {
          sprintf(sLine2Buf, "        \0");
        }
      } else if ( iExpCursor == 1 ) {
        if        (  iTitleNo == DISP_TITLE_AUTO  ) {      // AUTO
          sprintf(sLine2Buf, "Opt %s    \0", sList_Opt_Disp[iCurOpt]);                // "Opt ****"
        } else if (  iTitleNo == DISP_TITLE_SS    ) {      // SS
          sprintf(sLine2Buf, "Tv  %s    \0", sList_SS_Disp[iCurSS]);                  // "SS  ****"
        } else if ( (iTitleNo == DISP_TITLE_ISO ) ||       // ISO
                    (iTitleNo == DISP_TITLE_MANU) ){       // MANU
          sprintf(sLine2Buf, "ISO %s    \0", sList_ISO[iCurISO]);                     // "ISO ****"
        } else {
          sprintf(sLine2Buf, "        \0");
        }
      } else if ( iExpCursor == 2 ) {
        sprintf(sLine2Buf, "WB  %s\0", sList_WB_Disp[iCurWB]);                        // "WB  ****"
      } else if ( iExpCursor == 3 ) {
        //IntExp On/Off
        sprintf(sLine2Buf, "%s\0", sIntDisp[iIntExpOnOff]);                           // "Int *** "
      } else if ( iExpCursor == 4 ) {
        //IntExp Interval Time
        int iIntTimeMin; 
        int iIntTimeSec; 
        iIntTimeMin = iIntExpSec/60;
        iIntTimeSec = iIntExpSec%60;
        sprintf(sLine2Buf, "Int%02d:%02d\0", iIntTimeMin, iIntTimeSec);               // "Int00:00"
        if ( iIntExpSecBlink == 1 ) {
          if        ( iIntExpSecCursor == 0 ) {
            sLine2Buf[7] = ' ';
          } else if ( iIntExpSecCursor == 1 ) {
            sLine2Buf[6] = ' ';
          } else if ( iIntExpSecCursor == 2 ) {
            sLine2Buf[4] = ' ';
          } else {  //iIntExpSecCursor == 3
            sLine2Buf[3] = ' ';
          }
        }
        iIntExpSecBlinkCycle++;
        if ( iIntExpSecBlinkCycle == INTEXPSEC_BLINKCYCLE ) {
          iIntExpSecBlinkCycle=0;
          if ( iIntExpSecBlink == 0 ) {
            iIntExpSecBlink = 1;
          } else {
            iIntExpSecBlink = 0;
          }
        }
      } else {
        sprintf(sLine2Buf, "        \0");
      }
      break;
    case DISP_TITLE_MOVE:
      int iRecordedMin; 
      int iRecordedSec; 
      iRecordedMin = iRecordedTime/60;
      iRecordedSec = iRecordedTime%60;
      sprintf(sLine2Buf, " %02d:%02d  \0", iRecordedMin, iRecordedSec);                 // " 00:00  "
      break;
    case DISP_TITLE_VOL:
      sprintf(sLine2Buf,"Vol:%3d \0", iBeepVol);                                        // "Vol:*** "
      break;
    case DISP_TITLE_BOOT:
      char cBootModeChg;
      if (ucBootMode_tmp!=ucBootMode) {
        cBootModeChg = '*';
      } else {
        cBootModeChg = ' ';
      }
      memcpy( sBootModeBuf, sTitle[ucBootMode_tmp], 4);
      sprintf(sLine2Buf,"%c[%s] \0", cBootModeChg, sBootModeBuf);                       // " [****] "
      break;
    default :
      sprintf(sLine2Buf, "        \0");
      break;
  }
  LCD_write( (uint8_t*)sLine2Buf, 8 );
  
  return;
}

//-------------------------------------------
// Boot Mode functions
//-------------------------------------------
void    ReadBootMode(void)
{
  ucBootMode = EEPROM.read(EEPROM_BOOTMODE_TOP);
  if ( ucBootMode >= BOOT_MODE_NUM ) {
    ucBootMode = BOOT_MODE_AUTO;
  }
  ucBootMode_tmp = ucBootMode;
}

void    SaveBootMode(void)
{
  if ( ucBootMode != ucBootMode_tmp ) {
    EEPROM.write( EEPROM_BOOTMODE_TOP, ucBootMode_tmp );
    EEPROM.commit();
    delay(100);
    ucBootMode = ucBootMode_tmp;
  }
}

void    SendBootMode(void)
{
  iDispMode  = DISP_MODE_EXP;
  iExpCursor = 0;
  
  switch(ucBootMode) {
    case BOOT_MODE_AUTO :
      strCaptureMode = String(sList_CaptureMode[0]);
      strExpProg = String(sList_ExpProg[DISP_TITLE_AUTO]);
      
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                 //to "image"
      delay(10);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_AUTO] );   //to "AUTO"
      break;
    case BOOT_MODE_SS :
      strCaptureMode = String(sList_CaptureMode[0]);
      strExpProg = String(sList_ExpProg[DISP_TITLE_SS]);
      
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                 //to "image"
      delay(10);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_SS] );     //to "SS"
      break;
    case BOOT_MODE_ISO :
      strCaptureMode = String(sList_CaptureMode[0]);
      strExpProg = String(sList_ExpProg[DISP_TITLE_ISO]);
      
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                 //to "image"
      delay(10);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_ISO] );    //to "ISO"
      break;
    case BOOT_MODE_MANU :
      strCaptureMode = String(sList_CaptureMode[0]);
      strExpProg = String(sList_ExpProg[DISP_TITLE_MANU]);
      
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[0] );                 //to "image"
      delay(10);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_MANU] );   //to "MANU"
      break;
    case BOOT_MODE_MOVE :
      strCaptureMode = String(sList_CaptureMode[1]);
      strExpProg = String(sList_ExpProg[DISP_TITLE_AUTO]);
      
      ThetaAPI_Post_setOptions_captureMode( sList_CaptureMode[1] );                 //to "_video"
      delay(10);
      ThetaAPI_Post_setOptions_exposureProgram( sList_ExpProg[DISP_TITLE_AUTO] );   //to "AUTO"
      break;
    default :
      //NOP
      break;
  }
}

//-------------------------------------------
// Wi-Fi Connect functions
//-------------------------------------------
int ConnectTHETA(void)
{
  int iRet = WIFI_CONNECT_THETA;
  int iButtonState = 0;
  int iButtonCnt = 0;
  int iPushCnt = 0;

  Serial.println("");
  Serial.println("WiFi disconnected");
  iRelease = 0;

  iButtonCnt=0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    iButtonState = digitalRead(SW_C_Pin);
    if ( iButtonState == LOW ) {
      iButtonCnt++;
      if ( iButtonCnt >= 10 ) {
        iRet=WIFI_SCAN_THETA;
        break;
      }
    } else {
      iButtonCnt=0;
    }
    if ( iRelease == 1 ) {
       iRelease = 0;
       iPushCnt++;
    }
    delay(500);
  }

  if ( iRet != WIFI_SCAN_THETA ) {
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    /*
    Serial.println("");
    if ( iPushCnt >= PUSH_CMD_CNT_INTEXP ) {
      iIntExpOnOff = INT_EXP_ON;
      Serial.println("1 Period Interval Exp ON ! : PushCnt=" + String(iPushCnt));
    } else {
      iIntExpOnOff = INT_EXP_OFF;
      Serial.println("1 Period Interval Exp OFF  : PushCnt=" + String(iPushCnt));
    }
    Serial.println("");
    */
  }
  
  return iRet;
}
//----------------
int SearchAndEnterTHETA(void)
{
  int iRet = 0;
  int iThetaCnt=0;
  int aiSsidPosList[20];
  char ssidbuf[256];
  
  Serial.println("");
  Serial.println("Search THETA");
  
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") ");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");

      if( WiFi.RSSI(i) >= NEAR_RSSI_THRESHOLD ) {
        WiFi.SSID(i).toCharArray(ssidbuf, sizeof(ssidbuf));
        if ( CheckThetaSsid(ssidbuf) == 1 ) {
          aiSsidPosList[iThetaCnt]=i;
          iThetaCnt++;
        }
      }
      delay(10);
    }
    
    if (iThetaCnt>0) {
      iRet = 1;
      
      //Select Max RSSI THETA
      int iRssiMaxPos=0;
      for (int i = 0; i <iThetaCnt; i++) {
        if( WiFi.RSSI(aiSsidPosList[iRssiMaxPos]) < WiFi.RSSI(aiSsidPosList[i]) ) {
          iRssiMaxPos = i;
        }
      }
      //Enter THETA SSID to EEPROM
      WiFi.SSID(aiSsidPosList[iRssiMaxPos]).toCharArray(ssidbuf, sizeof(ssidbuf));
      SaveThetaSsid(ssidbuf);
      Serial.println("");
      Serial.println("--- Detected TEHTA ---");
      Serial.print("SSID=");
      Serial.print(WiFi.SSID(aiSsidPosList[iRssiMaxPos]));
      Serial.print(", RSSI=");
      Serial.print(WiFi.RSSI(aiSsidPosList[iRssiMaxPos]));
      Serial.println("[db]");
    }
  }
  Serial.println("");
  
  return iRet;
}
//----------------
int CheckThetaSsid( const char* pcSsid )
{
  int iRet=1;

  String strSsid = String(pcSsid);
  if ( strSsid.length() == THETA_SSID_BYTE_LEN ) {
    if ( ( strSsid.startsWith("THETAXS") == true ) &&
         ( strSsid.endsWith(".OSC") == true )      ){
      //Serial.print("pass=");
      for (int j=THETA_PASS_OFFSET; j<(THETA_PASS_OFFSET+THETA_PASS_BYTE_LEN); j++) {
        //Serial.print( *( pcSsid + j ) );
        if (  (*( pcSsid + j )<0x30) || (0x39<*( pcSsid + j ))  ) {
          iRet=0;
        }
      }
      Serial.println("");
    } else {
      iRet = 0;
    }
  } else {
    iRet = 0;
  }

  return iRet;  
}
//----------------
void SaveThetaSsid(char* pcSsid)
{
  for (int iCnt=0; iCnt<THETA_SSID_BYTE_LEN; iCnt++) {
     EEPROM.write( (EEPROM_THETA_SSID_TOP + iCnt), *(pcSsid+iCnt) );
  }
  EEPROM.commit();
  delay(100);
  
  return ;
}
//----------------
void ReadThetaSsid(char* pcSsid)
{
  for (int iCnt=0; iCnt<THETA_SSID_BYTE_LEN; iCnt++) {
    //Read EEPROM
    *(pcSsid+iCnt) = EEPROM.read(EEPROM_THETA_SSID_TOP + iCnt);
  }
  *(pcSsid+THETA_SSID_BYTE_LEN) = 0x00;

  return ;
}
//----------------
void SetThetaSsidToPassword(char* pcSsid, char* pcPass)
{
  for (int iCnt=0; iCnt<THETA_PASS_BYTE_LEN; iCnt++) {
    //Read EEPROM
    *(pcPass+iCnt) = *(pcSsid+THETA_PASS_OFFSET+iCnt);
  }
  *(pcPass+THETA_PASS_BYTE_LEN) = 0x00;
  
  return ;
}

//-------------------------------------------
// HTTP protocol functions
//-------------------------------------------
String SimpleHttpProtocol(const char* sPostGet, char* sUrl, char* sHost, int iPort, String strData, unsigned int uiTimeoutSec)
{
  unsigned long ulStartMs;
  unsigned long ulCurMs;
  unsigned long ulElapsedMs;
  unsigned long ulTimeoutMs;
  int           iDelimiter=0;
  int           iCheckLen=0;
  int           iResponse=0;
  String        strJson = String("") ;

  ulTimeoutMs = (unsigned long)uiTimeoutSec * 1000;
  client.flush();   //clear response buffer

  //Use WiFiClient class to create TCP connections
  if (!client.connect(sHost, iHttpPort)) {
    Serial.println("connection failed");

    delay(1000);
    WiFi.disconnect();
    return strJson;
  }
  
  //Send Request
  client.print(String(sPostGet) + " " + sUrl + " HTTP/1.1\r\n" +
               "Host: " + String(sHost) + ":" + String(iHttpPort)  + "\r\n" + 
               "Content-Type: application/json;charset=utf-8\r\n" + 
               "Accept: application/json\r\n" + 
               "Content-Length: " + String(strData.length()) + "\r\n" + 
               "Connection: close\r\n" + 
               "\r\n");
  if ( strData.length() != 0 ) {
    client.print(strData + "\r\n");
  }
  
  //Read Response
  ulStartMs = millis();
  while(1){
    delay(10);

    String line;
    while(client.available()){
      iResponse=1;
      #if 0  //-----------------------------------------------------
      line = client.readStringUntil('\r'); //This function is a defect.
      #else  //-----------------------------------------------------
      //Serial.println("res size=" + String(client.available()));
      char sBuf[client.available()];
      int iPos=0;
      while (client.available()) {
        char c = client.read();
        if (  c == '\r' ) {
          break;
        } else {
          sBuf[iPos] = c;
          iPos++;
        }
      }
      sBuf[iPos] = 0x00;
      line = String(sBuf);
      #endif //-----------------------------------------------------
      line.trim() ;
      //Serial.println("[" + line + "]");
      
      if (iDelimiter==1) {
        strJson = line;
      }
      if (line.length() == 0) {
        iDelimiter = 1;
      }
      if (line.indexOf("Content-Length:") >=0 ) {
        line.replace("Content-Length:","");
        if ( line.length() < 11 ) {
          char sBuff[11];
          line.toCharArray(sBuff, line.length()+1);
          iCheckLen = atoi(sBuff);
        } else {
          iCheckLen = -1;
        }
        //Serial.println("len=[" + String(iCheckLen) + "]");
      }
    }
    
    ulCurMs = millis();
    if ( ulCurMs > ulStartMs ) {
      ulElapsedMs = ulCurMs - ulStartMs;
    } else {
      ulElapsedMs = (4294967295 - ulStartMs) + ulCurMs + 1 ;
    }
    if ( (ulTimeoutMs!=0) && (ulElapsedMs>ulTimeoutMs) ) {
      break;
    }
    if ( (iResponse==1) && (strJson.length()!=0) ) {
      break;
    }
  }
  
  //Check Response
  if ( iCheckLen != strJson.length() ) {
    Serial.println("receive error : JSON[" + strJson + "], Content-Length:" + String(iCheckLen));
    strJson = String("");
  }
  //Serial.println( "time=" + String(ulElapsedMs) + "[ms]");

  return strJson;
}

//-------------------------------------------
// THETA API functions
//-------------------------------------------
int ThetaAPI_Get_Info(void)
{
  int iRet=0;
  
  String strSendData = String("");
  String strJson = SimpleHttpProtocol("GET", (char*)sUrlInfo, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<350> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Get_Info() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sModel = root["model"];
      const char* sSN    = root["serialNumber"];
      const char* sFwVer = root["firmwareVersion"];
      Serial.println("ThetaAPI_Get_Info() : Model[" + String(sModel) + "], S/N[" + String(sSN) + "], FW Ver[" + String(sFwVer) + "]");
      iRet=1;
    }
  }
  return iRet;
}
//----------------
int ThetaAPI_Post_State(void)
{
  int iRet=0;
  
  String strSendData = String("");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlState, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_STATE );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<350> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_State() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sessionId       = root["state"]["sessionId"];
      const char* batteryLevel    = root["state"]["batteryLevel"];
      const char* _captureStatus  = root["state"]["_captureStatus"];
      const char* _recordedTime   = root["state"]["_recordedTime"];
      const char* _recordableTime = root["state"]["_recordableTime"];
      Serial.println("ThetaAPI_Post_State() : sessionId[" + String(sessionId) + "], batteryLevel[" + String(batteryLevel) +
                      "], _captureStatus[" + String(_captureStatus) + 
                      "], _recordedTime[" + String(_recordedTime) + "], _recordableTime[" + String(_recordableTime) + "]");
      
      strSSID = String(sessionId);
      
      String strCaptureStatus  = String(_captureStatus);
      iRecordedTime = atoi(_recordedTime);
      iRecordableTime = atoi(_recordableTime);
      
      if ( strCaptureStatus.equals("idle") ) {
        iMoveStat = MOVE_STAT_STOP;
        iIntExpStat = INT_EXP_STAT_STOP;
      } else {
        if ( (iRecordedTime==0) && (iRecordableTime==0) ) {
          iMoveStat = MOVE_STAT_STOP;
          iIntExpOnOff= INT_EXP_ON;
          iIntExpStat = INT_EXP_STAT_RUN ;
        } else {
          iMoveStat = MOVE_STAT_REC;
          iIntExpStat = INT_EXP_STAT_STOP;
        }
      }
      iRet=1;
    }
  }
  return iRet;
}
//----------------
int ThetaAPI_Post_startSession(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.startSession\",  \"parameters\": {\"timeout\": 180}}");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_startSession() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sSessionId  = root["results"]["sessionId"];
      strSSID = String(sSessionId);
      Serial.println("ThetaAPI_Post_startSession() : sessionId[" + strSSID + "]" );
      iRet=1;
    }
  }
  return iRet;
}
//----------------
int ThetaAPI_Post_takePicture(void)
{
  int iRet=0;
  
  iTakePicStat = TAKE_PIC_STAT_BUSY;
  
  String strSendData = String("{\"name\": \"camera.takePicture\", \"parameters\": { \"sessionId\": \"" + strSSID + "\" } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_takePicture() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      Serial.print("ThetaAPI_Post_takePicture() : state[" + strState + "], " );
      if ( strState.equals("error") ) {
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println("Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      } else {  //inProgress
        const char* sId = root["id"];
        strTakePicLastId = String(sId);
        Serial.println("id[" + strTakePicLastId + "]");
        iRet=1;
      }
    }
  }
  return iRet;
}
//----------------
int ThetaAPI_Post__startCapture(void)
{
  int iRet=0;

  String strSendData = String("{\"name\": \"camera._startCapture\", \"parameters\": { \"sessionId\": \"" + strSSID + "\" } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_STARTCAP );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post__startCapture() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      Serial.print("ThetaAPI_Post__startCapture() : state[" + strState + "]" );
      if ( strState.equals("error") ) {
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println(", Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      } else {  //done
        Serial.println("");
        iRet=1;
      }
    }
  }
  
  return iRet;
}
//----------------
int ThetaAPI_Post__stopCapture(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera._stopCapture\", \"parameters\": { \"sessionId\": \"" + strSSID + "\" } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_STOPCAP );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post__stopCapture() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      Serial.print("ThetaAPI_Post__stopCapture() : state[" + strState + "]" );
      if ( strState.equals("error") ) {
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println(", Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      } else {  //done
        Serial.println("");
        iRet=1;
      }
    }
  }
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_getOptions(void)
{
  int iRet=0;
  
  //String strSendData = String("{\"name\": \"camera.getOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"optionNames\": [\"captureMode\"] } }");
  String strSendData = String("{\"name\": \"camera.getOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"optionNames\":[\"captureMode\",\"exposureProgram\",\"exposureCompensation\",\"iso\",\"shutterSpeed\",\"whiteBalance\",\"_filter\",\"_captureInterval\",\"_captureNumber\",\"_shutterVolume\"] } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );

  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_getOptions() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      Serial.print("ThetaAPI_Post_getOptions() : state[" + strState + "], " );
      if ( strState.equals("error") ) {
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println("Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      } else {  //done
        const char* sCaptureMode = root["results"]["options"]["captureMode"];
        strCaptureMode = String(sCaptureMode);
        
        const char* sExposureProgram = root["results"]["options"]["exposureProgram"];
        strExpProg = String(sExposureProgram);
        
        const char* sExposureCompensation = root["results"]["options"]["exposureCompensation"];
        String strEv = String(sExposureCompensation);
        for ( int iCnt=0; iCnt<LIST_NUM_EV; iCnt++ ) {
          if ( strEv.equals(sList_Ev[iCnt]) ) {
            iCurEv = iCnt;
            break;
          }
        }
        
        const char* sIso = root["results"]["options"]["iso"];
        String strIso = String(sIso);
        for ( int iCnt=0; iCnt<LIST_NUM_ISO; iCnt++ ) {
          if ( strIso.equals(sList_ISO[iCnt]) ) {
            iCurISO = iCnt;
            break;
          }
        }
        
        const char* sShutterSpeed = root["results"]["options"]["shutterSpeed"];
        String strShutterSpeed = String(sShutterSpeed);
        for ( int iCnt=0; iCnt<LIST_NUM_SS; iCnt++ ) {
          if ( strShutterSpeed.equals(sList_SS[iCnt]) ) {
            iCurSS = iCnt;
            break;
          }
        }
        
        const char* sWhiteBalance = root["results"]["options"]["whiteBalance"];
        String strWhiteBalance = String(sWhiteBalance);
        for ( int iCnt=0; iCnt<LIST_NUM_WB; iCnt++ ) {
          if ( strWhiteBalance.equals(sList_WB[iCnt]) ) {
            iCurWB = iCnt;
            break;
          }
        }
        
        const char* sOpt = root["results"]["options"]["_filter"];
        String strOpt = String(sOpt);
        for ( int iCnt=0; iCnt<LIST_NUM_OPT; iCnt++ ) {
          if ( strOpt.equals(sList_Opt[iCnt]) ) {
            iCurOpt = iCnt;
            break;
          }
        }
        
        const char* sIntExpSec = root["results"]["options"]["_captureInterval"];
        iIntExpSec = atoi(sIntExpSec);
        
        const char* sIntExpNum = root["results"]["options"]["_captureNumber"];
        iIntExpNum = atoi(sIntExpNum);
        
        const char* sShutterVol = root["results"]["options"]["_shutterVolume"];
        iBeepVol = atoi(sShutterVol);
        
        Serial.println("captureMod[" + strCaptureMode + "], exposureProgram[" + strExpProg + "]");
        iRet=1;
      }
    }
  }
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_commnads_status(void)
{
  int iRet=0;
  
  String strSendData = String("{\"id\":\"" + strTakePicLastId + "\"}");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdStat, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_CMDSTAT );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );

  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_commnads_status() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      Serial.print("ThetaAPI_Post_commnads_status() : state[" + strState + "]" );
      if ( strState.equals("error") ) {
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println(", Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      } else if ( strState.equals("done") ) {
        const char* sFileUri = root["results"]["fileUri"];
        Serial.println(", fileUri[" + String(sFileUri) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;        
        iRet=1;
      } else {  // inProgress
        const char* sId = root["id"];
        Serial.println(", id[" + String(sId) + "]");
        iRet=1;
      }
    }
  }
  
  return iRet;
}

//----------------
int     ThetaAPI_Res_setOptions(String strJson)
{
  int iRet;
  
  iRet = strJson.length();
  if ( iRet != 0 ) {
    char sJson[iRet+1];
    strJson.toCharArray(sJson,iRet+1);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(sJson);
    if (!root.success()) {
      Serial.println("ThetaAPI_Post_commnads_status() : parseObject() failed.");
      iRet=-1;
    } else {
      const char* sState  = root["state"];
      String strState = String(sState);
      //Serial.print("ThetaAPI_Res_setOptions() : state[" + strState + "]" );
      if ( strState.equals("done") ) {
        iRet=1;
      } else {  //error
        Serial.print("setOptions : state is not done. [" + strState + "]");
        const char* sErrorCode = root["error"]["code"];
        const char* sErrorMessage = root["error"]["message"];
        Serial.println(", Code[" + String(sErrorCode) + "], Message[" + String(sErrorMessage) + "]");
        iTakePicStat = TAKE_PIC_STAT_DONE;
        iRet=-1;
      }
    }
  }
  return iRet;    
}

//----------------
int     ThetaAPI_Post_setOptions_captureMode(char* psCapMode)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"captureMode\":\"" + String(psCapMode) +"\"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions_exposureProgram(char* psExpProg)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"exposureProgram\":" + String(psExpProg) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions_exposureCompensation(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"exposureCompensation\":" + String(sList_Ev[iCurEv]) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions_iso(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"iso\":" + String(sList_ISO[iCurISO]) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions_shutterSpeed(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"shutterSpeed\":" + String(sList_SS[iCurSS]) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions_whiteBalance(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"whiteBalance\":\"" + String(sList_WB[iCurWB]) +"\"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions__filter(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"_filter\":\"" + String(sList_Opt[iCurOpt]) +"\"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions__captureInterval(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"_captureInterval\":" + String(iIntExpSec) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions__captureNumber(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"_captureNumber\":" + String(iIntExpNum) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
//----------------
int     ThetaAPI_Post_setOptions__shutterVolume(void)
{
  int iRet=0;
  
  String strSendData = String("{\"name\": \"camera.setOptions\", \"parameters\": { \"sessionId\": \"" + strSSID + "\", \"options\":{\"_shutterVolume\":" + String(iBeepVol) +"} } }");
  String strJson = SimpleHttpProtocol("POST", (char*)sUrlCmdExe, (char*)sHost, iHttpPort, strSendData, HTTP_TIMEOUT_NORMAL );
  //Serial.println("JSON:[" + strJson + "], len=" + strJson.length() );
  
  iRet = ThetaAPI_Res_setOptions(strJson);
  
  return iRet;
}
