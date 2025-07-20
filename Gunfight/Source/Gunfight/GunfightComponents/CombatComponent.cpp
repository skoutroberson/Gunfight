// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/Weapon/Weapon.h"
#include "GripMotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Gunfight/Weapon/FullMagazine.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/Gunfight.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GripWeapon(AWeapon* WeaponToGrip, bool bLeft)
{
	if (WeaponToGrip == nullptr || Character == nullptr) return;
	WeaponToGrip->SetOwner(Character);
	WeaponToGrip->SetCharacterOwner(Character);

	// Initial setup
	UMotionControllerComponent* CurrentMotionController = nullptr;
	USceneComponent* WeaponHandIK = nullptr;
	if (bLeft)
	{
		CurrentMotionController = Character->LeftMotionController;
		LeftEquippedWeapon = WeaponToGrip; // replicates to clients
		WeaponHandIK = WeaponToGrip->GrabSlot2IKL;
	}
	else
	{
		CurrentMotionController = Character->RightMotionController;
		RightEquippedWeapon = WeaponToGrip; // replicates to clients
		WeaponHandIK = WeaponToGrip->GrabSlot2IKR;
	}
	if (CurrentMotionController == nullptr) return;
	////

	bool bSlot1 = false;
	// First hand gripping the weapon
	if (WeaponToGrip->Slot1MotionController == nullptr)
	{
		WeaponToGrip->Slot1MotionController = CurrentMotionController; // replicates to clients
		WeaponToGrip->SetReplicateMovement(false);
		WeaponToGrip->SetWeaponState(EWeaponState::EWS_Equipped);
		HandleWeaponAttach(WeaponToGrip, bLeft);
		UpdateHUDWeaponPickup(bLeft, WeaponToGrip);
		bSlot1 = true;
		if (Character->IsLocallyControlled() && WeaponToGrip->GetWeaponMesh())
		{
			WeaponToGrip->GetWeaponMesh()->OverrideMinLOD(0);
			WeaponToGrip->GetWeaponMesh()->SetForcedLOD(1);
		}
		if (WeaponToGrip == LeftHolsteredWeapon) LeftHolsteredWeapon = nullptr;
		else if (WeaponToGrip == RightHolsteredWeapon) RightHolsteredWeapon = nullptr;
	}
	else // grabbing slot 2
	{
		WeaponToGrip->Slot2MotionController = CurrentMotionController;
		if (Character->IsLocallyControlled())
		{
			AttachHandMeshToWeapon(WeaponToGrip, bLeft, false);
			// TODO: Start rotating two hand
			//WeaponToEquip->StartRotatingTwoHand(OtherController, CurrentHandWeaponOffset);
		}
		else
		{
			// update UBIK to have the hand follow the weapon IK slot
			Character->UpdateAnimationHandComponent(bLeft, WeaponHandIK);
		}
	}
	Character->SetHandState(bLeft, SlotToHandState(WeaponToGrip->GetWeaponType(), bSlot1));
}

void UCombatComponent::ReleaseWeapon(bool bLeft)
{
	if (Character == nullptr) return;

	AWeapon*& CurrentWeapon = bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
	if (CurrentWeapon == nullptr) return;

	UMotionControllerComponent* CurrentMotionController = bLeft ? Character->LeftMotionController : Character->RightMotionController;
	UMotionControllerComponent* OtherMotionController = bLeft ? Character->RightMotionController : Character->LeftMotionController;
	if (CurrentMotionController == nullptr) return;

	// Slot1 Dropped
	if (CurrentWeapon->Slot1MotionController == CurrentMotionController)
	{
		CurrentWeapon->Slot1MotionController = nullptr;
		Character->AttachHandMeshToMotionController(bLeft);
		UpdateHUDWeaponDropped(bLeft);

		// The other hand is grabbing slot 2
		if (CurrentWeapon->Slot2MotionController && CurrentWeapon->Slot2MotionController == OtherMotionController)
		{
			// Swap Weapon to other hand
			CurrentWeapon->Slot1MotionController = OtherMotionController;
			CurrentWeapon->Slot2MotionController = nullptr;
			HandleWeaponAttach(CurrentWeapon, !bLeft);
			UpdateHUDWeaponPickup(!bLeft, CurrentWeapon);
		} 
		else if(!HolsterWeaponDontDrop(bLeft, CurrentWeapon)) // see if we should holster the weapon
		{
			CurrentWeapon->Dropped(bLeft);
		}
	} // slot2 dropped
	else if (CurrentWeapon->Slot2MotionController == CurrentMotionController)
	{
		CurrentWeapon->Slot2MotionController = nullptr;
		Character->AttachHandMeshToMotionController(bLeft);
		// TODO: stop rotating two hand
	}

	CurrentWeapon = nullptr;
	Character->SetHandState(bLeft, EHandState::EHS_Idle);
}

void UCombatComponent::UpdateHUDWeaponDropped(bool bLeft)
{
	Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
	if (Controller && Controller->IsLocalPlayerController())
	{
		Controller->SetHUDWeaponAmmoVisible(bLeft, false);
	}
}

void UCombatComponent::UpdateHUDWeaponPickup(bool bLeft, AWeapon* WeaponToGrip)
{
	Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
	if (Controller && Controller->IsLocalPlayerController())
	{
		Controller->SetHUDWeaponAmmoVisible(bLeft, true);
		WeaponToGrip->SetHUDAmmo(bLeft);
		WeaponToGrip->SetHUDCarriedAmmo(bLeft);
	}
}

void UCombatComponent::ReleaseWeaponClient(bool bLeft)
{
	if (Character == nullptr) return;

	AWeapon*& CurrentWeapon = bLeft ? PreviousLeftEquippedWeapon : PreviousRightEquippedWeapon;
	if (CurrentWeapon == nullptr) return;

	UMotionControllerComponent* CurrentMotionController = bLeft ? Character->LeftMotionController : Character->RightMotionController;
	if (CurrentMotionController == nullptr) return;

	if (CurrentWeapon->Slot1MotionController == CurrentMotionController)
	{
		CurrentWeapon->Slot1MotionController = nullptr;
		//CurrentWeapon->Dropped(bLeft);
		DetachHandMeshFromWeapon(bLeft);
	}

	Character->SetHandState(bLeft, EHandState::EHS_Idle);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		/*FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;*/

		FHitResult HitResult;
		TraceUnderLeftCrosshairs(HitResult);
		HitTargetLeft = HitResult.ImpactPoint;
		HitResult.Reset(0.f, false);
		TraceUnderRightCrosshairs(HitResult);
		HitTargetRight = HitResult.ImpactPoint;

		// two hand weapon rotation
		/*if (LeftEquippedWeapon && LeftEquippedWeapon->bRotateTwoHand)
		{
			RotateWeaponTwoHand(DeltaTime);
		}*/

		// client second grab check
		if (!Character->HasAuthority())
		{
			if (LeftEquippedWeapon && LeftEquippedWeapon->Slot2MotionController)
			{
				if (!Character->HasAuthority())
				{
					SecondHandAttachmentGrabCheck();
				}
				CheckSecondHand(LeftEquippedWeapon->Slot2MotionController);
			}
			else
			{
				SecondHandAttachmentDropCheck();
			}
		}

		// check if second hand should drop the weapon if the distance or rotation offset is too big
		if (LeftEquippedWeapon && LeftEquippedWeapon->Slot2MotionController)
		{
			CheckSecondHand(LeftEquippedWeapon->Slot2MotionController);
		}
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, LeftEquippedWeapon);
	DOREPLIFETIME(UCombatComponent, RightEquippedWeapon);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, LeftHolsteredWeapon);
	DOREPLIFETIME(UCombatComponent, RightHolsteredWeapon);
	//DOREPLIFETIME(UCombatComponent, OwnedWeapons);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip, bool bLeftController)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	//if (!Character->GetOverlappingWeapon() || !Character->GetOverlappingWeapon()->CheckHandOverlap(bLeftController) || CheckEquippedWeapon(!bLeftController)) return;

	if (bLeftController && LeftEquippedWeapon == nullptr)
	{
		EquipPrimaryWeapon(WeaponToEquip, true);
	}
	else if(!bLeftController && RightEquippedWeapon == nullptr)
	{
		EquipPrimaryWeapon(WeaponToEquip, false);
	}
	WeaponToEquip->SetHUDAmmo(bLeftController);
	WeaponToEquip->SetHUDCarriedAmmo(bLeftController);
}

