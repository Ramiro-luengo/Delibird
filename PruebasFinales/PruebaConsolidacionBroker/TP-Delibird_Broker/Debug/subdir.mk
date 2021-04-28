################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Broker.c \
../Broker_Cache.c \
../Broker_Envio.c \
../Broker_Recepcion.c 

OBJS += \
./Broker.o \
./Broker_Cache.o \
./Broker_Envio.o \
./Broker_Recepcion.o 

C_DEPS += \
./Broker.d \
./Broker_Cache.d \
./Broker_Envio.d \
./Broker_Recepcion.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


