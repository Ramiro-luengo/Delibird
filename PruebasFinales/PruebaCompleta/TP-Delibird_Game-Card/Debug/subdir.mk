################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Game-Card.c \
../Game-Card_ConexionBroker.c \
../Game-Card_ConexionGameBoy.c \
../Game-Card_Procesar_CATCH_CAUGHT.c \
../Game-Card_Procesar_GET_LOCALIZED.c \
../Game-Card_Procesar_NEW_APPEARED.c 

OBJS += \
./Game-Card.o \
./Game-Card_ConexionBroker.o \
./Game-Card_ConexionGameBoy.o \
./Game-Card_Procesar_CATCH_CAUGHT.o \
./Game-Card_Procesar_GET_LOCALIZED.o \
./Game-Card_Procesar_NEW_APPEARED.o 

C_DEPS += \
./Game-Card.d \
./Game-Card_ConexionBroker.d \
./Game-Card_ConexionGameBoy.d \
./Game-Card_Procesar_CATCH_CAUGHT.d \
./Game-Card_Procesar_GET_LOCALIZED.d \
./Game-Card_Procesar_NEW_APPEARED.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


