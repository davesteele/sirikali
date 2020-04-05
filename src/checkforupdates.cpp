﻿/*
 *
 *  Copyright (c) 2015
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

#include "checkforupdates.h"
#include "settings.h"
#include "json_parser.hpp"
#include "engines.h"

checkUpdates::checkUpdates( QWidget * widget,checkforupdateswindow::functions ff ) :
	m_widget( widget ),
	m_timeOut( settings::instance().networkTimeOut() ),
	m_running( false ),
	m_functions( std::move( ff ) )
{
	m_networkRequest.setRawHeader( "Host","api.github.com" ) ;
	m_networkRequest.setRawHeader( "Accept-Encoding","text/plain" ) ;

	m_backends.emplace_back( "sirikali","https://api.github.com/repos/mhogomchungu/sirikali/releases" ) ;

	for( const auto& it : engines::instance().supportedEngines() ){

		const auto& e = it->releaseURL() ;

		if( !e.isEmpty() ){

			m_backends.emplace_back( it->name().toLower(),e ) ;
		}
	}
}

void checkUpdates::check( bool e )
{
	m_autocheck = e ;

	m_results.clear() ;

	m_running = true ;

	this->checkForUpdate() ;
}

void checkUpdates::run( bool e )
{
	if( m_running == false ){

		if( e ){

			if( settings::instance().autoCheck() ){

				this->check( e ) ;
			}
		}else{
			this->check( e ) ;
		}
	}
}

void checkUpdates::showResult()
{
	m_running = false ;

	bool show = false ;

	auto _tr = []( const QStringList& l ){

		auto s = QObject::tr( "\"%1\" Installed Version Is : %2.<br>Latest Version Is : %3.<br>" ) ;
		return s.arg( l.at( 0 ),l.at( 1 ),l.at( 2 ) ) ;
	} ;

	auto e = "<!DOCTYPE html><html><body><center>" + _tr( m_results.at( 0 ) ) ;

	for( int i = 1 ; i < m_results.size() ; i++ ){

		const auto& it = m_results.at( i ) ;

		e += "<br>" + _tr( it ) ;

		const auto& a = it.at( 1 ) ;

		const auto& b = it.at( 2 ) ;

		if( a != "N/A" && b != "N/A" ){

			if( a != b ){

				show = true ;
			}
		}
	}

	e += "</center></body></html>" ;

	if( m_autocheck ){

		if( show ){

			m_functions.first() ;

			checkforupdateswindow::instance( m_widget,m_functions ).ShowUI( e ) ;
		}
	}else{
		m_functions.first() ;

		checkforupdateswindow::instance( m_widget,m_functions ).ShowUI( e ) ;
	}
}

QString checkUpdates::InstalledVersion( const QString& e )
{
	if( e == "sirikali" ){

		return THIS_VERSION ;
	}else{
		auto s = utility::unwrap( utility::backEndInstalledVersion( e ) ) ;

		if( s ){

			return s.value() ;
		}else{
			return "N/A" ;
		}
	}
}

QString checkUpdates::latestVersion( const QByteArray& data )
{
	auto _found_release = []( const QString& e ){

		for( const auto& it : e ){

			/*
			 * A release version has version in format of "A.B.C"
			 *
			 * ie it only has dots and digits. Presence of any other
			 * character makes the release assumed to be a beta/alpha
			 * or prerelease version(something like "A.B.C-rc1" or
			 * "A.B.C.beta6"
			 */
			if( it != '.' && !( it >= '0' && it <= '9' ) ){

				return false ;
			}
		}

		return true ;
	} ;

	SirikaliJson json( data,SirikaliJson::type::CONTENTS ) ;

	for( auto& it : json.getTags( "tag_name" ) ){

		it.remove( 'v' ) ;
		it.remove( "sshfs-" ) ;

		if( _found_release( it ) ){

			return it ;
		}
	}

	return "N/A" ;
}

void checkUpdates::checkForUpdate( size_t position )
{
	if( position == m_backends.size() ){

		this->showResult() ;
	}else{
		const auto& e = m_backends[ position ] ;

		position++ ;

		auto exe = e.first ;

		auto f = this->InstalledVersion( exe ) ;

		if( exe == "ecryptfs" ){

			exe = "ecryptfs-simple" ;
		}

		if( f == "N/A" ){

			m_results += { exe,"N/A","N/A" } ;

			this->checkForUpdate( position ) ;
		}else {
			m_networkRequest.setUrl( QUrl( e.second ) ) ;

			m_network.get( m_timeOut,m_networkRequest,[ = ]( QNetworkReply& e ){

				try{
					m_results += { exe,f,this->latestVersion( e.readAll() ) } ;

				}catch( ... ){

					m_results += { exe,f,"N/A" } ;
				}

				this->checkForUpdate( position ) ;

			},[ this ](){

				auto s = QString::number( m_timeOut ) ;
				auto e = QObject::tr( "Network Request Failed To Respond Within %1 Seconds." ).arg( s ) ;

				DialogMsg( m_widget ).ShowUIOK( QObject::tr( "ERROR" ),e ) ;
				m_running = false ;
			} ) ;
		}
	}
}
