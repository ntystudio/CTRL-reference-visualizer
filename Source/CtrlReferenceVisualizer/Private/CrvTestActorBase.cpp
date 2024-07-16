// Fill out your copyright notice in the Description page of Project Settings.

#include "CrvTestActorBase.h"
#include "ObjectTools.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Components/TextRenderComponent.h"

UCrvTestComponentBase::UCrvTestComponentBase()
{
	TestObject = CreateDefaultSubobject<UCrvTestObject>(TEXT("TestObject"));
}

ACrvTestActorBase::ACrvTestActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;
	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(DefaultSceneRoot);
	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	TextRender->SetupAttachment(Sphere);
	TextRender->SetRelativeLocation(FVector(0, 0, 100));
	TextRender->SetRelativeRotation(FRotator(0, 180, 0));
	TextRender->HorizontalAlignment = EHorizTextAligment::EHTA_Center;
	TestComponent = CreateDefaultSubobject<UCrvTestComponentBase>(TEXT("TestComponent"));
	TestComponent->SetupAttachment(Sphere);
	TestComponent->SetRelativeLocation(FVector(0, 0, 200));
	TestObject = CreateDefaultSubobject<UCrvTestObject>(TEXT("TestObject"));
}

void ACrvTestActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TextRender->SetText(FText::FromName(TestName));
}

void ACrvTestActorBase::CopySettingsToTestComponent()
{
		TestComponent->ActorRef = ActorRef;
		TestComponent->ActorRef2 = ActorRef2;
		TestComponent->SoftActorRef = SoftActorRef;
		TestComponent->ComponentRef = ComponentRef;
		TestComponent->SoftComponentRef = SoftComponentRef;
		TestComponent->OptionalActorRef = OptionalActorRef;
		TestComponent->OptionalComponentRef = OptionalComponentRef;
		TestComponent->WeakActorRef = WeakActorRef;
		TestComponent->WeakComponentRef = WeakComponentRef;
		TestComponent->ActorRefArray= ActorRefArray;
		TestComponent->ActorRefMap = ActorRefMap;
		TestComponent->TestStruct = TestStruct;
		TestComponent->TestObject = TestObject;
		ActorRef = nullptr;
		ActorRef2 = nullptr;
		SoftActorRef = nullptr;
		ComponentRef = nullptr;
		SoftComponentRef = nullptr;
		OptionalActorRef = nullptr;
		OptionalComponentRef = nullptr;
		WeakActorRef = nullptr;
		WeakComponentRef = nullptr;
		ActorRefArray.Empty();
		ActorRefMap.Empty();
}
void ACrvTestActorBase::CopySettingsToTestObject()
{
	TestObject->ActorRef = ActorRef;
	TestObject->ActorRef2 = ActorRef2;
	TestObject->SoftActorRef = SoftActorRef;
	TestObject->ComponentRef = ComponentRef;
	TestObject->SoftComponentRef = SoftComponentRef;
	TestObject->OptionalActorRef = OptionalActorRef;
	TestObject->OptionalComponentRef = OptionalComponentRef;
	TestObject->WeakActorRef = WeakActorRef;
	TestObject->WeakComponentRef = WeakComponentRef;
	TestObject->ActorRefArray = ActorRefArray;
	TestObject->ActorRefMap = ActorRefMap;
	TestObject->TestStruct = TestStruct;
	ActorRef = nullptr;
	ActorRef2 = nullptr;
	SoftActorRef = nullptr;
	ComponentRef = nullptr;
	SoftComponentRef = nullptr;
	OptionalActorRef = nullptr;
	OptionalComponentRef = nullptr;
	WeakActorRef = nullptr;
	WeakComponentRef = nullptr;
	ActorRefArray.Empty();
	ActorRefMap.Empty();
}
void UCrvCopyReference::CopyReference()
{

	// auto CopyRef = [](UObject* Object)
	// {
	// 	if (!Object) { return; }
	// 	FPlatformApplicationMisc::ClipboardCopy(*Object->GetFullName());
	// };
	// if (What == ECrvCopyWhatReference::ChildActor)
	// {
	// 	if (auto OwnerWithChild = GetTypedOuter<ACrvTestActorWithChildActorBase>())
	// 	{
	// 		CopyRef(OwnerWithChild->ChildActor);
	// 	}
	// 	return;
	// }
	//
	// auto Owner = GetTypedOuter<ACrvTestActorBase>();
	// if (!Owner) { return; }
	// if (What == ECrvCopyWhatReference::ActorRef)
	// {
	// 	CopyRef(Owner->ActorRef);
	// 	return;
	// }
	//
	// if (What == ECrvCopyWhatReference::ActorRef2)
	// {
	// 	CopyRef(Owner->ActorRef2);
	// 	return;
	// }
}

ACrvTestActorWithChildActorBase::ACrvTestActorWithChildActorBase()
{
	ChildActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("ChildActorComponent"));
	ChildActorComponent->SetupAttachment(RootComponent);
	ChildActorComponent->SetRelativeLocation(FVector(200, 0, 0));
	ChildActorComponent->SetRelativeScale3D(FVector(0.5, 0.5, 0.5));
	ChildActorComponent->SetChildActorClass(ACrvTestActorBase::StaticClass());
}
void ACrvTestActorWithChildActorBase::CopySettingsToChildActor()
{
	if (auto Child = Cast<ACrvTestActorBase>(ChildActorComponent->GetChildActor()))
	{
		Child->ActorRef = ActorRef;
		Child->ActorRef2 = ActorRef2;
		Child->SoftActorRef = SoftActorRef;
		Child->ComponentRef = ComponentRef;
		Child->SoftComponentRef = SoftComponentRef;
		Child->OptionalActorRef = OptionalActorRef;
		Child->OptionalComponentRef = OptionalComponentRef;
		Child->WeakActorRef = WeakActorRef;
		Child->WeakComponentRef = WeakComponentRef;
		Child->ActorRefArray= ActorRefArray;
		Child->ActorRefMap = ActorRefMap;
		Child->TestStruct = TestStruct;
		Child->TestObject = TestObject;
		ActorRef = nullptr;
		ActorRef2 = nullptr;
		SoftActorRef = nullptr;
		ComponentRef = nullptr;
		SoftComponentRef = nullptr;
		OptionalActorRef = nullptr;
		OptionalComponentRef = nullptr;
		WeakActorRef = nullptr;
		WeakComponentRef = nullptr;
		ActorRefArray.Empty();
		ActorRefMap.Empty();
	}
}
void ACrvTestActorWithChildActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ChildActor = ChildActorComponent->GetChildActor();
}
