// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine\PhysicsAsset.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Gunfight.h"
#include "Gunfight/Character/GunfightCharacterDeprecated.h"
#include "MotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/Weapon/EmptyMagazine.h"
#include "Kismet/KismetMathLibrary.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "GripMotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Gunfight/Weapon/FullMagazine.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // this needs to be set true so a late joining player has OnRep_CurrentSkinIndex called for weapons already in the game.

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionObjectType(ECC_Weapon);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetUseCCD(true);
	AreaSphere->SetSphereRadius(10.f);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	MagSlideEnd = CreateDefaultSubobject<USceneComponent>(TEXT("MagSlideEnd"));
	MagSlideEnd->SetupAttachment(WeaponMesh);
	MagSlideStart = CreateDefaultSubobject<USceneComponent>(TEXT("MagSlideStart"));
	MagSlideStart->SetupAttachment(WeaponMesh);

	GrabSlot1R = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot1R"));
	GrabSlot1R->SetupAttachment(GetRootComponent());
	GrabSlot1L = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot1L"));
	GrabSlot1L->SetupAttachment(GetRootComponent());
	GrabSlot2R = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot2R"));
	GrabSlot2R->SetupAttachment(GetRootComponent());
	GrabSlot2L = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot2L"));
	GrabSlot2L->SetupAttachment(GetRootComponent());

	GrabSlot2IKL = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot2IKL"));
	GrabSlot2IKL->SetupAttachment(GetRootComponent());

	GrabSlot2IKR = CreateDefaultSubobject<USceneComponent>(TEXT("GrabSlot2IKR"));
	GrabSlot2IKR->SetupAttachment(GetRootComponent());
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME_CONDITION(AWeapon, CarriedMags, COND_OwnerOnly);
	//DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
	DOREPLIFETIME(AWeapon, WeaponSide);
	DOREPLIFETIME(AWeapon, CurrentSkinIndex);

	//DOREPLIFETIME(AWeapon, Slot1MotionController);
	DOREPLIFETIME(AWeapon, Slot2MotionController);
}

void AWeapon::Fire(const FVector& HitTarget, bool bLeft)
{
	SpendRound(bLeft);
	if (FireAnimation && FireEndAnimation)
	{
		if (Ammo <= 0)
		{
			WeaponMesh->PlayAnimation(FireEndAnimation, false);
		}
		else
		{
			WeaponMesh->PlayAnimation(FireAnimation, false);
		}
	}
}

void AWeapon::SetHUDAmmo(bool bLeft)
{
	if (CharacterOwner && CharacterOwner->IsLocallyControlled())
	{
		CharacterOwner->SetHUDAmmo(Ammo, bLeft);
	}
}

void AWeapon::SetHUDCarriedAmmo(bool bLeft)
{
	if (CharacterOwner && CharacterOwner->IsLocallyControlled())
	{
		CharacterOwner->SetHUDCarriedAmmo(CarriedAmmo, bLeft);
	}
}

void AWeapon::ClearHUDAmmo(bool bLeft)
{
	if (CharacterOwner)
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController)
		{
			GunfightOwnerController->SetHUDWeaponAmmoVisible(bLeft, false);
			//GunfightOwnerController->SetHUDWeaponAmmo(Ammo, bLeft);
			//GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo, bLeft);
		}
	}
}

void AWeapon::UnhideMag()
{
	if (WeaponMesh)
	{
		WeaponMesh->UnHideBoneByName(FName("Colt_Magazine"));
	}
}

void AWeapon::AddAmmo(int32 AmmoToAdd, bool bLeft)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo(bLeft);
	ClientAddAmmo(AmmoToAdd, bLeft);
}

void AWeapon::PlayReloadSound()
{
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
}

void AWeapon::PlayHolsterSound()
{
	if (HolsterSound && !bPlayingHolsterSound)
	{
		bPlayingHolsterSound = true;
		UGameplayStatics::PlaySoundAtLocation(this, HolsterSound, GetActorLocation());
		GetWorldTimerManager().SetTimer(HolsterSoundTimer, this, &AWeapon::HolsterSoundTimerFinished, 0.3f, false);
	}
}

void AWeapon::PlayDryFireSound()
{
	if (DryFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetActorLocation());
	}
}

