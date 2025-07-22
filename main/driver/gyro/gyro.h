#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#define I2C_MASTER_SCL_IO 18      /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 17      /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */
    void gyro_init();
#ifdef __cplusplus
}
#endif