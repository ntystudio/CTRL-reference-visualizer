// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrvTestActorBase.generated.h"

class UCrvTestObject;
class UTextRenderComponent;

USTRUCT(BlueprintType)
struct FCrvTestStructNested
{
	GENERATED_BODY()
	
	// Direct Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef;
};
USTRUCT(BlueprintType)
struct FCrvTestStruct
{
	GENERATED_BODY()
	
	// Direct Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef;

	// Direct Reference to another Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef2;
	
	// Soft Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<AActor> SoftActorRef;

	// Direct Reference to an Actor Component
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UActorComponent> ComponentRef;

	// Soft Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<AActor> SoftComponentRef;
	
	// Optional Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<AActor*> OptionalActorRef;
	
	// Optional Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<UActorComponent*> OptionalComponentRef;

	// Weak Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> WeakActorRef;

	// Weak Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UActorComponent> WeakComponentRef;

	// Array of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TArray<AActor*> ActorRefArray;

	// Map of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TMap<AActor*, int32> ActorRefMap;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Instanced)
	TObjectPtr<UCrvTestObject> TestObject;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	FCrvTestStructNested TestStructNested;
};

UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class UCrvTestObject: public UObject
{
	GENERATED_BODY()

public:
	
	// Direct Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef;

	// Direct Reference to another Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef2;
	
	// Soft Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<AActor> SoftActorRef;

	// Direct Reference to an Actor Component
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UActorComponent> ComponentRef;

	// Soft Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<AActor> SoftComponentRef;
	
	// Optional Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<AActor*> OptionalActorRef;
	
	// Optional Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<UActorComponent*> OptionalComponentRef;

	// Weak Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> WeakActorRef;

	// Weak Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UActorComponent> WeakComponentRef;

	// Array of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TArray<AActor*> ActorRefArray;

	// Map of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TMap<AActor*, int32> ActorRefMap;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	FCrvTestStruct TestStruct;
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyActorToWeakActor() 
    {
    
    	WeakActorRef = ActorRef;
    	ActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakActorRef()
    {
    	WeakActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyCompToWeakComp()
    {
    
    	WeakComponentRef = ComponentRef;
    	ComponentRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakCompRef()
    {
    	WeakComponentRef = nullptr;
    }
};

UCLASS(Blueprintable, BlueprintType)
class CTRLREFERENCEVISUALIZER_API UCrvTestComponentBase : public USceneComponent
{
	GENERATED_BODY()

public:

	UCrvTestComponentBase();
	// Direct Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef;

	// Direct Reference to another Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef2;
	
	// Soft Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<AActor> SoftActorRef;

	// Direct Reference to an Actor Component
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UActorComponent> ComponentRef;

	// Soft Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<AActor> SoftComponentRef;
	
	// Optional Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<AActor*> OptionalActorRef;
	
	// Optional Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TOptional<UActorComponent*> OptionalComponentRef;

	// Weak Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> WeakActorRef;

	// Weak Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UActorComponent> WeakComponentRef;

	// Array of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TArray<AActor*> ActorRefArray;

	// Map of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TMap<AActor*, int32> ActorRefMap;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	FCrvTestStruct TestStruct;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Instanced)
	TObjectPtr<UCrvTestObject> TestObject;
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyActorToWeakActor() 
    {
    
    	WeakActorRef = ActorRef;
    	ActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakActorRef()
    {
    	WeakActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyCompToWeakComp()
    {
    
    	WeakComponentRef = ComponentRef;
    	ComponentRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakCompRef()
    {
    	WeakComponentRef = nullptr;
    }

};

UCLASS(Blueprintable, BlueprintType)
class CTRLREFERENCEVISUALIZER_API ACrvTestActorBase : public AActor
{
	GENERATED_BODY()

public:
	ACrvTestActorBase();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(GetOptions="GetTestNames"))
	FName TestName = TEXT("A");

	UFUNCTION(BlueprintCallable, Category = "CrvTestActorBase")
	static TArray<FName> GetTestNames() { return TArray<FName>{TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F"), TEXT("G") }; }
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<USceneComponent> DefaultSceneRoot;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Sphere;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UCrvTestComponentBase> TestComponent;

	// Test Properties //

	// Direct Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef;

	// Direct Reference to another Actor
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<AActor> ActorRef2;
	
	// Soft Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<AActor> SoftActorRef;

	// Direct Reference to an Actor Component
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UActorComponent> ComponentRef;

	// Soft Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<AActor> SoftComponentRef;
	
	// Optional Reference to an Actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TOptional<AActor*> OptionalActorRef;
	
	// Optional Reference to an Actor Component
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TOptional<UActorComponent*> OptionalComponentRef;

	// Weak Reference to an Actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> WeakActorRef;

	// Weak Reference to an Actor Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UActorComponent> WeakComponentRef;

	// Array of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TArray<AActor*> ActorRefArray;

	// Map of Actors
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TMap<AActor*, int32> ActorRefMap;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	FCrvTestStruct TestStruct;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<UCrvTestObject> TestObject;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly)
	TObjectPtr<UObject> ObjectRef;

	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopySettingsToTestComponent();

	void CopySettingsToTestObject();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyActorToWeakActor() 
    {
    
    	WeakActorRef = ActorRef;
    	ActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakActorRef()
    {
    	WeakActorRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyCompToWeakComp()
    {
    
    	WeakComponentRef = ComponentRef;
    	ComponentRef = nullptr;
    }
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ClearWeakCompRef()
    {
    	WeakComponentRef = nullptr;
    }
};

UENUM()
enum class ECrvCopyWhatReference : uint8
{
	ChildActor,
	ChildActorComponent,
	TestComponent,
	TestObject,
	ActorRef,
	ActorRef2,
	SoftActorRef,
	ComponentRef,
	SoftComponentRef,
	OptionalActorRef,
};
UCLASS(BlueprintType)
class UCrvCopyReference: public UObject
{
	GENERATED_BODY()
	public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ECrvCopyWhatReference What;

	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopyReference();
	
};

UCLASS()
class ACrvTestActorWithChildActorBase : public ACrvTestActorBase
{
	GENERATED_BODY()
public:
	ACrvTestActorWithChildActorBase();
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UChildActorComponent> ChildActorComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<AActor> ChildActor;

	UFUNCTION(BlueprintCallable, CallInEditor)
	void CopySettingsToChildActor();

	virtual void OnConstruction(const FTransform& Transform) override;
};