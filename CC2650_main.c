// JOUNI LAMMI 
// & 
//OLLI TÖRRÖNEN 

/*
 *  ======== main.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <stdio.h>
#include <string.h>
/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"

// Global variables
float x_term, y_term;
float y_tilt, x_tilt;
double temp;
double pres;

char list0[8]; 
char list1[8];
char list2[8];
char list3[8];
char list4[8];
char list5[8];

uint8_t jump = 0;
uint8_t prevjump = 0;
uint8_t score = 0;
uint8_t highscore = 0;
char payload_message[16];

I2C_Handle      i2c;
I2C_Params      i2cParams;
I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
I2C_Params i2cMPUParams;

int firsttime = 1;
float ax, ay, az, gx, gy, gz;
char payload[16];
char ingame_score[16];
char menu_score[16];

uint8_t menu_state = 0;
enum system_states { GAME=0, CALIBRATION, MAINMENU, SETUP};
enum system_states global_state = SETUP;
enum player_states { LEFT, RIGHT};
enum player_states player_state = LEFT;
/* jtkj Header files */
#include "wireless/comm_lib.h"
#include "sensors/bmp280.h"
#include "sensors/mpu9250.h"



/* Prototypes of our own functions */

void main_menu(I2C_Handle *i2cMPU, float x_tilt, float y_tilt, float x_term, float y_term);
void calibration(char *str, I2C_Handle *i2cMPU, float x_tilt, float y_tilt, float x_term, float y_term);
void gaming();

/* Task Stacks */
#define STACKSIZE 2048

Char commTaskStack[STACKSIZE];
Char mainTaskStack[STACKSIZE];

/* jtkj: Display */
Display_Handle hDisplay;

/* jtkj: Pin Button1 configured as power button */
static PIN_Handle hPowerButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};

/* jtkj: Pin Button0 configured as input */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

/* jtkj: Leds */
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// *******************************
// MPU GLOBAL VARIABLES
// *******************************
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU9250 uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1

};


/* jtkj: Handle power button */
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {

    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

/*HANDLER FOR BUTTON0 PRESS */
void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    
    if (global_state == MAINMENU && menu_state == 0){
        global_state = GAME;
    }    
    
    else if (global_state == MAINMENU && menu_state == 1){
        global_state = CALIBRATION;
    }
    
    
    if ((global_state == GAME) && (score > 5)){
        char msg_str[20];
        uint16_t dest_addr = 0x1234;
        sprintf(msg_str, "scr: %d", score);                       
        Send6LoWPAN(dest_addr, msg_str, strlen(msg_str));
    }
    
}

///////////// Definitions of our own functions //////////////////////////////////
void calibration(char *str, I2C_Handle *i2cMPU, float x_tilt, float y_tilt, float x_term, float y_term){
    
    Task_sleep(3000000 / Clock_tickPeriod);
    Display_clear(hDisplay);
    
    Display_print0(hDisplay, 2, 1, "Tilt");
    Display_print0(hDisplay, 4, 1, "right");
    

    sprintf(str, "kalibroidaan sivuttaissuunta, kallista oikealle\n");
    System_printf(str);
    System_flush();
    
    int x_term_timer = 10000;

    while (x_term_timer >= 0){
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
        x_term = ax;
        x_term_timer = x_term_timer - 1;
        }


    Display_clear(hDisplay);
    Display_print0(hDisplay, 2, 1, "Measuring");
    Display_print0(hDisplay, 4, 1, "ready");
    Display_print0(hDisplay, 6, 1, "Straighten");
    Display_print0(hDisplay, 8, 1, "device");
    sprintf(str, "Sivuttaissuunnan arvot saatu, suorista\n");
    System_printf(str);
    System_flush();
    
    sprintf(str,"%f\n", x_term);
    System_printf(str); 
    System_flush();
    
    Task_sleep(3000000 / Clock_tickPeriod);
    
    Display_clear(hDisplay);
    Display_print0(hDisplay, 3, 1, "Tilt");
    Display_print0(hDisplay, 5, 1, "towards");
    Display_print0(hDisplay, 7, 1, "yourself");
    sprintf(str, "kalibroidaan pystysuunta, kallista itseesi päin\n");
    System_printf(str);
    System_flush();
    
    int y_term_timer = 10000;
    
    while (y_term_timer >= 0){
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
        y_term = ay;
        y_term_timer = y_term_timer - 1;
        
        }
    Display_clear(hDisplay);
    Display_print0(hDisplay, 2, 1, "Measuring");
    Display_print0(hDisplay, 4, 1, "ready");
    Display_print0(hDisplay, 6, 1, "Straighten");
    Display_print0(hDisplay, 8, 1, "device");
    Task_sleep(1000000 / Clock_tickPeriod);
    sprintf(str, "Pystysuunnan arvot saatu\n");
    System_printf(str);
    System_flush();
    
    
    sprintf(str,"%f\n", y_term);
    System_printf(str); 
    System_flush();
    Display_clear(hDisplay);
    }


