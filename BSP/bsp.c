/*****************************************************************************
 * BSP for EK-TM4C123GXL with QP/C framework
 *****************************************************************************/
#include "qpc.h"  /* QP/C API */
#include "bsp.h"

/* add other drivers if necessary... */
#include "stm32f10x.h"
#include "buttons_hal.h"




#ifdef Q_SPY
static QSTimeCtr QS_tickTime_;
static QSTimeCtr QS_tickPeriod_;

/* QSpy source IDs */
static QSpyId const l_SysTick = { 0U };

#endif

/* ISRs  ===============================================*/
void SysTick_Handler(void) {

	QK_ISR_ENTRY(); /* inform QK about entering an ISR */

#ifdef Q_SPY
	{
		//tmp = SysTick->CTRL; /* clear SysTick_CTRL_COUNTFLAG */
		QS_tickTime_ += QS_tickPeriod_; /* account for the clock rollover */
	}
#endif

	QTIMEEVT_TICK_X(0U, &l_SysTick); /* process time events for tick rate 0 */

	QK_ISR_EXIT()
	; /* inform QK about exiting an ISR */

}
/*..........................................................................*/
void QK_onIdle(void) {
#ifdef Q_SPY
	QS_rxParse(); /* parse all the received bytes */

	if ((USART1->SR & USART_FLAG_TXE) != 0) { /* TXE empty? */
		uint16_t b;

		QF_INT_DISABLE();
		b = QS_getByte();
		QF_INT_ENABLE();

		if (b != QS_EOD) { /* not End-Of-Data? */
			USART1->DR = (b & 0xFFU); /* put into the DR register */
		}
	}
#elif defined NDEBUG
	/* Put the CPU and peripherals to the low-power mode.
	 * you might need to customize the clock management for your application,
	 * see the datasheet for your particular Cortex-M MCU.
	 */
	/* !!!CAUTION!!!
	 * The WFI instruction stops the CPU clock, which unfortunately disables
	 * the JTAG port, so the ST-Link debugger can no longer connect to the
	 * board. For that reason, the call to __WFI() has to be used with CAUTION.
	 *
	 * NOTE: If you find your board "frozen" like this, strap BOOT0 to VDD and
	 * reset the board, then connect with ST-Link Utilities and erase the part.
	 * The trick with BOOT(0) is that it gets the part to run the System Loader
	 * instead of your broken code. When done disconnect BOOT0, and start over.
	 */
	//__WFI(); /* Wait-For-Interrupt */
#endif
}

/*..........................................................................*/
#ifdef Q_SPY
/*
 * ISR for receiving bytes from the QSPY Back-End
 * NOTE: This ISR is "QF-unaware" meaning that it does not interact with
 * the QF/QK and is not disabled. Such ISRs don't need to call QK_ISR_ENTRY/
 * QK_ISR_EXIT and they cannot post or publish events.
 */
void USART1_IRQHandler(void) {
	if ((USART1->SR & USART_SR_RXNE) != 0) {
		uint32_t b = USART1->DR;
		QS_RX_PUT(b);
	}
	QK_ARM_ERRATUM_838869();
}
#else
void USART2_IRQHandler(void) {}
#endif

/* BSP functions ===========================================================*/
void BSP_init(void) {
	/* NOTE: SystemInit() has been already called from the startup code
	 *  but SystemCoreClock needs to be updated
	 */
	SystemCoreClockUpdate();

    buttons_hal_init();

	__ISB();
	__DSB();

	QS_INIT((void * )0); /* initialize the QS software tracing */

	/* setup the QS filters... */
	QS_GLB_FILTER(QS_UA_RECORDS); /* all User records */
	//QS_GLB_FILTER(QS_SM_RECORDS); /* state machine records */
	QS_GLB_FILTER(-QS_ALL_RECORDS); /* all QS records */
	//QS_GLB_FILTER(-QS_QF_TICK); /* disable */
	QS_GLB_FILTER(QS_U0_RECORDS);

	/* prodice the QS dictionaries... */

}
/*..........................................................................*/
void QF_onStartup(void) {
	/* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate
	 * NOTE: do NOT call OS_CPU_SysTickInit() from uC/OS-II
	 */
	SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

	/* set priorities of ALL ISRs used in the system, see NOTE1 */
	//NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
	/* ... */

	/* assign all priority bits for preemption-prio. and none to sub-prio. */
	NVIC_SetPriorityGrouping(0U);

	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	 * Assign a priority to EVERY ISR explicitly, see NOTE00.
	 * DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
	 */
	/* kernel UNAWARE interrupts, see NOTE00 */
	NVIC_SetPriority(USART1_IRQn, 0U);
	/* ... */

	/* kernel AWARE interrupts, see NOTE00 */
	NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI + 0U);
	/* ... */
	NVIC_SetPriority(EXTI3_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
	NVIC_SetPriority(EXTI4_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
	NVIC_SetPriority(EXTI9_5_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);

	/* enable IRQs... */
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);

#ifdef Q_SPY
	NVIC_EnableIRQ(USART1_IRQn); /* USART3 interrupt used for QS-RX */
#endif
}
/*..........................................................................*/
void QF_onCleanup(void) {
}

/*..........................................................................*/