void AWeapon::SetWeaponSkin(int32 SkinIndex)
{
	if (SkinIndex >= WeaponSkins.Num() || SkinIndex < 0) return;

	CurrentSkinIndex = SkinIndex;
	WeaponMesh->SetMaterial(0, WeaponSkins[SkinIndex]);
}

void AWeapon::ServerSetWeaponSkin_Implementation(int32 SkinIndex)
{
	SetWeaponSkin(SkinIndex);
}

void AWeapon::HandleAttachmentReplication()
{
	bool bIsAttachedToHolster = AttachmentReplication.AttachSocket.ToString().Contains("Holster");

	if (AttachmentReplication.AttachParent)
	{
		SetReplicatingMovement(false);
		StopPhysics();

		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(AttachmentReplication.AttachParent);
		bool bAttachmentLeft = false;

		if (GChar && GChar->GetCombat() && !bIsAttachedToHolster) // Attached to hand
		{
			Slot1MotionController = GChar->GetMotionControllerFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent);
			bAttachmentLeft = GChar->GetLeftFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent);
			GChar->GetCombat()->HandleWeaponAttach(this, bAttachmentLeft);
			GChar->GetCombat()->UpdateHUDWeaponPickup(bAttachmentLeft, this);
			if (GChar->IsLocallyControlled() && WeaponMesh)
			{
				WeaponMesh->OverrideMinLOD(0);
				WeaponMesh->SetForcedLOD(1);
			}
		}

		// previous hand needs to be detached - This happens when the replication clearing AttachmentReplication wasn't received before we received a new attachment.
		if (//!bIsAttachedToHolster &&
			!PreviousAttachmentReplication.AttachSocket.ToString().Contains("Holster") &&
			PreviousAttachmentReplication.AttachComponent && 
			(PreviousAttachmentReplication.AttachComponent != AttachmentReplication.AttachComponent) || 
			AttachmentReplication.AttachSocket != PreviousAttachmentReplication.AttachSocket)
		{
			AGunfightCharacter* PreviousGChar = Cast<AGunfightCharacter>(PreviousAttachmentReplication.AttachParent);
			if (PreviousGChar && PreviousGChar->GetCombat())
			{
				// TODO: Only do this if the hand is not holding another weapon?
				bool bLeft = PreviousGChar->GetLeftFromAttachment(PreviousAttachmentReplication.AttachSocket, PreviousAttachmentReplication.AttachComponent);
				if(bLeft != bAttachmentLeft && !PreviousGChar->GetCombat()->IsHandHoldingAnotherWeapon(bLeft))
				{
					PreviousGChar->AttachHandMeshToMotionController(bLeft);
					PreviousGChar->SetHandState(bLeft, EHandState::EHS_Idle);
					PreviousGChar->GetCombat()->UpdateHUDWeaponDropped(bLeft);
				}
			}
		}
	}
	else // Deattached from hand
	{
		SetReplicatingMovement(true);
		StartPhysics();
		Slot1MotionController = nullptr;

		if (!PreviousAttachmentReplication.AttachSocket.ToString().Contains("Holster") && PreviousAttachmentReplication.AttachParent)
		{
			AGunfightCharacter* GChar = Cast<AGunfightCharacter>(PreviousAttachmentReplication.AttachParent);
			if (GChar && GChar->GetCombat())
			{
				bool bLeft = GChar->GetLeftFromAttachment(PreviousAttachmentReplication.AttachSocket, PreviousAttachmentReplication.AttachComponent);
				GChar->AttachHandMeshToMotionController(bLeft);
				GChar->SetHandState(bLeft, EHandState::EHS_Idle);
				GChar->GetCombat()->UpdateHUDWeaponDropped(bLeft);
			}
		}
	}
	PreviousAttachmentReplication = AttachmentReplication;
}

void AWeapon::HandleAttachmentReplication2()
{
	if (AttachmentReplication.AttachSocket.ToString().Contains("Holster")) // attached to holster
	{
		SetReplicatingMovement(false);
		StopPhysics();
	}
	if (AttachmentReplication.AttachParent) // grabbed
	{
		SetReplicatingMovement(false);
		StopPhysics();
	}
	else // dropped
	{
		SetReplicatingMovement(true);
		StartPhysics();
	}
	PreviousAttachmentReplication = AttachmentReplication;
}

