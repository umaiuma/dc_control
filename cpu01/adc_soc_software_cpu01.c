//###########################################################################
// FILE:   adc_soc_software_cpu01.c
// TITLE:  ADC software triggering for F2837xS.
//
//! \addtogroup cpu01_example_list
//! <h1> ADC SOC Software Force (adc_soc_software)</h1>
//!
//! This example converts some voltages on ADCA and ADCB  based on a software
//! trigger.
//!
//! After the program runs, the memory will contain:
//!
//! - \b AdcaResult0 \b: a digital representation of the voltage on pin A0\n
//! - \b AdcaResult1 \b: a digital representation of the voltage on pin A1\n
//! - \b AdcbResult0 \b: a digital representation of the voltage on pin B0\n
//! - \b AdcbResult1 \b: a digital representation of the voltage on pin B1\n
//!
//! Note: The software triggers for the two ADCs happen sequentially, so the
//! two ADCs will run asynchronously.
//!
//
//###########################################################################
// $TI Release: F2837xS Support Library v191 $
// $Release Date: Fri Mar 11 15:58:35 CST 2016 $
// $Copyright: Copyright (C) 2014-2016 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

//pin 29 A1
#include "F28x_Project.h"     // Device Headerfile and Examples Include File
//#include <stdio.h>

typedef struct
{
    volatile struct EPWM_REGS *EPwmRegHandle;
    Uint16 EPwm_CMPA_Direction;
    Uint16 EPwm_CMPB_Direction;
    Uint16 EPwmTimerIntCount;
    Uint16 EPwmMaxCMPA;
    Uint16 EPwmMinCMPA;
    Uint16 EPwmMaxCMPB;
    Uint16 EPwmMinCMPB;
}EPWM_INFO;
void InitEPwm3Example(void);
void ConfigureADC(void);
void SetupADCSoftware(void);
void SetupPIController(void);
//variables to store conversion results
Uint16 AdcaResult0;
Uint16 AdcaResult1;
Uint16 AdcbResult0;
Uint16 AdcbResult1;
//variables to PI controller

typedef struct {
	Uint16 Output;
	Uint16 PreviousError;
	Uint16 SetPoint;
	Uint16 MeasuredValue;
	Uint16 Kp;
	Uint16 Ki;
	Uint16 Error;
	Uint16 Integral;
}PIControl;
PIControl currentControl;


__interrupt void adca1_isr(void);
__interrupt void epwm3_isr(void);
__interrupt void cpu_timer0_isr(void);
void update_compare(EPWM_INFO*);
EPWM_INFO epwm3_info;
#define EPWM3_TIMER_TBPRD  64000  // Period register
#define EPWM3_MAX_CMPA     32000
#define EPWM3_MIN_CMPA     32000
#define EPWM3_MAX_CMPB     0
#define EPWM3_MIN_CMPB     0
#define EPWM_CMP_UP   1
#define EPWM_CMP_DOWN 0

void main(void)
{
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xS_SysCtrl.c file.
    InitSysCtrl();

// Step 2. Initialize GPIO:
// This example function is found in the F2837xS_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
    InitGpio();
    CpuSysRegs.PCLKCR2.bit.EPWM3=1;
    InitEPwm3Gpio();
    
// Step 3. Clear all interrupts and initialize PIE vector table:
// Disable CPU interrupts
    DINT;

// Initialize the PIE control registers to their default state.
// The default state is all PIE interrupts disabled and flags
// are cleared.
// This function is found in the F2837xS_PieCtrl.c file.
    InitPieCtrl();

// Disable CPU interrupts and clear all CPU interrupt flags:
    IER = 0x0000;
    IFR = 0x0000;

// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xS_DefaultIsr.c.
// This function is found in F2837xS_PieVect.c.
    InitPieVectTable();

    EALLOW;
    PieVectTable.ADCA1_INT = &adca1_isr;
    PieVectTable.EPWM3_INT = &epwm3_isr;
    PieVectTable.TIMER0_INT = &cpu_timer0_isr;
    EDIS;

    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 60, 10);
    CpuTimer0Regs.TCR.all = 0x4001;


    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC =0;
    EDIS;

    InitEPwm3Example();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC =1;
   	EDIS;
// Enable global Interrupts and higher priority real-time debug events:
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    IER |= M_INT1|M_INT3;

    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;


//Configure the ADCs and power them up
    ConfigureADC();

//Setup the ADCs for software conversions
    SetupADCSoftware();

//take conversions indefinitely in loop

   // SetupPIController();
    do
    {
    //convert, wait for completion, and store results
    	//start conversions immediately via software, ADCA

    	/*
    	while(AdcaRegs.ADCINTFLG.bit.ADCINT1 == 0);
    	AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    	AdcaResult1 = AdcaResultRegs.ADCRESULT1;
    	*/


    }while(1);
}

//Write ADC configurations and power up the ADC for both ADC A and ADC B
__interrupt void adca1_isr(void)
{
	AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
	AdcaResult1 = AdcaResultRegs.ADCRESULT1;
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
	//EPwm3Regs.CMPA.bit.CMPA = AdcaResult1/2;
	//printf("ADCA1: %d\n",AdcaResult1);
}


