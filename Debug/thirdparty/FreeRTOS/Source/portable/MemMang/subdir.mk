################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.c 

OBJS += \
./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.o 

C_DEPS += \
./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.d 


# Each subdirectory must supply rules for building sources it contributes
thirdparty/FreeRTOS/Source/portable/MemMang/%.o thirdparty/FreeRTOS/Source/portable/MemMang/%.su thirdparty/FreeRTOS/Source/portable/MemMang/%.cyclo: ../thirdparty/FreeRTOS/Source/portable/MemMang/%.c thirdparty/FreeRTOS/Source/portable/MemMang/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I"C:/Users/harih/STM32CubeIDE/workspace_1.16.0/Self_i2c/thirdparty/FreeRTOS/Source/include" -I"C:/Users/harih/STM32CubeIDE/workspace_1.16.0/Self_i2c/thirdparty/FreeRTOS/Source/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-thirdparty-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang

clean-thirdparty-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang:
	-$(RM) ./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.cyclo ./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.d ./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.o ./thirdparty/FreeRTOS/Source/portable/MemMang/heap_4.su

.PHONY: clean-thirdparty-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang
