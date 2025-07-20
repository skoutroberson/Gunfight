// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gunfight/Gunfight.h"
#include "Weapon.generated.h"

#define TRACE_LENGTH 80000.f

enum class EHandState : uint8;

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
	EWS_MIN				UMETA(DisplayName = "DefaultMIN"),
	EWS_Equipped		UMETA(DisplayName = "Equipped"),
	EWS_Dropped			UMETA(DisplayName = "Dropped"),
	EWS_Holstered		UMETA(DisplayName = "Holstered"),

	EWS_MAX				UMETA(DisplayName = "DefaultMAX"),
};

// might need to use this so attachment replication is never out of order.
USTRUCT(BlueprintType)
struct FWeaponStateGuarded
{
	GENERATED_BODY()

	UPROPERTY()
	EWeaponState State;

	UPROPERTY()
	AActor* Owner;

	UPROPERTY();
	ESide EquipSide; // if not none, attach to this side.
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan		UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile	UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun		UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX			UMETA(DisplayName = "DefaultMAX"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Pistol	UMETA(DisplayName = "Pistol"),
	EWT_M4		UMETA(DisplayName = "M4"),

	EWT_MAX		UMETA(DisplayName = "DefaultMAX"),
};

// variables we need to replicate to a client on join session
USTRUCT(BlueprintType)
struct FWeaponReplicate
{
	GENERATED_BODY()

	int32 SkinIndex;
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

	virtual void Fire(const FVector& HitTarget, bool bLeft);

	void SetHUDAmmo(bool bLeft);
	void SetHUDCarriedAmmo(bool bLeft);
	// called when EquippedWeapons are set to nullptr
	void ClearHUDAmmo(bool bLeft);

	/**
	* Automatic fire
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	USoundCue* DryFireSound;

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
	void Equipped(bool bLeftHand);

	UPROPERTY()
	class AGunfightCharacter* CharacterOwner;

	UPROPERTY()
	class AGunfightPlayerController* GunfightOwnerController;

	void UnhideMag();

	void SetCarriedAmmo(int32 NewAmmo) { CarriedAmmo = NewAmmo; }
	void AddAmmo(int32 AmmoToAdd, bool bLeft);

	void PlaySlideBackAnimation();
	void PlaySlideForwardAnimation();

	void SetServerSideRewind(bool bUseSSR);

	// Used to determine which hand controller velocity to apply to the weapon when dropped.
	UPROPERTY(Replicated, BlueprintReadWrite)
	ESide WeaponSide = ESide::ES_None;

	void PlayReloadSound();
	void PlayHolsterSound();
	void PlayDryFireSound();

	UPROPERTY(EditAnywhere)
	USoundCue* ReloadSound;

	UPROPERTY(EditAnywhere)
	USoundCue* HolsterSound;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropWeapon(FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity);
	void MulticastDropWeapon_Implementation(FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity);

	// Skins

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UMaterialInterface*> WeaponSkins;

	void SetWeaponSkin(int32 SkinIndex);

	UFUNCTION(Server, Reliable)
	void ServerSetWeaponSkin(int32 SkinIndex);
	void ServerSetWeaponSkin_Implementation(int32 SkinIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform GrabOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform GrabOffset2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform HandOffsetRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform HandOffsetLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform HandOffsetRight2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform HandOffsetLeft2;


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot1R;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot1L;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot2R;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot2L;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot2IKL;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USceneComponent* GrabSlot2IKR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bTwoHanded = false;

	virtual void OnRep_AttachmentReplication() override;

	UPROPERTY()
	FRepAttachment PreviousAttachmentReplication;

	UPROPERTY()
	AActor* PreviousAttachParent;

	UPROPERTY()
	USceneComponent* PreviousAttachComponent;

	UPROPERTY()
	FName PreviousAttachSocket;

	// don't allow putting the gun through walls and firing

	// returns true if the weapon is clipping through a wall/object
	bool IsObstructed() const;

	// distance to trace going backwards from the fire location to determine of the weapon is clipping through a wall/object.
	UPROPERTY(EditAnywhere)
	float ObstructedDistance = 20.f;

	// called when we drop the weapon in case we drop it while the mag is dropping out.
	void ClearWeaponMagTimer();

	// two hand
	void StartRotatingTwoHand(USceneComponent* NewHand1, USceneComponent* NewHand2);
	void StopRotatingTwoHand();

	bool bRotateTwoHand = false;

	UPROPERTY()
	USceneComponent* Hand1;

	UPROPERTY()
	USceneComponent* Hand2;

	UPROPERTY(EditAnywhere)
	float TwoHandRotationSpeed = 10.f;

	void ResetWeapon();

	bool bSlideBack = false;

	// GRIP REWORK
	void DetachAndSimulate();
	void AttachAndStopPhysics(USceneComponent* AttachComponent, const FName& SocketName, bool bLeft);
	void StopPhysics();
	void StartPhysics();

	//UPROPERTY(ReplicatedUsing= OnRep_Slot1MotionController)
	UPROPERTY()
	class UMotionControllerComponent* Slot1MotionController;

	UPROPERTY(ReplicatedUsing = OnRep_Slot2MotionController)
	UMotionControllerComponent* Slot2MotionController;

	UMotionControllerComponent* PreviousSlot1MotionController;
	UMotionControllerComponent* PreviousSlot2MotionController;

	//UFUNCTION()
	//void OnRep_Slot1MotionController();

	UFUNCTION()
	void OnRep_Slot2MotionController();

	/**
	* Attachment Replication
	* 
	* The Weapon can only be attached to a holster or hand
	*/
	void HandleAttachmentReplication();
	void HandleAttachmentReplication2();