void AWeapon::OnRep_AttachmentReplication()
{
	Super::OnRep_AttachmentReplication();

	HandleAttachmentReplication();

	/*
	if (AttachmentReplication.AttachParent)
	{
		SetReplicatingMovement(false);
		StopPhysics();

		// weapon has been holstered
		if (AttachmentReplication.AttachSocket.ToString().Contains("Holster"))
		{
			if (Slot1MotionController) // if we are being grabbed, make the hand let go
			{
				AGunfightCharacter* GChar = Cast<AGunfightCharacter>(Slot1MotionController->GetOwner());
				if (GChar && GChar->GetCombat())
				{
					bool bLeft = Slot1MotionController == GChar->LeftMotionController ? true : false;
					GChar->GetCombat()->DetachHandMeshFromWeapon(bLeft);
				}
			}
			Slot1MotionController = nullptr;
			PreviousSlot1MotionController = nullptr;
			PreviousAttachmentReplication = {};
			return;
		}

		// weapon has been grabbed by the player
		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(AttachmentReplication.AttachParent);
		if (GChar && GChar->GetCombat())
		{
			Slot1MotionController = GChar->GetMotionControllerFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent);
			bool bLeft = Slot1MotionController == GChar->LeftMotionController ? true : false;

			// weapon was grabbed by Slot1MotionController
			if (Slot1MotionController)
			{
				// weapon was dropped by PreviousSlot1MotionController
				if (PreviousSlot1MotionController && Slot1MotionController != PreviousSlot1MotionController)
				{
					GChar->GetCombat()->DetachHandMeshFromWeapon(!bLeft);
				}

				GChar->GetCombat()->HandleWeaponAttach(this, bLeft);
			}
		}
	}
	else // weapon was dropped by PreviousSlot1MotionController
	{
		SetReplicatingMovement(true);
		StartPhysics();
		Slot1MotionController = nullptr;

		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(PreviousAttachmentReplication.AttachParent);
		if (GChar && GChar->GetCombat())
		{
			GChar->GetCombat()->DetachHandMeshFromWeapon(GChar->GetLeftFromAttachment(PreviousAttachmentReplication.AttachSocket, PreviousAttachmentReplication.AttachComponent));
		}
	}
	PreviousAttachmentReplication = AttachmentReplication;
	PreviousSlot1MotionController = Slot1MotionController;
	*/
}

bool AWeapon::IsObstructed() const
{
	if (!GetWorld()) return false;
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return false;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
	const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
	const FVector Start = SocketTransform.GetLocation() - MuzzleDirection; // front of the barrel
	const FVector End = Start - MuzzleDirection * ObstructedDistance;	// back of the gun

	FHitResult HitResult;
	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_WorldDynamic);
}

void AWeapon::StartRotatingTwoHand(USceneComponent* NewHand1, USceneComponent* NewHand2)
{
	if (NewHand1 == nullptr || NewHand2 == nullptr) return;
	bRotateTwoHand = true;

	Hand1 = NewHand1;
	Hand2 = NewHand2;
}

void AWeapon::StopRotatingTwoHand()
{
	bRotateTwoHand = false;
}

void AWeapon::ResetWeapon()
{
	if (GetMagazine())
	{
		GetMagazine()->Destroy();
		UnhideMag();
		SetMagInserted(true);
	}

	SetAmmo(GetMagCapacity());
	SetCarriedAmmo(GetMaxCarriedAmmo());

	if (WeaponMesh == nullptr) return;

	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetVisibility(true);
	PlaySlideForwardAnimation();
}

void AWeapon::DetachAndSimulate()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	if (Magazine)
	{
		Magazine->Destroy();
		UnhideMag();
		SetMagInserted(true);
		ClearWeaponMagTimer();
	}

	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner == nullptr) return;

	GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
	if (GunfightOwnerController && HasAuthority() && GunfightOwnerController->HighPingDelegate.IsBound())
	{
		GunfightOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}
}

void AWeapon::AttachAndStopPhysics(USceneComponent* AttachComponent, const FName& SocketName, bool bLeft)
{
	if (SocketName == NAME_None)
	{
		// Attach to motion controller
		
	}
	else
	{
		// Attach to hand on body mesh
	}
}

void AWeapon::StopPhysics()
{
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
}