void main_menu(I2C_Handle *i2cMPU, float x_tilt, float y_tilt, float x_term, float y_term){
    Display_clear(hDisplay);
        
        //memset(list1,0,7);
        list1[0] = ' '; list1[1] = '|'; list1[2] = ' '; list1[3] = '|' ; list1[4] = ' '; list1[4] = ' '; list1[5] = '|'; list1[6] = ' ';
        //memset(list2,0,7);
        list2[0] = ' '; list2[1] = '|'; list2[2] = ' '; list2[3] = '|' ; list2[4] = ' '; list2[4] = ' '; list2[5] = '|'; list2[6] = ' ';
        //memset(list3,0,7);
        list3[0] = ' '; list3[1] = '|'; list3[2] = ' '; list3[3] = '|' ; list3[4] = ' '; list3[4] = ' '; list3[5] = '|'; list3[6] = ' ';
        //memset(list4,0,7);
        list4[0] = ' '; list4[1] = '|'; list4[2] = ' '; list4[3] = '|' ; list4[4] = ' '; list4[4] = ' '; list4[5] = '|'; list4[6] = ' ';
        //memset(list5,0,7);
        list5[0] = ' '; list5[1] = '|'; list5[2] = ' '; list5[3] = '|' ; list5[4] = ' '; list5[4] = ' '; list5[5] = '|'; list5[6] = ' ';

    
    if (temp>68){
	    menu_state = 0;
	    Display_print0(hDisplay, 4, 1, "> START GAME");
        Display_print0(hDisplay, 7, 3, "CALIBRATE");
        Display_print0(hDisplay, 1, 1, "IT'S HOT!");
        sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	    Display_print0(hDisplay, 10, 1, menu_score);
        
        }
	else if (temp<68){
	    menu_state = 0;
	    Display_print0(hDisplay, 4, 1, "> START GAME");
        Display_print0(hDisplay, 7, 3, "CALIBRATE");
        Display_print0(hDisplay, 1, 1, "IT'S COLD!");
        sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	    Display_print0(hDisplay, 10, 1, menu_score);
	    }
	
	
	
	if (temp <= 68){

        while(global_state == MAINMENU){ 
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
       
            if ((ay < -0.1) && (ay < -y_term)) {
                menu_state = 0;
                Display_print0(hDisplay, 4, 1, "> START GAME");
                Display_print0(hDisplay, 7, 3, "CALIBRATE");
                Display_print0(hDisplay, 1, 1, "IT'S COLD!");
                sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	            Display_print0(hDisplay, 10, 1, menu_score);
                }
            else if ((ay > 0.1) && (ay  > y_term)) {
                menu_state = 1;
                Display_print0(hDisplay, 4, 3, "START GAME");
                Display_print0(hDisplay, 7, 1, "> CALIBRATE");
                Display_print0(hDisplay, 1, 1, "IT'S COLD!");
                sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	            Display_print0(hDisplay, 10, 1, menu_score);
                }

            Task_sleep(100000 / Clock_tickPeriod);
                
        }
	}
	
	else if (temp > 68){
        while(global_state == MAINMENU){ 
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

                
                if ((ay < -0.1) && (ay < -y_term)) {
                    menu_state = 0;
                    Display_print0(hDisplay, 4, 1, "> START GAME");
                    Display_print0(hDisplay, 7, 3, "CALIBRATE");
                    Display_print0(hDisplay, 1, 1, "IT'S HOT!");
                    sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	                Display_print0(hDisplay, 10, 1, menu_score);
                    }
                else if ((ay > 0.1) && (ay  > y_term)) {
                    menu_state = 1;
                    Display_print0(hDisplay, 4, 3, "START GAME");
                    Display_print0(hDisplay, 7, 1, "> CALIBRATE");
                    Display_print0(hDisplay, 1, 1, "IT'S HOT!");
                    sprintf(menu_score,"HIGHSCORE: %d\n",highscore);
	                Display_print0(hDisplay, 10, 1, menu_score);
                    }

            Task_sleep(100000 / Clock_tickPeriod);

        }

    }

}
	
	
void gaming(){
    
    mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
        
        
        if ((ax > 0.1) && (ax  > x_term) && (global_state == GAME)) {
            player_state = RIGHT;
                }
        else if ((ax < -0.1) && (ax  < -x_term) && (global_state == GAME)) {   
            player_state = LEFT;
            }
        
        if (((ay > 0.3) || ay < -0.3) && (global_state == GAME) && (jump == 0)) {   
            if (player_state == LEFT){
                jump = 1;
                list5[2] = 'O';
                }
            
            else if (player_state == RIGHT){
                jump = 1;
                list5[4] = 'O';
                }
            
            }
            
        
        if (prevjump == 1){
            jump = 0;
        }
            
        if(global_state == GAME){
            
            if (jump == 0){
                if (player_state == LEFT){
                    if (list5[2] != ' '){
                        Display_clear(hDisplay);
                        Display_print0(hDisplay, 6, 2, "CRASHED");
                        Display_print0(hDisplay, 7, 2, "GAME OVER");
                        sprintf(ingame_score,"SCORE: %d\n",score);
	                    Display_print0(hDisplay, 8, 1, ingame_score);
	                    if (score < 15){
                            Display_print0(hDisplay, 10, 2, "YOU'RE BAD");
                                }
                        if (score > highscore){
                            highscore = score;
                            Display_print0(hDisplay, 9, 1, "NEW HIGHSCORE");
                            }
                            score = 0;
                        Task_sleep(3000000 / Clock_tickPeriod);
                        Display_clear(hDisplay);
                        global_state = MAINMENU;
                        }
                        
                        if(global_state == GAME){
                            list5[2] = 'o';
                            }
                    
                    }
                
                
                else if (player_state == RIGHT){
                    if (list5[4] != ' '){
                        Display_clear(hDisplay);
                        Display_print0(hDisplay, 6, 2, "CRASHED");
                        Display_print0(hDisplay, 7, 2, "GAME OVER");
                        sprintf(ingame_score,"SCORE: %d\n",score);
	                    Display_print0(hDisplay, 8, 1, ingame_score);
	                    if (score < 15){
                            Display_print0(hDisplay, 10, 2, "YOU'RE BAD");
                                }
                        if (score > highscore){
                            highscore = score;
                            Display_print0(hDisplay, 9, 1, "NEW HIGHSCORE");
                            }
                            score = 0;
                        Task_sleep(3000000 / Clock_tickPeriod);
                        Display_clear(hDisplay);
                        global_state = MAINMENU;
                        }
                    
                    
                    if(global_state == GAME){
                            list5[4] = 'o';
                            }
                    }
                    
                    }
                     
                    }       
            
            
            if(global_state == GAME){
            Display_print0(hDisplay, 3, 3, list0);
            Display_print0(hDisplay, 4, 3, list1);
            Display_print0(hDisplay, 5, 3, list2);
            Display_print0(hDisplay, 6, 3, list3);
            Display_print0(hDisplay, 7, 3, list4);
            Display_print0(hDisplay, 8, 3, list5);
            sprintf(ingame_score,"SCORE: %d\n",score);
	        Display_print0(hDisplay, 10, 1, ingame_score);
	        Display_print0(hDisplay, 11, 1, payload_message);

            
            //Move items inside list
            int u;
            
            for (u=0;u<7;u++){
                list5[u] = list4[u];
                }
                
                
            for (u=0;u<7;u++){    
                list4[u] = list3[u];
                }
                
                    
            for (u=0;u<7;u++){
                list3[u] = list2[u];
                }
                
                    
            for (u=0;u<7;u++){
                list2[u] = list1[u];
                }
                
                    
            for (u=0;u<7;u++){
                list1[u] = list0[u];
                }
  
        }
        if (jump == 1){
            prevjump = 1;
            }
        else if (jump == 0){
            prevjump = 0;
            }
        score = score + 1;
}


