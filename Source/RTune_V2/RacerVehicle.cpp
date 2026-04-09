// Fill out your copyright notice in the Description page of Project Settings.

#include "RacerVehicle.h"

#include "Combat/VehicleCombatComponent.h"
#include "UI/RacerInventoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"

ARacerVehicle::ARacerVehicle()
{
	CombatComponent = CreateDefaultSubobject<UVehicleCombatComponent>(TEXT("CombatComponent"));
}

void ARacerVehicle::BeginPlay()
{
	Super::BeginPlay();

	BindCombatEvents();
	CreateInventoryWidgetIfNeeded();
}

void ARacerVehicle::PawnClientRestart()
{
	Super::PawnClientRestart();

	CreateInventoryWidgetIfNeeded();
}

void ARacerVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CombatComponent)
	{
		CombatComponent->OnCombatInventoryChanged.RemoveDynamic(this, &ARacerVehicle::HandleCombatInventoryChanged);
	}

	if (InventoryWidgetInstance)
	{
		InventoryWidgetInstance->RemoveFromParent();
		InventoryWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ARacerVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAction(TEXT("UseRocket"), IE_Pressed, this, &ARacerVehicle::HandleUseRocket);
	PlayerInputComponent->BindAction(TEXT("UseMine"), IE_Pressed, this, &ARacerVehicle::HandleUseMine);
	PlayerInputComponent->BindAction(TEXT("UseShield"), IE_Pressed, this, &ARacerVehicle::HandleUseShield);
}

bool ARacerVehicle::IsShieldActive() const
{
	return CombatComponent && CombatComponent->IsShieldActive();
}

void ARacerVehicle::HandleCombatInventoryChanged(int32 RocketCharges, int32 MineCharges, int32 ShieldCharges, bool bShieldNowActive)
{
	if (bCachedShieldVisualState == bShieldNowActive)
	{
		return;
	}

	bCachedShieldVisualState = bShieldNowActive;
	OnShieldVisualStateChanged(bShieldNowActive);
}

void ARacerVehicle::BindCombatEvents()
{
	if (!CombatComponent)
	{
		return;
	}

	CombatComponent->OnCombatInventoryChanged.RemoveDynamic(this, &ARacerVehicle::HandleCombatInventoryChanged);
	CombatComponent->OnCombatInventoryChanged.AddDynamic(this, &ARacerVehicle::HandleCombatInventoryChanged);

	bCachedShieldVisualState = CombatComponent->IsShieldActive();
	OnShieldVisualStateChanged(bCachedShieldVisualState);
}

void ARacerVehicle::CreateInventoryWidgetIfNeeded()
{
	if (InventoryWidgetInstance || !InventoryWidgetClass || !IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	InventoryWidgetInstance = CreateWidget<URacerInventoryWidget>(PlayerController, InventoryWidgetClass);
	if (!InventoryWidgetInstance)
	{
		return;
	}

	InventoryWidgetInstance->SetObservedVehicle(this);
	InventoryWidgetInstance->AddToViewport();
}

void ARacerVehicle::HandleUseRocket()
{
	if (CombatComponent)
	{
		CombatComponent->UseRocket();
	}
}

void ARacerVehicle::HandleUseMine()
{
	if (CombatComponent)
	{
		CombatComponent->UseMine();
	}
}

void ARacerVehicle::HandleUseShield()
{
	if (CombatComponent)
	{
		CombatComponent->UseShield();
	}
}
