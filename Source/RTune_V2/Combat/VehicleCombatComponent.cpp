// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleCombatComponent.h"

#include "MineActor.h"
#include "RocketProjectile.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/RTuneVehicle.h"
#include "Engine/World.h"
#include "TimerManager.h"

UVehicleCombatComponent::UVehicleCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	RocketEffect.Damage = 20.f;
	RocketEffect.SlowMultiplier = 0.45f;
	RocketEffect.SlowDuration = 2.f;

	MineEffect.Damage = 15.f;
	MineEffect.SlowMultiplier = 0.55f;
	MineEffect.SlowDuration = 2.5f;
}

void UVehicleCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeCombat();
}

void UVehicleCombatComponent::InitializeCombat()
{
	CachedVehicle = Cast<ARTuneVehicle>(GetOwner());
	CurrentHP = MaxHP;

	if (CachedVehicle)
	{
		DefaultExternalResistanceForce = CachedVehicle->GetExternalResistanceForce();
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		StartRefillTimers();
	}

	BroadcastHealthChanged();
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::StartRefillTimers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(RocketRefillTimer, this, &UVehicleCombatComponent::HandleRocketRefill, RocketCooldown, true);
	World->GetTimerManager().SetTimer(MineRefillTimer, this, &UVehicleCombatComponent::HandleMineRefill, MineCooldown, true);
	World->GetTimerManager().SetTimer(ShieldRefillTimer, this, &UVehicleCombatComponent::HandleShieldRefill, ShieldCooldown, true);
}

void UVehicleCombatComponent::HandleRocketRefill()
{
	AddCharge(ECombatItemType::Rocket, 1);
}

void UVehicleCombatComponent::HandleMineRefill()
{
	AddCharge(ECombatItemType::Mine, 1);
}

void UVehicleCombatComponent::HandleShieldRefill()
{
	AddCharge(ECombatItemType::Shield, 1);
}

void UVehicleCombatComponent::AddCharge(ECombatItemType Type, int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return;
	}

	int32* ChargePtr = nullptr;
	switch (Type)
	{
	case ECombatItemType::Rocket:
		ChargePtr = &RocketCharges;
		break;
	case ECombatItemType::Mine:
		ChargePtr = &MineCharges;
		break;
	case ECombatItemType::Shield:
		ChargePtr = &ShieldCharges;
		break;
	default:
		break;
	}

	if (!ChargePtr)
	{
		return;
	}

	*ChargePtr = FMath::Clamp(*ChargePtr + Amount, 0, GetMaxCharges(Type));
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Added %d charge(s) of %s to %s. Counts => Rocket:%d Mine:%d Shield:%d"),
		Amount,
		*UEnum::GetValueAsString(Type),
		*GetNameSafe(GetOwner()),
		RocketCharges,
		MineCharges,
		ShieldCharges);
	BroadcastInventoryChanged();
}

bool UVehicleCombatComponent::CanUseItem(ECombatItemType Type) const
{
	return GetCharges(Type) > 0;
}

bool UVehicleCombatComponent::ConsumeCharge(ECombatItemType Type)
{
	bool bConsumed = false;

	if (!CanUseItem(Type))
	{
		return false;
	}

	switch (Type)
	{
	case ECombatItemType::Rocket:
		--RocketCharges;
		bConsumed = true;
		break;
	case ECombatItemType::Mine:
		--MineCharges;
		bConsumed = true;
		break;
	case ECombatItemType::Shield:
		--ShieldCharges;
		bConsumed = true;
		break;
	default:
		break;
	}

	if (bConsumed)
	{
		UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Consumed 1 charge of %s on %s. Remaining => Rocket:%d Mine:%d Shield:%d"),
			*UEnum::GetValueAsString(Type),
			*GetNameSafe(GetOwner()),
			RocketCharges,
			MineCharges,
			ShieldCharges);
		BroadcastInventoryChanged();
	}

	return bConsumed;
}