void UCombatComponent::EquipWeaponSecondary(AWeapon* WeaponToEquip, bool bLeftController)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	USceneComponent* OtherController = nullptr;
	USceneComponent* WeaponHandIK = nullptr;
	if (bLeftController)
	{
		LeftEquippedWeapon = WeaponToEquip;
		OwnedWeapons.LeftEquippedWeapon = WeaponToEquip;
		LeftEquippedWeapon->Slot2MotionController = Character->LeftMotionController;
		OtherController = Character->RightMotionController;
		WeaponHandIK = WeaponToEquip->GrabSlot2IKL;
	}
	else
	{
		RightEquippedWeapon = WeaponToEquip;
		OwnedWeapons.RightEquippedWeapon = WeaponToEquip;
		RightEquippedWeapon->Slot2MotionController = Character->RightMotionController;
		OtherController = Character->LeftMotionController;
		WeaponHandIK = WeaponToEquip->GrabSlot2IKR;
	}

	// rotate weapon based on both hand controllers
	USceneComponent* CurrentHandWeaponOffset = Character->GetHandWeaponOffset(WeaponToEquip->GetWeaponType(), bLeftController);
	if (Character->IsLocallyControlled())
	{
		AttachHandMeshToWeapon(WeaponToEquip, bLeftController, false);
		//WeaponToEquip->StartRotatingTwoHand(OtherController, CurrentHandWeaponOffset);
	}
	else
	{
		// update UBIK to have the hand follow the weapon IK slot

		Character->UpdateAnimationHandComponent(bLeftController, WeaponHandIK);
		Character->SetHandState(bLeftController, SlotToHandState(WeaponToEquip->GetWeaponType(), false));
	}
}

void UCombatComponent::DropWeaponSecondary(AWeapon* WeaponToRelease, bool bLeftController)
{
	if (Character == nullptr || WeaponToRelease == nullptr) return;

	if (bLeftController)
	{
		LeftEquippedWeapon = nullptr;
		OwnedWeapons.LeftEquippedWeapon = nullptr;
	}
	else
	{
		RightEquippedWeapon = nullptr;
		OwnedWeapons.RightEquippedWeapon = nullptr;
	}

	WeaponToRelease->Slot2MotionController = nullptr;

	if (Character->IsLocallyControlled())
	{
		//WeaponToRelease->StopRotatingTwoHand();
		DetachHandMeshFromWeapon(bLeftController);
		WeaponToRelease->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		HandleWeaponAttach(WeaponToRelease, !bLeftController);
		WeaponToRelease->Slot2MotionController = nullptr;
	}
	else
	{

	}


	// stop rotating based on both hands.
}

void UCombatComponent::DropWeaponPrimary(AWeapon* WeaponToRelease, bool bLeftController)
{
	if (WeaponToRelease == nullptr) return;

	//WeaponToRelease->Dropped(bLeftController);
	//HandleWeaponDrop(true);

	if (bLeftController)
	{
		LeftEquippedWeapon = nullptr;
		OwnedWeapons.LeftEquippedWeapon = nullptr;
		OnRep_LeftEquippedWeapon();
	}
	else
	{
		RightEquippedWeapon = nullptr;
		OwnedWeapons.RightEquippedWeapon = nullptr;
		OnRep_RightEquippedWeapon();
	}

	/*
	WeaponToRelease->Slot2MotionController = nullptr;
	DetachHandMeshFromWeapon(bLeftController);
	WeaponToRelease->ClearHUDAmmo(bLeftController);
	*/
	// swap weapon to other hand
	EquipPrimaryWeapon(WeaponToRelease, !bLeftController);
	WeaponToRelease->SetHUDAmmo(!bLeftController);
	WeaponToRelease->SetHUDCarriedAmmo(!bLeftController);

	// stop rotating based on both hands.
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip, bool bLeftHand)
{
	if (WeaponToEquip == nullptr || Character == nullptr) return;
	
	if (bLeftHand)
	{
		LeftEquippedWeapon = WeaponToEquip;
		LeftEquippedWeapon->SetOwner(Character);
		LeftEquippedWeapon->SetCharacterOwner(Character);
		LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		LeftEquippedWeapon->SetWeaponSide(ESide::ES_Left);
		//LeftEquippedWeapon->SetHUDAmmo(true);
		LeftEquippedWeapon->Slot1MotionController = Character->LeftMotionController;
		PreviousLeftEquippedWeapon = LeftEquippedWeapon;

		OwnedWeapons.LeftEquippedWeapon = LeftEquippedWeapon;

		if (!LeftEquippedWeapon->IsMagInserted() && !LeftEquippedWeapon->GetMagazine())
		{
			Character->SpawnFullMagazine(LeftEquippedWeapon->GetFullMagazineClass(), LeftEquippedWeapon->GetCurrentSkinIndex(), true);
		}
		else if (LeftEquippedWeapon->IsMagInserted() && LeftEquippedWeapon->bSlideBack && LeftEquippedWeapon->GetAmmo() > 0)
		{
			LeftEquippedWeapon->PlaySlideForwardAnimation();
			LeftEquippedWeapon->PlayReloadSound();
		}
		//LeftEquippedWeapon->SetHUDAmmo(true);
		//AttachActorToHand(LeftEquippedWeapon, bLeftHand);
	}
	else
	{
		RightEquippedWeapon = WeaponToEquip;
		RightEquippedWeapon->SetOwner(Character);
		RightEquippedWeapon->SetCharacterOwner(Character);
		RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		RightEquippedWeapon->SetWeaponSide(ESide::ES_Right);
		//RightEquippedWeapon->SetHUDAmmo(false);
		RightEquippedWeapon->Slot1MotionController = Character->RightMotionController;
		PreviousRightEquippedWeapon = RightEquippedWeapon;

		OwnedWeapons.RightEquippedWeapon = RightEquippedWeapon;

		if (!RightEquippedWeapon->IsMagInserted() && !RightEquippedWeapon->GetMagazine())
		{
			Character->SpawnFullMagazine(RightEquippedWeapon->GetFullMagazineClass(), RightEquippedWeapon->GetCurrentSkinIndex(), false);
		}
		else if (RightEquippedWeapon->IsMagInserted() && RightEquippedWeapon->bSlideBack && RightEquippedWeapon->GetAmmo() > 0)
		{
			RightEquippedWeapon->PlaySlideForwardAnimation();
			RightEquippedWeapon->PlayReloadSound();
		}

		//AttachActorToHand(RightEquippedWeapon, bLeftHand);
	}
	Character->SetHandState(bLeftHand, EHandState::EHS_HoldingPistol);
	HandleWeaponAttach(WeaponToEquip, bLeftHand);

	// equipped weapon from holster
	if (WeaponToEquip->GetHolsterSide() == ESide::ES_Right)
	{
		RightHolsteredWeapon = nullptr;
		OwnedWeapons.RightHolsteredWeapon = nullptr;
	}
	else if (WeaponToEquip->GetHolsterSide() == ESide::ES_Left)
	{
		LeftHolsteredWeapon = nullptr;
		OwnedWeapons.LeftHolsteredWeapon = nullptr;
	}
	WeaponToEquip->SetHolsterSide(ESide::ES_None);

	// didn't equip weapon from holster. check if holsters are full or if we already have a two handed weapon in a holster
	bool bEquippingTwoHand = WeaponToEquip->IsTwoHanded();
	if (LeftHolsteredWeapon)
	{
		if (bEquippingTwoHand && LeftHolsteredWeapon->IsTwoHanded())
		{
			LeftHolsteredWeapon->Dropped(1);
			LeftHolsteredWeapon = nullptr;
			OwnedWeapons.LeftHolsteredWeapon = nullptr;
		}
	}
	if (RightHolsteredWeapon)
	{
		if (bEquippingTwoHand && RightHolsteredWeapon->IsTwoHanded())
		{
			RightHolsteredWeapon->Dropped(1);
			RightHolsteredWeapon = nullptr;
			OwnedWeapons.RightHolsteredWeapon = nullptr;
		}
	}
	// if we already have two weapons in our holsters, drop the weapon in the same side holster
	if (RightHolsteredWeapon && LeftHolsteredWeapon)
	{
		if (bLeftHand)
		{
			LeftHolsteredWeapon->Dropped(1);
			LeftHolsteredWeapon = nullptr;
			OwnedWeapons.LeftHolsteredWeapon = nullptr;
		}
		else
		{
			RightHolsteredWeapon->Dropped(1);
			RightHolsteredWeapon = nullptr;
			OwnedWeapons.RightHolsteredWeapon = nullptr;
		}
	}

	// if we are equipping our second gun and we already have another gun in hand, drop any holstered weapons.
	if (RightEquippedWeapon && LeftEquippedWeapon)
	{
		if (RightHolsteredWeapon)
		{
			RightHolsteredWeapon->Dropped(1);
			RightHolsteredWeapon = nullptr;
			OwnedWeapons.RightHolsteredWeapon = nullptr;
		}
		if (LeftHolsteredWeapon)
		{
			LeftHolsteredWeapon->Dropped(1);
			LeftHolsteredWeapon = nullptr;
			OwnedWeapons.LeftHolsteredWeapon = nullptr;
		}
	}

	Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
	if (Controller && Controller->IsLocalPlayerController())
	{
		Controller->SetHUDWeaponAmmoVisible(bLeftHand, true);
	}

	// TODO
	/*
		EquippedWeapon->SetHUDAmmo();
		PlayEquipWeaponSound(WeaponToEquip);
		WeaponToEquip->ClientUpdateAmmoOnPickup(WeaponToEquip->GetAmmo());
		ReloadEmptyWeapon();
	*/
}

