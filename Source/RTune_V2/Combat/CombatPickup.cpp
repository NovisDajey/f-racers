// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

ACombatPickup::ACombatPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Root);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetupAttachment(Root);
	Collision->SetSphereRadius(120.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void ACombatPickup::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ACombatPickup::HandleOverlap);
	SetPickupActive(bIsAvailable);
}

void ACombatPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsAvailable || !OtherActor)
	{
		return;
	}

	if (UVehicleCombatComponent* CombatComponent = OtherActor->FindComponentByClass<UVehicleCombatComponent>())
	{
		UE_LOG(LogTemp, Log, TEXT("CombatPickup: %s picked up %s (Amount=%d) from %s"),
			*GetNameSafe(OtherActor),
			*UEnum::GetValueAsString(PickupType),
			ChargeAmount,
			*GetNameSafe(this));
		CombatComponent->AddCharge(PickupType, ChargeAmount);
		ConsumePickup();
	}
}

void ACombatPickup::ConsumePickup()
{
	UE_LOG(LogTemp, Log, TEXT("CombatPickup: %s consumed. Respawn in %.2f seconds"),
		*GetNameSafe(this),
		RespawnTime);
	SetPickupActive(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RespawnTimer, this, &ACombatPickup::RespawnPickup, RespawnTime, false);
	}
}

void ACombatPickup::RespawnPickup()
{
	UE_LOG(LogTemp, Log, TEXT("CombatPickup: %s respawned"), *GetNameSafe(this));
	SetPickupActive(true);
}

void ACombatPickup::SetPickupActive(bool bActive)
{
	bIsAvailable = bActive;
	PickupMesh->SetVisibility(bActive, true);
	PickupMesh->SetHiddenInGame(!bActive, true);
	Collision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	SetActorEnableCollision(bActive);
}

void ACombatPickup::OnRep_IsAvailable()
{
	SetPickupActive(bIsAvailable);
}

void ACombatPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatPickup, bIsAvailable);
}