void AWeapon::StartPhysics()
{
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	if (Magazine)
	{
		Magazine->Destroy();
		UnhideMag();
		SetMagInserted(true);
		ClearWeaponMagTimer();
	}
}


//void AWeapon::OnRep_Slot1MotionController()
//{
//	if (Slot1MotionController) // grabbed
//	{
//		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(Slot1MotionController->GetOwner());
//		if (GChar == nullptr || GChar->GetCombat() == nullptr) return;
//
//		// previous motion controller dropped the gun, need to set it to false
//		if (Slot1MotionController != PreviousSlot1MotionController)
//		{
//			if (GChar->LeftMotionController && GChar->LeftMotionController == PreviousSlot1MotionController)
//			{
//				GChar->bLeftHandAttached = false;
//				GChar->GetCombat()->DetachHandMeshFromWeapon(true);
//				GChar->SetHandState(true, EHandState::EHS_Idle);
//			}
//			else if (GChar->RightMotionController && GChar->RightMotionController == PreviousSlot1MotionController)
//			{
//				GChar->bRightHandAttached = false;
//				GChar->GetCombat()->DetachHandMeshFromWeapon(false);
//				GChar->SetHandState(false, EHandState::EHS_Idle);
//			}
//		}
//		PreviousSlot1MotionController = Slot1MotionController;
//
//		// 
//		if (GChar->LeftMotionController == Slot1MotionController)
//		{
//			GChar->bLeftHandAttached = true;
//		}
//		else if (GChar->RightMotionController == Slot1MotionController)
//		{
//			GChar->bRightHandAttached = true;
//		}
//		
//		HandleSlot1Grabbed(Slot1MotionController);
//	}
//	else // dropped
//	{
//		HandleSlot1Released(PreviousSlot1MotionController);
//		PreviousSlot1MotionController = nullptr;
//		
//		if (PreviousSlot1MotionController)
//		{
//			AGunfightCharacter* GChar = Cast<AGunfightCharacter>(PreviousSlot1MotionController->GetOwner());
//			if (GChar == nullptr) return;
//			if (GChar->LeftMotionController == PreviousSlot1MotionController)
//			{
//				GChar->bLeftHandAttached = false;
//			}
//			else if (GChar->RightMotionController == PreviousSlot1MotionController)
//			{
//				GChar->bRightHandAttached = false;
//			}
//
//			if (GChar == nullptr || GChar->GetCombat() == nullptr) return;
//			bool bLeft = PreviousSlot1MotionController == GChar->LeftMotionController ? true : false;
//			GChar->GetCombat()->DetachHandMeshFromWeapon(bLeft);
//		}
//		
//	}
//}

