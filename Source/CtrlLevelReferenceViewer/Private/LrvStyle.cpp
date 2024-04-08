#include "LrvStyle.h"

#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/StyleColors.h"

TSharedPtr<FLrvStyle> FLrvStyle::StyleSet = nullptr;

TSharedPtr<FLrvStyle> FLrvStyle::Get()
{
	ensure(StyleSet.IsValid());
	return StyleSet;
}

FLrvStyle::FLrvStyle()
	: FSlateStyleSet(FLrvStyle::GetStyleSetName())
{
}

void FLrvStyle::Startup()
{
	if (StyleSet.IsValid()) { return; } // only init once
	StyleSet = MakeShared<FLrvStyle>();
	StyleSet->Initialize();
}

void FLrvStyle::Register()
{
	if (!StyleSet.IsValid()) { return; }
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FLrvStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

const FName& FLrvStyle::GetStyleSetName() const
{
	static FName CtrlStyleName(TEXT("LrvStyle"));
	return CtrlStyleName;
}

void FLrvStyle::Initialize()
{
	const FVector2D Icon128x128(128.0f, 128.0f);
	const FVector2D Icon16x16(16.f, 16.f);
	const FVector2D Icon32x32(32.f, 32.f);

	SetContentRoot(FPaths::ProjectPluginsDir() / TEXT("CtrlLevelReferenceViewer/Resources"));
	Set("Ctrl.TabIcon", new FSlateImageBrush(GetContentRootDir() / TEXT("Icon128.png"), Icon128x128));
	Set("Ctrl.PlacementModeIcon", new FSlateImageBrush(GetContentRootDir() / TEXT("Icon16.png"), Icon16x16));
	// use outliner style to get colors for selection + unfocused selection + hover
	auto RowStyle = FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SceneOutliner.TableViewRow");
	// add alternating row color
	FLinearColor AltColor = FStyleColors::Panel.GetSpecifiedColor().CopyWithNewOpacity(0.2f);
	Set(
		"Ctrl.TreeView.TableRow",
		FTableRowStyle(RowStyle)
		.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed))
		.SetOddRowBackgroundBrush(FSlateColorBrush(FLinearColor(AltColor)))
	);
}
