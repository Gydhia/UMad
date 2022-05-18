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
    
    	GrappleAttachesCollider = CreateDefaultSubobject<USphereComponent>(TEXT("GrappleAttachesCollider"));
    	GrappleAttachesCollider->SetupAttachment((RootComponent));
    	GrappleAttachesCollider->InitSphereRadius(100);
    	
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
    			"Confirm","Cancel","EOrtharAbilityInputID",
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