void AWeapon::OnRep_Slot2MotionController()
{
	if (Slot2MotionController) // grabbed
	{
		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(Slot2MotionController->GetAttachParentActor());
		if (GChar)
		{
			bool bLeft = Slot2MotionController == GChar->LeftMotionController ? true : false;
			if (GetAttachParentActor() == GChar && bLeft != GChar->GetLeftFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent))
			{
				//GChar->SetHandState(bLeft, EHandState::EHS_HoldingPistol2);
			}

			// dropped by PreviousSlot2MotionController
			if (PreviousSlot2MotionController && Slot2MotionController != PreviousSlot2MotionController)
			{
				bLeft = PreviousSlot2MotionController == GChar->LeftMotionController ? true : false;
				if (AttachmentReplication.AttachParent == GChar && GChar->GetLeftFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent) == bLeft) return;
				GChar->SetHandState(bLeft, GChar->GetGripState(bLeft));
			}
		}
	}
	else if(PreviousSlot2MotionController) // dropped by PreviousSlot2MotionController
	{
		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(PreviousSlot2MotionController->GetAttachParentActor());
		if(GChar)
		{
			bool bLeft = PreviousSlot2MotionController == GChar->LeftMotionController ? true : false;
			if (GetAttachParentActor() == GChar && bLeft == GChar->GetLeftFromAttachment(AttachmentReplication.AttachSocket, AttachmentReplication.AttachComponent)) return;
			GChar->SetHandState(bLeft, GChar->GetGripState(bLeft));
		}
	}
	PreviousSlot2MotionController = Slot2MotionController;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponMesh)
	{
		WeaponMesh->SetMaskFilterOnBodyInstance(1 << 3); // Player leg IK will ignore the weapon mesh
		WeaponMesh->OverrideMinLOD(0);
	}

	//BulletQueryParams.bReturnPhysicalMaterial = true;
	//BulletQueryParams.bTraceComplex = true;

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	// initialize on spawn sphere overlaps
	SetActorTickEnabled(true);

	/*
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ShouldHolsterTimer, this, &AWeapon::ShouldAttachToHolster, 1.f, true, 1.f);
	}
	*/
	PreviousAttachmentReplication.AttachSocket = FName("Holster");
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && (GetOwner() == GunfightCharacter || GetOwner() == nullptr))
	{
		if (OtherComp == GunfightCharacter->GetLeftHandSphere())
		{
			bLeftControllerOverlap = true;
			GunfightCharacter->SetLeftOverlappingWeapon(this);
			GunfightCharacter->LeftOverlappingWeapons.AddUnique(this);
		}
		else if (OtherComp == GunfightCharacter->GetRightHandSphere())
		{
			bRightControllerOverlap = true;
			GunfightCharacter->SetRightOverlappingWeapon(this);
			GunfightCharacter->RightOverlappingWeapons.AddUnique(this);
		}

		/*
		if (OtherComp == GunfightCharacter->GetLeftHandSphere()) bLeftControllerOverlap = true;
		else if (OtherComp == GunfightCharacter->GetRightHandSphere()) bRightControllerOverlap = true;
		GunfightCharacter->SetOverlappingWeapon(this);
		*/

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, FString::Printf(TEXT("Overlap: %d"), GunfightCharacter->GetOverlappingWeapon()));
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && (GetOwner() == GunfightCharacter || GetOwner() == nullptr))
	{
		if (OtherComp == GunfightCharacter->GetLeftHandSphere())
		{
			bLeftControllerOverlap = false;
			//if (!bRightControllerOverlap)
			//{
				GunfightCharacter->LeftOverlappingWeapons.RemoveSingle(this);
				if (GunfightCharacter->LeftOverlappingWeapons.IsEmpty()) GunfightCharacter->SetLeftOverlappingWeapon(nullptr);
			//}
		}
		else if (OtherComp == GunfightCharacter->GetRightHandSphere())
		{
			bRightControllerOverlap = false;
			//if (!bLeftControllerOverlap)
			//{
				GunfightCharacter->RightOverlappingWeapons.RemoveSingle(this);
				if (GunfightCharacter->RightOverlappingWeapons.IsEmpty()) GunfightCharacter->SetRightOverlappingWeapon(nullptr);
			//}
		}
		//GunfightCharacter->OverlappingWeapons.IsEmpty() ? GunfightCharacter->SetOverlappingWeapon(nullptr) : GunfightCharacter->SetOverlappingWeapon(GunfightCharacter->OverlappingWeapons[0]);

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, FString::Printf(TEXT("End Overlap: %d"), CharacterOwner->GetOverlappingWeapon()));
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	PollInit();
}

void AWeapon::PollInit()
{
	if (!bInitDelayCompleted)
	{
		if (InitDelayFrame < 1)
		{
			InitDelayFrame++;
		}
		else
		{
			bInitDelayCompleted = true;
			SetActorTickEnabled(false);
			GetSpawnOverlaps();
		}
	}
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd, bool bLeft)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	SetHUDAmmo(bLeft);
}

void AWeapon::OnRep_CarriedAmmo()
{
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner && CharacterOwner->GetCombat())
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController)
		{
			if (CharacterOwner->GetCombat()->GetEquippedWeapon(true) == this)
			{
				GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo, true);
			}
			else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false) == this)
			{
				GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo, false);
			}
			
		}
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo, bool bLeft)
{
	if (HasAuthority()) return;

	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo(bLeft);
}

void AWeapon::SpendRound(bool bLeft)
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo(bLeft);
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo, bLeft);
	}
	else if (CharacterOwner && CharacterOwner->IsLocallyControlled())
	{
		++Sequence;
	}
}

void AWeapon::EjectMagazine()
{
	if (bMagInserted && MagEjectAnimation)
	{
		bSlideBack = true;
		bMagInserted = false;
		WeaponMesh->PlayAnimation(MagEjectAnimation, false);
		GetWorldTimerManager().SetTimer(EjectMagTimerHandle, this, &AWeapon::DropMag, MagDropDelay, false, MagDropDelay);
	}
}

