// Fill out your copyright notice in the Description page of Project Settings.

#include "RacerInventoryWidget.h"

#include "../Combat/VehicleCombatComponent.h"
#include "../RacerVehicle.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void URacerInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ObservedVehicle.IsValid())
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			SetObservedVehicle(Cast<ARacerVehicle>(Pawn));
			return;
		}
	}

	BindToCombatComponent();
	RefreshFromCombatComponent();
}

void URacerInventoryWidget::NativeDestruct()
{
	UnbindFromCombatComponent();
	Super::NativeDestruct();
}

void URacerInventoryWidget::SetObservedVehicle(ARacerVehicle* InVehicle)
{
	if (ObservedVehicle.Get() == InVehicle)
	{
		return;
	}

	UnbindFromCombatComponent();
	ObservedVehicle = InVehicle;
	BindToCombatComponent();
	RefreshFromCombatComponent();
}

void URacerInventoryWidget::BindToCombatComponent()
{
	if (!ObservedVehicle.IsValid())
	{
		return;
	}

	UVehicleCombatComponent* CombatComponent = ObservedVehicle->GetCombatComponent();
	if (!CombatComponent)
	{
		return;
	}

	ObservedCombatComponent = CombatComponent;
	CombatComponent->OnCombatInventoryChanged.AddDynamic(this, &URacerInventoryWidget::HandleInventoryChanged);
	CombatComponent->OnCombatHealthChanged.AddDynamic(this, &URacerInventoryWidget::HandleHealthChanged);
}

void URacerInventoryWidget::UnbindFromCombatComponent()
{
	if (ObservedCombatComponent.IsValid())
	{
		ObservedCombatComponent->OnCombatInventoryChanged.RemoveDynamic(this, &URacerInventoryWidget::HandleInventoryChanged);
		ObservedCombatComponent->OnCombatHealthChanged.RemoveDynamic(this, &URacerInventoryWidget::HandleHealthChanged);
	}

	ObservedCombatComponent.Reset();
}

void URacerInventoryWidget::RefreshFromCombatComponent()
{
	if (!ObservedCombatComponent.IsValid())
	{
		return;
	}

	HandleInventoryChanged(
		ObservedCombatComponent->GetRocketCharges(),
		ObservedCombatComponent->GetMineCharges(),
		ObservedCombatComponent->GetShieldCharges(),
		ObservedCombatComponent->IsShieldActive());

	HandleHealthChanged(
		ObservedCombatComponent->GetCurrentHP(),
		ObservedCombatComponent->GetMaxHP());
}

void URacerInventoryWidget::HandleInventoryChanged(int32 RocketCharges, int32 MineCharges, int32 ShieldCharges, bool bShieldActive)
{
	if (RocketCountText)
	{
		RocketCountText->SetText(FText::AsNumber(RocketCharges));
	}

	if (MineCountText)
	{
		MineCountText->SetText(FText::AsNumber(MineCharges));
	}

	if (ShieldCountText)
	{
		ShieldCountText->SetText(FText::AsNumber(ShieldCharges));
	}

	if (ShieldStateText)
	{
		ShieldStateText->SetText(bShieldActive ? FText::FromString(TEXT("ON")) : FText::FromString(TEXT("OFF")));
	}

	OnInventoryPresentationUpdated(RocketCharges, MineCharges, ShieldCharges, bShieldActive);
}

void URacerInventoryWidget::HandleHealthChanged(float CurrentHP, float MaxHP)
{
	const float HealthPercent = MaxHP > 0.f ? CurrentHP / MaxHP : 0.f;

	if (HealthText)
	{
		const FText HealthValue = FText::Format(
			NSLOCTEXT("RacerInventoryWidget", "HealthFormat", "{0}/{1}"),
			FText::AsNumber(FMath::RoundToInt(CurrentHP)),
			FText::AsNumber(FMath::RoundToInt(MaxHP)));
		HealthText->SetText(HealthValue);
	}

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthPercent);
	}

	OnHealthPresentationUpdated(CurrentHP, MaxHP, HealthPercent);
}
