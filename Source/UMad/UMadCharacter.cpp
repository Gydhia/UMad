// Fill out your copyright notice in the Description page of Project Settings.


#include "UMadCharacter.h"


#include "UMadAbilitySystemComponent.h"
#include "UMadAttributeSet.h"
#include "UUMadGameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "GrappleComponent.h"
#include "AI/NavigationSystemBase.h"

//UENUM(BlueprintType)
UENUM()
enum Status
{
	Retracted     UMETA(DisplayName = "Retracted"),
	Firing      UMETA(DisplayName = "Firing"),
	NearingTarget   UMETA(DisplayName = "NearingTarget"),
	OnTarget	UMETA(DisplayName = "OnTarget")
};

// Sets default values
AUMadCharacter::AUMadCharacter()
{
 	// Set size for collision capsule
    	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    
    	// set our turn rates for input
    	BaseTurnRate = 45.f;
    	BaseLookUpRate = 45.f;
    
    	// Don't rotate when the controller rotates. Let that just affect the camera.
    	bUseControllerRotationPitch = false;
    	bUseControllerRotationYaw = false;
    	bUseControllerRotationRoll = false;
    
    	// Configure character movement
    	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
    	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
    	GetCharacterMovement()->JumpZVelocity = 600.f;
    	GetCharacterMovement()->AirControl = 0.2f;
    
    	// Create a camera boom (pulls in towards the player if there is a collision)
    	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    	CameraBoom->SetupAttachment(RootComponent);
    	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
    	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

		GrappleComp = CreateDefaultSubobject<UGrappleComponent>(TEXT("Grapple Component"));
		GrappleComp->Owner = this;

		GrappleForce = NewObject<UCurveFloat>();
		GrappleForce->FloatCurve.AddKey(0.0f, 250.0f);
		GrappleForce->FloatCurve.AddKey(GrappleChargeTime, 1500.0f);
		GrappleForce->FloatCurve.AddKey(GrappleTimeBeforeExplosion, 15000.0f);
	
    	// Create a follow camera
    	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
    
    	AbilitySystemComponent = CreateDefaultSubobject<UUMadAbilitySystemComponent>(TEXT("AbilitySystemComp"));
    	AbilitySystemComponent->SetIsReplicated(true);
    	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
    
    	Attributes = CreateDefaultSubobject<UUMadAttributeSet>("Attributes");
    	
    	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

}

// Called to bind functionality to input
void AUMadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
    	check(PlayerInputComponent);
    	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
		PlayerInputComponent->BindAction("Grapple", IE_Pressed, this, &AUMadCharacter::StartGrappling);
		PlayerInputComponent->BindAction("Grapple", IE_Released, this, &AUMadCharacter::EndGrappling);
		PlayerInputComponent->BindAction("Ragdoll", IE_Pressed, this, &AUMadCharacter::Ragdoll);
	
    	PlayerInputComponent->BindAxis("MoveForward", this, &AUMadCharacter::MoveForward);
    	PlayerInputComponent->BindAxis("MoveRight", this, &AUMadCharacter::MoveRight);
    
    	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
    	// "turn" handles devices that provide an absolute delta, such as a mouse.
    	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
    	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    	PlayerInputComponent->BindAxis("TurnRate", this, &AUMadCharacter::TurnAtRate);
    	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
    	PlayerInputComponent->BindAxis("LookUpRate", this, &AUMadCharacter::LookUpAtRate);
	
    	if(AbilitySystemComponent && InputComponent)
    	{
    		const FGameplayAbilityInputBinds Binds (
    			"Confirm","Cancel","EUMadAbilityInputID",
    			static_cast<int32>(EUMadAbilityInputID::Confirm), static_cast<int32>(EUMadAbilityInputID::Cancel));
    		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
    	}

}

void AUMadCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AUMadCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

UAbilitySystemComponent* AUMadCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AUMadCharacter::InitializeAttributes()
{
	if(AbilitySystemComponent && DefaultAttributeEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1, EffectContext);

		if(SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle GEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AUMadCharacter::GiveAbilities()
{
	if(HasAuthority() && AbilitySystemComponent && !_initedInputs)
	{
		_initedInputs = true;
		
		for (TSubclassOf<UUMadGameplayAbility>& StartupAbility : DefaultAbilities)
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(StartupAbility, 1, static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
		}
	}
}

void AUMadCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Orthar server init
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	InitializeAttributes();
	GiveAbilities();
}

void AUMadCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// AbilitySystemComponent->InitAbilityActorInfo(this, this);
	// InitializeAttributes();

	// if(AbilitySystemComponent && InputComponent)
	// {
	// 	const FGameplayAbilityInputBinds Binds (
	// 		"Confirm","Cancel","EOrtharAbilityInputID",
	// 		static_cast<int32>(EOrtharAbilityInputID::Confirm), static_cast<int32>(EOrtharAbilityInputID::Cancel));
	// 	AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);
	// }
}

#pragma region GrapplingHook

void AUMadCharacter::AddPossibleGrapplingAttach(AGrapplingAttachActor* Actor)
{
	if(Actor != nullptr)
	{
		PossibleGrapplingAttaches.Add(Actor);
		
		GetNearestAttach();
	}
}

void AUMadCharacter::RemovePossibleGrapplingAttach(AGrapplingAttachActor* Actor)
{
	if(Actor != nullptr)
	{
		if(NearestGrapplingAttach == Actor)
		{
			NearestGrapplingAttach->UnselectAttach();
			NearestGrapplingAttach = nullptr;
		}
		
		PossibleGrapplingAttaches.Remove(Actor);
		
		GetNearestAttach();
	}
}