bool UVehicleCombatComponent::UseRocket()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: UseRocket requested on %s. Authority=%s"),
		*GetNameSafe(GetOwner()),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));

	if (!CachedVehicle)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseRocket failed because CachedVehicle is null on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerUseRocket();
		return true;
	}

	if (!RocketClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseRocket failed because RocketClass is not assigned on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!ConsumeCharge(ECombatItemType::Rocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseRocket failed because there are no rocket charges on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetOwner()->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLocation = GetSpawnLocationOffset(FVector(RocketForwardSpawnOffset, 0.f, 0.f));
	const FRotator SpawnRotation = GetOwner()->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AActor* TargetActor = FindRocketTarget();

	ARocketProjectile* Rocket = GetWorld()->SpawnActorDeferred<ARocketProjectile>(RocketClass, SpawnTransform, GetOwner(), GetOwner()->GetInstigator(), SpawnParams.SpawnCollisionHandlingOverride);
	if (!Rocket)
	{
		++RocketCharges;
		BroadcastInventoryChanged();
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: Rocket spawn failed for %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	Rocket->InitializeProjectile(GetOwner(), TargetActor, RocketEffect, RocketInitialFlightSpeed, RocketMaxFlightSpeed,
		RocketAccelerationRate, RocketHomingAccelerationMagnitude, RocketStraightFlightDelay, bRocketUseHomingIfTargetAvailable);
	Rocket->FinishSpawning(SpawnTransform);
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Rocket spawned successfully for %s at %s. Target=%s"),
		*GetNameSafe(GetOwner()),
		*SpawnLocation.ToString(),
		*GetNameSafe(TargetActor));
	return true;
}

bool UVehicleCombatComponent::UseMine()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: UseMine requested on %s. Authority=%s"),
		*GetNameSafe(GetOwner()),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));

	if (!CachedVehicle)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseMine failed because CachedVehicle is null on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerUseMine();
		return true;
	}

	if (!MineClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseMine failed because MineClass is not assigned on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!ConsumeCharge(ECombatItemType::Mine))
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseMine failed because there are no mine charges on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetOwner()->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLocation = GetSpawnLocationOffset(FVector(-MineBackwardSpawnOffset, 0.f, 0.f), MineVerticalSpawnOffset);
	const FRotator SpawnRotation = GetOwner()->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	AMineActor* Mine = GetWorld()->SpawnActorDeferred<AMineActor>(MineClass, SpawnTransform, GetOwner(), GetOwner()->GetInstigator(), SpawnParams.SpawnCollisionHandlingOverride);
	if (!Mine)
	{
		++MineCharges;
		BroadcastInventoryChanged();
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: Mine spawn failed for %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	Mine->InitializeMine(GetOwner(), MineEffect);
	Mine->FinishSpawning(SpawnTransform);
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Mine spawned successfully for %s at %s"),
		*GetNameSafe(GetOwner()),
		*SpawnLocation.ToString());
	return true;
}

bool UVehicleCombatComponent::UseShield()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: UseShield requested on %s. Authority=%s"),
		*GetNameSafe(GetOwner()),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));

	if (!CachedVehicle)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseShield failed because CachedVehicle is null on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		ServerUseShield();
		return true;
	}

	if (bShieldActive || !ConsumeCharge(ECombatItemType::Shield))
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleCombatComponent: UseShield failed because shield is active or there are no shield charges on %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	ActivateShield();
	return true;
}

bool UVehicleCombatComponent::ApplyDamageAndSlow(AActor* DamageCauser, const FCombatEffectData& Effect)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (bShieldActive)
	{
		UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Damage from %s blocked by shield on %s"),
			*GetNameSafe(DamageCauser),
			*GetNameSafe(GetOwner()));
		return false;
	}

	CurrentHP = FMath::Clamp(CurrentHP - FMath::Max(0.f, Effect.Damage), 0.f, MaxHP);
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: %s took %.2f damage from %s. HP => %.2f / %.2f"),
		*GetNameSafe(GetOwner()),
		Effect.Damage,
		*GetNameSafe(DamageCauser),
		CurrentHP,
		MaxHP);
	BroadcastHealthChanged();
	ApplySlow(Effect.SlowMultiplier, Effect.SlowDuration);

	if (CurrentHP <= 0.f)
	{
		HandleDeath();
	}

	return true;
}

void UVehicleCombatComponent::ActivateShield()
{
	bShieldActive = true;
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Shield activated on %s for %.2f seconds"),
		*GetNameSafe(GetOwner()),
		ShieldDuration);
	BroadcastInventoryChanged();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShieldTimer);
		World->GetTimerManager().SetTimer(ShieldTimer, this, &UVehicleCombatComponent::DeactivateShield, ShieldDuration, false);
	}
}

void UVehicleCombatComponent::DeactivateShield()
{
	bShieldActive = false;
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: Shield deactivated on %s"), *GetNameSafe(GetOwner()));
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::ApplySlow(float SlowMultiplier, float Duration)
{
	if (!CachedVehicle)
	{
		return;
	}

	ActiveSlowMultiplier = FMath::Clamp(FMath::Min(ActiveSlowMultiplier, SlowMultiplier), 0.f, 1.f);
	RefreshVehicleMovementState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlowTimer);
		World->GetTimerManager().SetTimer(SlowTimer, this, &UVehicleCombatComponent::ClearSlow, Duration, false);
	}
}

void UVehicleCombatComponent::ClearSlow()
{
	ActiveSlowMultiplier = 1.f;
	RefreshVehicleMovementState();
}

void UVehicleCombatComponent::RefreshVehicleMovementState()
{
	if (!CachedVehicle)
	{
		return;
	}

	const bool bSlowed = ActiveSlowMultiplier < 1.f;
	const float AppliedResistance = bSlowed ? DefaultExternalResistanceForce + SlowResistanceForce * (1.f - ActiveSlowMultiplier) : DefaultExternalResistanceForce;
	CachedVehicle->SetExternalResistanceForce(AppliedResistance);
}

