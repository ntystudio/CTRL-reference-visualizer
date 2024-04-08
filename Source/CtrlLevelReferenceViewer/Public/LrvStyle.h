// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class CTRLLEVELREFERENCEVIEWER_API FLrvStyle : public FSlateStyleSet
{
public:
	explicit FLrvStyle();

	static void Startup();
	static void Register();
	static void Shutdown();

	static TSharedPtr<FLrvStyle> Get();
	virtual const FName& GetStyleSetName() const override;
	void Initialize();

private:
	static TSharedPtr<FLrvStyle> StyleSet;
};
