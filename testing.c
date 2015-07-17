
#define     BYTE_A  1
#define     BYTE_B  2
#define     BYTE_C  3
#define     BYTE_D  4
#define     BYTE_E  5
#define     BYTE_F  6
#define     BYTE_G  7
#define     BYTE_H  8
#define     BYTE_I  9
#define     BYTE_J  10
#define     BYTE_K  11
#define     BYTE_L  12
#define     BYTE_M  13
#define     BYTE_N  14


//--------------------------------------
#define     IR_PORT                 4
#define     IR_PIN                  3

//--------------------------------------
#define     IR_STEP_STANDBY         0
#define     IR_STEP_LEADER          1
#define     IR_STEP_BIT             2
#define     IR_STEP_ENDBIT          3

//--------------------------------------
#define     IR_CARRY_PERIOD     26.25//(105)               // 26.25us*4
#define     IR_CARRY_DUTY       38000//(IR_CARRY_PERIOD*2/3)
#define     IR_LEADER_CARRY     8440//(8440UL * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_LEADER_DELAY     4200//(4200UL * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_BIT0_CARRY       540 //(540UL  * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_BIT0_DELAY       540//(540UL  * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_BIT1_CARRY       540//(540UL  * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_BIT1_DELAY       1620//(1620UL * 4 / IR_CARRY_PERIOD)         ///<
#define     IR_BYTE_TOTAL       (BYTE_N+1)


//------------------------------------------------------------------------------
ST_IR unsigned int gstIr;

static  void    GetPulseNext(void)
{
    if (0 == gstIr.u8IrBit) {
        gstIr.u8IrByte++;
        gstIr.u8IrBit = 8;
        if (gstIr.u8IrByte >= IR_BYTE_TOTAL) {
            gstIr.u8IrByte = 0;

            // gstIr.u8IrCarryNextH = (IR_BIT0_CARRY >> 8) | 0x80;                 //尾码
            // gstIr.u8IrCarryNextL = (IR_BIT0_CARRY & 0xff);
            // gstIr.u8IrDelayNextH = 0;
            // gstIr.u8IrDelayNextL = 0;

            REMNUMH = gstIr.u8IrCarryNextH;
            REMNUML = gstIr.u8IrCarryNextL;
            
            return;
        }
        else {
            gstIr.u8IrDataTransmit  = gstIr.u8IrData[gstIr.u8IrByte];
        }
    }

    //低位先发送
    if (gstIr.u8IrDataTransmit & 1) {
        //gstIr.u8IrCarryNextH = (IR_BIT1_CARRY >> 8) | 0x80;
        //gstIr.u8IrCarryNextL = (IR_BIT1_CARRY & 0xff);
        //gstIr.u8IrDelayNextH = (IR_BIT1_DELAY >> 8);
        //gstIr.u8IrDelayNextL = (IR_BIT1_DELAY & 0xff);
        PWM_H = IR_BIT1_CARRY;
        PWM_L = IR_BIT1_DELAY;
    }
    else {
        PWM_H = IR_BIT0_CARRY;
        PWM_L = IR_BIT0_DELAY;
    }

    gstIr.u8IrDataTransmit >>= 1;
    gstIr.u8IrBit--;

    PWM_H = gstIr.u8IrCarryNextH;
    PWM_L = gstIr.u8IrCarryNextL;
}

void timer_irq_handle(void)
{
    int i;

    //if (led_data.status_flag == 1) {
        /* restart timer1 */
        BSP_Timer1Init(10);

        /* Check red/green/blue/white led 
      
            if (led_pwm > LED_MIN_BRIGHTNESS &&led_pwm <= LED_MAX_BRIGHTNESS) 
            {
                if (timer_cnt == pwm &&led_data.timer_cnt != LED_MAX_BRIGHTNESS) {
                    BSP_GPIOSetValue(led_gpio, 0);

                } else if (timer_cnt == 0) {
                    BSP_GPIOSetValue(led_data.led_data.led_table[i].led_gpio, 1);
                }           
            }else if(led_pwm == MIN_BRIGHTNESS){
                if(led_data.timer_cnt == 0){
                    BSP_GPIOSetValue(led_gpio, 0);
                }
            }   
        }*/
        if(PWM_H>0)
        {
            BSP_GPIOSetValue(IR_GPIO, 1);
            PWM_H--;
        }      else if (PWM_L>=0 && PWM_H == 0)
        {
            BSP_GPIOSetValue(IR_GPIO, 0);
            PWM_L--;
        }
        
        timer_cnt++;
    }
}       /* -----  end of function light_timer_irq_handle  ----- */

// void airctrl_start(unsigned char *ctrl_data)
// {
    
//     log_info("Light PWM output is running.\n");  

//     //led_data.status_flag = 0;
//     //OSTimeDly(1);
//     //light_data_parse(action, attr_flags, ctrl_data);
//     OSTimeDly(1); 
//    // led_data.status_flag = 1;    
//     /* Start hard timer1 to output LED PWM signal */
//     BSP_Timer1Init(1);
// }   
static  void    RemInit(void)
{
             //
    memset(&gstIr.u8IrData, 0, sizeof(gstIr.u8IrData));
    BSP_GPIOPinMux(IR_GPIO);         /* WHITE */
    BSP_GPIOSetDir(IR_GPIO, 1);      /* output */
    BSP_GPIOSetValue(IR_GPIO, 0);    /* low level */

    PWM_H        = (IR_LEADER_DELAY >> 8);
    PWM_L        = (IR_LEADER_DELAY & 0xff);
    gstIr.u8IrStep = IR_STEP_LEADER;
    gstIr.u8IrByte = 0x0;
    gstIr.u8IrBit  = 8;
    gstIr.u8IrDataTransmit  = gstIr.u8IrData[0];
    CLR_BIT(gstIr.u8IrFlg, bIrDelay);   //发送头码载波部分

}

void main()
{
    log_info("IR output is running.\n");

    BSP_Timer1Init(10);
    if (TEST_BIT(gstIr.u8IrFlg, bIrTransmit)) {
        if (gstIr.u16DelayTransmit) {           //如果有延时发送，就先延时计数
            if (TEST_BIT(gstIr.u8IrFlg, bIrEn)) {
                 CLR_BIT(gstIr.u8IrFlg, bIrEn);
                gstIr.u16DelayTransmit--;
            }
            return;
        }  
   // return 0;
}
