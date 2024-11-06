#include "main.h"
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void I2C_Config(void);
uint8_t MPU6050_Init(void);
void MPU6050_Read_Accel(float* ax, float* ay, float* az);
void UpdateLEDs(float x, float y, float z);

static void task_mpu6050_read(void* parameters);
static void task_led_update(void* parameters);

#define MPU6050_ADDR 0xD0
#define ACCEL_XOUT_H 0x3B

I2C_HandleTypeDef hi2c1;

// Shared variables for accelerometer data
float ax = 0, ay = 0, az = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    I2C_Config();

    // Initialize all LEDs to ON state to verify they're working
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_SET);
    HAL_Delay(1000);  // Keep them on for 1 second
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

    // Initialize MPU6050
    if (MPU6050_Init() != HAL_OK) {
        // If initialization fails, blink red LED rapidly
        while (1) {
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
            HAL_Delay(100);
        }
    }

    // Create tasks
    xTaskCreate(task_mpu6050_read, "MPU6050 Read", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(task_led_update, "LED Update", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Start the scheduler
    vTaskStartScheduler();

    // We should never get here as control is now taken by the scheduler
    while (1)
    {



    }
}
uint8_t MPU6050_Init(void)
{
    uint8_t check;
    uint8_t Data;

    // Check device ID
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x75, 1, &check, 1, 1000);

    if (check == 0x68)  // 0x68 will be returned by the sensor if everything goes well
    {
        // Power management register 0X6B we should write all 0's to wake the sensor up
        Data = 0;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x6B, 1, &Data, 1, 1000);

        // Set DATA RATE of 1KHz by writing SMPLRT_DIV register
        Data = 0x07;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x19, 1, &Data, 1, 1000);

        // Set accelerometer configuration in ACCEL_CONFIG Register
        // XA_ST=0,YA_ST=0,ZA_ST=0, FS_SEL=0 -> ± 2g
        Data = 0x00;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, 0x1C, 1, &Data, 1, 1000);

        return HAL_OK;
    }
    return HAL_ERROR;
}

void MPU6050_Read_Accel(float* ax, float* ay, float* az)


{
    uint8_t Rec_Data[6];
    int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;



    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H, 1, Rec_Data, 6, 1000);

    Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);


    *ax = Accel_X_RAW / 16384.0;
    *ay = Accel_Y_RAW / 16384.0;
    *az = Accel_Z_RAW / 16384.0;
}

void UpdateLEDs(float x, float y, float z)




{
    const float threshold = 0.3;  // Lowered the threshold for easier triggering


    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);






    if (x > threshold || x < -threshold) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);  // Green LED for X-axis
    }
    if (y > threshold || y < -threshold) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);  // Orange LED for Y-axis
    }
    if (z > threshold) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);  // Red LED for positive Z-axis
    } else if (z < -threshold) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);  // Blue LED for negative Z-axis
    }
}
static void task_mpu6050_read(void* parameters)
{
    while (1)






    {
        MPU6050_Read_Accel(&ax, &ay, &az);
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay












    }

}

static void task_led_update(void* parameters)
{
    while (1)
    {
        UpdateLEDs(ax, ay, az);
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay
    }
}


void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);



    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)





    {
        Error_Handler();


    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();



    // LED pins config
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // I2C pins config
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void I2C_Config(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{


    if (htim->Instance == TIM5) {
        HAL_IncTick();
    }


}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
