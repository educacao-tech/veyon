/*
 * ComputerMonitoringView.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerMonitoringView.h"
#include "ComputerMonitoringModel.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"
#include "UserConfig.h"


ComputerMonitoringView::ComputerMonitoringView() :
	m_master( VeyonCore::instance()->findChild<VeyonMaster *>() )
{
	m_autoAdjustIconSize = VeyonCore::config().autoAdjustMonitoringIconSize() ||
						   master()->userConfig().autoAdjustMonitoringIconSize();

	m_iconSizeAutoAdjustTimer.setInterval( IconSizeAdjustDelay );
	m_iconSizeAutoAdjustTimer.setSingleShot( true );
}



void ComputerMonitoringView::initializeView( QObject* self )
{
	const auto autoAdjust = [this]() { initiateIconSizeAutoAdjust(); };

	QObject::connect( &m_iconSizeAutoAdjustTimer, &QTimer::timeout, self, [this]() { performIconSizeAutoAdjust(); } );
	QObject::connect( dataModel(), &ComputerMonitoringModel::rowsInserted, self, autoAdjust );
	QObject::connect( dataModel(), &ComputerMonitoringModel::rowsRemoved, self, autoAdjust );
	QObject::connect( &m_master->computerControlListModel(), &ComputerControlListModel::computerScreenSizeChanged, self,
					  [this]() { setIconSize( m_master->computerControlListModel().computerScreenSize() ); } );

	setColors( VeyonCore::config().computerMonitoringBackgroundColor(),
			   VeyonCore::config().computerMonitoringTextColor() );

	setComputerScreenSize( m_master->userConfig().monitoringScreenSize() );

	loadComputerPositions( m_master->userConfig().computerPositions() );
	setUseCustomComputerPositions( m_master->userConfig().useCustomComputerPositions() );
}



void ComputerMonitoringView::saveConfiguration()
{
	m_master->userConfig().setFilterPoweredOnComputers( dataModel()->stateFilter() != ComputerControlInterface::State::None );
	m_master->userConfig().setFilterComputersWithLoggedOnUsers( dataModel()->filterNonEmptyUserLoginNames() );
	m_master->userConfig().setComputerPositions( saveComputerPositions() );
	m_master->userConfig().setUseCustomComputerPositions( useCustomComputerPositions() );
}



ComputerMonitoringModel* ComputerMonitoringView::dataModel() const
{
	return m_master->computerMonitoringModel();
}



QString ComputerMonitoringView::searchFilter() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	return dataModel()->filterRegularExpression().pattern();
#else
	return dataModel()->filterRegExp().pattern();
#endif
}



void ComputerMonitoringView::setSearchFilter( const QString& searchFilter )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 1)
	dataModel()->setFilterRegularExpression( searchFilter );
#else
	dataModel()->setFilterRegExp( searchFilter );
#endif
}



void ComputerMonitoringView::setFilterPoweredOnComputers( bool enabled )
{
	dataModel()->setStateFilter( enabled ? ComputerControlInterface::State::Connected :
										   ComputerControlInterface::State::None );
}



void ComputerMonitoringView::setFilterComputersWithLoggedOnUsers( bool enabled )
{
	dataModel()->setFilterNonEmptyUserLoginNames( enabled );
}



void ComputerMonitoringView::setComputerScreenSize( int size )
{
	if( m_computerScreenSize != size )
	{
		const auto minSize = MinimumComputerScreenSize;
		const auto maxSize = MaximumComputerScreenSize;
		size = qBound<int>( minSize, size, maxSize );

		m_computerScreenSize = size;

		m_master->userConfig().setMonitoringScreenSize( size );

		m_master->computerControlListModel().updateComputerScreenSize();
	}
}



int ComputerMonitoringView::computerScreenSize() const
{
	return m_computerScreenSize;
}



void ComputerMonitoringView::setAutoAdjustIconSize( bool enabled )
{
	m_autoAdjustIconSize = enabled;

	if( m_autoAdjustIconSize )
	{
		performIconSizeAutoAdjust();
	}
}



bool ComputerMonitoringView::performIconSizeAutoAdjust()
{
	m_iconSizeAutoAdjustTimer.stop();

	return m_autoAdjustIconSize && dataModel()->rowCount() > 0;
}




void ComputerMonitoringView::initiateIconSizeAutoAdjust()
{
	m_iconSizeAutoAdjustTimer.start();
}



void ComputerMonitoringView::runFeature( const Feature& feature )
{
	auto computerControlInterfaces = selectedComputerControlInterfaces();
	if( computerControlInterfaces.isEmpty() )
	{
		computerControlInterfaces = m_master->filteredComputerControlInterfaces();
	}

	const auto alreadyActive = feature.testFlag( Feature::Flag::Mode ) &&
							   isFeatureOrRelatedFeatureActive( computerControlInterfaces, feature.uid() );

	if( feature.testFlag( Feature::Flag::Mode ) )
	{
		for( const auto& currentFeature : m_master->features() )
		{
			// stop already active or all other active mode features
			if( currentFeature.testFlag( Feature::Flag::Mode ) && ( alreadyActive || currentFeature != feature ) )
			{
				VeyonCore::featureManager().stopFeature( *m_master, currentFeature, computerControlInterfaces );

				const auto subFeatures = m_master->subFeatures( currentFeature.uid() );
				for( const auto& subFeature : subFeatures )
				{
					VeyonCore::featureManager().stopFeature( *m_master, subFeature, computerControlInterfaces );
				}
			}
		}
	}

	if( alreadyActive == false )
	{
		VeyonCore::featureManager().startFeature( *m_master, feature, computerControlInterfaces );
	}
}



bool ComputerMonitoringView::isFeatureOrRelatedFeatureActive( const ComputerControlInterfaceList& computerControlInterfaces,
															 Feature::Uid featureUid ) const
{
	const auto& relatedFeatures = VeyonCore::featureManager().relatedFeatures( featureUid );

	for( const auto& controlInterface : computerControlInterfaces )
	{
		if( controlInterface->activeFeatures().contains( featureUid ) ||
			std::find_if( relatedFeatures.begin(), relatedFeatures.end(),
						  [&controlInterface](const auto& relatedFeature) {
							  return controlInterface->activeFeatures().contains( relatedFeature.uid() );
						  } )
			!= relatedFeatures.end() )
		{
			return true;
		}
	}

	return false;
}
