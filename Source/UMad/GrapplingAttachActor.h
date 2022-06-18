// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrapplingAttachActor.generated.h"

UCLASS()
class UMAD_API AGrapplingAttachActor : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collider, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* AttachCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = WorldUI, meta = (AllowPrivateAccess = "true"))
	class UHookWidget* HookIcon;
	
public:	
	// Sets default values for this actor's properties
	AGrapplingAttachActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION()
	void NotifyActorEndOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintCallable, Category = Visibility)
	static void CallHideSelf(AActor* actor, FString funcName);
	UFUNCTION(BlueprintCallable, Category = Visibility)
	static void CallShowSelf(AActor* actor, FString funcName);

	
	void SelectAttach();
	void UnselectAttach();
};

