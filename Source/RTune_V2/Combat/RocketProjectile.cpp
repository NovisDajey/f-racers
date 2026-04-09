// Fill out your copyright notice in the Description page of Project Settings.

#include "RocketProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ARocketProjectile::ARocketProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetupAttachment(Root);
	Collision->SetSphereRadius(32.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetNotifyRigidBodyCollision(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2200.f;
	ProjectileMovement->MaxSpeed = 5200.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bIsHomingProjectile = false;
}

void ARocketProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ProjectileMovement)
	{
		return;
	}

	TimeSinceLaunch += DeltaSeconds;
	UpdateFlightSpeed(DeltaSeconds);

	if (!bHomingActivated && TimeSinceLaunch >= StraightFlightDelay)
	{
		EnableHomingIfPossible();
	}
}

void ARocketProjectile::InitializeProjectile(AActor* InOwningVehicle, AActor* InTargetActor, const FCombatEffectData& InEffectData,
	float InInitialFlightSpeed, float InMaxFlightSpeed, float InAccelerationRate, float InHomingAccelerationMagnitude,
	float InStraightFlightDelay, bool bInUseHomingIfTargetAvailable)
{
	OwningVehicle = InOwningVehicle;
	TargetActor = InTargetActor;
	EffectData = InEffectData;
	InitialFlightSpeed = InInitialFlightSpeed;
	MaxFlightSpeed = InMaxFlightSpeed;
	AccelerationRate = InAccelerationRate;
	HomingAccelerationMagnitude = InHomingAccelerationMagnitude;
	StraightFlightDelay = InStraightFlightDelay;
	bUseHomingIfTargetAvailable = bInUseHomingIfTargetAvailable;
	CurrentFlightSpeed = InitialFlightSpeed;
	TimeSinceLaunch = 0.f;
	bHomingActivated = false;
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ARocketProjectile::HandleOverlap);
	Collision->OnComponentHit.AddDynamic(this, &ARocketProjectile::HandleHit);
	SetLifeSpan(LifeSeconds);

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialFlightSpeed;
		ProjectileMovement->MaxSpeed = MaxFlightSpeed;
		ProjectileMovement->Velocity = GetActorForwardVector() * InitialFlightSpeed;
	}

	const FVector Location = GetActorLocation();
	const FVector Velocity = ProjectileMovement ? ProjectileMovement->Velocity : FVector::ZeroVector;
	UE_LOG(LogTemp, Log, TEXT("RocketProjectile: BeginPlay for %s. Owner=%s Target=%s Location=(X=%.3f Y=%.3f Z=%.3f) Velocity=(X=%.3f Y=%.3f Z=%.3f)"),
		*GetName(),
		OwningVehicle ? *OwningVehicle->GetName() : TEXT("None"),
		TargetActor ? *TargetActor->GetName() : TEXT("None"),
		Location.X, Location.Y, Location.Z,
		Velocity.X, Velocity.Y, Velocity.Z);
}

void ARocketProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this || OtherActor == OwningVehicle)
	{
		return;
	}

	if (TryApplyImpactToActor(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Destroying %s after overlap with %s"), *GetName(), *OtherActor->GetName());
		Destroy();
	}
}

void ARocketProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	if (OtherActor && OtherActor != this && OtherActor != OwningVehicle)
	{
		if (TryApplyImpactToActor(OtherActor))
		{
			UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Destroying %s after hit on %s"), *GetName(), *OtherActor->GetName());
			Destroy();
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Destroying %s after world hit at Location=(X=%.3f Y=%.3f Z=%.3f)"),
		*GetName(),
		Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
	Destroy();
}

void ARocketProjectile::EnableHomingIfPossible()
{
	bHomingActivated = true;

	if (!bUseHomingIfTargetAvailable || !ProjectileMovement || !TargetActor)
	{
		return;
	}

	if (USceneComponent* TargetComponent = TargetActor->GetRootComponent())
	{
		ProjectileMovement->HomingTargetComponent = TargetComponent;
		ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
		ProjectileMovement->bIsHomingProjectile = true;
		UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Homing enabled for %s onto %s"), *GetName(), *TargetActor->GetName());
	}
}

void ARocketProjectile::UpdateFlightSpeed(float DeltaSeconds)
{
	if (!ProjectileMovement)
	{
		return;
	}

	CurrentFlightSpeed = FMath::Min(CurrentFlightSpeed + AccelerationRate * DeltaSeconds, MaxFlightSpeed);
	ProjectileMovement->MaxSpeed = MaxFlightSpeed;

	if (ProjectileMovement->Velocity.IsNearlyZero())
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * CurrentFlightSpeed;
		return;
	}

	ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * CurrentFlightSpeed;
}

bool ARocketProjectile::TryApplyImpactToActor(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == OwningVehicle)
	{
		return false;
	}

	if (UVehicleCombatComponent* CombatComponent = OtherActor->FindComponentByClass<UVehicleCombatComponent>())
	{
		CombatComponent->ApplyDamageAndSlow(OwningVehicle, EffectData);
		return true;
	}

	return false;
}