void ConfigureADC(void)
{
	EALLOW;

	//write configurations
	AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
	AdcbRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_16BIT, ADC_SIGNALMODE_SINGLE);
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_16BIT, ADC_SIGNALMODE_SINGLE);

	//Set pulse positions to late
	AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;
	AdcbRegs.ADCCTL1.bit.INTPULSEPOS = 1;

	//power up the ADCs
	AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
	AdcbRegs.ADCCTL1.bit.ADCPWDNZ = 1;

	//delay for 1ms to allow ADC time to power up
	DELAY_US(1000);

	EDIS;
}

void SetupADCSoftware(void)
{
	Uint16 acqps;

	//determine minimum acquisition window (in SYSCLKS) based on resolution
	if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION){
		acqps = 14; //75ns
	}
	else { //resolution is 16-bit
		acqps = 63; //320ns
	}

//Select the channels to convert and end of conversion flag
    //ADCA
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCINTSEL1N2.bit.INT2SEL = 0;
    AdcaRegs.ADCINTSEL1N2.bit.INT2E = 1;
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;

    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin A1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 1; //end of SOC1 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared

    //ADCB
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin B0
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcbRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin B1
    AdcbRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcbRegs.ADCINTSEL1N2.bit.INT1SEL = 1; //end of SOC1 will set INT1 flag
    AdcbRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcbRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
}

__interrupt void epwm3_isr(void)
{

    // Update the CMPA and CMPB values
    update_compare(&epwm3_info);

    // Clear INT flag for this timer
    EPwm3Regs.ETCLR.bit.INT = 1;

    // Acknowledge this interrupt to receive more interrupts from group 3
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}
void InitEPwm3Example(void)
{

    // Setup TBCLK
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Count up/down and down
    EPwm3Regs.TBPRD = EPWM3_TIMER_TBPRD;           // Set timer period
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE;        // Disable phase loading
    EPwm3Regs.TBPHS.bit.TBPHS = 0x0000;            // Phase is 0
    EPwm3Regs.TBCTR = 0x0000;                      // Clear counter
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    // Setup shadow register load on ZERO
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    // Set Compare values
    EPwm3Regs.CMPA.bit.CMPA = EPWM3_MIN_CMPA;   // Set compare A value
    EPwm3Regs.CMPB.bit.CMPB = EPWM3_MAX_CMPB;   // Set Compare B value

    // Set Actions
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;           // Set PWM3A on period
    EPwm3Regs.AQCTLA.bit.CAD = AQ_SET;         // Clear PWM3A on event B, down
                                                 // count

    EPwm3Regs.AQCTLB.bit.CBU = AQ_CLEAR;         // Clear PWM3A on period
    EPwm3Regs.AQCTLB.bit.CBD = AQ_SET;           // Set PWM3A on event A, up
                                                 // count

    // Interrupt where we will change the Compare Values
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;    // Select INT on Zero event
    EPwm3Regs.ETSEL.bit.INTEN = 1;               // Enable INT
    EPwm3Regs.ETPS.bit.INTPRD = ET_3RD;          // Generate INT on 3rd event

    // Information this example uses to keep track
    // of the direction the CMPA/CMPB values are
    // moving, the min and max allowed values and
    // a pointer to the correct ePWM registers
    epwm3_info.EPwm_CMPA_Direction = EPWM_CMP_UP;   // Start by increasing CMPA &
    epwm3_info.EPwm_CMPB_Direction = EPWM_CMP_DOWN; // decreasing CMPB
    epwm3_info.EPwmTimerIntCount = 0;               // Zero the interrupt counter
    epwm3_info.EPwmRegHandle = &EPwm3Regs;          // Set the pointer to the
                                                    // ePWM module
    epwm3_info.EPwmMaxCMPA = EPWM3_MAX_CMPA;        // Setup min/max CMPA/CMPB
                                                    // values
    epwm3_info.EPwmMinCMPA = EPWM3_MIN_CMPA;
    epwm3_info.EPwmMaxCMPB = EPWM3_MAX_CMPB;
    epwm3_info.EPwmMinCMPB = EPWM3_MIN_CMPB;

}
/*
void SetupPIController(void){
	PreviousError = 0;
	Error = 0;
	SetPoint = 55500;
	Output = 0;
	MeasuredValue =0;
	Kp = 5;
	Ki = 200;


}
*/
__interrupt void cpu_timer0_isr(void)
{
	AdcaRegs.ADCSOCFRC1.all = 0x0003; //SOC0 and SOC1
	CpuTimer0.InterruptCount++;

	/*

   CpuTimer0.InterruptCount++;
   AdcaRegs.ADCSOCFRC1.all = 0x0003; //SOC0 and SOC1
   float dt = 0.00001;

   PreviousError = 0;
   Integral = 0;
   MeasuredValue =AdcaResult1;
   Error = SetPoint - MeasuredValue;
   Integral = Integral + Error*dt;
   Output = Kp*Error + Ki*Integral;
   PreviousError = Error;

   EPwm3Regs.CMPA.bit.CMPA = Output;   // Set compare A value


   // Acknowledge this __interrupt to receive more __interrupts from group 1

*/
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

void update_compare(EPWM_INFO *epwm_info)
{


}