bool AWeapon::CheckHandOverlap(bool bLeftHand)
{
	return bLeftHand ? bLeftControllerOverlap : bRightControllerOverlap;
}

FVector AWeapon::GetMagwellDirection() const
{
	return (MagSlideStart->GetComponentLocation() - MagSlideEnd->GetComponentLocation()).GetSafeNormal();
}

USceneComponent* AWeapon::GetSlot2IK(bool bLeft)
{
	return bLeft ? GrabSlot2IKL : GrabSlot2IKR;
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AWeapon::DropMag()
{
	if (WeaponMesh == nullptr) return;

	WeaponMesh->HideBoneByName(FName("Colt_Magazine"), EPhysBodyOp::PBO_None);
	WeaponMesh->PlayAnimation(SlideBackAnimationPose, false);

	if (EmptyMagazineClass)
	{
		const USkeletalMeshSocket* MeshSocket = WeaponMesh->GetSocketByName(TEXT("EmptyMag"));
		if (MeshSocket)
		{
			const FTransform SocketTransform = MeshSocket->GetSocketTransform(WeaponMesh);
			const FVector ImpulseDir = MagSlideEnd->GetComponentLocation() - MagSlideStart->GetComponentLocation();

			UWorld* World = GetWorld();
			if (World)
			{
				AEmptyMagazine* EmptyMag = World->SpawnActor<AEmptyMagazine>(EmptyMagazineClass, SocketTransform);
				if (EmptyMag && CharacterOwner && CharacterOwner->GetCombat())
				{
					if (EmptyMag->GetMagazineMesh() && WeaponMesh)
					{
						EmptyMag->GetMagazineMesh()->SetMaterial(0, WeaponMesh->GetMaterial(0));
					}

					if (CharacterOwner->GetCombat()->GetEquippedWeapon(true))
					{
						EmptyMag->GetMagazineMesh()->SetPhysicsLinearVelocity(CharacterOwner->GetLeftMotionControllerAverageVelocity() * 60.f);
						EmptyMag->GetMagazineMesh()->SetPhysicsAngularVelocityInRadians(CharacterOwner->LeftMotionControllerAverageAngularVelocity * 5.f);
					}
					else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false))
					{
						EmptyMag->GetMagazineMesh()->SetPhysicsLinearVelocity(CharacterOwner->GetRightMotionControllerAverageVelocity() * 60.f);
						EmptyMag->GetMagazineMesh()->SetPhysicsAngularVelocityInRadians(CharacterOwner->RightMotionControllerAverageAngularVelocity * 5.f);
					}
					EmptyMag->GetMagazineMesh()->AddImpulse(FVector(ImpulseDir * MagDropImpulse));
				}
			}
		}
	}
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;
	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}

	if (GetWorld()) LastTimeInteractedWith = GetWorld()->GetTimeSeconds();
}

void AWeapon::OnEquipped()
{
	// attach if we are not already attached.
	/*
	if (CharacterOwner && AttachmentReplication.AttachParent != CharacterOwner && CharacterOwner->GetCombat())
	{
		if (CharacterOwner->GetCombat()->GetEquippedWeapon(true) == this)
		{
			CharacterOwner->GetCombat()->HandleWeaponAttach(this, true);
		}
		else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false) == this)
		{
			CharacterOwner->GetCombat()->HandleWeaponAttach(this, false);
		}
		
	}
	*/

	//AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	//WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);

	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner)
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController && HasAuthority() && !GunfightOwnerController->HighPingDelegate.IsBound())
		{
			GunfightOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}

		/*
		if (CharacterOwner->IsLocallyControlled())
		{
			PlayEquipSound();
		}
		*/
	}
}

void AWeapon::PlayEquipSound()
{
	if (EquipSound && bPlayingEquipSound == false)
	{
		bPlayingEquipSound = true;
		UGameplayStatics::PlaySoundAtLocation(this, EquipSound, GetActorLocation());
		GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AWeapon::EquipSoundTimerEnd, 0.5f, false, 0.5f);
		/*
		if (CharacterOwner->GetCombat())
		{
			FVector SoundLocation = GetActorLocation();
			if (CharacterOwner->GetCombat()->GetEquippedWeapon(true) == this)
			{
				if (CharacterOwner->LeftMotionController)
				{
					SoundLocation = CharacterOwner->LeftMotionController->GetComponentLocation();
				}
			}
			else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false) == this)
			{
				if (CharacterOwner->RightMotionController)
				{
					SoundLocation = CharacterOwner->RightMotionController->GetComponentLocation();
				}
			}
			UGameplayStatics::PlaySoundAtLocation(this, EquipSound, SoundLocation);
		}*/
	}
}

