#include "main.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdio.h"
#include "fonts.h"
#include "ssd1306.h"



I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;


//MPU6050 I2C Address and Register Definitions
#define MPU6050_ADDR 0xD0
#define ACCEL_XOUT_H 0x3B
 //#define SSD1306_I2C_ADDR 0x3C
#define HIGHEST_TASK_PRIORITY      3    // Highest priority
#define LED_HIGH_PRIORITY      2    // Medium priority
#define LED_LOW_PRIORITY       1    // Lowest priority

#define MUTEX_CEILING_PRIORITY HIGHEST_TASK_PRIORITY  // Priority ceiling


// Shared variables for accelerometer data
float ax = 0, ay = 0, az = 0;

// Priority Ceiling Protocol Mutex for MPU6050 access
SemaphoreHandle_t xMPU6050Semaphore;

static UBaseType_t task_original_priority;
static UBaseType_t led_high_task_original_priority;
static UBaseType_t led_low_task_original_priority;
// Function prototypes
static void task_oled_update(void* parameters);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void I2C1_Config(void);
static void I2C2_Config(void);
uint8_t MPU6050_Init(void);
void MPU6050_Read_Accel(float* ax, float* ay, float* az);
void UpdateLEDs(float x, float y, float z);
static void task_mpu6050_read(void* parameters);
static void task_led_update(void* parameters);
void raise_priority_to_ceiling(TaskHandle_t task_handle);
void restore_task_priority(TaskHandle_t task_handle, UBaseType_t original_priority);


//MPU6050 initialization function
uint8_t MPU6050_Init(void) {
    uint8_t check;
    uint8_t Data;

    // Check device ID
    HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR, 0x75, 1, &check, 1, 1000);
    if (check == 0x68) {
        // Power up the sensor
        Data = 0;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, 0x6B, 1, &Data, 1, 1000);

        // Set sample rate
        Data = 0x07;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, 0x19, 1, &Data, 1, 1000);

        // Configure accelerometer
        Data = 0x00;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, 0x1C, 1, &Data, 1, 1000);

        return HAL_OK;
    }
    return HAL_ERROR;
}

// MPU6050 accelerometer read function
void MPU6050_Read_Accel(float* ax, float* ay, float* az) {
    uint8_t Rec_Data[6];
    int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;

    HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR, ACCEL_XOUT_H, 1, Rec_Data, 6, 1000);
    Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

    *ax = Accel_X_RAW / 16384.0;
    *ay = Accel_Y_RAW / 16384.0;
    *az = Accel_Z_RAW / 16384.0;
}



// LED update function
void UpdateLEDs(float x, float y, float z) {
    const float threshold = 0.3;
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

// Task to read accelerometer data
static void task_mpu6050_read(void* parameters) {
    while (1) {
    	TaskHandle_t current_task_handle = xTaskGetCurrentTaskHandle();
        if (xSemaphoreTake(xMPU6050Semaphore, portMAX_DELAY) == pdTRUE) {
        	raise_priority_to_ceiling(current_task_handle);
            MPU6050_Read_Accel(&ax, &ay, &az);
            xSemaphoreGive(xMPU6050Semaphore);
            restore_task_priority(current_task_handle, task_original_priority);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Task to update LEDs based on accelerometer data
static void task_led_update(void* parameters) {
    while (1) {
    	TaskHandle_t current_task_handle = xTaskGetCurrentTaskHandle();
    	if (xSemaphoreTake(xMPU6050Semaphore, portMAX_DELAY) == pdTRUE) {
    		raise_priority_to_ceiling(current_task_handle);

        	UpdateLEDs(ax, ay, az);
        	xSemaphoreGive(xMPU6050Semaphore);
        	restore_task_priority(current_task_handle, task_original_priority);

    	}
    	vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void task_oled_update(void* parameters) {
    char taskName[configMAX_TASK_NAME_LEN]; // Buffer to hold the task name

    while (1) {
        TaskHandle_t current_task_handle = xTaskGetCurrentTaskHandle();

        // Retrieve the name of the currently running task
        snprintf(taskName, configMAX_TASK_NAME_LEN, "%s", pcTaskGetName(current_task_handle));

        if (xSemaphoreTake(xMPU6050Semaphore, portMAX_DELAY) == pdTRUE) {
            //Raise task priority to ceiling level
            raise_priority_to_ceiling(current_task_handle);

            // Display "Current Task:" on the OLED
            SSD1306_GotoXY(0, 0);
            SSD1306_Puts("Current Task:", &Font_11x18, 1);

            // Display the current task name on the OLED
            SSD1306_GotoXY(0, 30);
            SSD1306_Puts(taskName, &Font_16x26, 1);

            // Update the screen to show the changes
            SSD1306_UpdateScreen();

            // Release the semaphore and restore the original task priority
            xSemaphoreGive(xMPU6050Semaphore);
            restore_task_priority(current_task_handle, task_original_priority);
        }

        HAL_Delay(50); // Delay to avoid frequent OLED updates
    }
}



void raise_priority_to_ceiling(TaskHandle_t task_handle)
{
    vTaskPrioritySet(task_handle, MUTEX_CEILING_PRIORITY);
}

/* Restore task priority after mutex release */
void restore_task_priority(TaskHandle_t task_handle, UBaseType_t original_priority)
{
    vTaskPrioritySet(task_handle, original_priority);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    I2C1_Config();
    I2C2_Config();
    SSD1306_Init();

    // Initialize MPU6050
    if (MPU6050_Init() != HAL_OK) {
        // Blink red LED rapidly if MPU6050 initialization fails
        while (1) {
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
            HAL_Delay(100);
        }
    }

    // Create mutex for MPU6050 access
    xMPU6050Semaphore = xSemaphoreCreateMutex();
    //xMPU6050Semaphore = xSemaphoreCreateCounting(5, 5);
    TaskHandle_t xTask1Handle, xTask2Handle, xTask3Handle;
    task_original_priority = HIGHEST_TASK_PRIORITY;
    led_high_task_original_priority = LED_HIGH_PRIORITY;
    led_low_task_original_priority = LED_LOW_PRIORITY;


    // Task creation
    xTaskCreate(task_mpu6050_read, "MPU6050_Read", configMINIMAL_STACK_SIZE, NULL, HIGHEST_TASK_PRIORITY, &xTask1Handle);

    xTaskCreate(task_led_update, "LED_Update", configMINIMAL_STACK_SIZE, NULL, LED_HIGH_PRIORITY, &xTask2Handle);


    xTaskCreate(task_oled_update, "OLED Update", configMINIMAL_STACK_SIZE, NULL, LED_LOW_PRIORITY, &xTask3Handle);
    vTaskSetDeadline(xTask1Handle, 500);
    vTaskSetDeadline(xTask2Handle, 500);
    vTaskSetDeadline(xTask3Handle, 500);





    //Start scheduler
    vTaskStartScheduler();




    // Main loop (should never reach here)
    while (1) {}
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

static void I2C1_Config(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

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
static void I2C2_Config(void){
	hi2c2.Instance = I2C2;
	hi2c2.Init.ClockSpeed = 100000;
	hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&hi2c2) != HAL_OK)
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
