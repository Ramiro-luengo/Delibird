################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: TP-Delibird_Broker

dependents:
	-cd /home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug && $(MAKE) all

# Tool invocations
TP-Delibird_Broker: $(OBJS) $(USER_OBJS) /home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/libTP-Delibird_SharedLib.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug" -o "TP-Delibird_Broker" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) TP-Delibird_Broker
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/libTP-Delibird_SharedLib.so:

-include ../makefile.targets
