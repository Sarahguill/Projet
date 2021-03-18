################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/bin/cl430" -vmsp --use_hw_mpy=none --include_path="C:/ti/ccs1011/ccs/ccs_base/msp430/include" --include_path="C:/Users/olliv_q9wubq4/OneDrive/Documents/OneDrive/Documents/ESIGELEC/2A/Cours/S8/ISEOC/BusCom/Projet_Serre_Co/PWM/ProjetPwm2" --include_path="C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-msp430_20.2.1.LTS/include" --advice:power=all --define=__MSP430G2553__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