/* jtkj: Communication Task */
Void commTask(UArg arg0, UArg arg1) {
    ///esim. 01100010esimsana
    uint16_t senderAddr;
    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

    while (1) {
        
        if ((GetRXFlag() == true)) {

            // Empty buffer to prevent data bits from previous messages
            memset(payload,0,16);
            // Luetaan viesti puskuriin payload
            Receive6LoWPAN(&senderAddr, payload, 16);   ///payload e.g. 01100010testword
            if (senderAddr == IEEE80154_SERVER_ADDR){
                uint8_t payloadtrack = payload[0];
                uint8_t mask0 = 0x40;
                uint8_t mask1 = 0x20;
                uint8_t mask2 = 0x10;
                uint8_t mask3 = 0x08;
                uint8_t mask4 = 0x04;
                uint8_t mask5 = 0x02;
                
                memset(list0,0,7);
                
                if ((payloadtrack & mask0) == mask0){ //moving obstacle outside of left lane
                    list0[0] = 'Y';
                    }
                else {
                    list0[0] = ' ';
                    }  
                    
                list0[1] = '|';
                
                if ((payloadtrack & mask1) == mask1){ //moving obstacle on the left lane
                    list0[2] = 'Y';
                    }
                else {
                    list0[2] = ' ';
                    }
                    
                    
                if ((payloadtrack & mask2) == mask2){  //static on the left lane
                    list0[2] = '#';
                    }
                else {
                    list0[2] = ' ';
                    }
                    
                    
                list0[3] = '|' ;   
                    
                if ((payloadtrack & mask3) == mask3){ //static on the right lane
                    list0[4] = '#';
                    }
                else {
                    list0[4] = ' ';
                    }
                    
                    
                if ((payloadtrack & mask4) == mask4){ //moving obstacle on the right lane
                    list0[4] = 'Y';
                    }
                else {
                    list0[4] = ' ';
                    }
                    
                list0[5] = '|';
                    
                if ((payloadtrack & mask5) == mask5){ //moving obstacle outside of right lane
                    list0[6] = 'Y';
                    }
                else {
                    list0[6] = ' ';
                    }
             
            sprintf(payload_message,"%c%c%c%c%c%c%c%c\n",payload[1],payload[2],payload[3],payload[4],payload[5],payload[6],payload[7],payload[8]);
        }
        }
   }
}

