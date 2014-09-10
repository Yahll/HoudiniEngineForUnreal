/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *      Mykola Konyk
 *      Side Effects Software Inc
 *      123 Front Street West, Suite 1401
 *      Toronto, Ontario
 *      Canada   M5J 2M2
 *      416-504-9876
 *
 */

#pragma once

class IDetailLayoutBuilder;
class UHoudiniAssetComponent;


class FHoudiniAssetComponentDetails : public IDetailCustomization
{
public:

	/** Constructor. **/
	FHoudiniAssetComponentDetails();

public: /** IDetailCustomization methods. **/

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

public:

	/** Create an instance of this detail layout class. **/
	static TSharedRef<IDetailCustomization> MakeInstance();

private:

	/** Button click handlers. **/
	FReply OnButtonClickedBakeSingle();
	FReply OnButtonClickedBakeMultiple();

private:

	/** Helper method used to create actual static meshes. **/
	void CreateStaticMeshes(bool bMultipleMeshes);

private:

	/** Components which are being customized. **/
	TArray<UHoudiniAssetComponent*> HoudiniAssetComponents;

	/** Whether baking option is enabled. **/
	bool bEnableBaking;
};