/* QS callbacks ============================================================*/
#ifdef Q_SPY
/*..........................................................................*/
uint8_t QS_onStartup(void const *arg) {
	Q_UNUSED_PAR(arg);/* avoid the "unused parameter" compiler warning */

	static uint8_t qsBuf[2 * 1024]; /* buffer for QS-TX channel */
	static uint8_t qsRxBuf[100]; /* buffer for QS-RX channel */

	QS_initBuf(qsBuf, sizeof(qsBuf));
	QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

	/* setup the QS filters... */
	QS_GLB_FILTER(QS_QEP_DISPATCH); /* all User records */
	QS_GLB_FILTER(QS_QF_PUBLISH); /* all User records */
    QS_GLB_FILTER(QS_UA_RECORDS); /* all User records */

    QS_GLB_FILTER(QS_U0_RECORDS);
    //QS_GLB_FILTER(-QS_U1_RECORDS);
    //QS_GLB_FILTER(-QS_U2_RECORDS);
    //QS_GLB_FILTER(-QS_U4_RECORDS);

    QS_GLB_FILTER(QS_SM_RECORDS); /* state machine records */
	//	QS_GLB_FILTER(QS_ALL_RECORDS); /* all QS records */
	//	QS_GLB_FILTER(-QS_QF_TICK); /* disable */

	/* NVIC Configuration */
	NVIC_InitTypeDef NVIC_InitStructure;
	/* Enable the USARTx Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable USART1 and GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* Configure the GPIOs */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Rx (PA.10) as input floating */
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	//GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitTypeDef USART_InitStructure;

	/* USART1 configuration ------------------------------------------------------*/
	/* USART1 configured as follow:
	 - BaudRate = 115200 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - No parity
	 - Hardware flow control disabled (RTS and CTS signals)
	 - Receive and transmit enabled
	 - USART Clock disabled
	 - USART CPOL: Clock is active low
	 - USART CPHA: Data is captured on the middle
	 - USART LastBit: The clock pulse of the last data bit is not output to
	 the SCLK pin
	 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); /* enable RX interrupt */

	/* Enable USART1 */
	USART_Cmd(USART1, ENABLE);

	QS_tickPeriod_ = SystemCoreClock / BSP_TICKS_PER_SEC;
	QS_tickTime_ = QS_tickPeriod_; /* to start the timestamp at zero */

	return 1U; /* return success */
}
/*..........................................................................*/
void QS_onCleanup(void) {
}
/*..........................................................................*/
QSTimeCtr QS_onGetTime(void) { /* NOTE: invoked with interrupts DISABLED */
	if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) { /* not set? */
		return QS_tickTime_ - (QSTimeCtr) SysTick->VAL;
	} else { /* the rollover occured, but the SysTick_ISR did not run yet */
		return QS_tickTime_ + QS_tickPeriod_ - (QSTimeCtr) SysTick->VAL;
	}
}
/*..........................................................................*/
void QS_onFlush(void) {
	uint16_t b;

	QF_INT_DISABLE();
	while ((b = QS_getByte()) != QS_EOD) { /* while not End-Of-Data... */
		QF_INT_ENABLE();
		while ((USART1->SR & USART_FLAG_TXE) == 0) { /* while TXE not empty */
		}
		USART1->DR = (b & 0xFFU); /* put into the DR register */
		QF_INT_DISABLE();
	}
	QF_INT_ENABLE();
}
/*..........................................................................*/
/*! callback function to reset the target (to be implemented in the BSP) */
void QS_onReset(void) {
	NVIC_SystemReset();
}
/*..........................................................................*/
/*! callback function to execute a user command (to be implemented in BSP) */
void QS_onCommand(uint8_t cmdId, uint32_t param1, uint32_t param2,
		uint32_t param3) {
	QS_BEGIN_ID(QS_USER + 1U, 0U)
	/* app-specific record */
			QS_U8(2, cmdId);
			QS_U32(8, param1);
			QS_U32(8, param2);
			QS_U32(8, param3);QS_END()
}

#endif /* Q_SPY */
/*--------------------------------------------------------------------------*/

/* error-handling function called by exception handlers in the startup code */
void Q_onAssert(char const *module, int loc) {
	/* TBD: damage control */
	(void) module; /* avoid the "unused parameter" compiler warning */
	(void) loc; /* avoid the "unused parameter" compiler warning */
	// GPIOF_AHB->DATA_Bits[LED_RED | LED_GREEN | LED_BLUE] = 0xFFU; /* all ON */
#ifndef NDEBUG /* debug build? */
	while (loc != 0) { /* tie the CPU in this endless loop */
	}
#endif
	NVIC_SystemReset(); /* reset the CPU */
}

/*****************************************************************************
* NOTE1:
* The QF_AWARE_ISR_CMSIS_PRI constant from the QF port specifies the highest
* ISR priority that is disabled by the QF framework. The value is suitable
* for the NVIC_SetPriority() CMSIS function.
*
* Only ISRs prioritized at or below the QF_AWARE_ISR_CMSIS_PRI level (i.e.,
* with the numerical values of priorities equal or higher than
* QF_AWARE_ISR_CMSIS_PRI) are allowed to call any QF services. These ISRs
* are "QF-aware".
*
* Conversely, any ISRs prioritized above the QF_AWARE_ISR_CMSIS_PRI priority
* level (i.e., with the numerical values of priorities less than
* QF_AWARE_ISR_CMSIS_PRI) are never disabled and are not aware of the kernel.
* Such "QF-unaware" ISRs cannot call any QF services. The only mechanism
* by which a "QF-unaware" ISR can communicate with the QF framework is by
* triggering a "QF-aware" ISR, which can post/publish events.
*
*/
