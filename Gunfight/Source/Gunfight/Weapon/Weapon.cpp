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
	AreaSphere->SetSphereRadius(0.f);
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

	DOREPLIFETIME_CONDITION(AWeapon, CarriedMags, COND_OwnerOnly);
	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	SetActorTickEnabled(true);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && GunfightCharacter->GetDefaultWeapon() == this)
	{
		GunfightCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && CharacterOwner == OtherActor)
	{
		GunfightCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::EjectMag()
{
}

void AWeapon::SlideMagOut(float DeltaTime)
{
	
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
			AreaSphere->SetSphereRadius(100.f, true);
			SetActorTickEnabled(false);
		}
	}
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
}

void AWeapon::SpendRound()
{
}

void AWeapon::OnRep_CarriedMags()
{

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

void AWeapon::DropMag()
{
	if (WeaponMesh == nullptr) return;

	WeaponMesh->HideBoneByName(FName("Colt_Magazine"), EPhysBodyOp::PBO_None);

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
	UE_LOG(LogTemp, Warning, TEXT("On Weapon State Set"));
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
	UE_LOG(LogTemp, Warning, TEXT("On Equipped"));
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
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	//GetWorldTimerManager().SetTimer(ShouldHolsterTimerHandle, this, &AWeapon::ShouldAttachToHolster, 0.25f, true, 0.25f);
}

void AWeapon::ShouldAttachToHolster()
{
	if (bBeingGripped)
	{
		GetWorldTimerManager().ClearTimer(ShouldHolsterTimerHandle);
		return;
	}
	if (CharacterOwner && CharacterOwner->GetMesh() && CharacterOwner->GetCombat())
	{
		const FVector HolsterLocation = CharacterOwner->GetRightHolsterPreferred() ? 
			CharacterOwner->GetMesh()->GetSocketLocation(FName("RightHolster")) : CharacterOwner->GetMesh()->GetSocketLocation(FName("LeftHolster"));
		const float DistanceFromHolsterSquared = FVector::DistSquared(HolsterLocation, GetActorLocation());
		if (DistanceFromHolsterSquared > FMath::Square(MaxDistanceFromHolster))
		{
			CharacterOwner->GetCombat()->AttachWeaponToHolster(this);
			GetWorldTimerManager().ClearTimer(ShouldHolsterTimerHandle);
		}
	}

}
