################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Ensc351Part2-NO-main.cpp \
../src/Medium.cpp \
../src/PeerX.cpp \
../src/ReceiverSS.cpp \
../src/ReceiverX.cpp \
../src/SenderSS.cpp \
../src/SenderX.cpp \
../src/main.cpp \
../src/myIO.cpp 

OBJS += \
./src/Ensc351Part2-NO-main.o \
./src/Medium.o \
./src/PeerX.o \
./src/ReceiverSS.o \
./src/ReceiverX.o \
./src/SenderSS.o \
./src/SenderX.o \
./src/main.o \
./src/myIO.o 

CPP_DEPS += \
./src/Ensc351Part2-NO-main.d \
./src/Medium.d \
./src/PeerX.d \
./src/ReceiverSS.d \
./src/ReceiverX.d \
./src/SenderSS.d \
./src/SenderX.d \
./src/main.d \
./src/myIO.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/media/sf_U_ensc251/workspace-cpp-Neon3/workspace-cpp-Neon3_gary_running.zip_expanded/Ensc351" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


