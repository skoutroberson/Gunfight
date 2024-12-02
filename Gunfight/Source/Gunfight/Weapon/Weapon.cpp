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

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME_CONDITION(AWeapon, CarriedMags, COND_OwnerOnly);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	SpendRound();
	if (FireAnimation && FireEndAnimation && CharacterOwner)
	{
		if (Ammo <= 0 && CharacterOwner->IsLocallyControlled())
		{
			WeaponMesh->PlayAnimation(FireEndAnimation, false);
		}
		else
		{
			WeaponMesh->PlayAnimation(FireAnimation, false);
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	if (CharacterOwner)
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController)
		{
			GunfightOwnerController->SetHUDWeaponAmmo(Ammo);
			GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo);
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

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	BulletQueryParams.bReturnPhysicalMaterial = true;

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	// initialize on spawn sphere overlaps
	SetActorTickEnabled(true);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && GunfightCharacter->GetDefaultWeapon() == this)
	{
		if (OtherComp == GunfightCharacter->GetLeftHandSphere()) bLeftControllerOverlap = true;
		else if (OtherComp == GunfightCharacter->GetRightHandSphere()) bRightControllerOverlap = true;
		GunfightCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (CharacterOwner && OtherActor == CharacterOwner)
	{
		if (OtherComp == CharacterOwner->GetLeftHandSphere())
		{
			bLeftControllerOverlap = false;
			if(!bRightControllerOverlap) CharacterOwner->SetOverlappingWeapon(nullptr);
		}
		else if (OtherComp == CharacterOwner->GetRightHandSphere())
		{
			bRightControllerOverlap = false;
			if(!bLeftControllerOverlap) CharacterOwner->SetOverlappingWeapon(nullptr);
		}
		//GunfightCharacter->SetOverlappingWeapon(nullptr);
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

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	SetHUDAmmo();
}

void AWeapon::OnRep_CarriedAmmo()
{
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner && CharacterOwner->GetDefaultWeapon())
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController && CharacterOwner->GetDefaultWeapon() == this)
		{
			GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
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
				if (EmptyMag)
				{
					EmptyMag->GetMagazineMesh()->AddImpulse(FVector(ImpulseDir * MagDropImpulse));
				}
			}
		}
	}

	// spawn a full mag on opposite preferred holster

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
}

void AWeapon::OnEquipped()
{
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeapon::Dropped(bool bLeftHand)
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

}

void AWeapon::OnDropped()
{
	
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	
	// Delay and check if server location and client location is nearly equal or currently equipped, if not, then multicast move the gun to the correct location/rotation
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
	}
}

void AWeapon::SetServerSideRewind(bool bUseSSR)
{
	bUseServerSideRewind = bUseSSR;
}

void AWeapon::ShouldAttachToHolster()
{
	if (CharacterOwner)
	{
		UCombatComponent* Combat = CharacterOwner->GetCombat();
		if (Combat && !Combat->GetEquippedWeapon(true) || !Combat->GetEquippedWeapon(false))
		{
			const float DistSquared = FVector::DistSquared(CharacterOwner->GetActorLocation(), GetActorLocation());
			if (DistSquared > 14400.f)
			{
				Combat->AttachWeaponToHolster(this);
				return;
			}
		}
		else
		{
			return;
		}
		GetWorldTimerManager().SetTimer(ShouldHolsterTimerHandle, this, &AWeapon::ShouldAttachToHolster, 0.5f, false, 0.5f);
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
			}
			if (RightDistance <= FMath::Square(RightHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bRightControllerOverlap = true;
			}
		}
	}
}
