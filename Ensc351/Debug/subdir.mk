################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ScopedMutex.cpp \
../ss_api.cpp 

C_SRCS += \
../AtomicConsole.c \
../SocketReadcond.c \
../VNPE.c 

OBJS += \
./AtomicConsole.o \
./ScopedMutex.o \
./SocketReadcond.o \
./VNPE.o \
./ss_api.o 

CPP_DEPS += \
./ScopedMutex.d \
./ss_api.d 

C_DEPS += \
./AtomicConsole.d \
./SocketReadcond.d \
./VNPE.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