void UCombatComponent::EquipMagazine(AFullMagazine* MagToEquip, bool bLeftController)
{
	if (bLeftController)
	{
		LeftEquippedMagazine = MagToEquip;

	}
	else if (!bLeftController)
	{
		RightEquippedMagazine = MagToEquip;
	}

	MagToEquip->Equipped();
	AttachMagazineToMotionController(MagToEquip, bLeftController);
	//AttachActorToHand(MagToEquip, bLeftController, MagToEquip->GetHandSocketOffset());
	
	// add magazine offset too i think
}

void UCombatComponent::DropMagazine(bool bLeftHand)
{
	if (bLeftHand && LeftEquippedMagazine)
	{
		LeftEquippedMagazine->Dropped();
		LeftEquippedMagazine = nullptr;
	}
	else if(!bLeftHand && RightEquippedMagazine)
	{
		RightEquippedMagazine->Dropped();
		RightEquippedMagazine = nullptr;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed, bool bLeftHand)
{
	bLeftHand ? bFireButtonPressedLeft = bPressed : bFireButtonPressedRight = bPressed;

	if (bPressed)
	{
		Fire(bLeftHand);
	}
}

void UCombatComponent::HandleWeaponAttach(AWeapon* WeaponToAttach, bool bLeftHand)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || WeaponToAttach == nullptr) return;

	if (Character->IsLocallyControlled())
	{
		AttachWeaponToMotionController(WeaponToAttach, bLeftHand);
		WeaponToAttach->PlayEquipSound();
	}
	else
	{
		AttachActorToHand(WeaponToAttach, bLeftHand);
	}
	Character->SetHandState(bLeftHand, SlotToHandState(WeaponToAttach->GetWeaponType(), true));
}

void UCombatComponent::AttachActorToHand(AActor* ActorToAttach, bool bLeftHand, FVector RelativeOffset)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const FName SocketName = bLeftHand ? FName("LeftHandWeapon") : FName("RightHandWeapon");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		//ActorToAttach->AttachToC
		ActorToAttach->SetActorRelativeLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
		ActorToAttach->SetActorRelativeRotation(FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
		ActorToAttach->AddActorLocalOffset(RelativeOffset, false, nullptr, ETeleportType::TeleportPhysics);
		//ActorToAttach->SetActorRelativeLocation(RelativeOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}
	//Character->SetHandState(bLeftHand, EHandState::EHS_HoldingPistol);
}

void UCombatComponent::AttachWeaponToMotionController(AWeapon* WeaponToAttach, bool bLeftHand)
{
	if (Character == nullptr || WeaponToAttach == nullptr) return;
	
	USceneComponent* WeaponOffset = Character->GetWeaponOffset(WeaponToAttach->GetWeaponType(), bLeftHand);
	if (WeaponOffset == nullptr) return;

	WeaponToAttach->AttachToComponent(WeaponOffset, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	USkeletalMeshComponent* HandMesh = nullptr;
	USceneComponent* GrabSlot = nullptr;

	// hands

	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		GrabSlot = WeaponToAttach->GrabSlot1L;
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		GrabSlot = WeaponToAttach->GrabSlot1R;
	}

	if (HandMesh == nullptr || GrabSlot == nullptr) return;

	HandMesh->AttachToComponent(GrabSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	//Character->SetHandState(bLeftHand, SlotToHandState(WeaponToAttach->GetWeaponType(), true));
}

void UCombatComponent::AttachHandMeshToWeapon(AWeapon* Weapon, bool bLeftHand, bool bSlot1)
{
	if (Character == nullptr || Weapon == nullptr) return;

	USkeletalMeshComponent* HandMesh = nullptr;
	USceneComponent* GrabSlot = nullptr;

	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		GrabSlot = bSlot1 ? Weapon->GrabSlot1L : Weapon->GrabSlot2L;
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		GrabSlot = bSlot1 ? Weapon->GrabSlot1R : Weapon->GrabSlot2R;
	}
	if (HandMesh == nullptr || GrabSlot == nullptr) return;

	//HandMesh->GetAnimInstance()->UpdateAnimation(0.f, false, UAnimInstance::EUpdateAnimationFlag::Default);
	EHandState HandState = SlotToHandState(Weapon->GetWeaponType(), bSlot1);
	Character->SetHandState(bLeftHand, HandState);
	HandMesh->AttachToComponent(GrabSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UCombatComponent::HandleWeaponDrop(bool bLeftHand)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || !Character->IsLocallyControlled()) return;
	USkeletalMeshComponent* HandMesh;
	UMotionControllerComponent* MotionController;
	FVector LocationOffset;
	FRotator RotationOffset;

	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		MotionController = Character->LeftMotionController.Get();
		LocationOffset = Character->LeftHandMeshLocationOffset;
		RotationOffset = Character->LeftHandMeshRotationOffset;
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		MotionController = Character->RightMotionController.Get();
		LocationOffset = Character->RightHandMeshLocationOffset;
		RotationOffset = Character->RightHandMeshRotationOffset;
	}

	if (HandMesh == nullptr || MotionController == nullptr) return;

	HandMesh->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HandMesh->SetRelativeLocationAndRotation(LocationOffset, RotationOffset);
}

