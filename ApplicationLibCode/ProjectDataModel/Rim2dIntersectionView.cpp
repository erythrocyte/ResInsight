/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018-     Equinor ASA
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

#include "Rim2dIntersectionView.h"

#include "Rim3dOverlayInfoConfig.h"
#include "RimCase.h"
#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseView.h"
#include "RimExtrudedCurveIntersection.h"
#include "RimGeoMechCellColors.h"
#include "RimGeoMechView.h"
#include "RimGridView.h"
#include "RimIntersectionResultDefinition.h"
#include "RimRegularLegendConfig.h"
#include "RimSimWellInView.h"
#include "RimTernaryLegendConfig.h"
#include "RimViewNameConfig.h"
#include "RimWellPath.h"

#include "RiuMainWindow.h"
#include "RiuViewer.h"

#include "RivExtrudedCurveIntersectionPartMgr.h"
#include "RivSimWellPipesPartMgr.h"
#include "RivTernarySaturationOverlayItem.h"
#include "RivWellHeadPartMgr.h"
#include "RivWellPathPartMgr.h"

#include "cafDisplayCoordTransform.h"
#include "cafPdmUiTreeOrdering.h"

#include "cvfModelBasicList.h"
#include "cvfPart.h"
#include "cvfScene.h"
#include "cvfTransform.h"

#include <QDateTime>

CAF_PDM_SOURCE_INIT( Rim2dIntersectionView, "Intersection2dView" );

const cvf::Mat4d Rim2dIntersectionView::sm_defaultViewMatrix =
    cvf::Mat4d( 1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 1000, 0, 0, 0, 1 );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
Rim2dIntersectionView::Rim2dIntersectionView( void )
{
    CAF_PDM_InitObject( "Intersection View", ":/CrossSection16x16.png", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_intersection, "Intersection", "Intersection", ":/CrossSection16x16.png", "", "" );
    m_intersection.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_legendConfig, "LegendDefinition", "Color Legend", "", "", "" );
    m_legendConfig.uiCapability()->setUiHidden( true );
    m_legendConfig.uiCapability()->setUiTreeChildrenHidden( true );
    m_legendConfig.xmlCapability()->disableIO();
    m_legendConfig = new RimRegularLegendConfig();

    CAF_PDM_InitFieldNoDefault( &m_ternaryLegendConfig, "TernaryLegendDefinition", "Ternary Color Legend", "", "", "" );
    m_ternaryLegendConfig.uiCapability()->setUiTreeHidden( true );
    m_ternaryLegendConfig.uiCapability()->setUiTreeChildrenHidden( true );
    m_ternaryLegendConfig.xmlCapability()->disableIO();
    m_ternaryLegendConfig = new RimTernaryLegendConfig();

    CAF_PDM_InitField( &m_showDefiningPoints, "ShowDefiningPoints", true, "Show Points", "", "", "" );
    CAF_PDM_InitField( &m_showAxisLines, "ShowAxisLines", false, "Show Axis Lines", "", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_nameProxy, "NameProxy", "Name", "", "", "" );
    m_nameProxy.xmlCapability()->disableIO();
    m_nameProxy.registerGetMethod( this, &Rim2dIntersectionView::getName );
    m_nameProxy.registerSetMethod( this, &Rim2dIntersectionView::setName );

    m_showWindow           = false;
    m_scaleTransform       = new cvf::Transform();
    m_intersectionVizModel = new cvf::ModelBasicList;

    hasUserRequestedAnimation = true;

    ( (RiuViewerToViewInterface*)this )->setCameraPosition( sm_defaultViewMatrix );

    disableGridBoxField();
    disablePerspectiveProjectionField();

    nameConfig()->hideCaseNameField( true );
    nameConfig()->hideAggregationTypeField( true );
    nameConfig()->hidePropertyField( true );
    nameConfig()->hideSampleSpacingField( true );

    hideComparisonViewField();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
