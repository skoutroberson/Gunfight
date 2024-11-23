// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine\PhysicsAsset.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Gunfight.h"
#include "Gunfight/Character/GunfightCharacterDeprecated.h"
#include "MotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EmptyMagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmptyMag"));
	EmptyMagazineMesh->SetupAttachment(GetRootComponent());
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionObjectType(ECC_Weapon);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetUseCCD(true);
	AreaSphere->SetSphereRadius(0.f);
	
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

	AGunfightCharacterDeprecated* GunfightCharacter = Cast<AGunfightCharacterDeprecated>(GetOwner());

	if (GunfightCharacter)
	{
		Character = GunfightCharacter;

		if (GunfightCharacter->IsLocallyControlled())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
			AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
			AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
		}
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGunfightCharacterDeprecated* GunfightCharacterDeprecated = Cast<AGunfightCharacterDeprecated>(OtherActor);
	if (GunfightCharacterDeprecated && GunfightCharacterDeprecated == GetOwner())
	{
		GunfightCharacterDeprecated->SetOverlappingWeapon(this);

		if (IsOverlappingControllerSideLeft(OtherComp))
		{
			bLeftControllerOverlap = true;
		}
		else
		{
			bRightControllerOverlap = true;
		}
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGunfightCharacterDeprecated* GunfightCharacter = Cast<AGunfightCharacterDeprecated>(OtherActor);
	if (GunfightCharacter && GunfightCharacter == GetOwner())
	{
		GunfightCharacter->SetOverlappingWeapon(nullptr);

		if (IsOverlappingControllerSideLeft(OtherComp))
		{
			bLeftControllerOverlap = false;
		}
		else
		{
			bRightControllerOverlap = false;
		}
	}
}

bool AWeapon::IsOverlappingControllerSideLeft(UPrimitiveComponent* OverlappingController)
{
	if (OverlappingController && Character)
	{
		return OverlappingController == Character->GetLeftHandController() ? true : false;
	}
	return false;
}

void AWeapon::SetMagazineSimulatePhysics(bool bTrue)
{
	FBodyInstance* Magazine = WeaponMesh->GetBodyInstance(FName("Colt_Magazine"));
	if (Magazine)
	{
		Magazine->SetInstanceSimulatePhysics(bTrue);
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
	
	//PollInit();
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

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;

}

void AWeapon::EjectMagazine()
{
	if (bMagInserted && MagEjectAnimation)
	{
		bMagInserted = false;
		WeaponMesh->PlayAnimation(MagEjectAnimation, false);
		GetWorldTimerManager().SetTimer(EjectMagTimerHandle, this, &AWeapon::DropMag, 0.2f, false, 0.2f);
	}
}

void AWeapon::DropMag()
{
	if (WeaponMesh == nullptr || EmptyMagazineMesh == nullptr) return;
	
	const USkeletalMeshSocket* MeshSocket = WeaponMesh->GetSocketByName(TEXT("EmptyMag"));
	const FTransform SocketTransform = MeshSocket->GetSocketTransform(WeaponMesh);

	EmptyMagazineMesh->SetWorldLocationAndRotation(SocketTransform.GetLocation(), SocketTransform.GetRotation());
	EmptyMagazineMesh->SetVisibility(true);
	EmptyMagazineMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	EmptyMagazineMesh->SetSimulatePhysics(true);
	EmptyMagazineMesh->SetEnableGravity(true);
	WeaponMesh->HideBoneByName(FName("Colt_Magazine"), EPhysBodyOp::PBO_None);
	GetWorldTimerManager().SetTimer(EjectMagTimerHandle, this, &AWeapon::ResetMag, 3.f, false, 3.f);

	// spawn a full mag on opposite preferred holster

}

void AWeapon::ResetMag()
{
	EmptyMagazineMesh->SetVisibility(false);
	EmptyMagazineMesh->SetSimulatePhysics(false);
	EmptyMagazineMesh->SetEnableGravity(false);
	EmptyMagazineMesh->SetRelativeLocation(FVector::ZeroVector);
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{

}