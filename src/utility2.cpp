/*
 *
 *  Copyright (c) 2017
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utility2.h"
#include "utility.h"

#include <QByteArray>
#include <QTranslator>

#include <QCoreApplication>

void utility2::translator::setLanguage( const QByteArray& e )
{
	QCoreApplication::installTranslator( [ & ](){

		this->clear() ;

		m_translator = new QTranslator() ;

		m_translator->load( e.constData(),utility::localizationLanguagePath() ) ;

		return m_translator ;
	}() ) ;
}

utility2::translator::~translator()
{
	this->clear() ;
}

void utility2::translator::clear()
{
	if( m_translator ){

		QCoreApplication::removeTranslator( m_translator ) ;
		delete m_translator ;
	}
}