void mainTask(UArg arg1, UArg arg0) {

	char str[80];
	int i = 1;
    /* Init display */
    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }
    
    Display_print0(hDisplay, 2, 1, "KEEP FLAT");
    Display_print0(hDisplay, 4, 1, "STARTING");
    Display_print0(hDisplay, 6, 1, "SENSORS");
    Task_sleep(2000000 / Clock_tickPeriod);
    
    
    /* jtkj: Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // *******************************
    // OTHER SENSOR OPEN I2C
    // *******************************
    i2c = I2C_open(Board_I2C, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
   
    // JTKJ: SETUP BMP280 SENSOR HERE
    bmp280_setup(&i2c);
    
    
    bmp280_get_data(&i2c, &pres, &temp);
                                                                                                 
    	    sprintf(str,"%f %f\n",pres,temp);
    	    System_printf(str);
    	    System_flush();
            
    
    
    // *******************************
    // OTHER SENSOR CLOSE I2C
    // *******************************
    I2C_close(i2c);
    
    // *******************************
    // MPU OPEN I2C
    // *******************************
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    
    // *******************************
    // MPU POWER ON
    // *******************************
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();
   
    
    // *******************************
    // MPU9250 SETUP AND CALIBRATION
    // *******************************
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();
	
    
    
    
    Display_clear(hDisplay);
    global_state = MAINMENU;
    
    while(1){

    
        if (global_state == GAME){
            gaming();
            }
        else if (global_state == CALIBRATION){
            Display_clear(hDisplay);
            calibration(str, i2cMPU, x_tilt, y_tilt, x_term, y_term);
            global_state = MAINMENU;
            }
        else if (global_state == MAINMENU){    
            main_menu(i2cMPU, x_tilt, y_tilt, x_term, y_term);
        }
        Task_sleep(1000000 / Clock_tickPeriod);
            
        }
        
    }


Int main(void) {
 // Task variables
	Task_Handle hLabTask;
	Task_Params labTaskParams;
	Task_Handle hCommTask;
	Task_Params commTaskParams;
	Task_Handle hMainTask;
	Task_Params mainTaskParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();
    
     // *******************************
    // OPEN MPU POWER PIN
    // *******************************
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }
    

	/* jtkj: Power Button */
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing power button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering power button callback function");
	}

    // JTKJ: INITIALIZE BUTTON0 HERE
    
    /* jtkj: Init Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }

    hButton0 = PIN_open(&sButton0, cButton0);
   
    if(!hButton0) {
      System_abort("Error initializing button pins\n");
   }


   //Exception handler for another pin
   //Exception happens when the button is pressed
   if (PIN_registerIntCb(hButton0, &buttonFxn) != 0) {
      System_abort("Error registering button callback function");
   }
   

    
    /* jtkj: Init Communication Task */
    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;

    
    Init6LoWPAN();
    


    hCommTask = Task_create(commTask, &commTaskParams, NULL);
    if (hCommTask == NULL) {
    System_abort("Task create failed!");
    }


    /* Init main task*/
    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=2;

    hMainTask = Task_create(mainTask, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Start BIOS */
    BIOS_start();

    return (0);
}
