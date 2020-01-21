// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Pickup.generated.h"

UCLASS()
class TESTCONTROL_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* MyMesh;

	UPROPERTY(EditAnywhere)
	USceneComponent* HoldingComp;

	UFUNCTION()
	void PickUp();

	bool isHolding;
	bool bGravity;

	ACharacter* MyCharacter;
	UCameraComponent* PlayerCamera;
	FVector ForwardVector;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
