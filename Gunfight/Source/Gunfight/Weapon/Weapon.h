// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gunfight/Gunfight.h"
#include "Weapon.generated.h"

#define TRACE_LENGTH 80000.f

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
	EWS_Min				UMETA(DisplayName = "DefaultMIN"),
	EWS_Equipped		UMETA(DisplayName = "Equipped"),
	EWS_Dropped			UMETA(DisplayName = "Dropped"),
	EWS_Ready			UMETA(DisplayName = "Ready"),
	EWS_Empty			UMETA(DisplayName = "Empty"),

	EWS_MAX				UMETA(DisplayName = "DefaultMAX"),
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Fire(const FVector& HitTarget);

	void SetHUDAmmo();

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

	FVector TraceEndWithScatter(const FVector& HitTarget);

	bool bLeftControllerOverlap = false;
	bool bRightControllerOverlap = false;

	// called at the end of the slide end animation to hide the magazine bone, and spawn an empty magazine to simulate physics
	UFUNCTION(BlueprintCallable)
	void DropMag();

	void InitializeAnim();

	FTimerHandle EjectMagTimerHandle;

	void Dropped(bool bLeftHand);

	UPROPERTY()
	class AGunfightCharacter* CharacterOwner;

	UPROPERTY()
	class AGunfightPlayerController* GunfightOwnerController;

	void UnhideMag();

	void SetCarriedAmmo(int32 NewAmmo) { CarriedAmmo = NewAmmo; }
	void AddAmmo(int32 AmmoToAdd);

	void PlaySlideBackAnimation();
	void PlaySlideForwardAnimation();

	void SetServerSideRewind(bool bUseSSR);

	// Used to determine which hand controller velocity to apply to the weapon when dropped.
	UPROPERTY(Replicated, BlueprintReadWrite)
	ESide WeaponSide = ESide::ES_None;

protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();

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

	/**
	* Trace end with scatter
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	FCollisionQueryParams BulletQueryParams;

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USceneComponent* MagSlideEnd;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USceneComponent* MagSlideStart;

	UPROPERTY(VisibleAnywhere)
	EEquipState EquipState;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	void OnEquipped();
	void OnEquippedRight();
	void OnDropped();


	void SetMagazineSimulatePhysics(bool bTrue);

	void EjectMag();

	void SlideMagOut(float DeltaTime);

	bool bSlidingMag = false;

	bool bInitDelayCompleted = false;
	uint8 InitDelayFrame = 0;

	void PollInit();

	UPROPERTY(VisibleAnywhere)
	bool bBeingGripped = false;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo, EditAnywhere)
	int32 CarriedAmmo = 0;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	void ClientUpdateAmmo_Implementation(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);
	void ClientAddAmmo_Implementation(int32 AmmoToAdd);

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// The number of unprocessed server requests for Ammo
	// Incremented in SpendRound, decremented in ClientUpdateAmmo
	int32 Sequence = 0;

	// Carried ammo for this weapon
	//UPROPERTY(ReplicatedUsing = OnRep_CarriedMags, EditAnywhere)
	//int32 CarriedMags = 0;

	//UFUNCTION()
	//void OnRep_CarriedMags();

	UPROPERTY(VisibleAnywhere)
	bool bMagInserted = true;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireEndAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* MagEjectAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* SlideBackAnimationPose;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* SlideBackAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* SlideForwardAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AEmptyMagazine> EmptyMagazineClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AFullMagazine> FullMagazineClass;

	UPROPERTY(EditAnywhere)
	float MagDropDelay = 0.22f;

	UPROPERTY(EditAnywhere)
	float MagDropImpulse = 5.f;

	// for holstering the weapon if it is far from the player
	FTimerHandle ShouldHolsterTimer;

	void ShouldHolster();

	bool bDropRPCCalled = true;
	bool bEquipRPCCalled = false;

	void ShouldAttachToHolster();

	// if weapon is unequipped and greater than this distance, attach to holster
	UPROPERTY(EditAnywhere)
	float MaxDistanceFromHolster = 80.f;

	void GetSpawnOverlaps();

	FVector DropVelocity;
	FQuat DropAngularVelocity;

	void ApplyVelocityOnDropped();

public:

	bool IsOverlappingHand(bool bLeftHand);

	inline bool IsBeingGripped() { return bBeingGripped; }
	inline void SetBeingGripped(bool bGripped) { bBeingGripped = bGripped; }
	inline void SetEquipState(EEquipState NewState) { EquipState = NewState; }
	void SetWeaponState(EWeaponState NewState);
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE void SetAmmo(int32 NewAmmo) { Ammo = NewAmmo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	//FORCEINLINE int32 GetCarriedMags() const { return CarriedMags; }
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; }
	FORCEINLINE bool IsMagInserted() const { return bMagInserted; }
	FORCEINLINE void SetMagInserted(bool bIsInserted) { bMagInserted = bIsInserted; }
	void EjectMagazine();
	FORCEINLINE bool WasDropRPCCalled() const { return bDropRPCCalled; }
	// sets bDropRPCCalled to false
	FORCEINLINE void ResetDropRPCCalled() { bDropRPCCalled = false; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }
	FORCEINLINE void SetCharacterOwner(AGunfightCharacter* Character) { CharacterOwner = Character; }
	FORCEINLINE TSubclassOf<AFullMagazine> GetFullMagazineClass() { return FullMagazineClass; }
	// returns true if the passed in hand class variable: bOverlappingLeftHand or bOverlappingRightHand is true.
	bool CheckHandOverlap(bool bLeftHand);
	FVector GetMagwellDirection() const;
	FORCEINLINE USceneComponent* GetMagwellEnd() { return MagSlideEnd; }
	FORCEINLINE USceneComponent* GetMagwellStart() { return MagSlideStart; }
	FORCEINLINE EWeaponState GetWeaponState() { return WeaponState; }
	FORCEINLINE float GetDamage() { return Damage; }
	FORCEINLINE void SetDropVelocity(FVector NewVelocity) { DropVelocity = NewVelocity; }
	FORCEINLINE void SetDropAngularVelocity(FQuat NewAngularVelocity) { DropAngularVelocity = NewAngularVelocity; }
	FORCEINLINE void SetWeaponSide(ESide NewSide) { WeaponSide = NewSide; }
};
