// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CtrlReferenceVisualizer.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrvUtils.h"
#include "CrvSampleFloorBase.generated.h"

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECrvHorizontalDirection: uint32
{
	None = 0,
	North = 1 << 0,
	NorthEast = 1 << 1,
	East = 1 << 2,
	SouthEast = 1 << 3,
	South = 1 << 4,
	SouthWest = 1 << 5,
	West = 1 << 6,
	NorthWest = 1 << 7,
	Middle = 1 << 8,
	All = North | NorthEast | East | SouthEast | South | SouthWest | West | NorthWest | Middle,
};

ENUM_CLASS_FLAGS(ECrvHorizontalDirection);

constexpr bool EnumHasAnyFlags(const int32 Flags, ECrvHorizontalDirection Contains) { return (Flags & static_cast<int32>(Contains)) != 0; }

/**
 * Base class for sample floor.
 * Handles neatly auto-positioning heading text.
 * Ok to scale floor to change floor size.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ACrvSampleFloorBase : public AActor
{
	GENERATED_BODY()

public:
	ACrvSampleFloorBase()
	{
		SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
		RootComponent = SceneRoot;
		FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
		FloorMesh->SetupAttachment(RootComponent);
		SampleHeading = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SampleHeading"));
		SampleHeading->SetupAttachment(RootComponent);
		Update();
	}

	// position heading if auto position is enabled
	UFUNCTION(BlueprintCallable)
	void AutoPositionHeading() const
	{
		if (!bAutoPositionHeading) { return; }
		PositionHeading();
	}

	UFUNCTION(BlueprintCallable, CallInEditor)
	void PositionHeading() const
	{
		FVector Min, Max;
		FloorMesh->GetLocalBounds(Min, Max);
		// rotate text to look up
		SampleHeading->SetRelativeRotation(FRotator(90, 180, 0));
		// bottom left corner
		const auto NewLocation = GetCoordinate(ECrvHorizontalDirection::SouthWest);
		SampleHeading->SetRelativeLocation(NewLocation);
		OnSampleFloorBaseUpdated.Broadcast(Min, Max);
	}

	UFUNCTION(BlueprintCallable)
	FVector GetCoordinate(const ECrvHorizontalDirection Direction) const
	{
		FVector Min, Max;
		FloorMesh->GetLocalBounds(Min, Max);
		switch (Direction)
		{
			case ECrvHorizontalDirection::North:
				return FVector((Max.X - Min.X) / 2, Max.Y - HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::East:
				return FVector(Max.X - HeadingPadding.X, (Max.Y - Min.Y) / 2, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::West:
				return FVector(Min.X + HeadingPadding.X, (Max.Y - Min.Y) / 2, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::South:
				return FVector((Max.X - Min.X) / 2, Min.Y + HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::SouthWest:
				return FVector(Min.X + HeadingPadding.X, Min.Y + HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::SouthEast:
				return FVector(Max.X - HeadingPadding.X, Min.Y + HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::NorthEast:
				return FVector(Max.X - HeadingPadding.X, Max.Y - HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::NorthWest:
				return FVector(Min.X + HeadingPadding.X, Max.Y - HeadingPadding.Y, Max.Z + HeadingPadding.Z) * FloorMesh->GetComponentScale();
			case ECrvHorizontalDirection::None:
			case ECrvHorizontalDirection::Middle:
			case ECrvHorizontalDirection::All:
			default:
				UE_LOG(LogCrv, Warning, TEXT("%s Invalid direction %s"), *CRV_LOG_LINE, *UEnum::GetValueAsString(Direction));
				break;
		}
		return FVector::ZeroVector;
	}

	UFUNCTION(BlueprintCallable)
	void Update() const
	{
		SampleHeading->SetWorldSize(HeadingTextSize);
		SampleHeading->SetText(HeadingText);
		AutoPositionHeading();
	}

protected:
	virtual void OnConstruction(const FTransform& Transform) override
	{
		Super::OnConstruction(Transform);
		if (SampleHeading == nullptr)
		{
			SampleHeading = NewObject<UTextRenderComponent>(this);
			SampleHeading->SetupAttachment(RootComponent);
		}
		Update();
	}

	virtual void PostInitializeComponents() override
	{
		Super::PostInitializeComponents();
		Update();
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		Update();
	}
#endif

public:
	// exists so floor can be scaled independently of text
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> FloorMesh = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> SampleHeading = nullptr;

	/** e.g. name of sample */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn="true"))
	FText HeadingText = FText::FromString("Sample");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn="true"))
	float HeadingTextSize = 350.f;

	// disable to manually position heading
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAutoPositionHeading = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(MultiLine="true", ExposeOnSpawn="true", EditCondition="bAutoPositionHeading"))
	FVector HeadingPadding = FVector(50.0, 100.0, 1.0);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSampleFloorBaseUpdated, FVector, Min, FVector, Max);

	UPROPERTY(BlueprintAssignable)
	FOnSampleFloorBaseUpdated OnSampleFloorBaseUpdated;
};
