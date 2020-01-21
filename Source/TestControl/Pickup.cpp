// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APickup::APickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("My Mesh"));
	MyMesh -> SetSimulatePhysics(true);
	RootComponent = MyMesh;

	isHolding = false;
	bGravity = true;
}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();

	MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	PlayerCamera = MyCharacter->FindComponentByClass<UCameraComponent>();

	TArray<USceneComponent*> Components;

	MyCharacter->GetComponents(Components);

	if (Components.Num() > 0)
	{
		for (auto& Comp : Components)
		{
			if (Comp->GetName() == "HoldingComponent")
			{
				HoldingComp = Cast<USceneComponent>(Comp);
			}
		}
	}
	
}

// Called every frame
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (isHolding && HoldingComp)
	{
		FVector location = MyCharacter->GetActorLocation();
		SetActorLocationAndRotation(location + HoldingComp->GetComponentLocation(), HoldingComp->GetComponentRotation());
	}
}

void APickup::PickUp() {
	isHolding = !isHolding;
	bGravity = !bGravity;
	MyMesh->SetEnableGravity(bGravity);
	MyMesh->SetSimulatePhysics(isHolding ? false : true);

	if (!isHolding)
	{
		ForwardVector = PlayerCamera->GetForwardVector();
		MyMesh->AddForce(ForwardVector * 1500 * MyMesh->GetMass());
	}
}

