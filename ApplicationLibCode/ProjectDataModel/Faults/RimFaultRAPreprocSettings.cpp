/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2021  Equinor ASA
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

#include "RimFaultRAPreprocSettings.h"

#include "RiaApplication.h"
#include "RiaPreferences.h"
#include "RimEclipseResultCase.h"
#include "RimGeoMechCase.h"
#include "RimProject.h"
#include "RimTools.h"

#include "cafPdmFieldScriptingCapability.h"
#include "cafPdmObjectScriptingCapability.h"
#include "cafPdmUiComboBoxEditor.h"
#include "cafPdmUiFilePathEditor.h"

#include <QDir>
#include <QFileInfo>

CAF_PDM_SOURCE_INIT( RimFaultRAPreprocSettings, "RimFaultRAPreprocSettings" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFaultRAPreprocSettings::RimFaultRAPreprocSettings()
{
    CAF_PDM_InitObject( "Fault RA Preproc Settings", ":/fault_react_24x24.png", "", "" );

    CAF_PDM_InitField( &m_startTimestepEclipse, "StartTimeStepEclipse", 0, "Start Time Step", "", "", "" );
    m_startTimestepEclipse.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitField( &m_endTimestepEclipse, "EndTimeStepEclipse", 0, "End Time Step", "", "", "" );
    m_endTimestepEclipse.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_startTimestepGeoMech, "StartTimeStepGeoMech", 0, "Start Time Step", "", "", "" );
    m_startTimestepGeoMech.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitField( &m_endTimestepGeoMech, "EndTimeStepGeoMech", 0, "End Time Step", "", "", "" );
    m_endTimestepGeoMech.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_eclipseCase, "EclipseCase", "Eclipse Case", "", "", "" );
    m_eclipseCase.setValue( nullptr );

    CAF_PDM_InitFieldNoDefault( &m_geomechCase, "GeomechCase", "GeoMech Case", "", "", "" );
    m_geomechCase.setValue( nullptr );

    CAF_PDM_InitFieldNoDefault( &m_baseDir, "BaseDir", "Output Directory", "", "", "" );
    m_baseDir.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_cleanBaseDir, "CleanBaseDir", false, "Clean Output Directory", "", "", "" );
    CAF_PDM_InitField( &m_smoothEclipseData, "SmoothEclipseData", true, "Smooth Eclipse Data", "", "", "" );

    CAF_PDM_InitField( &m_elasticTableFilename, "ElasticTableFilename", QString( "" ), "Elastic Table", "", "", "" );
    m_elasticTableFilename.uiCapability()->setUiEditorTypeName( caf::PdmUiFilePathEditor::uiEditorTypeName() );
    m_elasticTableFilename.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::HIDDEN );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFaultRAPreprocSettings::~RimFaultRAPreprocSettings()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                  const QVariant&            oldValue,
                                                  const QVariant&            newValue )
{
    if ( ( changedField == &m_eclipseCase ) && ( m_startTimestepEclipse == m_endTimestepEclipse ) )
    {
        m_endTimestepEclipse = m_eclipseCase()->timeStepStrings().size() - 1;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                       QString                    uiConfigName,
                                                       caf::PdmUiEditorAttribute* attribute )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    if ( m_geomechCase() )
    {
        auto geomechGroup = uiOrdering.addNewGroup( "GeoMechanical Model" );
        geomechGroup->add( &m_geomechCase );
        geomechGroup->add( &m_startTimestepGeoMech );
        geomechGroup->add( &m_endTimestepGeoMech );

        auto tableGroup = uiOrdering.addNewGroup( "Elastic Table" );
        tableGroup->add( &m_elasticTableFilename );
    }

    auto eclipseGroup = uiOrdering.addNewGroup( "Eclipse Model" );
    eclipseGroup->add( &m_eclipseCase );
    eclipseGroup->add( &m_smoothEclipseData );
    eclipseGroup->add( &m_startTimestepEclipse );
    eclipseGroup->add( &m_endTimestepEclipse );

    auto outputGroup = uiOrdering.addNewGroup( "Output Settings" );
    outputGroup->add( &m_baseDir );
    outputGroup->add( &m_cleanBaseDir );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo>
    RimFaultRAPreprocSettings::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_eclipseCase )
    {
        RimTools::eclipseCaseOptionItems( &options );
    }
    else if ( fieldNeedingOptions == &m_geomechCase )
    {
        RimTools::geoMechCaseOptionItems( &options );
    }

    if ( m_geomechCase() )
    {
        if ( fieldNeedingOptions == &m_startTimestepGeoMech )
        {
            RimTools::timeStepsForCase( m_geomechCase, &options );
        }
        else if ( fieldNeedingOptions == &m_endTimestepGeoMech )
        {
            RimTools::timeStepsForCase( m_geomechCase, &options );
        }
    }

    if ( m_eclipseCase() )
    {
        if ( fieldNeedingOptions == &m_startTimestepEclipse )
        {
            RimTools::timeStepsForCase( m_eclipseCase, &options );
        }
        else if ( fieldNeedingOptions == &m_endTimestepEclipse )
        {
            RimTools::timeStepsForCase( m_eclipseCase, &options );
        }
    }

    return options;
}

