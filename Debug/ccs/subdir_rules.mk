################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
ccs/%.obj: ../ccs/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/home/nicholas/ti/ccs930/ccs/tools/compiler/ti-cgt-arm_18.12.5.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/home/nicholas/Vineyard Based Sensor Networks/Meesters" --include_path="/home/nicholas/ti/ti/simplelink_msp432p4_sdk_3_40_01_02/source" --include_path="/home/nicholas/ti/ti/simplelink_msp432p4_sdk_3_40_01_02/source/third_party/CMSIS/Include" --include_path="/home/nicholas/ti/ccs930/ccs/tools/compiler/ti-cgt-arm_18.12.5.LTS/include" --advice:power=none --define=__MSP432P401R__ --define=DeviceFamily_MSP432P401x -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="ccs/$(basename $(<F)).d_raw" --obj_directory="ccs" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


