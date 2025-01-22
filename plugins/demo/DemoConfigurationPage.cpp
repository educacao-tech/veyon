/*
 * DemoConfigurationPage.cpp - implementation of DemoConfigurationPage
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

#include "DemoConfiguration.h"
#include "DemoConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_DemoConfigurationPage.h"

DemoConfigurationPage::DemoConfigurationPage( DemoConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::DemoConfigurationPage ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags( this, Configuration::Property::Flag::Advanced );
}



DemoConfigurationPage::~DemoConfigurationPage()
{
	delete ui;
}



void DemoConfigurationPage::resetWidgets()
{
	FOREACH_DEMO_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void DemoConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_DEMO_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void DemoConfigurationPage::applyConfiguration()
{
}
