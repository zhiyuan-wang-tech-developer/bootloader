################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Sources/bootloader.c" \
"../Sources/main.c" \
"../Sources/pc_communication.c" \

C_SRCS += \
../Sources/bootloader.c \
../Sources/main.c \
../Sources/pc_communication.c \

OBJS_OS_FORMAT += \
./Sources/bootloader.o \
./Sources/main.o \
./Sources/pc_communication.o \

C_DEPS_QUOTED += \
"./Sources/bootloader.d" \
"./Sources/main.d" \
"./Sources/pc_communication.d" \

OBJS += \
./Sources/bootloader.o \
./Sources/main.o \
./Sources/pc_communication.o \

OBJS_QUOTED += \
"./Sources/bootloader.o" \
"./Sources/main.o" \
"./Sources/pc_communication.o" \

C_DEPS += \
./Sources/bootloader.d \
./Sources/main.d \
./Sources/pc_communication.d \


# Each subdirectory must supply rules for building sources it contributes
Sources/bootloader.o: ../Sources/bootloader.c
	@echo 'Building file: $<'
	@echo 'Executing target #35 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/bootloader.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Sources/bootloader.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Sources/main.o: ../Sources/main.c
	@echo 'Building file: $<'
	@echo 'Executing target #36 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/main.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Sources/main.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Sources/pc_communication.o: ../Sources/pc_communication.c
	@echo 'Building file: $<'
	@echo 'Executing target #37 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Sources/pc_communication.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Sources/pc_communication.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '


