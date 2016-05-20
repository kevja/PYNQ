/******************************************************************************
 *  Copyright (c) 2016, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 *
 * @file pmod_pwm.c
 *
 * IOP code (MicroBlaze) for PMOD Timer.
 * Pulses can be generated by the PMOD timer.
 * The timer can also detect pulses on the PMOD pin.
 * The input / output pin is assumed to be at Pin 1 of any PMOD.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- ------- -----------------------------------------------
 * 1.00a pp  05/10/16 release
 *
 * </pre>
 *
 *****************************************************************************/
/*****************************************************************************
 *
 * IOP code (MicroBlaze) for PWM.
 * The interface is driven by the timer's PWM output
 * The output is controlled by the timer's PWM output having period and
 * duty cycle as the input parameters
 *
 *****************************************************************************/

/*
 * Since the AXI TmrCtr IP's device driver does not support
 * PWM feature, it is generated using low-level driver calls. Hence
 * SetupTimers() function is used. Also, pmod_init() function is not called
 * since IIC and SPI are not used, as well as high-level timer functions are not used.
 * When AXI Timer is used in PWM mode, both timer sub-modules loose their
 * independent functionality.
 */
#include "xparameters.h"
#include "xtmrctr_l.h"
#include "xgpio.h"
#include "pmod.h"

/*
 * TIMING_INTERVAL = (TLRx + 2) * AXI_CLOCK_PERIOD
 * PWM_PERIOD = (TLR0 + 2) * AXI_CLOCK_PERIOD
 * PWM_HIGH_TIME = (TLR1 + 2) * AXI_CLOCK_PERIOD
 */

// TCSR0 Timer 0 Control and Status Register
#define TCSR0 0x00
// TLR0 Timer 0 Load Register
#define TLR0 0x04
// TCR0 Timer 0 Counter Register
#define TCR0 0x08
// TCSR1 Timer 1 Control and Status Register
#define TCSR1 0x10
// TLR1 Timer 1 Load Register
#define TLR1 0x14
// TCR1 Timer 1 Counter Register
#define TCR1 0x18
// Default period value for 100000 us
#define MS1_VALUE 99998
// Default period value for 50% duty cycle
#define MS2_VALUE 49998

/*
 * Parameters passed in MAILBOX_WRITE_CMD
 * bits 31:16 => period in us
 * bits 15:8 is not used
 * bits 7:1 => duty cycle in %, valid range is 1 to 99
 */

/************************** Function Prototypes ******************************/
void SetupTimers(void);

int main(void) {
	u32 cmd;
	u32 Timer1Value, Timer2Value;

	SetupTimers();
    //	Configuring PMOD IO Switch to connect GPIO to pmod
	// bit-0 is controlled by the pwm
	configureSwitch(PWM, GPIO_1, GPIO_2, GPIO_3, GPIO_4, GPIO_5, GPIO_6, GPIO_7);	// LED is connected to bit[0] of the Channel 1 of AXI GPIO instance

	while(1){
		while(MAILBOX_CMD_ADDR==0); // wait for command word to become non-zero
		cmd = MAILBOX_CMD_ADDR;		// if bit[0] is 1 then stop both timers, if bit[1] is 1 the generate
		if(cmd & 0x01) {
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 0, TCSR0, 0);	// disable timer 0
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 1, TCSR0, 0);	// disable timer 1
		}
		else {
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 0, TCSR0, 0x296);
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 1, TCSR0, 0x296);
			Timer1Value = (cmd >> 16) & 0x0ffff; // period in us
			Timer2Value = ((cmd >> 1) & 0x07f)*Timer1Value/100; // high value in us
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 0, TLR0, Timer1Value*100);	// period
			XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 1, TLR0, Timer2Value*100);	// high time
		}
		MAILBOX_CMD_ADDR = 0x0;
	}
	return 0;
}

void SetupTimers(void) {

	// Load timer's Load registers
	XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 0, TLR0, MS1_VALUE);	// period
	XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 1, TLR0, MS2_VALUE);	// high time
	// 0010 1011 0110 => no cascade, no all timers, enable pwm, interrupt status, enable timer,
	// no interrupt, no load timer, reload, no capture, enable external generate, down counter, generate mode
	XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 0, TCSR0, 0x296);
	XTmrCtr_WriteReg(XPAR_TMRCTR_0_BASEADDR, 1, TCSR0, 0x296);
}


