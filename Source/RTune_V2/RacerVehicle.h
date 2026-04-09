// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/RTuneVehicle.h"
#include "RacerVehicle.generated.h"

class UVehicleCombatComponent;
class URacerInventoryWidget;

UCLASS()
class RTUNE_V2_API ARacerVehicle : public ARTuneVehicle
{
	GENERATED_BODY()

public:
	ARacerVehicle();

	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "RacerVehicle|Combat")
	UVehicleCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "RacerVehicle|UI")
	URacerInventoryWidget* GetInventoryWidget() const { return InventoryWidgetInstance; }

	UFUNCTION(BlueprintPure, Category = "RacerVehicle|Combat")
	bool IsShieldActive() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "RacerVehicle|Effects")
	void OnShieldVisualStateChanged(bool bShieldNowActive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RacerVehicle|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleCombatComponent> CombatComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RacerVehicle|UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<URacerInventoryWidget> InventoryWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "RacerVehicle|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URacerInventoryWidget> InventoryWidgetInstance;

private:
	UFUNCTION()
	void HandleCombatInventoryChanged(int32 RocketCharges, int32 MineCharges, int32 ShieldCharges, bool bShieldNowActive);

	void BindCombatEvents();
	void CreateInventoryWidgetIfNeeded();
	void HandleUseRocket();
	void HandleUseMine();
	void HandleUseShield();

	bool bCachedShieldVisualState = false;
};
