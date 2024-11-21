// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EEquipState : uint8
{
	EES_Holstered		UMETA(DisplayName = "Holstered"),
	EES_Equipped		UMETA(DisplayName = "Equipped"),
	EES_Dropped			UMETA(DisplayName = "Dropped"),

	EES_MAX				UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Ready		UMETA(DisplayName = "Ready"),
	EWS_Ejecting	UMETA(DisplayName = "Ejecting"),
	EWS_Ejected		UMETA(DisplayName = "Ejected"),
	EWS_Empty		UMETA(DisplayName = "Empty"),

	EWS_MAX			UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan		UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile	UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun		UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX			UMETA(DisplayName = "DefaultMAX"),
};

UCLASS()
class GUNFIGHT_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

	/**
	* Automatic fire
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	bool bLeftControllerOverlap = false;
	bool bRightControllerOverlap = false;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	// assumes that OverlappingController is a UMotionController, returns true if its the left side
	bool IsOverlappingControllerSideLeft(UPrimitiveComponent* OverlappingController);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USceneComponent* MagSlideEnd;

	EEquipState EquipState;

	EWeaponState WeaponState;


	void SetMagazineSimulatePhysics(bool bTrue);

	void EjectMag();

	void SlideMagOut(float DeltaTime);

	bool bSlidingMag = false;

	UPROPERTY()
	class AGunfightCharacterDeprecated* Character;

	bool bInitDelayCompleted = false;
	uint8 InitDelayFrame = 0;

	void PollInit();

public:
	
};
