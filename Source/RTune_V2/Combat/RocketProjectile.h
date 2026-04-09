// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleCombatComponent.h"
#include "RocketProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class RTUNE_V2_API ARocketProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARocketProjectile();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeProjectile(AActor* InOwningVehicle, AActor* InTargetActor, const FCombatEffectData& InEffectData,
		float InInitialFlightSpeed, float InMaxFlightSpeed, float InAccelerationRate, float InHomingAccelerationMagnitude,
		float InStraightFlightDelay, bool bInUseHomingIfTargetAvailable);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

	void EnableHomingIfPossible();
	void UpdateFlightSpeed(float DeltaSeconds);
	bool TryApplyImpactToActor(AActor* OtherActor);

private:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSeconds = 6.f;

	UPROPERTY()
	AActor* OwningVehicle = nullptr;

	UPROPERTY()
	AActor* TargetActor = nullptr;

	UPROPERTY()
	FCombatEffectData EffectData;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight", meta = (ClampMin = "0.0"))
	float InitialFlightSpeed = 2200.f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight", meta = (ClampMin = "0.0"))
	float MaxFlightSpeed = 5200.f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight", meta = (ClampMin = "0.0"))
	float AccelerationRate = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight", meta = (ClampMin = "0.0"))
	float HomingAccelerationMagnitude = 18000.f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight", meta = (ClampMin = "0.0"))
	float StraightFlightDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Projectile|Flight")
	bool bUseHomingIfTargetAvailable = true;

	UPROPERTY(Transient)
	float CurrentFlightSpeed = 0.f;

	UPROPERTY(Transient)
	float TimeSinceLaunch = 0.f;

	UPROPERTY(Transient)
	bool bHomingActivated = false;
};
