/*
 * CommandLinePluginInterface.h - interface class for command line plugins
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

#pragma once

#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT CommandLinePluginInterface
{
	Q_GADGET
public:
	enum RunResult
	{
		Unknown,
		Successful,
		Failed,
		InvalidArguments,
		NotEnoughArguments,
		InvalidCommand,
		NotLicensed,
		NoResult,
		RunResultCount
	} ;

	Q_ENUM(RunResult)

	virtual QString commandLineModuleName() const = 0;
	virtual QString commandLineModuleHelp() const = 0;
	virtual QStringList commands() const = 0;
	virtual QString commandHelp( const QString& command ) const = 0;

};

using CommandLinePluginInterfaceList = QList<CommandLinePluginInterface *>;

#define CommandLinePluginInterface_iid "io.veyon.Veyon.Plugins.CommandLinePluginInterface"

Q_DECLARE_INTERFACE(CommandLinePluginInterface, CommandLinePluginInterface_iid)
