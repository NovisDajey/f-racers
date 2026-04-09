// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleCombatComponent.generated.h"

class AMineActor;
class ARocketProjectile;
class ARTuneVehicle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnCombatInventoryChanged, int32, RocketCharges, int32, MineCharges, int32, ShieldCharges, bool, bShieldActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatHealthChanged, float, CurrentHP, float, MaxHP);

UENUM(BlueprintType)
enum class ECombatItemType : uint8
{
	Rocket UMETA(DisplayName = "Rocket"),
	Mine UMETA(DisplayName = "Mine"),
	Shield UMETA(DisplayName = "Shield")
};

USTRUCT(BlueprintType)
struct FCombatEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float SlowDuration = 1.5f;
};

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class RTUNE_V2_API UVehicleCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVehicleCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AddCharge(ECombatItemType Type, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseRocket();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseMine();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool UseShield();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ApplyDamageAndSlow(AActor* DamageCauser, const FCombatEffectData& Effect);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanUseItem(ECombatItemType Type) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetRocketCharges() const { return RocketCharges; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetMineCharges() const { return MineCharges; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetShieldCharges() const { return ShieldCharges; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsShieldActive() const { return bShieldActive; }

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatInventoryChanged OnCombatInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatHealthChanged OnCombatHealthChanged;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerUseRocket();

	UFUNCTION(Server, Reliable)
	void ServerUseMine();

	UFUNCTION(Server, Reliable)
	void ServerUseShield();

	UFUNCTION()
	void OnRep_CurrentHP();

	UFUNCTION()
	void OnRep_ShieldActive();

	UFUNCTION()
	void OnRep_RocketCharges();

	UFUNCTION()
	void OnRep_MineCharges();

	UFUNCTION()
	void OnRep_ShieldCharges();

private:
	void BroadcastInventoryChanged();
	void BroadcastHealthChanged();
	void InitializeCombat();
	void StartRefillTimers();
	void HandleRocketRefill();
	void HandleMineRefill();
	void HandleShieldRefill();
	bool ConsumeCharge(ECombatItemType Type);
	void ActivateShield();
	void DeactivateShield();
	void ApplySlow(float SlowMultiplier, float Duration);
	void ClearSlow();
	void RefreshVehicleMovementState();
	void HandleDeath();
	AActor* FindRocketTarget() const;
	FVector GetSpawnLocationOffset(const FVector& ForwardOffset, float VerticalOffset = 0.f) const;

	int32 GetCharges(ECombatItemType Type) const;
	int32 GetMaxCharges(ECombatItemType Type) const;

	UPROPERTY(EditAnywhere, Category = "Combat|Health", meta = (ClampMin = "1.0"))
	float MaxHP = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Category = "Combat|Health")
	float CurrentHP = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_RocketCharges, VisibleInstanceOnly, Category = "Combat|Charges")
	int32 RocketCharges = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MineCharges, VisibleInstanceOnly, Category = "Combat|Charges")
	int32 MineCharges = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ShieldCharges, VisibleInstanceOnly, Category = "Combat|Charges")
	int32 ShieldCharges = 0;

	UPROPERTY(EditAnywhere, Category = "Combat|Charges", meta = (ClampMin = "1"))
	int32 MaxRocketCharges = 99;

	UPROPERTY(EditAnywhere, Category = "Combat|Charges", meta = (ClampMin = "1"))
	int32 MaxMineCharges = 99;

	UPROPERTY(EditAnywhere, Category = "Combat|Charges", meta = (ClampMin = "1"))
	int32 MaxShieldCharges = 99;

	UPROPERTY(EditAnywhere, Category = "Combat|Cooldowns", meta = (ClampMin = "0.1"))
	float RocketCooldown = 8.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Cooldowns", meta = (ClampMin = "0.1"))
	float MineCooldown = 10.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Cooldowns", meta = (ClampMin = "0.1"))
	float ShieldCooldown = 14.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Shield", meta = (ClampMin = "0.1"))
	float ShieldDuration = 4.f;

	UPROPERTY(ReplicatedUsing = OnRep_ShieldActive, VisibleInstanceOnly, Category = "Combat|Shield")
	bool bShieldActive = false;

	UPROPERTY(EditAnywhere, Category = "Combat|Projectiles")
	TSubclassOf<ARocketProjectile> RocketClass;

	UPROPERTY(EditAnywhere, Category = "Combat|Projectiles")
	TSubclassOf<AMineActor> MineClass;

	UPROPERTY(EditAnywhere, Category = "Combat|Effects")
	FCombatEffectData RocketEffect;

	UPROPERTY(EditAnywhere, Category = "Combat|Effects")
	FCombatEffectData MineEffect;

	UPROPERTY(EditAnywhere, Category = "Combat|Projectiles")
	float RocketForwardSpawnOffset = 250.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Targeting", meta = (ClampMin = "0.0"))
	float RocketTargetingRange = 6000.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Targeting", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float RocketTargetingConeDot = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight", meta = (ClampMin = "0.0"))
	float RocketInitialFlightSpeed = 2200.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight", meta = (ClampMin = "0.0"))
	float RocketMaxFlightSpeed = 5200.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight", meta = (ClampMin = "0.0"))
	float RocketAccelerationRate = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight", meta = (ClampMin = "0.0"))
	float RocketHomingAccelerationMagnitude = 18000.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight", meta = (ClampMin = "0.0"))
	float RocketStraightFlightDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Combat|Rocket|Flight")
	bool bRocketUseHomingIfTargetAvailable = true;

	UPROPERTY(EditAnywhere, Category = "Combat|Projectiles")
	float MineBackwardSpawnOffset = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Projectiles")
	float MineVerticalSpawnOffset = 12.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Slow", meta = (ClampMin = "0.0"))
	float SlowResistanceForce = 7500.f;

	UPROPERTY(Transient)
	ARTuneVehicle* CachedVehicle = nullptr;

	UPROPERTY(Transient)
	float DefaultExternalResistanceForce = 0.f;

	UPROPERTY(Transient)
	float ActiveSlowMultiplier = 1.f;

	FTimerHandle RocketRefillTimer;
	FTimerHandle MineRefillTimer;
	FTimerHandle ShieldRefillTimer;
	FTimerHandle ShieldTimer;
	FTimerHandle SlowTimer;
};
