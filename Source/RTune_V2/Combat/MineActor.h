// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleCombatComponent.h"
#include "MineActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class RTUNE_V2_API AMineActor : public AActor
{
	GENERATED_BODY()

public:
	AMineActor();

	void InitializeMine(AActor* InOwningVehicle, const FCombatEffectData& InEffectData);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void ArmMine();

	UPROPERTY(VisibleAnywhere, Category = "Mine")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Mine")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Mine")
	USphereComponent* Trigger;

	UPROPERTY(EditAnywhere, Category = "Mine", meta = (ClampMin = "0.0"))
	float ArmingDelay = 0.5f;

	UPROPERTY()
	AActor* OwningVehicle = nullptr;

	UPROPERTY()
	FCombatEffectData EffectData;

	bool bArmed = false;

	FTimerHandle ArmingTimer;
};