void UCombatComponent::DetachHandMeshFromWeapon(bool bLeftHand)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || !Character->IsLocallyControlled()) return;

	USkeletalMeshComponent* HandMesh = nullptr;
	UMotionControllerComponent* MotionController = nullptr;
	USceneComponent* HandOffset = nullptr;
	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		MotionController = Character->LeftMotionController;
		HandOffset = Character->LeftHandMeshOffset;
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		MotionController = Character->RightMotionController;
		HandOffset = Character->RightHandMeshOffset;
	}

	if (HandMesh == nullptr || MotionController == nullptr) return;

	HandMesh->AttachToComponent(HandOffset, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UCombatComponent::DetachHandMeshFromWeapon(AWeapon* WeaponToDetachFrom)
{
	if (Character == nullptr || !Character->IsLocallyControlled() && WeaponToDetachFrom == nullptr) return;

	bool bLeft = false;
	if (WeaponToDetachFrom == LeftEquippedWeapon || WeaponToDetachFrom == PreviousLeftEquippedWeapon)
	{
		bLeft = true;
	}
	
}

void UCombatComponent::AttachMagazineToMotionController(AFullMagazine* MagToAttach, bool bLeftHand)
{
	if (Character == nullptr || MagToAttach == nullptr) return;
	UGripMotionControllerComponent* MotionController = bLeftHand ? Character->LeftMotionController.Get() : Character->RightMotionController.Get();
	if (MotionController == nullptr) return;

	FTransform OffsetTransform = bLeftHand ? MagToAttach->GrabOffsetLeft : MagToAttach->GrabOffsetRight;

	MagToAttach->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MagToAttach->SetActorRelativeTransform(OffsetTransform);
}

void UCombatComponent::HandleMagDrop(bool bLeftHand)
{
}

void UCombatComponent::OnRep_OwnedWeapons()
{
	if (PreviousLeftEquippedWeapon != OwnedWeapons.LeftEquippedWeapon)
	{
		LeftEquippedWeapon = OwnedWeapons.LeftEquippedWeapon;
		OnRep_LeftEquippedWeapon();
	}
	if (PreviousRightEquippedWeapon != OwnedWeapons.RightEquippedWeapon)
	{
		RightEquippedWeapon = OwnedWeapons.RightEquippedWeapon;
		OnRep_RightEquippedWeapon();
	}
	if(PreviousLeftHolsteredWeapon != OwnedWeapons.LeftHolsteredWeapon)
	{
		LeftHolsteredWeapon = OwnedWeapons.LeftHolsteredWeapon;
		OnRep_LeftHolsteredWeapon();
	}
	if (PreviousRightHolsteredWeapon != OwnedWeapons.RightHolsteredWeapon)
	{
		RightHolsteredWeapon = OwnedWeapons.RightHolsteredWeapon;
		OnRep_RightHolsteredWeapon();
	}

	PreviousLeftEquippedWeapon = OwnedWeapons.LeftEquippedWeapon;
	PreviousRightEquippedWeapon = OwnedWeapons.RightEquippedWeapon;
	PreviousLeftHolsteredWeapon = OwnedWeapons.LeftHolsteredWeapon;
	PreviousRightHolsteredWeapon = OwnedWeapons.RightHolsteredWeapon;
}

void UCombatComponent::OnRep_LeftEquippedWeapon()
{
	if (Character == nullptr || Character->LeftMotionController == nullptr) return;

	if (LeftEquippedWeapon)
	{
		//LeftEquippedWeapon->SetReplicateMovement(false);
		//LeftEquippedWeapon->StopPhysics();
		//LeftEquippedWeapon->AttachToComponent(Character->LeftMotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		if (PreviousLeftEquippedWeapon)
		{
			//PreviousLeftEquippedWeapon->SetReplicateMovement(true);
			//PreviousLeftEquippedWeapon->DetachAndSimulate(); // calls DetachFromActor and simulates physics to drop on the ground.
		}
	}
	PreviousLeftEquippedWeapon = LeftEquippedWeapon;
	/*
	if (LeftEquippedWeapon) // grabbed weapon
	{
		PreviousLeftEquippedWeapon = LeftEquippedWeapon;

		// grab slot 1
		if (LeftEquippedWeapon->Slot1MotionController == nullptr)
		{
			LeftEquippedWeapon->SetOwner(Character);
			LeftEquippedWeapon->SetCharacterOwner(Character);
			LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
			LeftEquippedWeapon->SetHUDAmmo(true);
			LeftEquippedWeapon->SetHUDCarriedAmmo(true);
			LeftEquippedWeapon->Slot1MotionController = Character->LeftMotionController;
			HandleWeaponAttach(LeftEquippedWeapon, true);
			Character->SetHandState(true, SlotToHandState(LeftEquippedWeapon->GetWeaponType(), true));

			if (!LeftEquippedWeapon->IsMagInserted() && !LeftEquippedWeapon->GetMagazine())
			{
				Character->SpawnFullMagazine(LeftEquippedWeapon->GetFullMagazineClass(), LeftEquippedWeapon->GetCurrentSkinIndex(), true);
			}
			else if (LeftEquippedWeapon->IsMagInserted() && LeftEquippedWeapon->bSlideBack && LeftEquippedWeapon->GetAmmo() > 0)
			{
				LeftEquippedWeapon->PlaySlideForwardAnimation();
				LeftEquippedWeapon->PlayReloadSound();
			}
			Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
			if (Controller && Controller->IsLocalPlayerController())
			{
				Controller->SetHUDWeaponAmmoVisible(true, true);
			}
		}
		else // grab slot 2
		{
			LeftEquippedWeapon->Slot2MotionController = Character->LeftMotionController;

			if (Character->IsLocallyControlled())
			{
				AttachHandMeshToWeapon(LeftEquippedWeapon, true, false);
			}
			else
			{

			}
			// local
			// attach hand mesh to weapon slot 2
			// rotate weapon based on both motion controllers

			// not local
			// Update hand IK to follow weapon IK slot
			// update skeletal mesh hand animation to weapon2
		}
		
		return;
	}
	else // dropped weapon
	{
		HandleWeaponDrop(true);

		// weapon dropped while other hand is still holding the weapon.
		if (RightEquippedWeapon && RightEquippedWeapon == PreviousLeftEquippedWeapon)
		{
			// slot 1 dropped
			if (PreviousLeftEquippedWeapon->Slot1MotionController && PreviousLeftEquippedWeapon->Slot1MotionController == Character->LeftMotionController)
			{
				// swap weapon to right hand

				HandleWeaponAttach(RightEquippedWeapon, false);
				RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
				RightEquippedWeapon->SetHUDAmmo(false);
				RightEquippedWeapon->SetHUDCarriedAmmo(false);
				RightEquippedWeapon->Slot1MotionController = Character->RightMotionController;
				RightEquippedWeapon->Slot2MotionController = nullptr;
				Character->SetHandState(false, SlotToHandState(RightEquippedWeapon->GetWeaponType(), true));

				Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
				if (Controller && Controller->IsLocalController())
				{
					Controller->SetHUDWeaponAmmoVisible(true, false);
					Controller->SetHUDWeaponAmmoVisible(false, true);
				}
			} //slot 2 was dropped
			else if (PreviousLeftEquippedWeapon && 
				PreviousLeftEquippedWeapon->Slot2MotionController && 
				PreviousLeftEquippedWeapon->Slot2MotionController == Character->LeftMotionController)
			{
				PreviousLeftEquippedWeapon->Slot2MotionController = nullptr;

				if (Character->IsLocallyControlled())
				{
					DetachHandMeshFromWeapon(true);
				}
				else
				{

				}
				// LOCAL
				// stop rotating based on both motion controllers

				// NOT LOCAL
				// set hand IK to follow the motion controller and not be attached to the weapon.
			}
		} 
		else if (PreviousLeftEquippedWeapon)
		{ 
			// slot1 dropped while not holding slot 2
			if (PreviousLeftEquippedWeapon->Slot1MotionController &&
				PreviousLeftEquippedWeapon->Slot1MotionController == Character->LeftMotionController)
			{
				PreviousLeftEquippedWeapon->Slot1MotionController = nullptr;

				Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
				if (Controller && Controller->IsLocalController())
				{
					Controller->SetHUDWeaponAmmoVisible(true, false);
				}

				
				PreviousLeftEquippedWeapon->Dropped(true);
				

				// replication order guard TEST

				//if (!PreviousLeftEquippedWeapon->GetWeaponMesh()->IsSimulatingPhysics()) // this should only occur if the replication came out of order and the weapon is attached to the hand but should be dropped
				//{
				//	PreviousLeftEquippedWeapon->Dropped(true);
				//}
			}
		}
	}
	*/
}

void UCombatComponent::OnRep_RightEquippedWeapon()
{
	if (Character == nullptr || Character->RightMotionController == nullptr) return;

	if (RightEquippedWeapon)
	{

		//RightEquippedWeapon->SetReplicateMovement(false);
		//RightEquippedWeapon->StopPhysics();
		//RightEquippedWeapon->AttachToComponent(Character->RightMotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		if (PreviousRightEquippedWeapon)
		{
			//PreviousRightEquippedWeapon->SetReplicateMovement(true);
			//PreviousRightEquippedWeapon->DetachAndSimulate(); // calls DetachFromActor and simulates physics to drop on the ground.
		}
	}
	PreviousRightEquippedWeapon = RightEquippedWeapon;

	/*
	if (RightEquippedWeapon) // grabbed weapon
	{
		PreviousRightEquippedWeapon = RightEquippedWeapon;

		// grab slot 1
		if (RightEquippedWeapon->Slot1MotionController == nullptr)
		{
			RightEquippedWeapon->SetOwner(Character);
			RightEquippedWeapon->SetCharacterOwner(Character);
			RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
			RightEquippedWeapon->SetHUDAmmo(false);
			RightEquippedWeapon->SetHUDCarriedAmmo(false);
			RightEquippedWeapon->Slot1MotionController = Character->RightMotionController;
			HandleWeaponAttach(RightEquippedWeapon, false);
			Character->SetHandState(false, SlotToHandState(RightEquippedWeapon->GetWeaponType(), true));

			if (!RightEquippedWeapon->IsMagInserted() && !RightEquippedWeapon->GetMagazine())
			{
				Character->SpawnFullMagazine(RightEquippedWeapon->GetFullMagazineClass(), RightEquippedWeapon->GetCurrentSkinIndex(), true);
			}
			else if (RightEquippedWeapon->IsMagInserted() && RightEquippedWeapon->bSlideBack && RightEquippedWeapon->GetAmmo() > 0)
			{
				RightEquippedWeapon->PlaySlideForwardAnimation();
				RightEquippedWeapon->PlayReloadSound();
			}
			Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
			if (Controller && Controller->IsLocalPlayerController())
			{
				Controller->SetHUDWeaponAmmoVisible(false, true);
			}
		}
		else // grab slot 2
		{
			RightEquippedWeapon->Slot2MotionController = Character->RightMotionController;

			if (Character->IsLocallyControlled())
			{
				AttachHandMeshToWeapon(RightEquippedWeapon, false, false);
			}
			else
			{

			}
			// local
			// attach hand mesh to weapon slot 2
			// rotate weapon based on both motion controllers

			// not local
			// Update hand IK to follow weapon IK slot
			// update skeletal mesh hand animation to weapon2
		}

		return;
	}
	else // dropped weapon
	{
		HandleWeaponDrop(false);

		// weapon dropped while other hand is still holding the weapon.
		if (LeftEquippedWeapon && LeftEquippedWeapon == PreviousRightEquippedWeapon)
		{
			// slot 1 dropped
			if (PreviousRightEquippedWeapon->Slot1MotionController && PreviousRightEquippedWeapon->Slot1MotionController == Character->RightMotionController)
			{
				// swap weapon to the left hand.

				
				HandleWeaponAttach(LeftEquippedWeapon, true);
				LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
				LeftEquippedWeapon->SetHUDAmmo(true);
				LeftEquippedWeapon->SetHUDCarriedAmmo(true);
				LeftEquippedWeapon->Slot1MotionController = Character->LeftMotionController;
				LeftEquippedWeapon->Slot2MotionController = nullptr;
				Character->SetHandState(true, SlotToHandState(LeftEquippedWeapon->GetWeaponType(), true));

				Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
				if (Controller && Controller->IsLocalController())
				{
					Controller->SetHUDWeaponAmmoVisible(false, false);
					Controller->SetHUDWeaponAmmoVisible(true, true);
				}
			} // slot 2 was dropped
			else if (PreviousRightEquippedWeapon && 
				PreviousRightEquippedWeapon->Slot2MotionController && 
				PreviousRightEquippedWeapon->Slot2MotionController == Character->RightMotionController)
			{
				PreviousRightEquippedWeapon->Slot2MotionController = nullptr;

				if (Character->IsLocallyControlled())
				{
					DetachHandMeshFromWeapon(false);
				}
				else
				{

				}
				// LOCAL
				// stop rotating based on both motion controllers
				
				// NOT LOCAL
				// set hand IK to follow the motion controller and not be attached to the weapon.
			}
		}
		else if(PreviousRightEquippedWeapon)
		{
			// slot1 dropped while not holding slot 2
			if (PreviousRightEquippedWeapon->Slot1MotionController && 
				PreviousRightEquippedWeapon->Slot1MotionController == Character->RightMotionController)
			{
				PreviousRightEquippedWeapon->Slot1MotionController = nullptr;
				Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
				if (Controller && Controller->IsLocalController())
				{
					Controller->SetHUDWeaponAmmoVisible(false, false);
				}

				
				PreviousRightEquippedWeapon->Dropped(false);
				
				
				// replication order guard TEST

				//if (!PreviousRightEquippedWeapon->GetWeaponMesh()->IsSimulatingPhysics()) // this should only occur if the replication came out of order and the weapon is attached to the hand but should be dropped
				//{
				//	PreviousRightEquippedWeapon->Dropped(true);
				//}
			} // slot 2 dropped SHOULDN'T EVER HAPPEN
			else if (PreviousRightEquippedWeapon->Slot2MotionController && 
				PreviousRightEquippedWeapon->Slot2MotionController == Character->RightMotionController)
			{
				PreviousRightEquippedWeapon->Slot2MotionController = nullptr;
			}
		}
	}
	*/
}

void UCombatComponent::OnRep_LeftHolsteredWeapon()
{
	if (Character == nullptr) return;

	if (LeftHolsteredWeapon)
	{
		if (Character->IsLocallyControlled() && !Character->IsEliminated())
		{
			LeftHolsteredWeapon->PlayHolsterSound();
		}
	}
	/*if (LeftHolsteredWeapon)
	{
		AttachWeaponToHolster(LeftHolsteredWeapon, ESide::ES_Left);
	}*/
}

void UCombatComponent::OnRep_RightHolsteredWeapon()
{
	if (Character == nullptr) return;

	if (RightHolsteredWeapon)
	{
		if (Character->IsLocallyControlled() && !Character->IsEliminated())
		{
			RightHolsteredWeapon->PlayHolsterSound();
		}
	}
	/*if (RightHolsteredWeapon)
	{
		AttachWeaponToHolster(RightHolsteredWeapon, ESide::ES_Right);
	}*/
}

void UCombatComponent::Fire(bool bLeft)
{
	bool& bCurrentCanFire = bLeft ? bCanFireLeft : bCanFireRight;
	AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);

	if (CanFire(bLeft))
	{
		if (CurrentWeapon && Character)
		{
			bCurrentCanFire = false;

			switch (CurrentWeapon->FireType)
			{
			case EFireType::EFT_HitScan:
				FireHitScanWeapon(bLeft);
				break;
			}
			StartFireTimer(CurrentWeapon->FireDelay, bLeft);
			Character->FireWeaponHaptic(bLeft);
		}
	}
	else if(bCurrentCanFire && CurrentWeapon)
	{
		bCurrentCanFire = false;
		StartFireTimer(CurrentWeapon->FireDelay, bLeft);
		CurrentWeapon->PlayDryFireSound();
	}
}

