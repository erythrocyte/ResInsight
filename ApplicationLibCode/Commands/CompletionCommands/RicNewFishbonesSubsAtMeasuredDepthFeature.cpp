/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 Statoil ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RicNewFishbonesSubsAtMeasuredDepthFeature.h"

#include "RicNewFishbonesSubsFeature.h"
#include "WellPathCommands/RicWellPathsUnitSystemSettingsImpl.h"

#include "RimFishbones.h"
#include "RimFishbonesCollection.h"
#include "RimProject.h"
#include "RimWellPath.h"

#include "Riu3DMainWindowTools.h"
#include "Riu3dSelectionManager.h"

#include "cafSelectionManager.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicNewFishbonesSubsAtMeasuredDepthFeature, "RicNewFishbonesSubsAtMeasuredDepthFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewFishbonesSubsAtMeasuredDepthFeature::onActionTriggered( bool isChecked )
{
    RiuWellPathSelectionItem* wellPathSelItem = RiuWellPathSelectionItem::wellPathSelectionItem();
    CVF_ASSERT( wellPathSelItem );

    RimWellPath* wellPath = wellPathSelItem->m_wellpath;
    CVF_ASSERT( wellPath );

    if ( !RicWellPathsUnitSystemSettingsImpl::ensureHasUnitSystem( wellPath ) ) return;

    RimFishbones* obj = new RimFishbones;
    wellPath->fishbonesCollection()->appendFishbonesSubs( obj );

    obj->setMeasuredDepthAndCount( wellPathSelItem->m_measuredDepth, 12.5, 13 );

    RicNewFishbonesSubsFeature::adjustWellPathScaling( wellPath->fishbonesCollection() );

    wellPath->updateConnectedEditors();
    Riu3DMainWindowTools::selectAsCurrentItem( obj );

    RimProject* proj;
    wellPath->firstAncestorOrThisOfTypeAsserted( proj );
    proj->reloadCompletionTypeResultsInAllViews();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewFishbonesSubsAtMeasuredDepthFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/FishBoneGroup16x16.png" ) );
    actionToSetup->setText( "Create Fishbones at this Depth" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicNewFishbonesSubsAtMeasuredDepthFeature::isCommandEnabled()
{
    if ( RiuWellPathSelectionItem::wellPathSelectionItem() )
    {
        return true;
    }

    return false;
}
