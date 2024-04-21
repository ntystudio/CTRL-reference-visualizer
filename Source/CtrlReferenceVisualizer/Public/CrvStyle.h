// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class CTRLREFERENCEVISUALIZER_API FCrvStyle : public FSlateStyleSet
{
public:
	explicit FCrvStyle();

	static void Startup();
	static void Register();
	static void Shutdown();

	static TSharedPtr<FCrvStyle> Get();
	virtual const FName& GetStyleSetName() const override;
	void Initialize();

private:
	static TSharedPtr<FCrvStyle> StyleSet;
};
