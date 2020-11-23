


/* header definition ******************************************************** */
#ifndef XDK110_EXTERNAL_H_
#define XDK110_EXTERNAL_H_

/* public interface declaration ********************************************* */
#include "BCDS_Retcode.h"
/* public type and macro definitions */

/* public function prototype declarations */

/**
 * @brief The function initializes BMG160 sensor and creates, starts timer task in autoreloaded mode
 * every three second which reads and prints the Gyro sensor data .
 *
 * @retval Retcode_T RETCODE_OK Initialization Success
 *
 * @retval Retcode_T Composed RETCODE error Initialization Failed,use Retcode_getCode() API to get the error code
 *
 */
char* processExternalData(void * param1, uint32_t param2);
void externalSensorInit(void);



/* public global variable declarations */

/* inline function definitions */

#endif /* XDK110_EXTERNAL_H_ */

/** ************************************************************************* */