void UCombatComponent::FireHitScanWeapon(bool bLeft)
{
	AWeapon* Weapon = nullptr;
	FVector Target;
	if (bLeft)
	{
		Weapon = LeftEquippedWeapon;
		Target = HitTargetLeft;
	}
	else
	{
		Weapon = RightEquippedWeapon;
		Target = HitTargetRight;
	}

	if (Weapon)
	{
		Target = Weapon->bUseScatter ? Weapon->TraceEndWithScatter(Target) : Target;
		if (!Character->HasAuthority()) LocalFire(Target, bLeft);
		ServerFire(Target, Weapon->FireDelay, bLeft);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (Character && Character->GetDefaultWeapon())
	{
		UWorld* World = GetWorld();
		const USkeletalMeshComponent* WeaponMesh = Character->GetDefaultWeapon()->GetWeaponMesh();
		if (!WeaponMesh || !World) return;
		const USkeletalMeshSocket* MuzzleSocket = Character->GetDefaultWeapon()->GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
		if (!MuzzleSocket) return;

		const FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
		const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + MuzzleDirection * TRACE_LENGTH;

		World->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::TraceUnderLeftCrosshairs(FHitResult& TraceHitResult)
{
	if (LeftEquippedWeapon)
	{
		UWorld* World = GetWorld();
		const USkeletalMeshComponent* WeaponMesh = LeftEquippedWeapon->GetWeaponMesh();
		if (!WeaponMesh || !World) return;
		const USkeletalMeshSocket* MuzzleSocket = LeftEquippedWeapon->GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
		if (!MuzzleSocket) return;

		const FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
		const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + MuzzleDirection * TRACE_LENGTH;

		World->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::TraceUnderRightCrosshairs(FHitResult& TraceHitResult)
{
	if (RightEquippedWeapon)
	{
		UWorld* World = GetWorld();
		const USkeletalMeshComponent* WeaponMesh = RightEquippedWeapon->GetWeaponMesh();
		if (!WeaponMesh || !World) return;
		const USkeletalMeshSocket* MuzzleSocket = RightEquippedWeapon->GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
		if (!MuzzleSocket) return;

		const FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
		const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + MuzzleDirection * TRACE_LENGTH;

		World->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget, bool bLeft)
{
	AWeapon* Weapon = bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
	if (Weapon && CombatState == ECombatState::ECS_Unoccupied)
	{
		//Character->PlayFireMontage(bLeft);
		Weapon->Fire(TraceHitTarget, bLeft);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, const float FireDelay, bool bLeft)
{
	MultiCastFire(TraceHitTarget, bLeft);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay, bool bLeft)
{
	AWeapon* Weapon = bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
	if (Weapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(Weapon->FireDelay, FireDelay, 0.002f);
		return bNearlyEqual;
	}
	return false;
}

void UCombatComponent::MultiCastFire_Implementation(const FVector_NetQuantize& TraceHitTarget, bool bLeft)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(TraceHitTarget, bLeft);
}

void UCombatComponent::StartFireTimer(float FireDelay, bool bLeft)
{
	bLeft ? StartFireTimerLeft(FireDelay) : StartFireTimerRight(FireDelay);

	/*if (Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		FireDelay
	);*/
}

void UCombatComponent::StartFireTimerLeft(float FireDelay)
{
	if (Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimerLeft,
		this,
		&UCombatComponent::FireTimerFinishedLeft,
		FireDelay
	);
}

void UCombatComponent::StartFireTimerRight(float FireDelay)
{
	if (Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimerRight,
		this,
		&UCombatComponent::FireTimerFinishedRight,
		FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && IsWeaponEquipped() && Character->GetDefaultWeapon()->bAutomatic)
	{
		if (GetEquippedWeapon(true))
		{
			Fire(true);
		}
		else if (GetEquippedWeapon(false))
		{
			Fire(false);
		}
	}
	if (Character->GetDefaultWeapon()->IsEmpty())
	{
		Character->GetDefaultWeapon()->PlaySlideBackAnimation();
	}
}

void UCombatComponent::FireTimerFinishedLeft()
{
	bCanFireLeft = true;
	if (LeftEquippedWeapon == nullptr) return;
	if (bFireButtonPressedLeft && LeftEquippedWeapon->bAutomatic)
	{
		Fire(true);
	}
	if (LeftEquippedWeapon->IsEmpty())
	{
		LeftEquippedWeapon->PlaySlideBackAnimation();
	}
}

void UCombatComponent::FireTimerFinishedRight()
{
	bCanFireRight = true;
	if (RightEquippedWeapon == nullptr) return;
	if (bFireButtonPressedRight && RightEquippedWeapon->bAutomatic)
	{
		Fire(false);
	}
	if (RightEquippedWeapon->IsEmpty())
	{
		RightEquippedWeapon->PlaySlideBackAnimation();
	}
}

bool UCombatComponent::CanFire(bool bLeft)
{
	if (Character == nullptr) return false;
	const AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
	if (CurrentWeapon == nullptr) return false;
	bool bCurrentLocallyReloading = bLeft ? bLocallyReloadingLeft : bLocallyReloadingRight;
	if (bCurrentLocallyReloading) return false;
	if (CurrentWeapon->IsObstructed()) return false;

	const UMotionControllerComponent* CurrentMotionController = bLeft ? Character->LeftMotionController : Character->RightMotionController;
	if (CurrentMotionController == nullptr) return false;
	if (CurrentWeapon->Slot1MotionController != CurrentMotionController) return false;

	bool bCurrentCanFire = bLeft ? bCanFireLeft : bCanFireRight;

	return !CurrentWeapon->IsEmpty() && bCurrentCanFire && CombatState == ECombatState::ECS_Unoccupied && CurrentWeapon->IsMagInserted();
}

void UCombatComponent::DropWeapon(bool bLeftHand)
{
	if (Character == nullptr || Character->GetMesh() == nullptr) return;

	if (bLeftHand && LeftEquippedWeapon)
	{
		if (LeftEquippedWeapon == Character->GetDefaultWeapon()) Character->SetDefaultWeapon(nullptr);

		LeftEquippedWeapon = nullptr;
		OwnedWeapons.LeftEquippedWeapon = nullptr;
		OnRep_LeftEquippedWeapon();

		if (!HolsterWeaponDontDrop(bLeftHand, PreviousLeftEquippedWeapon))
		{
			//LeftEquippedWeapon->Dropped(true);
			//HandleWeaponDrop(true);
		}
		
	}
	else if (!bLeftHand && RightEquippedWeapon)
	{
		if (RightEquippedWeapon == Character->GetDefaultWeapon()) Character->SetDefaultWeapon(nullptr);

		RightEquippedWeapon = nullptr;
		OwnedWeapons.RightEquippedWeapon = nullptr;
		OnRep_RightEquippedWeapon();

		if (!HolsterWeaponDontDrop(bLeftHand, PreviousRightEquippedWeapon))
		{
			//RightEquippedWeapon->Dropped(true);
			//HandleWeaponDrop(false);
		}
	}
}

void UCombatComponent::ServerDropWeapon_Implementation(bool bLeft, FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity)
{
	if (bLeft && LeftEquippedWeapon)
	{
		LeftEquippedWeapon->MulticastDropWeapon(StartLocation, StartRotation, LinearVelocity, AngularVelocity);
		LeftEquippedWeapon = nullptr;
	}
	else if (!bLeft && RightEquippedWeapon)
	{
		RightEquippedWeapon->MulticastDropWeapon(StartLocation, StartRotation, LinearVelocity, AngularVelocity);
		RightEquippedWeapon = nullptr;
	}
}

void UCombatComponent::Reload(bool bLeft)
{
	AWeapon* CurrentWeapon = nullptr;
	bool* bCurrentLocallyReloading = nullptr;

	if (bLeft)
	{
		CurrentWeapon = LeftEquippedWeapon;
		bCurrentLocallyReloading = &bLocallyReloadingLeft;
	}
	else
	{
		CurrentWeapon = RightEquippedWeapon;
		bCurrentLocallyReloading = &bLocallyReloadingRight;
	}

	
	if (CurrentWeapon && CurrentWeapon->GetCarriedAmmo() > 0 && CombatState == ECombatState::ECS_Unoccupied && !CurrentWeapon->IsFull() && !*bCurrentLocallyReloading)
	{
		ServerReload(bLeft);
		//HandleReload(); // Maybe I should do this instead of MulticastReload? And play reload sound with OnRep_CombatState?
		if(bCurrentLocallyReloading != nullptr) *bCurrentLocallyReloading = false;
	}
}

void UCombatComponent::ServerReload_Implementation(bool bLeft)
{
	AWeapon* CurrentWeapon = bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
	if (CurrentWeapon == nullptr) return;

	MulticastReload(bLeft);
}

void UCombatComponent::MulticastReload_Implementation(bool bLeft)
{
	HandleReload(bLeft);
}

bool UCombatComponent::HolsterWeaponDontDrop(bool bLeftHand, AWeapon* WeaponToDrop)
{
	if (LeftHolsteredWeapon && RightHolsteredWeapon) return false; // no holsters open
	if (Character == nullptr || Character->IsEliminated()) return false;

	if (WeaponToDrop)
	{
		FVector WeaponLocation = WeaponToDrop->GetActorLocation();

		float HolsterDistance = 10000000.f;
		ESide HolsterSide = GetClosestHolster(WeaponLocation, HolsterDistance);

		if (HolsterDistance < 1000.f) // 1ft
		{
			HolsterSide == ESide::ES_Left ? LeftHolsteredWeapon = WeaponToDrop : RightHolsteredWeapon = WeaponToDrop;
			WeaponToDrop->SetReplicateMovement(false);
			AttachWeaponToHolster(WeaponToDrop, HolsterSide);
			return true;
		}
	}
	return false;
}

void UCombatComponent::HandleReload(bool bLeft)
{
	AWeapon* CurrentWeapon = nullptr;
	bool* bCurrentLocallyReloading = nullptr;

	if (bLeft)
	{
		CurrentWeapon = LeftEquippedWeapon;
		bCurrentLocallyReloading = &bLocallyReloadingLeft;
	}
	else
	{
		CurrentWeapon = RightEquippedWeapon;
		bCurrentLocallyReloading = &bLocallyReloadingRight;
	}

	if (CurrentWeapon == nullptr || bCurrentLocallyReloading == nullptr) return;
	*bCurrentLocallyReloading = false;
	if (Character->HasAuthority())
	{
		UpdateWeaponAmmos(bLeft);
	}
	CombatState = ECombatState::ECS_Unoccupied;
	CurrentWeapon->SetMagInserted(true);
	CurrentWeapon->PlaySlideForwardAnimation();

	if (!Character->IsEliminated())
	{
		CurrentWeapon->PlayReloadSound();
	}
}

void UCombatComponent::ResetWeapon(AWeapon* WeaponToReset)
{
	if (WeaponToReset == nullptr) return;

	if (WeaponToReset->GetMagazine())
	{
		WeaponToReset->GetMagazine()->Destroy();
		WeaponToReset->SetMagazine(nullptr);
	}
	WeaponToReset->UnhideMag();
	WeaponToReset->SetMagInserted(true);
	WeaponToReset->ClearWeaponMagTimer();
	WeaponToReset->SetAmmo(WeaponToReset->GetMagCapacity());
	WeaponToReset->SetCarriedAmmo(WeaponToReset->GetMaxCarriedAmmo());
}

bool UCombatComponent::ResetOwnedWeapons()
{
	bool bOwnsAWeapon = false;
	if (LeftEquippedWeapon)
	{
		bOwnsAWeapon = true;
		LeftEquippedWeapon->ResetWeapon();
		LeftEquippedWeapon->SetHUDAmmo(true);
	}
	if (RightEquippedWeapon)
	{
		bOwnsAWeapon = true;
		RightEquippedWeapon->ResetWeapon();
		RightEquippedWeapon->SetHUDAmmo(false);
	}
	if (LeftHolsteredWeapon)
	{
		bOwnsAWeapon = true;
		LeftHolsteredWeapon->ResetWeapon();
	}
	if (RightHolsteredWeapon)
	{
		bOwnsAWeapon = true;
		RightHolsteredWeapon->ResetWeapon();
	}
	return bOwnsAWeapon;
}

void UCombatComponent::AddWeaponToHolster(AWeapon* WeaponToHolster, bool bLeft)
{
	if (Character == nullptr) return;

	if (bLeft)
	{
		LeftHolsteredWeapon = WeaponToHolster;
		OnRep_LeftHolsteredWeapon();
	}
	else
	{
		RightHolsteredWeapon = WeaponToHolster;
		OnRep_RightHolsteredWeapon();
	}
}

void UCombatComponent::AttachWeaponToHolster(AWeapon* WeaponToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || WeaponToAttach == nullptr || WeaponToAttach->GetWeaponMesh() == nullptr) return;
	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HolsterSocket)
	{
		HolsterSocket->AttachActor(WeaponToAttach, Character->GetMesh());
		WeaponToAttach->SetActorRelativeLocation(FVector::ZeroVector);
		WeaponToAttach->SetActorRelativeRotation(FRotator::ZeroRotator);
		WeaponToAttach->GetWeaponMesh()->SetSimulatePhysics(false);
		WeaponToAttach->GetWeaponMesh()->SetEnableGravity(false);
		WeaponToAttach->GetWeaponMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
}

void UCombatComponent::MulticastAttachToHolster_Implementation(ESide HandSide)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	AWeapon* Weapon = Character->GetDefaultWeapon();

	if (Weapon->GetWeaponMesh() == nullptr || Weapon->GetAreaSphere() == nullptr) return;

	Weapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	Weapon->GetWeaponMesh()->SetEnableGravity(false);
	Weapon->GetWeaponMesh()->SetSimulatePhysics(false);
	Weapon->GetWeaponMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Weapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	Weapon->SetActorRelativeLocation(FVector::ZeroVector);
	Weapon->SetActorRelativeRotation(FRotator::ZeroRotator);

	if (Weapon->GetMagazine())
	{
		Weapon->GetMagazine()->Destroy();
		Weapon->UnhideMag();
		Weapon->SetMagInserted(true);
		Weapon->ClearWeaponMagTimer();
	}

	if (Character->IsLocallyControlled() && !Character->IsEliminated())
	{
		Weapon->PlayHolsterSound();
	}

	Weapon->SetWeaponState(EWeaponState::EWS_Holstered);

	if (HandSide == ESide::ES_None) return;

	HandSide == ESide::ES_Left ? HandleWeaponDrop(true) : HandleWeaponDrop(false);
}

void UCombatComponent::AttachWeaponToHolster(AWeapon* WeaponToHolster, ESide HolsterSide)
{
	if (WeaponToHolster == nullptr) return;
	if (Character == nullptr || Character->GetMesh() == nullptr || HolsterSide == ESide::ES_None) return;
	FName HolsterName = SideToHolsterName(HolsterSide);
	if (HolsterName.IsNone()) return;

	if (WeaponToHolster->GetAreaSphere() == nullptr || WeaponToHolster->GetWeaponMesh() == nullptr) return;

	WeaponToHolster->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponToHolster->GetWeaponMesh()->SetEnableGravity(false);
	WeaponToHolster->GetWeaponMesh()->SetSimulatePhysics(false);
	WeaponToHolster->GetWeaponMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	WeaponToHolster->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsterName);
	//WeaponToHolster->SetActorRelativeLocation(FVector::ZeroVector);
	//WeaponToHolster->SetActorRelativeRotation(FRotator::ZeroRotator);

	if (WeaponToHolster->GetMagazine())
	{
		WeaponToHolster->GetMagazine()->Destroy();
		WeaponToHolster->SetMagazine(nullptr);
		WeaponToHolster->UnhideMag();
		WeaponToHolster->SetMagInserted(true);
		WeaponToHolster->ClearWeaponMagTimer();
	}

	if (Character->IsLocallyControlled() && !Character->IsEliminated())
	{
		WeaponToHolster->PlayHolsterSound();
	}

	WeaponToHolster->SetWeaponState(EWeaponState::EWS_Holstered);
	WeaponToHolster->SetHolsterSide(HolsterSide);
}

FName UCombatComponent::SideToHolsterName(ESide HolsterSide)
{
	if (HolsterSide == ESide::ES_Right)
	{
		return FName("RightHolster");
	}
	else if (HolsterSide == ESide::ES_Left)
	{
		return FName("LeftHolster");
	}
	return FName();
}

void UCombatComponent::CheckSecondHand(UMotionControllerComponent* MC)
{
	if (MC == nullptr || Character == nullptr) return;
	
	USkeletalMeshComponent* HandMesh = nullptr;
	bool bLeft = false;
	if (MC == Character->LeftMotionController)
	{
		HandMesh = Character->GetLeftHandMesh();
		bLeft = true;
	}
	else if (MC == Character->RightMotionController)
	{
		HandMesh = Character->GetRightHandMesh();
	}
	if (HandMesh == nullptr) return;

	// distance check
	if (FVector::DistSquared(MC->GetComponentLocation(), HandMesh->GetComponentLocation()) > 1400.f) // ? inches
	{
		Character->ReleaseGrip(bLeft);
		return;
	}
	// rotation check

	FVector HandUpVector = HandMesh->GetUpVector();
	FVector MotionControllerUpVector = MC->GetUpVector();
	FVector MotionControllerRightVector = MC->GetRightVector();
	MotionControllerRightVector = bLeft ? MotionControllerRightVector * -1.f : MotionControllerRightVector;

	FVector HandLocation = HandMesh->GetComponentLocation();
	FVector HandUpSocketLocation = HandMesh->GetSocketLocation(FName("UpSocket"));
	FVector HandMeshUpVector = (HandUpSocketLocation - HandLocation).GetSafeNormal();

	/*DrawDebugSphere(GetWorld(), MC->GetComponentLocation(), 10.f, 12, FColor::Red, false, GetWorld()->DeltaTimeSeconds * 1.1f);
	DrawDebugSphere(GetWorld(), HandMesh->GetComponentLocation(), 10.f, 12, FColor::Green, false, GetWorld()->DeltaTimeSeconds * 1.1f);

	DrawDebugLine(GetWorld(), HandLocation, HandLocation + HandMeshUpVector * 10.f, FColor::Black, false, GetWorld()->DeltaRealTimeSeconds * 1.1f);
	DrawDebugLine(GetWorld(), MC->GetComponentLocation(), MC->GetComponentLocation() + MotionControllerUpVector * 10.f, FColor::Black, false, GetWorld()->DeltaRealTimeSeconds * 1.1f);*/


	if (FVector::DotProduct(HandUpVector, MotionControllerRightVector) < -0.6f)
	{
		Character->ReleaseGrip(bLeft);
		return;
	}
}

void UCombatComponent::SecondHandAttachmentGrabCheck()
{
	if (Character == nullptr) return;
	bool bLeft = LeftEquippedWeapon->Slot2MotionController == Character->LeftMotionController ? true : false;
	USkeletalMeshComponent* CurrentHandMesh = bLeft ? Character->GetLeftHandMesh() : Character->GetRightHandMesh();
	if (CurrentHandMesh == nullptr) return;
	if (LeftEquippedWeapon->GetRootComponent()->IsSimulatingPhysics()) return;
	
	if (CurrentHandMesh->GetAttachParentActor() != LeftEquippedWeapon)
	{
		AttachHandMeshToWeapon(LeftEquippedWeapon, bLeft, false);
	}
}

void UCombatComponent::SecondHandAttachmentDropCheck()
{
	if (Character == nullptr) return;

	USkeletalMeshComponent* LeftHandMesh = Character->GetLeftHandMesh();
	USkeletalMeshComponent* RightHandMesh = Character->GetRightHandMesh();

	if (IsHandMeshAttachedToSlot2(LeftHandMesh, true))
	{
		Character->SetHandState(true, Character->GetGripState(true));
		DetachHandMeshFromWeapon(true);
	}
	if (IsHandMeshAttachedToSlot2(RightHandMesh, false))
	{
		Character->SetHandState(false, Character->GetGripState(false));
		DetachHandMeshFromWeapon(false);
	}
}

bool UCombatComponent::IsHandMeshAttachedToSlot2(USkeletalMeshComponent* HandMesh, bool bLeft)
{
	if (HandMesh == nullptr) return false;
	AWeapon* Weapon = Cast<AWeapon>(HandMesh->GetAttachParentActor());
	if (Weapon == nullptr) return false;
	USceneComponent* Slot2AttachComponent = bLeft ? Weapon->GrabSlot2L : Weapon->GrabSlot2R;
	if (Slot2AttachComponent == nullptr) return false;

	if (HandMesh->GetAttachParent() == Slot2AttachComponent)
	{
		return true;
	}
	return false;
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{

}

void UCombatComponent::OnRep_CombatState()
{
	/*
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OnRep Reloading"));
		}
		break;
	case ECombatState::ECS_Unoccupied:
		if (Character && Character->GetDefaultWeapon())
		{
			Character->GetDefaultWeapon()->PlaySlideForwardAnimation();
			//
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OnRep Unocuupied"));
			}
		}
		break;
	}*/

}

void UCombatComponent::UpdateWeaponAmmos(bool bLeft)
{
	AWeapon* CurrentWeapon = bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
	if (CurrentWeapon == nullptr) return;

	int32 ReloadAmount = AmountToReload(CurrentWeapon);

	CurrentWeapon->SetCarriedAmmo(CurrentWeapon->GetCarriedAmmo() - ReloadAmount);

	CurrentWeapon->SetHUDCarriedAmmo(bLeft);
	CurrentWeapon->AddAmmo(ReloadAmount, bLeft);
}

void UCombatComponent::UpdateCarriedAmmo()
{
}

void UCombatComponent::UpdateAmmoValues()
{
}

int32 UCombatComponent::AmountToReload(AWeapon* Weapon)
{
	if (Weapon == nullptr) return 0;
	int32 RoomInMag = Weapon->GetMagCapacity() - Weapon->GetAmmo();
	int32 AmountCarried = Weapon->GetCarriedAmmo();
	int32 Least = FMath::Min(RoomInMag, AmountCarried);
	return FMath::Clamp(RoomInMag, 0, Least);
}

EHandState UCombatComponent::SlotToHandState(EWeaponType WeaponType, bool bSlot1)
{
	EHandState HandState = EHandState::EHS_Idle;

	if (WeaponType == EWeaponType::EWT_Pistol)
	{
		HandState = bSlot1 ? EHandState::EHS_HoldingPistol : EHandState::EHS_HoldingPistol2;
	}
	if (WeaponType == EWeaponType::EWT_M4)
	{
		//HandState = bSlot1 ? EHandState::EHS_HoldingM4 : EHandState::EHS_HoldingM42;
	}
	

	return HandState;
}

bool UCombatComponent::IsHandHoldingAnotherWeapon(bool bLeft)
{
	if (Character == nullptr) return false;

	if (bLeft)
	{
		if (LeftEquippedWeapon && LeftEquippedWeapon != RightEquippedWeapon && LeftEquippedWeapon->Slot1MotionController == Character->LeftMotionController)
		{
			return true;
		}
	}
	else
	{
		if (RightEquippedWeapon && LeftEquippedWeapon != RightEquippedWeapon && RightEquippedWeapon->Slot1MotionController == Character->RightMotionController)
		{
			return true;
		}
	}
	return false;
}

void UCombatComponent::RotateWeaponTwoHand(float DeltaTime)
{
	if (LeftEquippedWeapon == nullptr || LeftEquippedWeapon->Hand1 == nullptr || LeftEquippedWeapon->Hand2 == nullptr) return;

	const FVector Hand1Location = LeftEquippedWeapon->Hand1->GetComponentLocation();
	const FVector Hand2Location = LeftEquippedWeapon->Hand2->GetComponentLocation();
	const FQuat Hand1Rotation = LeftEquippedWeapon->Hand1->GetComponentQuat();
	const FQuat Hand2Rotation = LeftEquippedWeapon->Hand2->GetComponentQuat();



	//LeftEquippedWeapon->SetActorLocationAndRotation(FMath::VInterpTo(Hand1)
}

ESide UCombatComponent::GetClosestHolster(FVector WeaponLocation, float &OutDistance)
{
	if (Character == nullptr || Character->GetMesh() == nullptr) return ESide::ES_None;

	ESide ReturnSide = ESide::ES_None;

	float ClosestDistance = 100000000.f;
	float d = ClosestDistance;
	if (!LeftHolsteredWeapon)
	{
		d = FVector::DistSquared(Character->GetMesh()->GetSocketLocation(FName("LeftHolster")), WeaponLocation);
		if (d < ClosestDistance)
		{
			ClosestDistance = d;
			ReturnSide = ESide::ES_Left;
		}
	}
	if (!RightHolsteredWeapon)
	{
		d = FVector::DistSquared(Character->GetMesh()->GetSocketLocation(FName("RightHolster")), WeaponLocation);
		if (d < ClosestDistance)
		{
			ClosestDistance = d;
			ReturnSide = ESide::ES_Right;
		}
	}

	OutDistance = ClosestDistance;
	return ReturnSide;
}

AWeapon* UCombatComponent::GetEquippedWeapon(bool bLeft)
{
	return bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
}

bool UCombatComponent::CheckEquippedWeapon(bool bLeft)
{
	if (bLeft && LeftEquippedWeapon)
	{
		return true;
	}
	else if (!bLeft && RightEquippedWeapon)
	{
		return true;
	}
	return false;
}

bool UCombatComponent::CheckEquippedMagazine(bool bLeft)
{
	if (bLeft && LeftEquippedMagazine)
	{
		return true;
	}
	else if(!bLeft && RightEquippedMagazine)
	{
		return true;
	}
	return false;
}

AFullMagazine* UCombatComponent::GetEquippedMagazine(bool bLeft)
{
	return bLeft ? LeftEquippedMagazine : RightEquippedMagazine;
}

void UCombatComponent::SetEquippedMagazine(AFullMagazine* NewMag, bool bLeft)
{
	if (bLeft)
	{
		LeftEquippedMagazine = NewMag;
	}
	else
	{
		RightEquippedMagazine = NewMag;
	}
}

bool UCombatComponent::IsWeaponEquipped()
{
	if (LeftEquippedWeapon)
	{
		return true;
	}
	if (RightEquippedWeapon)
	{
		return true;
	}
	return false;
}

AWeapon* UCombatComponent::GetHolsteredWeapon(bool bLeft)
{
	return bLeft ? LeftHolsteredWeapon : RightHolsteredWeapon;
}
