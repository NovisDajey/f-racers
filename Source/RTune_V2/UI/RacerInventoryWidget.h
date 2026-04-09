// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RacerInventoryWidget.generated.h"

class ARacerVehicle;
class UProgressBar;
class UTextBlock;
class UVehicleCombatComponent;

UCLASS(Abstract, Blueprintable)
class RTUNE_V2_API URacerInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetObservedVehicle(ARacerVehicle* InVehicle);

	UFUNCTION(BlueprintPure, Category = "UI")
	ARacerVehicle* GetObservedVehicle() const { return ObservedVehicle.Get(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleInventoryChanged(int32 RocketCharges, int32 MineCharges, int32 ShieldCharges, bool bShieldActive);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnInventoryPresentationUpdated(int32 RocketCharges, int32 MineCharges, int32 ShieldCharges, bool bShieldActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnHealthPresentationUpdated(float CurrentHP, float MaxHP, float HealthPercent);

private:
	void BindToCombatComponent();
	void UnbindFromCombatComponent();
	void RefreshFromCombatComponent();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RocketCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MineCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldStateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	TWeakObjectPtr<ARacerVehicle> ObservedVehicle;
	TWeakObjectPtr<UVehicleCombatComponent> ObservedCombatComponent;
};
