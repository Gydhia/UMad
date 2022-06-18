// Fill out your copyright notice in the Description page of Project Settings.


#include "UMadCharacter.h"

#include "GrappleLine.h"
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
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "CableComponent.h"
#include "GrappleComponent.h"

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
	
		GrappleBeginEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("GrappleBeginEffect"));
		GrappleBeginEffect->SetupAttachment(RootComponent);
	
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
	}
	if(NearestGrapplingAttach != nullptr)
		NearestGrapplingAttach->UnselectAttach();
	NearestGrapplingAttach = PossibleGrapplingAttaches[LowestI];
	NearestGrapplingAttach->SelectAttach();

	_attachesTimer = 0;
}

void AUMadCharacter::StartGrappling()
{
	if(NearestGrapplingAttach != nullptr)
	{
	    IsUsingGrapple = true;
		_beginGrapple = UGameplayStatics::GetRealTimeSeconds(GetWorld());
		GrappleComp->BeginGrapple(NearestGrapplingAttach);
	}
}

void AUMadCharacter::EndGrappling()
{
	if(NearestGrapplingAttach == nullptr)
		return;
	
	bool HasReleasedGrapple = true;
	float chargingTime = UGameplayStatics::GetRealTimeSeconds(GetWorld()) - _beginGrapple;

	if(M_GrapplePull)
		PlayAnimMontage(M_GrapplePull, 1, NAME_None);
	FVector dir = NearestGrapplingAttach->GetActorLocation() - GetActorLocation();
	dir *= 2;
	//LaunchCharacter(dir, true, true);
}

#pragma endregion GrapplingHook

void AUMadCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(_attachesTimer != -1)
	{
		FHitResult hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		
		GetWorld()->LineTraceSingleByChannel(hit, GetActorLocation(), NearestGrapplingAttach->GetActorLocation(), ECollisionChannel::ECC_Visibility, Params,FCollisionResponseParams());
		DrawDebugLine(GetWorld(),GetActorLocation(), NearestGrapplingAttach->GetActorLocation(), FColor::Red, false, 5.0f);
		
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
}