void AWeapon::MulticastDropWeapon_Implementation(FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity)
{
	if (WeaponMesh == nullptr || AreaSphere == nullptr) return;

	WeaponState = EWeaponState::EWS_Dropped;
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner == nullptr || CharacterOwner->GetCombat() == nullptr) return;



	GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
	if (GunfightOwnerController && HasAuthority() && GunfightOwnerController->HighPingDelegate.IsBound())
	{
		GunfightOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}

	SetActorLocationAndRotation(StartLocation, StartRotation);
	WeaponMesh->SetPhysicsLinearVelocity(LinearVelocity);
	WeaponMesh->SetPhysicsAngularVelocityInRadians(AngularVelocity);
}

void AWeapon::Dropped(bool bLeftHand)
{
	WeaponSide = bLeftHand ? ESide::ES_Left : ESide::ES_Right;
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetHolsterSide(ESide::ES_None);
	SetOwner(nullptr);
	CharacterOwner = nullptr;
	SetReplicateMovement(true);
}

void AWeapon::Equipped(bool bLeftHand)
{
	WeaponSide = bLeftHand ? ESide::ES_Left : ESide::ES_Right;

}

void AWeapon::OnDropped()
{
	//DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	//AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	//AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	if (Magazine)
	{
		Magazine->Destroy();
		UnhideMag();
		SetMagInserted(true);
		ClearWeaponMagTimer();
	}
	
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner == nullptr) return;

	GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
	if (GunfightOwnerController && HasAuthority() && GunfightOwnerController->HighPingDelegate.IsBound())
	{
		GunfightOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}

	// apply hand controller velocities on drop
	if (WeaponSide == ESide::ES_Left)
	{
		WeaponMesh->SetPhysicsLinearVelocity(CharacterOwner->GetLeftMotionControllerAverageVelocity() * 50.f);
		WeaponMesh->SetPhysicsAngularVelocityInRadians(CharacterOwner->LeftMotionControllerAverageAngularVelocity * 14.f);
	}
	else if (WeaponSide == ESide::ES_Right)
	{
		WeaponMesh->SetPhysicsLinearVelocity(CharacterOwner->GetRightMotionControllerAverageVelocity() * 50.f);
		WeaponMesh->SetPhysicsAngularVelocityInRadians(CharacterOwner->RightMotionControllerAverageAngularVelocity * 14.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Dropped weapon WeaponSide is not set"));
	}

	//SetOwner(nullptr);
	//SetCharacterOwner(nullptr);

	// set owner to nullptr
	WeaponSide = ESide::ES_None;
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::PlaySlideBackAnimation()
{
	if (SlideBackAnimation)
	{
		WeaponMesh->PlayAnimation(SlideBackAnimation, false);
	}
}

void AWeapon::PlaySlideForwardAnimation()
{
	if (SlideForwardAnimation)
	{
		WeaponMesh->PlayAnimation(SlideForwardAnimation, false);
		bSlideBack = false;
	}
}

void AWeapon::SetServerSideRewind(bool bUseSSR)
{
	bUseServerSideRewind = bUseSSR;
}

void AWeapon::ShouldAttachToHolster()
{
	if (HasAuthority() && CharacterOwner && CharacterOwner->GetMesh())
	{
		UCombatComponent* Combat = CharacterOwner->GetCombat();
		if (Combat && !Combat->GetEquippedWeapon(true) && !Combat->GetEquippedWeapon(false))
		{
			const FName SocketName = CharacterOwner->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
			const USkeletalMeshSocket* HolsterSocket = CharacterOwner->GetMesh()->GetSocketByName(SocketName);
			if (HolsterSocket == nullptr) return;

			const float DistSquared = FVector::DistSquared(HolsterSocket->GetSocketLocation(CharacterOwner->GetMesh()), GetActorLocation());
			if (DistSquared > 45522.f) // 7ft
			{
				Combat->MulticastAttachToHolster(ES_None);
			}
		}
	}
}

void AWeapon::GetSpawnOverlaps()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetOwner());
	if (GunfightCharacter)
	{
		USphereComponent* LeftHandSphere = GunfightCharacter->GetLeftHandSphere();
		USphereComponent* RightHandSphere = GunfightCharacter->GetRightHandSphere();

		if (LeftHandSphere && RightHandSphere)
		{
			const float LeftDistance = FVector::DistSquared(LeftHandSphere->GetComponentLocation(), GetActorLocation());
			const float RightDistance = FVector::DistSquared(RightHandSphere->GetComponentLocation(), GetActorLocation());

			if (LeftDistance <= FMath::Square(LeftHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bLeftControllerOverlap = true;
				GunfightCharacter->SetLeftOverlappingWeapon(this);
				GunfightCharacter->LeftOverlappingWeapons.AddUnique(this);
			}
			if (RightDistance <= FMath::Square(RightHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bRightControllerOverlap = true;
				GunfightCharacter->SetRightOverlappingWeapon(this);
				GunfightCharacter->RightOverlappingWeapons.AddUnique(this);
			}
		}
	}
}

void AWeapon::OnRep_CurrentSkinIndex()
{
	SetWeaponSkin(CurrentSkinIndex);
}

void AWeapon::HandleSlot1Grabbed(UMotionControllerComponent* GrabbingController)
{
	SetReplicateMovement(false);
	StopPhysics();
	PreviousSlot1MotionController = GrabbingController;
	if (GrabbingController == nullptr) return;
	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GrabbingController->GetOwner());
	if (GChar == nullptr || GChar->GetCombat() == nullptr) return;
	bool bLeft = GrabbingController == GChar->LeftMotionController ? true : false;
	GChar->GetCombat()->HandleWeaponAttach(this, bLeft);
	GChar->SetHandState(bLeft, SlotToHandState(true));
}

