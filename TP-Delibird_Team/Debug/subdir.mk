################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Comunicacion.c \
../Entrenadores.c \
../Planificador.c \
../Team.c 

OBJS += \
./Comunicacion.o \
./Entrenadores.o \
./Planificador.o \
./Team.o 

C_DEPS += \
./Comunicacion.d \
./Entrenadores.d \
./Planificador.d \
./Team.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