void AUMadCharacter::GetNearestAttach()
{
	const int NbOfAttaches = PossibleGrapplingAttaches.Num();
	int LowestI = 0;
	_attachesTimer = -1;
	
	if(NbOfAttaches == 0)
		return;
	else if(NbOfAttaches > 1)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		int vX, vY;
		PlayerController->GetViewportSize(vX, vY);
		FVector2D vDimensions = FVector2D(vX, vY);
		FVector2D finalLocation;
	
		float LowestDist = std::numeric_limits<float>::max();
		for (int i = 0; i < NbOfAttaches; i++)
		{
			PlayerController->ProjectWorldLocationToScreen(PossibleGrapplingAttaches[i]->GetTransform().GetLocation(), finalLocation);
			FVector2D resultVector = (vDimensions / 2) - finalLocation;

			int dist = resultVector.Length();
		
			if(dist < LowestDist) {
				LowestDist = dist;
				LowestI = i;
			}
		}

		_attachesTimer = 0;
	}
	if(NearestGrapplingAttach != nullptr)
		NearestGrapplingAttach->UnselectAttach();
	NearestGrapplingAttach = PossibleGrapplingAttaches[LowestI];
	NearestGrapplingAttach->SelectAttach();
}

void AUMadCharacter::StartGrappling()
{
	if(NearestGrapplingAttach != nullptr)
	{
		_grappleTimer = 0.0f;
		CurrentGrapplingAttach = NearestGrapplingAttach;

		FInputAxisBinding axisBind = FInputAxisBinding("MoveForward");
		ResetBinding(axisBind);
		axisBind = FInputAxisBinding("MoveRight");
		ResetBinding(axisBind);

		FRotator PlayerRot = UKismetMathLibrary::FindLookAtRotation(this->GetActorLocation(), CurrentGrapplingAttach->GetActorLocation());
		PlayerRot.Pitch = 0;
		PlayerRot.Roll = 0;
		PlayerRot.Yaw -= 30.0f;
		SetActorRotation(PlayerRot);
		
	    this->IsUsingGrapple = true;
		_beginGrapple = UGameplayStatics::GetRealTimeSeconds(GetWorld());
		GrappleComp->BeginGrapple(CurrentGrapplingAttach);
	}
}
void AUMadCharacter::ResetBinding(FInputAxisBinding bind)
{
	for (int i = 0; i < InputComponent->AxisBindings.Num(); i++)
	{
		if(CompareInputActionBindings(InputComponent->AxisBindings[i], bind))
		{
			InputComponent->AxisBindings.RemoveAt(i);
			i--;
			continue;
		}
	}
}
bool AUMadCharacter::CompareInputActionBindings(FInputAxisBinding lhs, FInputAxisBinding rhs)
{
	return lhs.AxisDelegate.GetDelegateForManualSet().GetHandle() == rhs.AxisDelegate.GetDelegateForManualSet().GetHandle() &&
		lhs.AxisName == rhs.AxisName;
}

void AUMadCharacter::EndGrappling()
{
	if(CurrentGrapplingAttach == nullptr)
		return;

	InputComponent->BindAxis("MoveForward", this, &AUMadCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AUMadCharacter::MoveRight);
	
	HasReleasedGrapple = true;
	float chargingTime = UGameplayStatics::GetRealTimeSeconds(GetWorld()) - _beginGrapple;

	FVector dir = CurrentGrapplingAttach->GetActorLocation() - GetActorLocation();
	dir.Normalize();
	dir *= GrappleForce->GetFloatValue(chargingTime);

	this->HasReleasedGrapple = true;
	GrappleComp->EndGrapple();
	LaunchCharacter(dir, true, true);

	CurrentGrapplingAttach = nullptr;
}

#pragma endregion GrapplingHook

void AUMadCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(this->IsUsingGrapple)
	{
		_grappleTimer += DeltaSeconds;
		if(_grappleTimer >= GrappleTimeBeforeExplosion)
		{
			EndGrappling();
			_grappleTimer = 0;
		}
	}
	
	if(_attachesTimer != -1)
	{
		_attachesTimer += DeltaSeconds;
		if(_attachesTimer > 0.2)
			GetNearestAttach();
	}
}



void AUMadCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AUMadCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}



void AUMadCharacter::Ragdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	this->DisableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, this, &AUMadCharacter::EndRagdoll, RagdollDelay, false);
}

void AUMadCharacter::EndRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	this->EnableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	MeshComp->SetAllBodiesSimulatePhysics(false);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FVector NewLocation = FVector(0.0f, 0.0f, -90.0f);
	FRotator NewRotator = FRotator(0.0f, -90.0f, 0.0f);
	for(auto boneName : MeshComp->GetAllSocketNames())
	{
		MeshComp->PutRigidBodyToSleep(boneName);
		MeshComp->SetRelativeLocationAndRotation(NewLocation, NewRotator, false, false);
		FAttachmentTransformRules Rules = FAttachmentTransformRules(FAttachmentTransformRules::SnapToTargetIncludingScale);
		Rules.ScaleRule = EAttachmentRule::KeepRelative;
		Rules.LocationRule = EAttachmentRule::SnapToTarget;
		Rules.RotationRule = EAttachmentRule::SnapToTarget;
		Rules.bWeldSimulatedBodies = false;
		MeshComp->AttachToComponent(GetCapsuleComponent(), Rules, FName("None"));
	}
}
