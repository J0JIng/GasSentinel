################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c 

OBJS += \
./gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.o 

C_DEPS += \
./gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.d 


# Each subdirectory must supply rules for building sources it contributes
gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.o: ../gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM C Compiler'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m33 -mthumb -std=c99 '-DMGM240PB32VNA=1' '-DSL_COMPONENT_CATALOG_PRESENT=1' '-DMBEDTLS_CONFIG_FILE=<sl_mbedtls_config.h>' '-DOPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS=0' '-DOPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE="openthread-core-efr32-config-check.h"' '-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-efr32-config.h"' '-DOPENTHREAD_CONFIG_FILE="sl_openthread_generic_config.h"' '-DOPENTHREAD_MTD=1' '-DSL_OPENTHREAD_STACK_FEATURES_CONFIG_FILE="sl_openthread_features_config.h"' '-DBUFFER_SIZE_DOWN=0' '-DBUFFER_SIZE_UP=768' '-DMBEDTLS_PSA_CRYPTO_CONFIG_FILE=<psa_crypto_config.h>' '-DSL_RAIL_LIB_MULTIPROTOCOL_SUPPORT=0' '-DSL_RAIL_UTIL_PA_CONFIG_HEADER=<sl_rail_util_pa_config.h>' '-DRTT_USE_ASM=0' '-DSEGGER_RTT_SECTION="SEGGER_RTT"' '-DSLI_RADIOAES_REQUIRES_MASKING=1' -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\BME68x_API" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\BSEC2" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\config" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\autogen" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\Device\SiliconLabs\MGM24\Include" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\common\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\CMSIS\Core\Include" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\device_init\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emdrv\dmadrv\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emdrv\common\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emlib\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\plugin\fem_util" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emdrv\gpiointerrupt\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\hfxo_manager\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\driver\i2cspm\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\sl_mbedtls_support\config" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\sl_mbedtls_support\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\mbedtls\include" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\mbedtls\library" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\mpu\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emdrv\nvm3\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\src" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\src\cli" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\include" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\include\openthread" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\src\core" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\third_party\tcplp" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\examples\platforms" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\openthread\examples\platforms\utils" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\protocol\openthread\platform-abstraction\efr32" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\peripheral\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\power_manager\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\sl_psa_driver\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\sl_psa_driver\inc\public" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\common" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\protocol\ble" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\protocol\ieee802154" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\protocol\zwave" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\chip\efr32\efr32xg2x" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\plugin\pa-conversions" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\plugin\pa-conversions\efr32xg24" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\plugin\rail_util_pti" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\radio\rail_lib\plugin\rail_util_rssi" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\se_manager\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\se_manager\src" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\plugin\security_manager" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\third_party\segger\systemview\SEGGER" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\util\silicon_labs\silabs_core\memory_manager" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\common\toolchain\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\system\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\sleeptimer\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\security\sl_component\sl_protocol_crypto\src" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\emdrv\uartdrv\inc" -I"C:\Users\Workstation\Documents\GitHub\OdorGuard\Code\odorguard-sw\gecko_sdk_4.2.3\platform\service\udelay\inc" -Os -Wall -Wextra -ffunction-sections -fdata-sections -imacrossl_gcc_preinclude.h -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse -Wno-sign-compare -Wno-unused-parameter --specs=nano.specs -c -fmessage-length=0 -MMD -MP -MF"gecko_sdk_4.2.3/platform/emdrv/gpiointerrupt/src/gpiointerrupt.d" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


