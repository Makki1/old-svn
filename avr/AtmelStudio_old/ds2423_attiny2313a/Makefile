################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../ow_slave_DS2423.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
ow_slave_DS2423.o


OBJS_AS_ARGS +=  \
ow_slave_DS2423.o


C_DEPS +=  \
ow_slave_DS2423.d


C_DEPS_AS_ARGS +=  \
ow_slave_DS2423.d


OUTPUT_FILE_PATH +=ow_slave_DS2423.elf

OUTPUT_FILE_PATH_AS_ARGS +=ow_slave_DS2423.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler



./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler : (AVR_8_bit_GNU_Toolchain_3.4.1_830) 4.6.2
	$(QUOTE)C:\Program Files\Atmel\Atmel Studio 6.0\extensions\Atmel\AVRGCC\3.4.1.95\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -O2 -c -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -mmcu=attiny25   -o"$@" "$<" 
	@echo Finished building: $<



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU Linker : (AVR_8_bit_GNU_Toolchain_3.4.1_830) 4.6.2
	avr-gcc.exe -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="ow_slave_DS2423.map" -Wl,--start-group -Wl,-lm  -Wl,--end-group  -mmcu=attiny25  
	@echo Finished building target: $@
	avr-objcopy.exe -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "ow_slave_DS2423.elf" "ow_slave_DS2423.hex"
	avr-objcopy.exe -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "ow_slave_DS2423.elf" "ow_slave_DS2423.eep" || exit 0
	avr-objdump.exe -h -S "ow_slave_DS2423.elf" > "ow_slave_DS2423.lss"
	avr-objcopy.exe -O srec -R .eeprom -R .fuse -R .lock -R .signature  "ow_slave_DS2423.elf" "ow_slave_DS2423.srec"
	avr-size.exe "ow_slave_DS2423.elf"
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "ow_slave_DS2423.elf" "ow_slave_DS2423.a" "ow_slave_DS2423.hex" "ow_slave_DS2423.lss" "ow_slave_DS2423.eep" "ow_slave_DS2423.map" "ow_slave_DS2423.srec"