	void PlayEquipSound();

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

	//UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
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

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 100;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo, bool bLeft);
	void ClientUpdateAmmo_Implementation(int32 ServerAmmo, bool bLeft);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd, bool bLeft);
	void ClientAddAmmo_Implementation(int32 AmmoToAdd, bool bLeft);

	void SpendRound(bool bLeft);

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

	void EquipSoundTimerEnd();
	FTimerHandle EquipSoundTimer;
	bool bPlayingEquipSound = false;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentSkinIndex)
	int32 CurrentSkinIndex = 0;

	UFUNCTION()
	void OnRep_CurrentSkinIndex();

	// The magazine spawned locally that will trigger a reload if inserted by the player.
	UPROPERTY()
	AFullMagazine* Magazine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EWeaponType WeaponType;

	ESide HolsterSide = ESide::ES_None;

	float LastTimeInteractedWith = WEAPON_START_TIME;

	// Grab Rework

	void HandleSlot1Grabbed(UMotionControllerComponent* GrabbingController);
	void HandleSlot1Released(UMotionControllerComponent* ReleasingController);

	void HandleSlot2Grabbed(UMotionControllerComponent* GrabbingController);
	void HandleSlot2Released(UMotionControllerComponent* ReleasingController);

	EHandState SlotToHandState(bool bSlot1);

	bool bPlayingHolsterSound = false;
	FTimerHandle HolsterSoundTimer;
	void HolsterSoundTimerFinished() { bPlayingHolsterSound = false; }

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
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE void SetDropVelocity(FVector NewVelocity) { DropVelocity = NewVelocity; }
	FORCEINLINE void SetDropAngularVelocity(FQuat NewAngularVelocity) { DropAngularVelocity = NewAngularVelocity; }
	FORCEINLINE void SetWeaponSide(ESide NewSide) { WeaponSide = NewSide; }
	FORCEINLINE USphereComponent* GetAreaSphere() { return AreaSphere; }
	FORCEINLINE int32 GetCurrentSkinIndex() const { return CurrentSkinIndex; }
	FORCEINLINE AFullMagazine* GetMagazine() { return Magazine; }
	FORCEINLINE void SetMagazine(AFullMagazine* NewMag) { Magazine = NewMag; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE ESide GetHolsterSide() const { return HolsterSide; }
	FORCEINLINE void SetHolsterSide(ESide NewHolsterSide) { HolsterSide = NewHolsterSide; }
	FORCEINLINE int32 GetMaxCarriedAmmo() const { return MaxCarriedAmmo; }
	FORCEINLINE float GetLastTimeInteractedWith() const { return LastTimeInteractedWith; }
	FORCEINLINE bool IsTwoHanded() const { return bTwoHanded; }
	FORCEINLINE void SetLastTimeInteractedWith(float NewLastTime) { LastTimeInteractedWith = NewLastTime; }
	USceneComponent* GetSlot2IK(bool bLeft);
};