Rim2dIntersectionView::~Rim2dIntersectionView( void )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::setVisible( bool isVisible )
{
    m_showWindow = isVisible;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::setIntersection( RimExtrudedCurveIntersection* intersection )
{
    CAF_ASSERT( intersection );

    m_intersection = intersection;

    this->updateName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimExtrudedCurveIntersection* Rim2dIntersectionView::intersection() const
{
    return m_intersection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::isUsingFormationNames() const
{
    // Todo:

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::scheduleGeometryRegen( RivCellSetEnum geometryType )
{
    m_flatIntersectionPartMgr = nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCase* Rim2dIntersectionView::ownerCase() const
{
    RimCase* rimCase = nullptr;
    if ( m_intersection && m_intersection->activeSeparateResultDefinition() )
    {
        rimCase = m_intersection->activeSeparateResultDefinition()->activeCase();
    }

    if ( !rimCase )
    {
        this->firstAncestorOrThisOfTypeAsserted( rimCase );
    }

    return rimCase;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::isTimeStepDependentDataVisible() const
{
    if ( m_intersection() )
    {
        if ( RimIntersectionResultDefinition* sepInterResultDef = m_intersection->activeSeparateResultDefinition() )
        {
            return sepInterResultDef->isEclipseResultDefinition()
                       ? sepInterResultDef->eclipseResultDefinition()->hasDynamicResult()
                       : sepInterResultDef->geoMechResultDefinition()->hasResult();
        }
        else
        {
            RimGridView* gridView = nullptr;
            m_intersection->firstAncestorOrThisOfTypeAsserted( gridView );
            return gridView->isTimeStepDependentDataVisibleInThisOrComparisonView();
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::update3dInfo()
{
    if ( !nativeOrOverrideViewer() ) return;

    QString overlayInfoText;

    Rim3dOverlayInfoConfig*     overlayInfoConfig = nullptr;
    RimGeoMechResultDefinition* geomResDef        = nullptr;
    RimEclipseResultDefinition* eclResDef         = nullptr;

    {
        RimEclipseView* originEclView = nullptr;
        m_intersection->firstAncestorOrThisOfType( originEclView );
        RimGeoMechView* originGeoView = nullptr;
        m_intersection->firstAncestorOrThisOfType( originGeoView );

        if ( originEclView )
        {
            overlayInfoConfig = originEclView->overlayInfoConfig();
            eclResDef         = originEclView->cellResult();
        }
        else if ( originGeoView )
        {
            overlayInfoConfig = originGeoView->overlayInfoConfig();
            geomResDef        = originGeoView->cellResultResultDefinition();
        }
    }

    if ( !overlayInfoConfig->isActive() )
    {
        nativeOrOverrideViewer()->showInfoText( false );
        nativeOrOverrideViewer()->showHistogram( false );
        nativeOrOverrideViewer()->showAnimationProgress( false );
        nativeOrOverrideViewer()->showVersionInfo( false );

        nativeOrOverrideViewer()->update();
        return;
    }

    if ( RimIntersectionResultDefinition* sepInterResultDef = m_intersection->activeSeparateResultDefinition() )
    {
        if ( sepInterResultDef->isEclipseResultDefinition() )
        {
            eclResDef  = sepInterResultDef->eclipseResultDefinition();
            geomResDef = nullptr;
        }
        else
        {
            geomResDef = sepInterResultDef->geoMechResultDefinition();
            eclResDef  = nullptr;
        }
    }

    if ( overlayInfoConfig->showCaseInfo() )
    {
        overlayInfoText += "<b>--" + ownerCase()->caseUserDescription() + "--</b>";
    }

    overlayInfoText += "<p>";
    overlayInfoText += "<b>Z-scale:</b> " + QString::number( scaleZ() ) + "<br> ";

    if ( m_intersection->simulationWell() )
    {
        overlayInfoText += "<b>Simulation Well:</b> " + m_intersection->simulationWell()->name() + "<br>";
    }
    else if ( m_intersection->wellPath() )
    {
        overlayInfoText += "<b>Well Path:</b> " + m_intersection->wellPath()->name() + "<br>";
    }
    else
    {
        overlayInfoText += "<b>Intersection:</b> " + m_intersection->name() + "<br>";
    }

    if ( overlayInfoConfig->showAnimProgress() )
    {
        nativeOrOverrideViewer()->showAnimationProgress( true );
    }
    else
    {
        nativeOrOverrideViewer()->showAnimationProgress( false );
    }

    if ( overlayInfoConfig->showResultInfo() )
    {
        if ( eclResDef )
        {
            overlayInfoText += "<b>Cell Result:</b> " + eclResDef->resultVariableUiShortName() + "<br>";
        }
        else if ( geomResDef )
        {
            QString resultPos;
            QString fieldName = geomResDef->resultFieldUiName();
            QString compName  = geomResDef->resultComponentUiName();

            switch ( geomResDef->resultPositionType() )
            {
                case RIG_NODAL:
                    resultPos = "Nodal";
                    break;

                case RIG_ELEMENT_NODAL:
                    resultPos = "Element nodal";
                    break;

                case RIG_INTEGRATION_POINT:
                    resultPos = "Integration point";
                    break;

                case RIG_ELEMENT:
                    resultPos = "Element";
                    break;
                default:
                    break;
            }

            if ( compName == "" )
            {
                overlayInfoText += QString( "<b>Cell result:</b> %1, %2<br>" ).arg( resultPos ).arg( fieldName );
            }
            else
            {
                overlayInfoText +=
                    QString( "<b>Cell result:</b> %1, %2, %3<br>" ).arg( resultPos ).arg( fieldName ).arg( compName );
            }
        }
    }

    overlayInfoText += "</p>";
    nativeOrOverrideViewer()->setInfoText( overlayInfoText );
    nativeOrOverrideViewer()->showInfoText( true );
    nativeOrOverrideViewer()->update();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::updateName()
{
    if ( m_intersection )
    {
        Rim3dView* parentView = nullptr;
        m_intersection->firstAncestorOrThisOfTypeAsserted( parentView );
        this->setName( parentView->name() + ": " + m_intersection->name() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RivExtrudedCurveIntersectionPartMgr* Rim2dIntersectionView::flatIntersectionPartMgr() const
{
    return m_flatIntersectionPartMgr.p();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::isGridVisualizationMode() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Vec3d Rim2dIntersectionView::transformToUtm( const cvf::Vec3d& unscaledPointInFlatDomain ) const
{
    if ( m_flatIntersectionPartMgr.notNull() )
    {
        cvf::Mat4d unflatXf = flatIntersectionPartMgr()->unflattenTransformMatrix( unscaledPointInFlatDomain );

        return unscaledPointInFlatDomain.getTransformedPoint( unflatXf );
    }

    return unscaledPointInFlatDomain;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::ref<caf::DisplayCoordTransform> Rim2dIntersectionView::displayCoordTransform() const
{
    cvf::ref<caf::DisplayCoordTransform> dispTx = new caf::DisplayCoordTransform();
    dispTx->setScale( cvf::Vec3d( 1.0, 1.0, scaleZ() ) );
    return dispTx;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::showDefiningPoints() const
{
    return m_showDefiningPoints;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimLegendConfig*> Rim2dIntersectionView::legendConfigs() const
{
    std::vector<RimLegendConfig*> legendsIn3dView;

    // Return empty list, as the intersection view has a copy of the legend items. Picking and selection of the
    // corresponding item is handeled by handleOverlayItemPicked()
    return legendsIn3dView;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::handleOverlayItemPicked( const cvf::OverlayItem* pickedOverlayItem ) const
{
    if ( m_legendObjectToSelect )
    {
        RiuMainWindow::instance()->selectAsCurrentItem( m_legendObjectToSelect );

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo>
    Rim2dIntersectionView::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly )
{
    QList<caf::PdmOptionItemInfo> options;

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::hasResults()
{
    if ( !m_intersection() ) return false;

    if ( RimIntersectionResultDefinition* sepInterResultDef = m_intersection->activeSeparateResultDefinition() )
    {
        if ( sepInterResultDef->isEclipseResultDefinition() )
        {
            RimEclipseResultDefinition* eclResDef = sepInterResultDef->eclipseResultDefinition();
            return eclResDef->hasResult() || eclResDef->isTernarySaturationSelected();
        }
        else
        {
            RimGeoMechResultDefinition* geomResDef = sepInterResultDef->geoMechResultDefinition();
            return geomResDef->hasResult();
        }
    }

    RimEclipseView* eclView = nullptr;
    m_intersection->firstAncestorOrThisOfType( eclView );
    if ( eclView )
    {
        return ( eclView->cellResult()->hasResult() || eclView->cellResult()->isTernarySaturationSelected() );
    }

    RimGeoMechView* geoView = nullptr;
    m_intersection->firstAncestorOrThisOfType( geoView );
    if ( geoView )
    {
        return geoView->cellResult()->hasResult();
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t Rim2dIntersectionView::onTimeStepCountRequested()
{
    if ( isTimeStepDependentDataVisible() )
    {
        return this->ownerCase()->timeStepStrings().size();
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString Rim2dIntersectionView::createAutoName() const
{
    return nameConfig()->customName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString Rim2dIntersectionView::getName() const
{
    return nameConfig()->customName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::setName( const QString& name )
{
    nameConfig()->setCustomName( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool Rim2dIntersectionView::isWindowVisible() const
{
    return m_showWindow();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::defineAxisLabels( cvf::String* xLabel, cvf::String* yLabel, cvf::String* zLabel )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onCreateDisplayModel()
{
    if ( !nativeOrOverrideViewer() ) return;
    if ( !m_intersection() ) return;

    onUpdateScaleTransform();

    nativeOrOverrideViewer()->removeAllFrames( isUsingOverrideViewer() );

    size_t tsCount = this->timeStepCount();

    for ( size_t i = 0; i < tsCount; ++i )
    {
        nativeOrOverrideViewer()->addFrame( new cvf::Scene(), isUsingOverrideViewer() );
    }

    m_flatIntersectionPartMgr = new RivExtrudedCurveIntersectionPartMgr( m_intersection(), true );

    m_intersectionVizModel->removeAllParts();

    m_flatIntersectionPartMgr->appendIntersectionFacesToModel( m_intersectionVizModel.p(), scaleTransform() );
    m_flatIntersectionPartMgr->appendMeshLinePartsToModel( m_intersectionVizModel.p(), scaleTransform() );
    m_flatIntersectionPartMgr->appendPolylinePartsToModel( *this, m_intersectionVizModel.p(), scaleTransform() );

    m_flatIntersectionPartMgr->applySingleColorEffect();

    m_flatSimWellPipePartMgr = nullptr;
    m_flatWellHeadPartMgr    = nullptr;

    if ( m_intersection->type() == RimExtrudedCurveIntersection::CrossSectionEnum::CS_SIMULATION_WELL &&
         m_intersection->simulationWell() )
    {
        RimEclipseView* eclipseView = nullptr;
        m_intersection->firstAncestorOrThisOfType( eclipseView );

        // if ( eclipseView ) Do we need this ?
        {
            m_flatSimWellPipePartMgr = new RivSimWellPipesPartMgr( m_intersection->simulationWell() );
            m_flatWellHeadPartMgr    = new RivWellHeadPartMgr( m_intersection->simulationWell() );
        }
    }

    m_flatWellpathPartMgr = nullptr;
    if ( m_intersection->type() == RimExtrudedCurveIntersection::CrossSectionEnum::CS_WELL_PATH &&
         m_intersection->wellPath() )
    {
        Rim3dView* settingsView = nullptr;
        m_intersection->firstAncestorOrThisOfType( settingsView );
        if ( settingsView )
        {
            m_flatWellpathPartMgr = new RivWellPathPartMgr( m_intersection->wellPath(), settingsView );
            m_flatWellpathPartMgr->appendFlattenedStaticGeometryPartsToModel( m_intersectionVizModel.p(),
                                                                              this->displayCoordTransform().p(),
                                                                              this->ownerCase()->characteristicCellSize(),
                                                                              this->ownerCase()->activeCellsBoundingBox() );
        }
    }

    nativeOrOverrideViewer()->addStaticModelOnce( m_intersectionVizModel.p(), isUsingOverrideViewer() );

    m_intersectionVizModel->updateBoundingBoxesRecursive();

    if ( this->hasUserRequestedAnimation() )
    {
        if ( viewer() ) viewer()->setCurrentFrame( m_currentTimeStep );
    }

    if ( this->viewer()->mainCamera()->viewMatrix() == sm_defaultViewMatrix )
    {
        this->zoomAll();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onUpdateDisplayModelForCurrentTimeStep()
{
    update3dInfo();
    onUpdateLegends();

    if ( m_flatSimWellPipePartMgr.notNull() )
    {
        cvf::Scene* frameScene = nativeOrOverrideViewer()->frame( m_currentTimeStep, isUsingOverrideViewer() );
        if ( frameScene )
        {
            {
                cvf::String name = "SimWellPipeMod";
                Rim3dView::removeModelByName( frameScene, name );

                cvf::ref<cvf::ModelBasicList> simWellModelBasicList = new cvf::ModelBasicList;
                simWellModelBasicList->setName( name );

                m_flatSimWellPipePartMgr->appendFlattenedDynamicGeometryPartsToModel( simWellModelBasicList.p(),
                                                                                      m_currentTimeStep,
                                                                                      this->displayCoordTransform().p(),
                                                                                      m_intersection->extentLength(),
                                                                                      m_intersection->branchIndex() );

                for ( double offset : m_flatSimWellPipePartMgr->flattenedBranchWellHeadOffsets() )
                {
                    m_flatWellHeadPartMgr->appendFlattenedDynamicGeometryPartsToModel( simWellModelBasicList.p(),
                                                                                       m_currentTimeStep,
                                                                                       this->displayCoordTransform().p(),
                                                                                       offset );
                }

                simWellModelBasicList->updateBoundingBoxesRecursive();
                frameScene->addModel( simWellModelBasicList.p() );
                m_flatSimWellPipePartMgr->updatePipeResultColor( m_currentTimeStep );
            }
        }
    }

    if ( m_flatWellpathPartMgr.notNull() )
    {
        cvf::Scene* frameScene = nativeOrOverrideViewer()->frame( m_currentTimeStep, isUsingOverrideViewer() );
        if ( frameScene )
        {
            {
                cvf::String name = "WellPipeDynMod";
                Rim3dView::removeModelByName( frameScene, name );
                cvf::ref<cvf::ModelBasicList> dynWellPathModel = new cvf::ModelBasicList;
                dynWellPathModel->setName( name );

                m_flatWellpathPartMgr
                    ->appendFlattenedDynamicGeometryPartsToModel( dynWellPathModel.p(),
                                                                  m_currentTimeStep,
                                                                  this->displayCoordTransform().p(),
                                                                  this->ownerCase()->characteristicCellSize(),
                                                                  this->ownerCase()->activeCellsBoundingBox() );
                dynWellPathModel->updateBoundingBoxesRecursive();
                frameScene->addModel( dynWellPathModel.p() );
            }
        }
    }

    if ( ( this->hasUserRequestedAnimation() && this->hasResults() ) )
    {
        m_flatIntersectionPartMgr->updateCellResultColor( m_currentTimeStep,
                                                          m_legendConfig->scalarMapper(),
                                                          m_ternaryLegendConfig()->scalarMapper() );
    }
    else
    {
        m_flatIntersectionPartMgr->applySingleColorEffect();
    }
}
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onUpdateLegends()
{
    m_legendObjectToSelect = nullptr;

    if ( !nativeOrOverrideViewer() ) return;

    nativeOrOverrideViewer()->removeAllColorLegends();

    if ( !hasResults() ) return;

    RimGeoMechResultDefinition* geomResDef          = nullptr;
    RimEclipseResultDefinition* eclResDef           = nullptr;
    RimRegularLegendConfig*     regularLegendConfig = nullptr;
    RimTernaryLegendConfig*     ternaryLegendConfig = nullptr;

    {
        RimEclipseView* originEclView = nullptr;
        m_intersection->firstAncestorOrThisOfType( originEclView );
        RimGeoMechView* originGeoView = nullptr;
        m_intersection->firstAncestorOrThisOfType( originGeoView );

        if ( originEclView )
        {
            eclResDef           = originEclView->cellResult();
            regularLegendConfig = originEclView->cellResult()->legendConfig();
            ternaryLegendConfig = originEclView->cellResult()->ternaryLegendConfig();
        }
        else if ( originGeoView )
        {
            geomResDef          = originGeoView->cellResultResultDefinition();
            regularLegendConfig = originGeoView->cellResult()->legendConfig();
        }
    }

    if ( RimIntersectionResultDefinition* sepInterResultDef = m_intersection->activeSeparateResultDefinition() )
    {
        if ( sepInterResultDef->isEclipseResultDefinition() )
        {
            eclResDef           = sepInterResultDef->eclipseResultDefinition();
            regularLegendConfig = sepInterResultDef->regularLegendConfig();
            ternaryLegendConfig = sepInterResultDef->ternaryLegendConfig();
            geomResDef          = nullptr;
        }
        else
        {
            geomResDef          = sepInterResultDef->geoMechResultDefinition();
            regularLegendConfig = sepInterResultDef->regularLegendConfig();
            eclResDef           = nullptr;
        }
    }

    caf::TitledOverlayFrame* legend = nullptr;

    // Copy the legend settings from the real view

    m_legendConfig()->setUiValuesFromLegendConfig( regularLegendConfig );
    if ( ternaryLegendConfig ) m_ternaryLegendConfig()->setUiValuesFromLegendConfig( ternaryLegendConfig );

    if ( eclResDef )
    {
        eclResDef->updateRangesForExplicitLegends( m_legendConfig(), m_ternaryLegendConfig(), m_currentTimeStep() );

        if ( eclResDef->isTernarySaturationSelected() )
        {
            m_ternaryLegendConfig()->setTitle( "Cell Result:\n" );
            legend = m_ternaryLegendConfig()->titledOverlayFrame();

            m_legendObjectToSelect = ternaryLegendConfig;
        }
        else
        {
            eclResDef->updateLegendTitle( m_legendConfig, "Cell Result:\n" );
            legend = m_legendConfig()->titledOverlayFrame();

            m_legendObjectToSelect = regularLegendConfig;
        }
    }

    if ( geomResDef )
    {
        geomResDef->updateLegendTextAndRanges( m_legendConfig(), "Cell Result:\n", m_currentTimeStep() );
        legend = m_legendConfig()->titledOverlayFrame();

        m_legendObjectToSelect = regularLegendConfig;
    }

    if ( legend )
    {
        nativeOrOverrideViewer()->addColorLegendToBottomLeftCorner( legend, isUsingOverrideViewer() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onResetLegendsInViewer()
{
    nativeOrOverrideViewer()->showAxisCross( false );
    nativeOrOverrideViewer()->showAnimationProgress( true );
    nativeOrOverrideViewer()->showHistogram( false );
    nativeOrOverrideViewer()->showInfoText( false );
    nativeOrOverrideViewer()->showVersionInfo( false );
    nativeOrOverrideViewer()->showEdgeTickMarksXZ( true, m_showAxisLines() );

    nativeOrOverrideViewer()->setMainScene( new cvf::Scene(), isUsingOverrideViewer() );
    nativeOrOverrideViewer()->enableNavigationRotation( false );

    m_ternaryLegendConfig()->recreateLegend();
    m_legendConfig()->recreateLegend();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onCreatePartCollectionFromSelection( cvf::Collection<cvf::Part>* parts )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onClampCurrentTimestep()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onUpdateStaticCellColors()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onUpdateScaleTransform()
{
    cvf::Mat4d scale = cvf::Mat4d::IDENTITY;
    scale( 2, 2 )    = scaleZ();

    this->scaleTransform()->setLocalTransform( scale );

    if ( nativeOrOverrideViewer() ) nativeOrOverrideViewer()->updateCachedValuesInScene();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Transform* Rim2dIntersectionView::scaleTransform()
{
    return m_scaleTransform.p();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::onLoadDataAndUpdate()
{
    updateMdiWindowVisibility();

    this->scheduleCreateDisplayModelAndRedraw();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                              const QVariant&            oldValue,
                                              const QVariant&            newValue )
{
    Rim3dView::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_intersection || changedField == &m_showDefiningPoints )
    {
        this->loadDataAndUpdate();
    }
    else if ( changedField == &m_showAxisLines )
    {
        nativeOrOverrideViewer()->showEdgeTickMarksXZ( true, m_showAxisLines() );
        this->loadDataAndUpdate();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_nameProxy );

    Rim3dView::defineUiOrdering( uiConfigName, uiOrdering );
    caf::PdmUiGroup* viewGroup = uiOrdering.findGroup( "ViewGroup" );
    if ( viewGroup )
    {
        viewGroup->add( &m_showAxisLines );
    }

    uiOrdering.skipRemainingFields( true );

    if ( m_intersection->hasDefiningPoints() )
    {
        caf::PdmUiGroup* plGroup = uiOrdering.addNewGroup( "Defining Points" );
        plGroup->add( &m_showDefiningPoints );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void Rim2dIntersectionView::defineUiTreeOrdering( caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/ )
{
    uiTreeOrdering.skipRemainingChildren( true );
}
