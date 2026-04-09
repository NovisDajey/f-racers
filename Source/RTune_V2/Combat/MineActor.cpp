// Fill out your copyright notice in the Description page of Project Settings.

#include "MineActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AMineActor::AMineActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetSphereRadius(150.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AMineActor::InitializeMine(AActor* InOwningVehicle, const FCombatEffectData& InEffectData)
{
	OwningVehicle = InOwningVehicle;
	EffectData = InEffectData;
}

void AMineActor::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMineActor::HandleOverlap);

	const FVector Location = GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("MineActor: BeginPlay for %s. Owner=%s Location=(X=%.3f Y=%.3f Z=%.3f) ArmingDelay=%.2f"),
		*GetName(),
		OwningVehicle ? *OwningVehicle->GetName() : TEXT("None"),
		Location.X, Location.Y, Location.Z,
		ArmingDelay);

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ArmingTimer, this, &AMineActor::ArmMine, ArmingDelay, false);
		}
	}
}

void AMineActor::ArmMine()
{
	bArmed = true;
	const FVector Location = GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("MineActor: %s armed at Location=(X=%.3f Y=%.3f Z=%.3f)"),
		*GetName(),
		Location.X, Location.Y, Location.Z);
}

void AMineActor::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bArmed || !OtherActor || OtherActor == this || OtherActor == OwningVehicle)
	{
		return;
	}

	UVehicleCombatComponent* CombatComponent = OtherActor->FindComponentByClass<UVehicleCombatComponent>();
	if (!CombatComponent)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("MineActor: %s overlapped %s at Location=(X=%.3f Y=%.3f Z=%.3f). Armed=%s"),
		*GetName(),
		*OtherActor->GetName(),
		Location.X, Location.Y, Location.Z,
		bArmed ? TEXT("true") : TEXT("false"));

	CombatComponent->ApplyDamageAndSlow(OwningVehicle, EffectData);
	UE_LOG(LogTemp, Log, TEXT("MineActor: Destroying %s after applying effect to %s"), *GetName(), *OtherActor->GetName());
	Destroy();
}
