################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.local

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_QUOTED += \
"../Generated_Code/Cpu.c" \
"../Generated_Code/Flash.c" \
"../Generated_Code/clockManager1.c" \
"../Generated_Code/dmaController1.c" \
"../Generated_Code/lpTmr1.c" \
"../Generated_Code/lpit1.c" \
"../Generated_Code/lpspi0_adc.c" \
"../Generated_Code/lpspi2_dac.c" \
"../Generated_Code/lpuart0.c" \
"../Generated_Code/pin_mux.c" \

C_SRCS += \
../Generated_Code/Cpu.c \
../Generated_Code/Flash.c \
../Generated_Code/clockManager1.c \
../Generated_Code/dmaController1.c \
../Generated_Code/lpTmr1.c \
../Generated_Code/lpit1.c \
../Generated_Code/lpspi0_adc.c \
../Generated_Code/lpspi2_dac.c \
../Generated_Code/lpuart0.c \
../Generated_Code/pin_mux.c \

C_DEPS_QUOTED += \
"./Generated_Code/Cpu.d" \
"./Generated_Code/Flash.d" \
"./Generated_Code/clockManager1.d" \
"./Generated_Code/dmaController1.d" \
"./Generated_Code/lpTmr1.d" \
"./Generated_Code/lpit1.d" \
"./Generated_Code/lpspi0_adc.d" \
"./Generated_Code/lpspi2_dac.d" \
"./Generated_Code/lpuart0.d" \
"./Generated_Code/pin_mux.d" \

OBJS_QUOTED += \
"./Generated_Code/Cpu.o" \
"./Generated_Code/Flash.o" \
"./Generated_Code/clockManager1.o" \
"./Generated_Code/dmaController1.o" \
"./Generated_Code/lpTmr1.o" \
"./Generated_Code/lpit1.o" \
"./Generated_Code/lpspi0_adc.o" \
"./Generated_Code/lpspi2_dac.o" \
"./Generated_Code/lpuart0.o" \
"./Generated_Code/pin_mux.o" \

C_DEPS += \
./Generated_Code/Cpu.d \
./Generated_Code/Flash.d \
./Generated_Code/clockManager1.d \
./Generated_Code/dmaController1.d \
./Generated_Code/lpTmr1.d \
./Generated_Code/lpit1.d \
./Generated_Code/lpspi0_adc.d \
./Generated_Code/lpspi2_dac.d \
./Generated_Code/lpuart0.d \
./Generated_Code/pin_mux.d \

OBJS_OS_FORMAT += \
./Generated_Code/Cpu.o \
./Generated_Code/Flash.o \
./Generated_Code/clockManager1.o \
./Generated_Code/dmaController1.o \
./Generated_Code/lpTmr1.o \
./Generated_Code/lpit1.o \
./Generated_Code/lpspi0_adc.o \
./Generated_Code/lpspi2_dac.o \
./Generated_Code/lpuart0.o \
./Generated_Code/pin_mux.o \

OBJS += \
./Generated_Code/Cpu.o \
./Generated_Code/Flash.o \
./Generated_Code/clockManager1.o \
./Generated_Code/dmaController1.o \
./Generated_Code/lpTmr1.o \
./Generated_Code/lpit1.o \
./Generated_Code/lpspi0_adc.o \
./Generated_Code/lpspi2_dac.o \
./Generated_Code/lpuart0.o \
./Generated_Code/pin_mux.o \


# Each subdirectory must supply rules for building sources it contributes
Generated_Code/Cpu.o: ../Generated_Code/Cpu.c
	@echo 'Building file: $<'
	@echo 'Executing target #1 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/Cpu.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/Cpu.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/Flash.o: ../Generated_Code/Flash.c
	@echo 'Building file: $<'
	@echo 'Executing target #2 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/Flash.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/Flash.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/clockManager1.o: ../Generated_Code/clockManager1.c
	@echo 'Building file: $<'
	@echo 'Executing target #3 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/clockManager1.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/clockManager1.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/dmaController1.o: ../Generated_Code/dmaController1.c
	@echo 'Building file: $<'
	@echo 'Executing target #4 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/dmaController1.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/dmaController1.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/lpTmr1.o: ../Generated_Code/lpTmr1.c
	@echo 'Building file: $<'
	@echo 'Executing target #5 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/lpTmr1.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/lpTmr1.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/lpit1.o: ../Generated_Code/lpit1.c
	@echo 'Building file: $<'
	@echo 'Executing target #6 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/lpit1.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/lpit1.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/lpspi0_adc.o: ../Generated_Code/lpspi0_adc.c
	@echo 'Building file: $<'
	@echo 'Executing target #7 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/lpspi0_adc.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/lpspi0_adc.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/lpspi2_dac.o: ../Generated_Code/lpspi2_dac.c
	@echo 'Building file: $<'
	@echo 'Executing target #8 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/lpspi2_dac.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/lpspi2_dac.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/lpuart0.o: ../Generated_Code/lpuart0.c
	@echo 'Building file: $<'
	@echo 'Executing target #9 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/lpuart0.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/lpuart0.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Generated_Code/pin_mux.o: ../Generated_Code/pin_mux.c
	@echo 'Building file: $<'
	@echo 'Executing target #10 $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Generated_Code/pin_mux.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "Generated_Code/pin_mux.o" "$<"
	@echo 'Finished building: $<'
	@echo ' '


