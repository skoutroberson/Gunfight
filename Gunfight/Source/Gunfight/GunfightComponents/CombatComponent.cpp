// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacterDeprecated.h"
#include "Gunfight/Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip, bool bLeftController)
{
	const FName HandToAttach = bLeftController ? FName("LeftHandWeapon") : FName("RightHandWeapon");
	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (WeaponToEquip && Character && CharacterMesh)
	{
		WeaponToEquip->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, HandToAttach);
	}
}

bool UCombatComponent::CanPickupGun(UMotionControllerComponent* MotionController)
{
	return false;
}

