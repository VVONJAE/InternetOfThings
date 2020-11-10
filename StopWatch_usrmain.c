/*
  Copyright (C) 2009 Sung Ho Park
  Contact: ubinos.org@gmail.com

  This file is part of the exe_helloworld component of the Ubinos.

  GNU General Public License Usage
  This file may be used under the terms of the GNU
  General Public License version 3.0 as published by the Free Software
  Foundation and appearing in the file license_gpl3.txt included in the
  packaging of this file. Please review the following information to
  ensure the GNU General Public License version 3.0 requirements will be
  met: http://www.gnu.org/copyleft/gpl.html.

  GNU Lesser General Public License Usage
  Alternatively, this file may be used under the terms of the GNU Lesser
  General Public License version 2.1 as published by the Free Software
  Foundation and appearing in the file license_lgpl.txt included in the
  packaging of this file. Please review the following information to
  ensure the GNU Lesser General Public License version 2.1 requirements
  will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

  Commercial Usage
  Alternatively, licensees holding valid commercial licenses may
  use this file in accordance with the commercial license agreement
  provided with the software or, alternatively, in accordance with the
  terms contained in a written agreement between you and rightful owner.
*/

/* -------------------------------------------------------------------------
   Include
 ------------------------------------------------------------------------- */
#include "../ubiconfig.h"

// standard c library include
#include <stdio.h>
#include <sam4e.h>

// ubinos library include
#include "itf_ubinos/itf/bsp.h"
#include "itf_ubinos/itf/ubinos.h"
#include "itf_ubinos/itf/bsp_fpu.h"
#include "itf_ubinos/itf/bsp_intr.h"

// chipset driver include
#include "ioport.h"
#include "pio/pio.h"

// new estk driver include
#include "lib_new_estk_api/itf/new_estk_glcd.h"
#include "lib_switch/itf/lib_switch.h"

// custom library header file include
//#include "../../lib_default/itf/lib_default.h"

// user header file include

/* -------------------------------------------------------------------------
   Global variables
 ------------------------------------------------------------------------- */
sem_pt _g_sem;
int count = 0;

#define STOP      0
#define CONTINUE   1

static char g_state  = STOP;			//초기 값은 STOP

/* -------------------------------------------------------------------------
   Prototypes
 ------------------------------------------------------------------------- */
static void timer_isr(void);
static void sw1(void);
static void sw2(void);
static void rootfunc(void* arg);

/* -------------------------------------------------------------------------
   Function Definitions
 ------------------------------------------------------------------------- */
int usrmain(int argc, char * argv[]) {
   int r;

   printf("\n\n\n\r");
   printf("================================================================================\n\r");
   printf("exe_ubinos_test (build time: %s %s)\n\r", __TIME__, __DATE__);
   printf("================================================================================\n\r");

   r = sem_create(&_g_sem);
   if(0!=r){
      logme("fail at task_create\r\n");
   }

   r = task_create(NULL, rootfunc, NULL, task_getmiddlepriority(), 256, "root");
   if (0 != r) {
      logme("fail at task_create\r\n");
   }

   PMC->PMC_PCER0 = 1 << ID_TC3;

   TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;
   TC1->TC_CHANNEL[0].TC_IDR = 0xFFFFFFFF;


   TC1->TC_CHANNEL[0].TC_CMR = (TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_CPCTRG);
   TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;
   TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;


   intr_connectisr(TC3_IRQn, timer_isr, 0x40, INTR_OPT__LEVEL);

   intr_enable(TC3_IRQn);

   TC1->TC_CHANNEL[0].TC_RC = 32768;

   TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;

   intr_disable(TC3_IRQn);

   ubik_comp_start();

   return 0;
}
static void rootfunc(void* arg){
	glcd_init();
    switch_init(sw1,sw2);

    for(;;){
    		//실습2에 있던 print_lcd를 for문에 넣어 계속 동작하게 만들었다.
         sem_take(_g_sem);
         glcdGotoChar(0,0);
         glcd_printf("HW_TIMER : %3d", count);
         task_sleep(100);
   }
}

static void timer_isr(void){
   unsigned int reg;

   sem_give(_g_sem);
   count++;

   reg = TC1->TC_CHANNEL[0].TC_SR;
   printf("HW_TIMER [TC:%d] \r\n", reg); //콘솔창으로 값을 확인하기 위함.
   	   	   	   	   	   	   	   	   	   	   //하지만 쓰지는 않았다.
}

//pause기능
static void sw1(void){
   if(g_state == STOP){					//멈췄을 때 스위치를 누르면 다시 동작
      intr_enable(TC3_IRQn);			//인터럽트
      g_state = CONTINUE;
   }
   else if(g_state == CONTINUE){		//동작 중에 스위치를 누르면 멈춤
      intr_disable(TC3_IRQn);			//인터럽트 해제
      g_state = STOP;
   }

}

//값을 리셋.
static void sw2(void){
	sem_give(_g_sem);
   count = 0;							//값을 0으로 취한다.
}
