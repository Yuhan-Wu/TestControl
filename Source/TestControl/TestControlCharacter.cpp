// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TestControlCharacter.h"

#include "TestCable.h"
#include "Bar.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ATestControlCharacter

#define TOTAL_RUNNING 100

ATestControlCharacter::ATestControlCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ATestControlCharacter::BeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ATestControlCharacter::EndOverlap);
	
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ArduinoInput = CreateDefaultSubobject<UArduinoInput>(TEXT("ArduinoInput"));

	HoldingComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HoldingComponent"));
	HoldingComponent->RelativeLocation.X = 50.0f;
	HoldingComponent->SetupAttachment(FP_MuzzleLocation);

	CurrentItem = NULL;
	bCanMove = true;
	bInspecting = false;
	
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)


}

//////////////////////////////////////////////////////////////////////////
// Input

void ATestControlCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATestControlCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATestControlCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATestControlCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATestControlCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATestControlCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATestControlCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATestControlCharacter::OnResetVR); \

	PlayerInputComponent->BindAction("HandsUp", IE_Pressed, this, &ATestControlCharacter::HandsUp);
	PlayerInputComponent->BindAction("HandsDown", IE_Released, this, &ATestControlCharacter::HandsDown);

}

void ATestControlCharacter::BeginPlay() {
	Super::BeginPlay();

	PitchMax = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax;
	PitchMin = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin;
}

void ATestControlCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FString getInput;
	if (isRunning) {
		running_counter++;
		if (running_counter != TOTAL_RUNNING) {
			ATestControlCharacter::MoveForward(0.07);
		}
		else {
			isRunning = false;
		}
		// This is for jump+run()
		// TODO Hopefully, we'll have time to refactor this part :)
		if (ArduinoInput->ReturnNextInputInQueue(getInput)) {
			if (getInput == "J") {
				ACharacter::Jump();
			}
		}
	}
	else if (ArduinoInput->ReturnNextInputInQueue(getInput)) {
		// TODO change to more graceful code after the rest is done
		if (getInput == "J") {
			//climb
			
			ACharacter::Jump();
		}
		else if (getInput == "W") {
			// swing
			
			isRunning = true;
			running_counter = 0;
			
		}
		else if (getInput == "U") {
			ATestControlCharacter::HandsUp();
		}
		else if (getInput == "D") {
			ATestControlCharacter::HandsDown();
		}
		else if (getInput == "1") {
			APawn::AddControllerYawInput(-10);
		}
		else if (getInput == "2") {
			APawn::AddControllerYawInput(10);
		}
	}

	Start = FollowCamera->GetComponentLocation();
	ForwardVector = FollowCamera->GetForwardVector();
	End = ((ForwardVector * 200.f) + Start);

	if (!bHoldingItem)
	{
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, DefaultComponentQueryParams, DefaultResponseParam))
		{
			if (Hit.GetActor()->GetClass()->IsChildOf(APickup::StaticClass()))
			{
				CurrentItem = Cast<APickup>(Hit.GetActor());
			}
		}
		else
		{
			CurrentItem = NULL;
		}
	}

	FollowCamera->SetFieldOfView(FMath::Lerp(FollowCamera->FieldOfView, 90.0f, 0.1f));

	if (bHoldingItem)
	{
		HoldingComponent->SetRelativeLocation(FVector(13.0f, 0.0f, 5.f));
	}
}


void ATestControlCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATestControlCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ATestControlCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ATestControlCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATestControlCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ATestControlCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATestControlCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATestControlCharacter::BeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult) {
	
	UE_LOG(LogTemp, Warning, TEXT("Enter"));
	// TODO I'll change this part after the controller is done
	// It's only for testing right now
	if (Cast<ATestCable>(OtherActor)) {
		GetCharacterMovement()->GravityScale = 0;
		GetCharacterMovement()->Velocity.Z = 0;
	}
	else if (Cast<ABar>(OtherActor)) {
		GetCharacterMovement()->GravityScale = 0;
		GetCharacterMovement()->Velocity.Z = 0;
		UE_LOG(LogTemp, Warning, TEXT("Overlap"));
	}
}

void ATestControlCharacter::EndOverlap (UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex){
}

void ATestControlCharacter::HandsUp() {
	areHandsUp = true;
	if (CurrentItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("PickUp"));
		ToggleItemPickUp();
	}
}

void ATestControlCharacter::ToggleItemPickUp() {
	if (CurrentItem)
	{
		bHoldingItem = !bHoldingItem;
		CurrentItem->PickUp();

		if (!bHoldingItem)
		{
			CurrentItem = NULL;
		}
	}
}

void ATestControlCharacter::HandsDown() {
	areHandsUp = false;
	bHoldingItem = false;
}

