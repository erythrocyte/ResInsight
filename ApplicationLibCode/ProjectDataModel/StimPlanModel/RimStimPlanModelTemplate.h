/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2020-     Equinor ASA
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

#pragma once

#include "RiaStimPlanModelDefines.h"

#include "RimFaciesInitialPressureConfig.h"
#include "RimNamedObject.h"

#include "cafPdmChildArrayField.h"
#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"
#include "cafPdmPtrField.h"

#include <map>

class RimEclipseCase;
class RimElasticProperties;
class RigEclipseCaseData;
class RimFaciesProperties;
class RimNonNetLayers;
class RimFaciesInitialPressureConfig;
class RimPressureTable;

//==================================================================================================
///
///
//==================================================================================================
class RimStimPlanModelTemplate : public RimNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimStimPlanModelTemplate( void );
    ~RimStimPlanModelTemplate( void ) override;

    caf::Signal<> changed;

    void setId( int id );
    int  id() const;

    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

    double defaultPorosity() const;
    double defaultPermeability() const;

    double overburdenHeight() const;
    double underburdenHeight() const;

    double defaultOverburdenPorosity() const;
    double defaultUnderburdenPorosity() const;

    double defaultOverburdenPermeability() const;
    double defaultUnderburdenPermeability() const;

    QString overburdenFormation() const;
    QString overburdenFacies() const;

    QString underburdenFormation() const;
    QString underburdenFacies() const;

    double overburdenFluidDensity() const;
    double underburdenFluidDensity() const;

    double referenceTemperature() const;
    double referenceTemperatureGradient() const;
    double referenceTemperatureDepth() const;

    double verticalStress() const;
    double verticalStressGradient() const;
    double stressDepth() const;

    void setDynamicEclipseCase( RimEclipseCase* eclipseCase );
    void setTimeStep( int timeStep );
    void setStaticEclipseCase( RimEclipseCase* eclipseCase );
    void setInitialPressureEclipseCase( RimEclipseCase* eclipseCase );

    RimEclipseCase* dynamicEclipseCase() const;
    int             timeStep() const;
    RimEclipseCase* staticEclipseCase() const;
    RimEclipseCase* initialPressureEclipseCase() const;

    std::map<int, double> faciesWithInitialPressure() const;

    void loadDataAndUpdate();

    void                  setElasticProperties( RimElasticProperties* elasticProperties );
    RimElasticProperties* elasticProperties() const;

    void                 setFaciesProperties( RimFaciesProperties* faciesProperties );
    RimFaciesProperties* faciesProperties() const;

    void             setNonNetLayers( RimNonNetLayers* nonNetLayers );
    RimNonNetLayers* nonNetLayers() const;

    void              setPressureTable( RimPressureTable* pressureTable );
    RimPressureTable* pressureTable() const;

    void updateReferringPlots();

    bool usePressureTableForProperty( RiaDefines::CurveProperty curveProperty ) const;

    bool useEqlnumForPressureInterpolation() const;

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;

    void defineUiTreeOrdering( caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/ ) override;

    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions,
                                                         bool*                      useOptionsOnly ) override;
    void                          initAfterRead() override;
    void                          defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                         QString                    uiConfigName,
                                                         caf::PdmUiEditorAttribute* attribute ) override;

private:
    RimEclipseCase*     getEclipseCase() const;
    RigEclipseCaseData* getEclipseCaseData() const;

    void faciesPropertiesChanged( const caf::SignalEmitter* emitter );
    void elasticPropertiesChanged( const caf::SignalEmitter* emitter );
    void nonNetLayersChanged( const caf::SignalEmitter* emitter );
    void pressureTableChanged( const caf::SignalEmitter* emitter );

    double      computeDefaultStressDepth() const;
    static bool shouldProbablyUseInitialPressure( const QString& faciesName );

    caf::PdmField<int>                        m_id;
    caf::PdmPtrField<RimEclipseCase*>         m_dynamicEclipseCase;
    caf::PdmField<int>                        m_timeStep;
    caf::PdmPtrField<RimEclipseCase*>         m_initialPressureEclipseCase;
    caf::PdmField<bool>                       m_useTableForInitialPressure;
    caf::PdmField<bool>                       m_useTableForPressure;
    caf::PdmField<bool>                       m_useEqlnumForPressureInterpolation;
    caf::PdmField<bool>                       m_editPressureTable;
    caf::PdmPtrField<RimEclipseCase*>         m_staticEclipseCase;
    caf::PdmField<double>                     m_defaultPorosity;
    caf::PdmField<double>                     m_defaultPermeability;
    caf::PdmField<double>                     m_verticalStress;
    caf::PdmField<double>                     m_verticalStressGradient;
    caf::PdmField<double>                     m_stressDepth;
    caf::PdmField<double>                     m_referenceTemperature;
    caf::PdmField<double>                     m_referenceTemperatureGradient;
    caf::PdmField<double>                     m_referenceTemperatureDepth;
    caf::PdmField<double>                     m_overburdenHeight;
    caf::PdmField<double>                     m_overburdenPorosity;
    caf::PdmField<double>                     m_overburdenPermeability;
    caf::PdmField<QString>                    m_overburdenFormation;
    caf::PdmField<QString>                    m_overburdenFacies;
    caf::PdmField<double>                     m_overburdenFluidDensity;
    caf::PdmField<double>                     m_underburdenHeight;
    caf::PdmField<double>                     m_underburdenPorosity;
    caf::PdmField<double>                     m_underburdenPermeability;
    caf::PdmField<QString>                    m_underburdenFormation;
    caf::PdmField<QString>                    m_underburdenFacies;
    caf::PdmField<double>                     m_underburdenFluidDensity;
    caf::PdmChildField<RimElasticProperties*> m_elasticProperties;
    caf::PdmChildField<RimFaciesProperties*>  m_faciesProperties;
    caf::PdmChildField<RimNonNetLayers*>      m_nonNetLayers;

    caf::PdmChildArrayField<RimFaciesInitialPressureConfig*> m_faciesInitialPressureConfigs;
    caf::PdmChildField<RimPressureTable*>                    m_pressureTable;
};
