# JCY8001 固件编译 Makefile
# 阶段1：最小通讯系统

# 工具链路径
TOOLCHAIN = /Users/jcy/toolchains/bin
CC = $(TOOLCHAIN)/arm-none-eabi-gcc
AS = $(TOOLCHAIN)/arm-none-eabi-as
LD = $(TOOLCHAIN)/arm-none-eabi-ld
OBJCOPY = $(TOOLCHAIN)/arm-none-eabi-objcopy
SIZE = $(TOOLCHAIN)/arm-none-eabi-size

# 项目路径
PROJECT_ROOT = /Users/jcy/Projects/JCY8001_new
CORE_DIR = $(PROJECT_ROOT)/Core
DRIVERS_DIR = $(PROJECT_ROOT)/Drivers
MODULES_DIR = $(PROJECT_ROOT)/Modules
UTILS_DIR = $(PROJECT_ROOT)/Utils
OUTPUT_DIR = $(PROJECT_ROOT)/build

# 目标设备
DEVICE = STM32F103RC
CPU = cortex-m3

# 编译选项
CFLAGS = -mcpu=$(CPU) -mthumb
CFLAGS += -Wall -O2 -g
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -DSTM32F103xE -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER

# 链接选项
LDFLAGS = -mcpu=$(CPU) -mthumb -T$(PROJECT_ROOT)/linker.ld
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(OUTPUT_DIR)/firmware.map
LDFLAGS += -specs=nosys.specs -specs=nano.specs

# 包含路径
INCLUDES = -I$(CORE_DIR)
INCLUDES += -I$(DRIVERS_DIR)/BSP
INCLUDES += -I$(DRIVERS_DIR)/CMSIS/Core/Include
INCLUDES += -I$(DRIVERS_DIR)/CMSIS/Device/ST/STM32F1xx/Include
INCLUDES += -I$(DRIVERS_DIR)/STM32F10x_StdPeriph_Driver/inc
INCLUDES += -I$(DRIVERS_DIR)/FreeRTOS/include
INCLUDES += -I$(DRIVERS_DIR)/FreeRTOS/portable/ARM_CM3
INCLUDES += -I$(MODULES_DIR)/Modbus
INCLUDES += -I$(MODULES_DIR)/CRC
INCLUDES += -I$(UTILS_DIR)

# 源文件
C_SOURCES = $(CORE_DIR)/main.c
C_SOURCES += $(CORE_DIR)/system_stm32f10x.c

# 标准外设库
C_SOURCES += $(DRIVERS_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
C_SOURCES += $(DRIVERS_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
C_SOURCES += $(DRIVERS_DIR)/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
C_SOURCES += $(DRIVERS_DIR)/STM32F10x_StdPeriph_Driver/src/misc.c

# 功能模块
C_SOURCES += $(MODULES_DIR)/CRC/CRC.c

# 汇编源文件
ASM_SOURCES = $(CORE_DIR)/startup_stm32f10x_hd.s

# 目标文件
OBJECTS = $(patsubst %.c,$(OUTPUT_DIR)/%.o,$(notdir $(C_SOURCES)))
OBJECTS += $(patsubst %.s,$(OUTPUT_DIR)/%.o,$(notdir $(ASM_SOURCES)))

# 输出文件
TARGET = $(OUTPUT_DIR)/firmware
TARGET_ELF = $(TARGET).elf
TARGET_HEX = $(TARGET).hex
TARGET_BIN = $(TARGET).bin

# 默认目标
all: $(TARGET_HEX) $(TARGET_BIN)

# 创建输出目录
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# 编译 C 文件
$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Core/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Core/%.s | $(OUTPUT_DIR)
	$(AS) -mcpu=$(CPU) -mthumb $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Drivers/BSP/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Modules/Modbus/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Modules/CRC/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Utils/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Drivers/FreeRTOS/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Drivers/FreeRTOS/portable/ARM_CM3/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Drivers/FreeRTOS/portable/MemMang/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_DIR)/%.o: $(PROJECT_ROOT)/Drivers/STM32F10x_StdPeriph_Driver/src/%.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 链接
$(TARGET_ELF): $(OBJECTS) | $(OUTPUT_DIR)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	$(SIZE) $@

# 生成 HEX 和 BIN 文件
$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) -O ihex $< $@

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

# 清理
clean:
	rm -rf $(OUTPUT_DIR)

# 烧录
flash: $(TARGET_HEX)
	JLinkExe -device $(DEVICE) -if swd -speed 4000 -CommanderScript $(PROJECT_ROOT)/flash.jlink

.PHONY: all clean flash
