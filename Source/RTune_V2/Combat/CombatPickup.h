// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleCombatComponent.h"
#include "CombatPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class RTUNE_V2_API ACombatPickup : public AActor
{
	GENERATED_BODY()

public:
	ACombatPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void ConsumePickup();
	void RespawnPickup();
	void SetPickupActive(bool bActive);

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	USphereComponent* Collision;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	ECombatItemType PickupType = ECombatItemType::Rocket;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "1"))
	int32 ChargeAmount = 1;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "0.1"))
	float RespawnTime = 5.f;

	UFUNCTION()
	void OnRep_IsAvailable();

	UPROPERTY(ReplicatedUsing = OnRep_IsAvailable, VisibleInstanceOnly, Category = "Pickup")
	bool bIsAvailable = true;

	FTimerHandle RespawnTimer;
};
