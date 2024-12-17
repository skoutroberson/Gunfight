// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "Gunfight/Gunfight.h"
#include "GunfightCharacter.generated.h"

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

	UFUNCTION(BlueprintCallable)
	void SetVREnabled(bool bEnabled);

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	void SpawnDefaultWeapon();

	void AttachMagazineToHolster();

	void UpdateHUDAmmo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStereoLayerComponent* VRStereoLayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWidgetComponent* CharacterOverlayWidget;

	void Elim(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastElim(bool bPlayerLeftGame);
	void MultiCastElim_Implementation(bool bPlayerLeftGame);

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

	UFUNCTION()
	void OnRep_DefaultWeapon();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();
	void InitializeHUD();

	// set in the blueprint after initializing the stereo layer. Set true in BeginPlay for all non locally controlled players so we don't try to initialize the stereo layer
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bStereoLayerInitialized = false;

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

	bool bElimmed = false;
	FTimerHandle ElimTimer;
	void ElimTimerFinished();

	bool bWeaponInitialized = false;

	UPROPERTY()
	UWorld* World;

	class UGunfightAnimInstance* GunfightAnimInstance;

	void UpdateAnimInstanceIK();
	FVector TraceFootLocationIK(bool bLeft);
	FCollisionQueryParams IKQueryParams;

	void MoveMeshToCamera();

	void UpdateAnimation();

	UPROPERTY(VisibleAnywhere);
	bool bIsInVR = true;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedLeft();
	void ServerEquipButtonPressedLeft_Implementation();
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedRight();
	void ServerEquipButtonPressedRight_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerDropWeaponLeft();
	void ServerDropWeaponLeft_Implementation();
	UFUNCTION(Server, Reliable)
	void ServerDropWeaponRight();
	void ServerDropWeaponRight_Implementation();

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY()
	class AFullMagazine* OverlappingMagazine;

	UPROPERTY()
	AFullMagazine* DefaultMagazine;

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

	void SpawnFullMagazine(TSubclassOf<AFullMagazine> FullMagClass);

	void DebugMagOverlap(bool bLeft);

	// UI

	void SetupUI();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UMaterialInstance* WidgetMaterial;

	UPROPERTY()
	class AGunfightGameMode* GunfightGameMode;

	bool bLeftGame = false;

	void Ragdoll();

	FCollisionResponseContainer MeshCollisionResponses;

	UPROPERTY()
	class AGunfightPlayerState* GunfightPlayerState;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	FORCEINLINE AWeapon* GetOverlappingWeapon() { return OverlappingWeapon; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	// returns true if right holster is preferred
	inline bool GetRightHolsterPreferred() { return bRightHolsterPreferred; }
	inline bool GetLocallyGrippingWeapon() { return bLocallyGrippingWeapon; }
	void SetHandState(bool bLeftHand, EHandState NewState);
	inline EHandState GetHandState(bool bLeftHand) { return (bLeftHand) ? LeftHandState : RightHandState; }
	FORCEINLINE AWeapon* GetDefaultWeapon() { return DefaultWeapon; }
	FORCEINLINE void SetOverlappingMagazine(AFullMagazine* OverlapMag) { OverlappingMagazine = OverlapMag; }
	FORCEINLINE USphereComponent* GetLeftHandSphere() { return LeftHandSphere; }
	FORCEINLINE USphereComponent* GetRightHandSphere() { return RightHandSphere; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() {return LagCompensation;}
	FORCEINLINE bool IsEliminated() const { return bElimmed; }
};
