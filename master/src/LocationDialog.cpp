/*
 * LocationDialog.cpp - header file for LocationDialog
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

#include "LocationDialog.h"

#include "ui_LocationDialog.h"

LocationDialog::LocationDialog( QAbstractItemModel* locationListModel, QWidget* parent ) :
	QDialog(parent),
	ui(new Ui::LocationDialog)
{
	ui->setupUi( this );

	m_networkObjectFilterProxyModel.setSourceModel(locationListModel);
	m_networkObjectFilterProxyModel.setComputersExcluded(true);

	m_sortFilterProxyModel.setSourceModel(&m_networkObjectFilterProxyModel);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	m_sortFilterProxyModel.setRecursiveFilteringEnabled(true);
#endif
	m_sortFilterProxyModel.setFilterCaseSensitivity( Qt::CaseInsensitive );
	m_sortFilterProxyModel.sort( 0 );

	ui->treeView->setModel(&m_sortFilterProxyModel);

	connect (ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
			 this, &LocationDialog::updateSelection);
	connect (ui->treeView, &QTreeView::activated,
			 this, &LocationDialog::accept);

	updateSearchFilter();
}



LocationDialog::~LocationDialog()
{
	delete ui;
}



void LocationDialog::updateSearchFilter()
{
	ui->treeView->expandAll();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 1)
	m_sortFilterProxyModel.setFilterRegularExpression( ui->filterLineEdit->text() );
#else
	m_sortFilterProxyModel.setFilterRegExp( ui->filterLineEdit->text() );
#endif

	ui->treeView->selectionModel()->setCurrentIndex( m_sortFilterProxyModel.index( 0, 0 ),
														 QItemSelectionModel::ClearAndSelect );
}



void LocationDialog::updateSelection( const QModelIndex& current, const QModelIndex& previous )
{
	Q_UNUSED(previous)

	m_selectedLocation = m_sortFilterProxyModel.data( current ).toString();
}