void AWeapon::HandleSlot1Released(UMotionControllerComponent* ReleasingController)
{
	SetReplicateMovement(true);
	if (ReleasingController == nullptr) return;
	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(ReleasingController->GetOwner());
	if (GChar == nullptr || GChar->GetCombat() == nullptr) return;
	bool bLeft = ReleasingController == GChar->LeftMotionController ? true : false;
	GChar->GetCombat()->DetachHandMeshFromWeapon(bLeft);
	GChar->SetHandState(bLeft, EHandState::EHS_Idle);
	DetachAndSimulate();
}

void AWeapon::HandleSlot2Grabbed(UMotionControllerComponent* GrabbingController)
{

}

void AWeapon::HandleSlot2Released(UMotionControllerComponent* ReleasingController)
{

}

EHandState AWeapon::SlotToHandState(bool bSlot1)
{
	EHandState ReturnState = EHandState::EHS_Idle;
	if (WeaponType == EWeaponType::EWT_Pistol)
	{
		return bSlot1 ? EHandState::EHS_HoldingPistol : EHandState::EHS_HoldingPistol2;
	}
	else if (WeaponType == EWeaponType::EWT_M4)
	{
		//return bSlot1 ? EHandState::EHS_HoldingPistol : EHandState::EHS_HoldingPistol2;
	}
	return ReturnState;
}

bool AWeapon::IsOverlappingHand(bool bLeftHand)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetOwner());
	if (GunfightCharacter == nullptr) return false;
	USphereComponent* HandSphere = bLeftHand ? GunfightCharacter->GetLeftHandSphere() : GunfightCharacter->GetRightHandSphere();
	if (HandSphere == nullptr) return false;
	const float Distance = FVector::DistSquared(HandSphere->GetComponentLocation(), AreaSphere->GetComponentLocation());

	if (Distance <= FMath::Square(HandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
	{
		return true;
	}
	return false;
}

void AWeapon::ApplyVelocityOnDropped()
{
	WeaponMesh->SetPhysicsAngularVelocityInRadians(DropAngularVelocity.Vector());
	WeaponMesh->SetPhysicsLinearVelocity(DropVelocity);
}

void AWeapon::EquipSoundTimerEnd()
{
	bPlayingEquipSound = false;
}

void AWeapon::ClearWeaponMagTimer()
{
	GetWorldTimerManager().ClearTimer(EjectMagTimerHandle);
}