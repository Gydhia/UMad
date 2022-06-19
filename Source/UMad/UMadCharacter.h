// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include <GameplayEffectTypes.h>

#include "CableActor.h"
#include "GrapplingAttachActor.h"
#include "UMadCharacter.generated.h"

UCLASS(Config=Game)
class UMAD_API AUMadCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GAS, meta = (AllowPrivateAccess = "true"))
	class UUMadAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collider, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* GrappleAttachesCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Grapple, meta = (AllowPrivateAccess = "true"))
	class UGrappleComponent* GrappleComp;
	
	UPROPERTY()
	class UUMadAttributeSet* Attributes;
	
public:
	AUMadCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(BlueprintReadWrite)
	bool IsDead = false;
	
	UPROPERTY(BlueprintReadWrite)
	bool IsUsingGrapple;
	
	UPROPERTY(BlueprintReadWrite)
	bool HasReleasedGrapple;

	UPROPERTY(BlueprintReadWrite)
	float RagdollDelay = 2.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveFloat* GrappleForce = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GrapplingHook, meta = (AllowPrivateAccess = "true"))
	float GrappleTimeBeforeExplosion = 2.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GrappleChargeTime = 0.6f;
	
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitializeAttributes();
	virtual void GiveAbilities();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void AddPossibleGrapplingAttach(AGrapplingAttachActor* Actor);
	void RemovePossibleGrapplingAttach(AGrapplingAttachActor* Actor);
	void GetNearestAttach();

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UMad")
	TSubclassOf<class UGameplayEffect> DefaultAttributeEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UMad")
	TArray<TSubclassOf<class UUMadGameplayAbility>> DefaultAbilities;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "UMad")
	TArray<AGrapplingAttachActor*> PossibleGrapplingAttaches;

	AGrapplingAttachActor* NearestGrapplingAttach = nullptr;
	AGrapplingAttachActor* CurrentGrapplingAttach = nullptr;

	UFUNCTION(BlueprintCallable)
	void HitByTrap(FVector Impulse);
	
	virtual void Tick(float DeltaSeconds) override;
	float GetAngleFromAttach(FVector Start, FVector Target);
protected:
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);
	void StartGrappling();
	void ResetBinding(FInputAxisBinding bind);
	bool CompareInputActionBindings(FInputAxisBinding lhs, FInputAxisBinding rhs);
	void EndGrappling();
	void Ragdoll();
	void EndRagdoll();

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
private:
	bool _initedInputs = false;
	bool _isUnderRagdoll = false;
	float _ragdollTimer = 0;
	float _attachesTimer = -1;
	float _beginGrapple = -1;
	float _grappleTimer = -1;
	ACableActor* _grappleLine;
	FVector _impulse = FVector::Zero();
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface
	
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	virtual void BeginPlay() override;
	void SetupGrappleForce();
};
