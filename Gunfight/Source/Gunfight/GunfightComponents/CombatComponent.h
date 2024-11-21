// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class AGunfightCharacterDeprecated;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(class AWeapon* WeaponToEquip, bool bLeftController);

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY();
	class AGunfightCharacterDeprecated* Character;

	bool CanPickupGun(class UMotionControllerComponent* MotionController);

public:	
	
};