void UVehicleCombatComponent::HandleDeath()
{
	ClearSlow();
}

AActor* UVehicleCombatComponent::FindRocketTarget() const
{
	if (!GetWorld() || !GetOwner())
	{
		return nullptr;
	}

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector OwnerForward = GetOwner()->GetActorForwardVector().GetSafeNormal();
	AActor* BestTarget = nullptr;
	float BestDot = RocketTargetingConeDot;
	float BestDistanceSq = TNumericLimits<float>::Max();
	const float MaxRangeSq = FMath::Square(RocketTargetingRange);

	for (TActorIterator<ARTuneVehicle> It(GetWorld()); It; ++It)
	{
		ARTuneVehicle* Candidate = *It;
		if (!Candidate || Candidate == GetOwner())
		{
			continue;
		}

		if (!Candidate->FindComponentByClass<UVehicleCombatComponent>())
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - OwnerLocation;
		const float DistanceSq = ToCandidate.SizeSquared();
		if (DistanceSq <= KINDA_SMALL_NUMBER || DistanceSq > MaxRangeSq)
		{
			continue;
		}

		const FVector DirectionToCandidate = ToCandidate.GetSafeNormal();
		const float Dot = FVector::DotProduct(OwnerForward, DirectionToCandidate);
		if (Dot < RocketTargetingConeDot)
		{
			continue;
		}

		const bool bIsBetterDot = Dot > BestDot + KINDA_SMALL_NUMBER;
		const bool bIsSimilarDotButCloser = FMath::IsNearlyEqual(Dot, BestDot, KINDA_SMALL_NUMBER) && DistanceSq < BestDistanceSq;
		if (bIsBetterDot || bIsSimilarDotButCloser)
		{
			BestTarget = Candidate;
			BestDot = Dot;
			BestDistanceSq = DistanceSq;
		}
	}

	return BestTarget;
}

FVector UVehicleCombatComponent::GetSpawnLocationOffset(const FVector& ForwardOffset, float VerticalOffset) const
{
	if (!GetOwner())
	{
		return FVector::ZeroVector;
	}

	return GetOwner()->GetActorLocation()
		+ GetOwner()->GetActorForwardVector() * ForwardOffset.X
		+ GetOwner()->GetActorRightVector() * ForwardOffset.Y
		+ GetOwner()->GetActorUpVector() * (ForwardOffset.Z + VerticalOffset);
}

int32 UVehicleCombatComponent::GetCharges(ECombatItemType Type) const
{
	switch (Type)
	{
	case ECombatItemType::Rocket:
		return RocketCharges;
	case ECombatItemType::Mine:
		return MineCharges;
	case ECombatItemType::Shield:
		return ShieldCharges;
	default:
		return 0;
	}
}

int32 UVehicleCombatComponent::GetMaxCharges(ECombatItemType Type) const
{
	switch (Type)
	{
	case ECombatItemType::Rocket:
		return MaxRocketCharges;
	case ECombatItemType::Mine:
		return MaxMineCharges;
	case ECombatItemType::Shield:
		return MaxShieldCharges;
	default:
		return 0;
	}
}

void UVehicleCombatComponent::ServerUseRocket_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: ServerUseRocket_Implementation on %s"), *GetNameSafe(GetOwner()));
	UseRocket();
}

void UVehicleCombatComponent::ServerUseMine_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: ServerUseMine_Implementation on %s"), *GetNameSafe(GetOwner()));
	UseMine();
}

void UVehicleCombatComponent::ServerUseShield_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("VehicleCombatComponent: ServerUseShield_Implementation on %s"), *GetNameSafe(GetOwner()));
	UseShield();
}

void UVehicleCombatComponent::OnRep_CurrentHP()
{
	BroadcastHealthChanged();
}

void UVehicleCombatComponent::OnRep_ShieldActive()
{
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::OnRep_RocketCharges()
{
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::OnRep_MineCharges()
{
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::OnRep_ShieldCharges()
{
	BroadcastInventoryChanged();
}

void UVehicleCombatComponent::BroadcastInventoryChanged()
{
	OnCombatInventoryChanged.Broadcast(RocketCharges, MineCharges, ShieldCharges, bShieldActive);
}

void UVehicleCombatComponent::BroadcastHealthChanged()
{
	OnCombatHealthChanged.Broadcast(CurrentHP, MaxHP);
}

void UVehicleCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVehicleCombatComponent, CurrentHP);
	DOREPLIFETIME(UVehicleCombatComponent, RocketCharges);
	DOREPLIFETIME(UVehicleCombatComponent, MineCharges);
	DOREPLIFETIME(UVehicleCombatComponent, ShieldCharges);
	DOREPLIFETIME(UVehicleCombatComponent, bShieldActive);
}
