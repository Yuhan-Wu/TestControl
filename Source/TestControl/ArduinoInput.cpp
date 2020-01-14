// Fill out your copyright notice in the Description page of Project Settings.


#include "ArduinoInput.h"

// Sets default values for this component's properties
UArduinoInput::UArduinoInput()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UArduinoInput::BeginPlay()
{
	Super::BeginPlay();

	// ...
	PortOpen();
	if (!mySerialPort.OpenListenThread()) {
		UE_LOG(LogTemp, Warning, TEXT("OpenListenThread fail !"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("OpenListenThread success !"));
	}
}

void UArduinoInput::PortOpen() {
	if (!mySerialPort.InitPort(port, 9600, 'N', 8, 1, EV_RXCHAR))
	{
		UE_LOG(LogTemp, Warning, TEXT("initPort fail !"));
		// PortOpen();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("initPort success !"));
	}
}


// Called every frame
void UArduinoInput::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	// UINT BytesInQue = mySerialPort.GetBytesInCOM();
	AnalyzeInput();
	// Sleep(1000);
}

void UArduinoInput::AnalyzeInput() {
	char temp;
	FString instruction = "";
	while (mySerialPort.ReturnNextCharFromQueue(temp)) {
		instruction += temp;
		mySerialPort.ReturnNextCharFromQueue(temp);
		instruction += temp;
		UE_LOG(LogTemp, Warning, TEXT("%s"), *instruction);
		input_queue.Enqueue(instruction);
	}
}

bool UArduinoInput::ReturnNextInputInQueue(FString& return_value) {
	if (!input_queue.IsEmpty()) {
		input_queue.Dequeue(return_value);
		return true;
	}
	return false;
}

