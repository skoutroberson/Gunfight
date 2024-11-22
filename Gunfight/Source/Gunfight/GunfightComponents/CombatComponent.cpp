// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/Weapon/Weapon.h"
#include "GripMotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"

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

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip, bool bLeftController)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	
	if (!Character->IsLocallyControlled())
	{
		AttachWeaponToHand(bLeftController);
		Character->SetHandState(bLeftController, EHandState::EHS_HoldingPistol);
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetBeingGripped(true);
	UpdateWeaponStateOnPickup(EquippedWeapon);
}

void UCombatComponent::AttachWeaponToHand(bool bLeftHand)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || Character->GetWeapon() == nullptr) return;

	FName SocketName = bLeftHand ? FName("LeftHandWeapon") : FName("RightHandWeapon");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(Character->GetWeapon(), Character->GetMesh());
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{

}

bool UCombatComponent::CanPickupGun(bool bLeft)
{
	if (Character == nullptr || Character->GetWeapon() == nullptr || Character->GetWeapon()->IsBeingGripped()) return false;

	const UGripMotionControllerComponent* MotionController = bLeft ? Character->LeftMotionController : Character->RightMotionController;
	const float DistSquared = FVector::DistSquared(MotionController->GetComponentLocation(), Character->GetWeapon()->GetActorLocation());

	return DistSquared < FMath::Square(GunPickupDistance) ? true : false;
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{

}

void UCombatComponent::UpdateWeaponStateOnPickup(AWeapon* WeaponPickedUp)
{
	// if weapon ammo > 0, set state to ready
	// else, set state to empty
}
