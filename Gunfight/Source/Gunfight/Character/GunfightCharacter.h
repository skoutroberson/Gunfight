// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "Gunfight/Gunfight.h"
#include "Gunfight/GunfightTypes/Hitbox.h"
#include "Gunfight/GunfightTypes/Team.h"
#include "GunfightCharacter.generated.h"

enum class EWeaponType : uint8;
enum class ESide : uint8;

class UMotionControllerComponent;

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightCharacter : public AVRCharacter
{
	GENERATED_BODY()
	
public:
	
	AGunfightCharacter();
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Restart() override;

	UFUNCTION(BlueprintCallable)
	void SetVREnabled(bool bEnabled);

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UPROPERTY(Replicated)
	bool bDisableShooting = false;

	void SpawnDefaultWeapon();

	void AttachMagazineToHolster(class AFullMagazine* FullMag, ESide HolsterSide = ESide::ES_None);

	void UpdateHUDAmmo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStereoLayerComponent* VRStereoLayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWidgetComponent* CharacterOverlayWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class AGunfightHUD* GunfightHUD;

	void Elim(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastElim(bool bPlayerLeftGame);
	void MultiCastElim_Implementation(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnBlood(const FVector_NetQuantize Location, const FVector_NetQuantize Normal, EHitbox HitType, int32 BoneIndex);
	void MulticastSpawnBlood_Implementation(const FVector_NetQuantize Location, const FVector_NetQuantize Normal, EHitbox HitType, int32 BoneIndex);

	// resets
	void Respawn(FVector_NetQuantize SpawnLocation, FRotator SpawnRotation, bool bInputDisabled = false);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRespawn(FVector_NetQuantize SpawnLocation, FRotator SpawnRotation, bool bInputDisabled = false);
	void MulticastRespawn_Implementation(FVector_NetQuantize SpawnLocation, FRotator SpawnRotation, bool bInputDisabled = false);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEnableInput();
	void MulticastEnableInput_Implementation();

	/**
	* Shared Spaces blueprint events for Meta Quest Multiplayer
	*/

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetCharacterMasterClientStatus();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetCharacterAttributes();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetCharacterIdentifier();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetCharacterName();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetCharacterDestination();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SaveNetID(const FString& NetID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RemoveNetID(const FString& NetID);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void NewHost();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void FireWeaponHaptic(bool bLeft);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ReceiveDamageHaptic();

	// Average velocity of the last 4 frames
	FVector LeftMotionControllerAverageVelocity = FVector::ZeroVector;
	// Average velocity of the last 4 frames
	FVector RightMotionControllerAverageVelocity = FVector::ZeroVector;
	// Average angular velocity of the last 4 frames
	FVector LeftMotionControllerAverageAngularVelocity = FVector::ZeroVector;
	// Average angular velocity of the last 4 frames
	FVector RightMotionControllerAverageAngularVelocity = FVector::ZeroVector;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ToggleMenu();

	void DestroyDefaultWeapon();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DeathPostProcess();
	void DeathPP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SpawnPostProcess();
	void SpawnPP();

	UFUNCTION(BlueprintCallable)
	void ReInitializeSubsystem();

	UFUNCTION(BlueprintCallable)
	void ShutDownGameInstance();

	UFUNCTION(BlueprintCallable)
	void RestartGameInstance();

	void SetTeamColor(ETeam Team);

	/**
	* Hand Offsets
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FVector RightHandMeshLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandMeshRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FVector LeftHandMeshLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FRotator LeftHandMeshRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* LeftHandMeshOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* RightHandMeshOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* LeftHandPistolOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* RightHandPistolOffset;

	USceneComponent* GetWeaponOffset(EWeaponType WeaponType, bool bLeft);


	bool bLeftHandAttached = false; // true if attached to a weapon
	bool bRightHandAttached = false; // true if attached to a weapon

	virtual void OnRep_AttachmentReplication() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAudioComponent* FootstepAudio;

	TArray<AWeapon*> LeftOverlappingWeapons;
	TArray<AWeapon*> RightOverlappingWeapons;

	// Weapon Skin
	void InitMyWeaponSkin(AWeapon* MyWeapon);

	void SpawnFullMagazine(TSubclassOf<AFullMagazine> FullMagClass, int32 SkinIndex, bool bLeft);

	void SetHUDAmmo(int32 Ammo, bool bLeft);
	void SetHUDCarriedAmmo(int32 Ammo, bool bLeft);

	UMotionControllerComponent* GetMotionControllerFromAttachment(FName AttachSocket, USceneComponent* AttachComponent);

	// will return right if given bad attachment info
	bool GetLeftFromAttachment(FName AttachSocket, USceneComponent* AttachComponent);

	void AttachHandMeshToMotionController(bool bLeft);

protected:
	virtual void BeginPlay() override;

	// input
	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	void TurnRight(float Throttle);
	void LookUp(float Throttle);

	DECLARE_DELEGATE_OneParam(FLeftRightButtonDelegate, const bool);

	void GripPressed(bool bLeftController);
	void GripReleased(bool bLeftController);
	void TriggerPressed(bool bLeftController);
	void TriggerReleased(bool bLeftController);
	void AButtonPressed(bool bLeftController);
	void AButtonReleased(bool bLeftController);
	void BButtonPressed(bool bLeftController);
	void BButtonReleased(bool bLeftController);

	void LeftStickPressed();
	void LeftStickReleased();
	bool bPressingLeftStick = false;

	void RightStickPressed();
	void RightStickReleased();
	bool bPressingRightStick = false;

	// For interacting with widgets like the pause menu or the main menu
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RightTriggerPressedUI();

	// For interacting with widgets like the pause menu or the main menu
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void LeftTriggerPressedUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RightTriggerReleasedUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void LeftTriggerReleasedUI();


	void MenuButtonPressed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bMenuOpen = false;

	UFUNCTION()
	void OnRep_DefaultWeapon();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();
	void InitializeHUD();

	// set in the blueprint after initializing the stereo layer. Set true in BeginPlay for all non locally controlled players so we don't try to initialize the stereo layer
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bStereoLayerInitialized = false;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void InitializeStereoLayer();

	UFUNCTION(BlueprintImplementableEvent)
	void InitializeHandAnimation(USkeletalMeshComponent* Hand);

	void HandleWeaponGripped(bool bLeftController, AWeapon* WeaponGripped);
	void HandleWeaponReleased(bool bLeftController, AWeapon* WeaponReleased);

	AWeapon* GetClosestOverlappingWeapon(bool bLeft);

	bool bPlayerInitialized = false;

private:

	/**
	* Gunfight components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRComponent> VRComponent;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent*  LagCompensation;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gun, meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<class UChildActorComponent> DefaultWeapon;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* LeftHandSphere;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* RightHandSphere;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AWeapon> WeaponClass;

	UPROPERTY()
	class AGunfightPlayerController* GunfightPlayerController;

	/**
	* Player health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	// blood spray

	UPROPERTY(EditAnywhere)
	UParticleSystem* BloodParticles;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* BloodNiagaraSystem;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactBodySound;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactHeadSound;

	bool bElimmed = false;
	FTimerHandle ElimTimer;
	void ElimTimerFinished();

	bool bWeaponInitialized = false;

	UPROPERTY()
	UWorld* World;

	class UGunfightAnimInstance* GunfightAnimInstance;

	void UpdateAnimInstanceIK(float DeltaTime);
	FVector TraceFootLocationIK(bool bLeft);
	FCollisionQueryParams IKQueryParams;

	void MoveMeshToCamera();

	void UpdateAnimation(float DeltaTime);

	UPROPERTY(VisibleAnywhere);
	bool bIsInVR = true;

	UFUNCTION(Server, Reliable)
	void ServerGripWeapon(bool bLeft);
	void ServerGripWeapon_Implementation(bool bLeft);

	UFUNCTION(Server, Reliable)
	void ServerReleaseWeapon(bool bLeft);
	void ServerReleaseWeapon_Implementation(bool bLeft);

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedLeft();
	void ServerEquipButtonPressedLeft_Implementation();
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedRight();
	void ServerEquipButtonPressedRight_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerEquipWeaponSecondaryLeft();
	void ServerEquipWeaponSecondaryLeft_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerEquipWeaponSecondaryRight();
	void ServerEquipWeaponSecondaryRight_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponLeft();
	void ServerDropWeaponLeft_Implementation();
	UFUNCTION(Server, Reliable)
	void ServerDropWeaponRight();
	void ServerDropWeaponRight_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponPrimaryLeft();
	void ServerDropWeaponPrimaryLeft_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponPrimaryRight();
	void ServerDropWeaponPrimaryRight_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponSecondaryLeft();
	void ServerDropWeaponSecondaryLeft_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponSecondaryRight();
	void ServerDropWeaponSecondaryRight_Implementation();

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(Replicated)
	class AWeapon* LeftOverlappingWeapon;

	UPROPERTY(Replicated)
	class AWeapon* RightOverlappingWeapon;


	UPROPERTY()
	class AFullMagazine* OverlappingMagazine;

	UPROPERTY()
	AFullMagazine* DefaultMagazine;

	UPROPERTY()
	AFullMagazine* LeftMagazine;

	UPROPERTY()
	AFullMagazine* RightMagazine;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(EditAnywhere, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState LeftHandState;

	UPROPERTY(EditAnywhere, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState RightHandState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bRightHolsterPreferred = true;

	void PollInit();

	bool bLocallyGrippingWeapon = false;

	UPROPERTY(ReplicatedUsing=OnRep_DefaultWeapon)
	AWeapon* DefaultWeapon;

	void DebugMagOverlap(bool bLeft);

	// UI

	void SetupUI();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UMaterialInstance* WidgetMaterial;

	UPROPERTY()
	class AGunfightGameMode* GunfightGameMode;

	bool bLeftGame = false;

	void Ragdoll();
	void UnRagdoll();

	FCollisionResponseContainer MeshCollisionResponses;

	UPROPERTY()
	class AGunfightPlayerState* GunfightPlayerState;

	/**
	* Keep track of average hand controller velocities
	* 
	* Used for updating dropped item velocities
	*/
	
	TArray<FVector> LeftMotionControllerVelocities;
	TArray<FVector> RightMotionControllerVelocities;
	TArray<FQuat> LastLeftMotionControllerRotations;
	TArray<FQuat> LastRightMotionControllerRotations;

	FVector LastLeftMotionControllerLocation = FVector::ZeroVector;
	FVector LastLeftMotionControllerRotation = FVector::ZeroVector;
	FVector LastRightMotionControllerLocation = FVector::ZeroVector;
	FVector LastRightMotionControllerRotation = FVector::ZeroVector;

	void UpdateAverageMotionControllerVelocities();

	UPROPERTY()
	class UGunfightSaveGame* GSaveGame;

	bool bSaveInit = false;

	// Settings

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bSnapTurning = false;
	bool bSnapped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float TurnSensitivity = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 MaxFPS = 90.f;

	// load saved game and apply saved settings
	void InitializeSettings();

	/**
	* Team colors
	*/

	UPROPERTY(EditAnywhere)
	UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere)
	UMaterialInstance* BlueMaterial;

	UPROPERTY(EditAnywhere)
	UMaterialInstance* OriginalMaterial;

	bool bDisableMovement = false; // if true, MoveForward() and MoveRight() will be disabled.

	/**
	* Team swap
	*/
	FTimerHandle TeamSwapTimer;
	void TeamSwapTimerFinished();

	bool bLocalHandsInitialized = false;
	void TryInitializeLocalHands();

	void SetLocalArmsVisible(bool bVisible);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class USkeletalMesh* HandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* RightHandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* LeftHandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UHandAnimInstance* RightHandAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UHandAnimInstance* LeftHandAnim;

	void UpdateFootstepSound(TWeakObjectPtr<class UPhysicalMaterial> PhysMat);

	AWeapon* ClosestValidOverlappingWeapon(bool bLeft);

	/**
	* Weapon two hand rotation offsets
	* 
	* These are used for rotating the weapon while holding it with both hands.
	* 
	* These offsets will be applied to rotate the weapon based on MotionController1 - HandOffsetPistolLeft
	* 
	*/

	void InitWeaponOffsets();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandOffsetPistolLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandOffsetPistolRight;

	// grip cooldowns to reduce frequency of grip rpcs. This will also act as a guard for OnReps coming out of order (doesn't fix the core problem though).
	bool bGripCooldownLeft = false;
	bool bGripCooldownRight = false;

	bool bPressingGripLeft = false;
	bool bPressingGripRight = false;

	bool bGripPressedLastLeft = false;
	bool bGripPressedLastRight = false;

	bool bRightGripFavored = true; // for if we are gripping both, flip this so each grip can be favored

	FTimerHandle GripLeftTimer;
	FTimerHandle GripRightTimer;

	void GripCooldownLeftFinished();
	void GripCooldownRightFinished();

	float GripCooldownTime = 0.05f;

	FTimerHandle CheckSlot2Timer;
	void CheckSlot2TimerFinished();

	UPROPERTY()
	USceneComponent* LeftHandIKComponent;

	UPROPERTY()
	USceneComponent* RightHandIKComponent;

public:
	void SetDefaultWeaponSkin(int32 SkinIndex);
	void SetOverlappingWeapon(AWeapon* Weapon);
	FORCEINLINE void SetLeftOverlappingWeapon(AWeapon* Weapon) { LeftOverlappingWeapon = Weapon; }
	FORCEINLINE void SetRightOverlappingWeapon(AWeapon* Weapon) { RightOverlappingWeapon = Weapon; }
	FORCEINLINE AWeapon* GetOverlappingWeapon() { return OverlappingWeapon; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	// returns true if right holster is preferred
	inline bool GetRightHolsterPreferred() { return bRightHolsterPreferred; }
	inline bool GetLocallyGrippingWeapon() { return bLocallyGrippingWeapon; }
	void SetHandState(bool bLeftHand, EHandState NewState);
	inline EHandState GetHandState(bool bLeftHand) { return (bLeftHand) ? LeftHandState : RightHandState; }
	FORCEINLINE AWeapon* GetDefaultWeapon() { return DefaultWeapon; }
	void SetDefaultWeapon(AWeapon* NewDefaultWeapon);
	FORCEINLINE void SetOverlappingMagazine(AFullMagazine* OverlapMag) { OverlappingMagazine = OverlapMag; }
	FORCEINLINE USphereComponent* GetLeftHandSphere() { return LeftHandSphere; }
	FORCEINLINE USphereComponent* GetRightHandSphere() { return RightHandSphere; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() {return LagCompensation;}
	FORCEINLINE bool IsEliminated() const { return bElimmed; }
	FORCEINLINE void SetDisableMovement(bool bMovementDisabled) { bDisableMovement = bMovementDisabled; }
	FORCEINLINE USkeletalMeshComponent* GetRightHandMesh() { return RightHandMesh; }
	FORCEINLINE USkeletalMeshComponent* GetLeftHandMesh() { return LeftHandMesh; }
	FORCEINLINE USkeletalMeshComponent* GetHandMesh(bool bLeft) { return bLeft ? LeftHandMesh : RightHandMesh; }
	FORCEINLINE AGunfightPlayerController* GetGunfightPlayerController() { return GunfightPlayerController; }
	EHandState GetGripState(bool bLeft);

	USceneComponent* GetHandWeaponOffset(EWeaponType WeaponType, bool bLeft);

	void UpdateAnimationHandComponent(bool bLeft, USceneComponent* NewHandComponent);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE void SetSnapTurning(bool bShouldSnap) { bSnapTurning = bShouldSnap; }

	UFUNCTION(BlueprintCallable)
	FVector GetLeftMotionControllerAverageVelocity();
	UFUNCTION(BlueprintCallable)
	FVector GetRightMotionControllerAverageVelocity();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DebugLogMessage(const FString& LogText);

	bool IsGunCloserThanMag(AActor* Gun, AActor* Mag, USceneComponent* Hand);
	void ReleaseGrip(bool bLeft);

	bool DoesHandHaveWeapon(bool bLeft);

	UFUNCTION(BlueprintCallable)
	void InitializeHUDOnWidgetInit();
};
