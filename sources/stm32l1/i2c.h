#ifndef __STM32L1_I2C_H__
#define __STM32L1_I2C_H__

#include <i2c/i2c-interface.h>

typedef struct _stm32l1_i2c_t
{
	i2cif_t             i2cif;
	
	int                 port;
	I2C_t               *reg;
	mutex_t             error_mutex;
	mutex_t             trx_mutex;
	mutex_t             tx_dma_mutex;
	mutex_t             rx_dma_mutex;
	unsigned            cur_mode;
	i2c_transaction_t   *cur_trans;
} stm32l1_i2c_t;

// Нумерация портов начиная с 1.
int stm32l1_i2c_init(stm32l1_i2c_t *i2c, int port);

#endif // __STM32L1_I2C_H__
