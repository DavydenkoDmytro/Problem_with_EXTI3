#include "qpc.h"
#include "bsp.h"
#include "stm32f10x.h"

/*..........................................................................*/
int main()
{
	SystemInit();                 //Hardware dependant system initialisation

	QF_init(); /* initialize the framework and the underlying RT kernel */
	BSP_init(); /* initialize the Board Support Package */

	return QF_run(); /* run the QF application */
}
