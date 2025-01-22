/*
 * ShellCommands.cpp - implementation of ShellCommands class
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QFile>
#include <QProcess>

#include "CommandLineIO.h"
#include "ShellCommands.h"


ShellCommands::ShellCommands( QObject* parent ) :
	QObject( parent ),
	m_commands( {
		{ QStringLiteral("run"), tr( "Run command file" ) },
		} )
{
}



QStringList ShellCommands::commands() const
{
	return m_commands.keys();
}



QString ShellCommands::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult ShellCommands::handle_main()
{
	QTextStream stream( stdin );

	while( true )
	{
		printf("VEYON> ");

		QString line;
		if( stream.readLineInto( &line ) && line != QLatin1String("exit") )
		{
			runCommand( line );
		}
		else
		{
			break;
		}
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult ShellCommands::handle_run( const QStringList& arguments )
{
	QFile scriptFile( arguments.value( 0 ) );
	if( scriptFile.exists() == false )
	{
		CommandLineIO::error( tr( "File \"%1\" does not exist!" ).arg( scriptFile.fileName() ) );
		return Failed;
	}

	while( scriptFile.canReadLine() )
	{
		runCommand( QString::fromUtf8( scriptFile.readLine() ) );
	}

	return Successful;
}



void ShellCommands::runCommand( const QString& command )
{
	// TODO: properly split arguments containing spaces
	QProcess::execute( QCoreApplication::applicationFilePath(), command.split( QLatin1Char(' ') ) );
}