RimCase* RimFaultRAPreprocSettings::startCase() const
{
    if ( m_geomechCase() ) return m_geomechCase();
    return m_eclipseCase();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimFaultRAPreprocSettings::startTimeStepEclipseIndex() const
{
    return m_startTimestepEclipse();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::startTimeStepEclipse() const
{
    if ( m_eclipseCase() )
    {
        if ( ( m_startTimestepEclipse >= 0 ) && ( m_startTimestepEclipse <= m_eclipseCase->timeStepStrings().size() ) )
            return m_eclipseCase->timeStepStrings()[m_startTimestepEclipse];
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimFaultRAPreprocSettings::endTimeStepEclipseIndex() const
{
    return m_endTimestepEclipse();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::endTimeStepEclipse() const
{
    if ( m_eclipseCase() )
    {
        if ( ( m_endTimestepEclipse >= 0 ) && ( m_endTimestepEclipse <= m_eclipseCase->timeStepStrings().size() ) )
            return m_eclipseCase->timeStepStrings()[m_endTimestepEclipse];
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::eclipseCaseFilename() const
{
    if ( m_eclipseCase ) return m_eclipseCase->gridFileName();
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimFaultRAPreprocSettings::startTimeStepGeoMechIndex() const
{
    return m_startTimestepGeoMech();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::startTimeStepGeoMech() const
{
    if ( m_geomechCase() )
    {
        if ( ( m_startTimestepGeoMech >= 0 ) && ( m_startTimestepGeoMech <= m_geomechCase->timeStepStrings().size() ) )
            return m_geomechCase->timeStepStrings()[m_startTimestepGeoMech];
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimFaultRAPreprocSettings::endTimeStepGeoMechIndex() const
{
    return m_endTimestepGeoMech();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::endTimeStepGeoMech() const
{
    if ( m_geomechCase() )
    {
        if ( ( m_endTimestepGeoMech >= 0 ) && ( m_endTimestepGeoMech <= m_geomechCase->timeStepStrings().size() ) )
            return m_geomechCase->timeStepStrings()[m_endTimestepGeoMech];
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::geomechCaseFilename() const
{
    if ( m_geomechCase ) return m_geomechCase->gridFileName();
    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::elasticTableFilename() const
{
    return m_elasticTableFilename();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::preprocParameterFilename() const
{
    return m_baseDir + "/tmp/pre_processing.json";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::outputBaseDirectory() const
{
    return m_baseDir();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFaultRAPreprocSettings::cleanBaseDirectory() const
{
    return m_cleanBaseDir();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::setCleanBaseDirectory( bool clean )
{
    m_cleanBaseDir = clean;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFaultRAPreprocSettings::smoothEclipseData() const
{
    return m_smoothEclipseData();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::setGeoMechCase( RimGeoMechCase* geomechCase )
{
    m_geomechCase = geomechCase;
    if ( geomechCase ) m_endTimestepGeoMech = geomechCase->timeStepStrings().size() - 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGeoMechCase* RimFaultRAPreprocSettings::geoMechCase() const
{
    return m_geomechCase();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::setEclipseCase( RimEclipseResultCase* eclipseCase )
{
    m_eclipseCase = eclipseCase;
    if ( eclipseCase ) m_endTimestepEclipse = eclipseCase->timeStepStrings().size() - 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEclipseCase* RimFaultRAPreprocSettings::eclipseCase() const
{
    return m_eclipseCase();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFaultRAPreprocSettings::setOutputBaseDirectory( QString baseDir )
{
    m_baseDir = baseDir;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::outputEclipseFilename() const
{
    QString retval = outputEclipseDirectory();
    retval += QDir::separator();

    QFileInfo fi( eclipseCaseFilename() );
    retval += fi.baseName() + "_RI.GRDECL";

    return retval;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::outputEclipseDirectory() const
{
    QString retval = m_baseDir;
    retval += "/Eclipse";

    return retval;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFaultRAPreprocSettings::outputAbaqusDirectory() const
{
    QString retval = m_baseDir;
    retval += "/Abaqus";

    return retval;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RimFaultRAPreprocSettings::preprocParameterList() const
{
    QStringList retlist;
    retlist << preprocParameterFilename();
    return retlist;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RimFaultRAPreprocSettings::macrisPrepareParameterList() const
{
    QStringList retlist;

    retlist << "prepare";

    retlist << "-o";

    retlist << outputEclipseDirectory();

    if ( m_smoothEclipseData )
    {
        retlist << "-ds";
    }
    else
    {
        retlist << "-d";
    }

    retlist << m_eclipseCase()->gridFileName();

    return retlist;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFaultRAPreprocSettings::geoMechSelected() const
{
    return m_geomechCase.value() != nullptr;
}
