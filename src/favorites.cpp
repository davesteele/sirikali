﻿/*
 *
 *  Copyright ( c ) 2011-2015
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  ( at your option ) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "favorites.h"

#include "utility.h"
#include "settings.h"
#include "crypto.h"

#include <QDir>
#include <QFile>
#include <QCryptographicHash>

static utility::qstring_result _config_path()
{
	QString m = settings::instance().ConfigLocation() + "/favorites/" ;

	if( utility::pathExists( m ) ){

		return m ;
	}else{
		if( QDir().mkpath( m ) ){

			return m ;
		}else{
			return {} ;
		}
	}
}

static QString _create_path( const QString& m,const favorites::entry& e,bool newFormat )
{
	auto a = utility::split( e.volumePath,'@' ).last() ;

	a = utility::split( a,'/' ).last() ;

	a.replace( ":","" ) ;

	if( newFormat ){

		auto b = e.volumePath + e.mountPointPath ;

		return m + a + "-" + crypto::sha256( b ) + ".json" ;
	}else{
		auto b = a + e.mountPointPath ;

		return m + a + "-" + crypto::sha256( b ) + ".json" ;
	}
}

static QString _create_entry_path( const favorites::entry& e,bool newFormat )
{
	auto s = _config_path() ;

	if( s.has_value() ){

		return _create_path( s.value(),e,newFormat ) ;
	}else{
		return {} ;
	}
}

static favorites::entry _favorites( const QString& path )
{
	SirikaliJson json( QFile( path ),utility::jsonLogger() ) ;

	favorites::entry m ;

	if( json.passed() ){

		m.internalConfigPath   = QDir( path ).absolutePath() ;

		m.reverseMode          = json.getBool( "reverseMode" ) ;
		m.volumeNeedNoPassword = json.getBool( "volumeNeedNoPassword" ) ;
		m.volumePath           = json.getString( "volumePath" ) ;
		m.mountPointPath       = json.getString( "mountPointPath" ) ;
		m.configFilePath       = json.getString( "configFilePath" ) ;
		m.idleTimeOut          = json.getString( "idleTimeOut" ) ;
		m.mountOptions         = json.getString( "mountOptions" ) ;
		m.preMountCommand      = json.getString( "preMountCommand" ) ;
		m.postMountCommand     = json.getString( "postMountCommand" ) ;
		m.preUnmountCommand    = json.getString( "preUnmountCommand" ) ;
		m.postUnmountCommand   = json.getString( "postUnmountCommand" ) ;
		m.keyFile              = json.getString( "keyFilePath" ) ;

		m.password             = json.getByteArray( "password" ) ;

		favorites::triState::readTriState( json,m.readOnlyMode,"mountReadOnly" ) ;
		favorites::triState::readTriState( json,m.autoMount,"autoMountVolume" ) ;
	}

	return m ;
}

favorites::favorites()
{
	this->reload() ;
}

void favorites::reload()
{
	m_favorites.clear() ;

	const auto m = _config_path() ;

	if( m.has_value() ){

		const auto& a = m.value() ;

		const auto s = QDir( a ).entryList( QDir::Filter::Files | QDir::Filter::Hidden ) ;

		for( const auto& it : s ){

			auto m = _favorites( a + it ) ;

			if( m.hasValue() ){

				m_favorites.emplace_back( std::move( m ) ) ;
			}
		}
	}else{
		utility::debug() << "Failed To Get Favorites List" ;
	}
}

favorites::error favorites::add( const favorites::entry& e )
{
	auto m = _config_path() ;

	if( !m.has_value() ){

		utility::debug() << "Failed To Get Favorites Path" ;

		return error::FAILED_TO_CREATE_ENTRY ;
	}

	auto a = _create_path( m.value(),e,true ) ;

	SirikaliJson json( utility::jsonLogger() ) ;

	json[ "volumePath" ]           = e.volumePath ;
	json[ "mountPointPath" ]       = e.mountPointPath ;
	json[ "configFilePath" ]       = e.configFilePath ;
	json[ "keyFilePath" ]          = e.keyFile ;
	json[ "idleTimeOut" ]          = e.idleTimeOut ;
	json[ "mountOptions" ]         = e.mountOptions ;
	json[ "preMountCommand" ]      = e.preMountCommand ;
	json[ "postMountCommand" ]     = e.postMountCommand ;
	json[ "preUnmountCommand" ]    = e.preUnmountCommand ;
	json[ "postUnmountCommand" ]   = e.postUnmountCommand ;
	json[ "reverseMode" ]          = e.reverseMode ;
	json[ "volumeNeedNoPassword" ] = e.volumeNeedNoPassword ;
	json[ "password" ]             = e.password ;

	favorites::triState::writeTriState( json,e.readOnlyMode,"mountReadOnly" ) ;
	favorites::triState::writeTriState( json,e.autoMount,"autoMountVolume" ) ;

	if( utility::pathExists( a ) ){

		utility::debug() << "Favorite Entry Already Exist" ;
		return error::ENTRY_ALREADY_EXISTS ;
	}else{
		if( json.passed() && json.toFile( a ) ){

			this->reload() ;
			return error::SUCCESS ;
		}else{
			utility::debug() << "Failed To Create Favorite Entry" ;
			return error::FAILED_TO_CREATE_ENTRY ;
		}
	}
}

const std::vector< favorites::entry >& favorites::readFavorites() const
{
	return m_favorites ;
}

favorites::volumeList favorites::readVolumeList() const
{
	favorites::volumeList e ;

	for( const auto& it : m_favorites ){

		e.emplace_back( it ) ;
	}

	return e ;
}

const favorites::entry& favorites::readFavoriteByPath( const QString& configPath ) const
{
	auto path = QDir( configPath ).absolutePath() ;

	for( const auto& it : m_favorites ){

		if( it.internalConfigPath == path ){

			return it ;
		}
	}

	return m_empty ;
}

favorites::entry favorites::readFavoriteByFileSystemPath( const QString& path ) const
{
	return _favorites( path ) ;
}

const favorites::entry& favorites::unknown() const
{
	return m_empty ;
}

const favorites::entry& favorites::readFavorite( const QString& volumePath,
						 const QString& mountPath ) const
{
	if( mountPath.isEmpty() ){

		for( const auto& it : m_favorites ){

			if( it.volumePath == volumePath ){

				return it ;
			}
		}
	}else{
		for( const auto& it : m_favorites ){

			if( it.volumePath == volumePath && it.mountPointPath == mountPath ){

				return it ;
			}
		}
	}

	return m_empty ;
}

favorites::temporaryFavoriteEntries& favorites::getTmpFavoriteEntries()
{
	m_tmpFe.clear() ;
	return m_tmpFe ;
}

void favorites::replaceFavorite( const favorites::entry& old,const favorites::entry& New )
{
	this->removeFavoriteEntry( old ) ;
	this->add( New ) ;
}

template< typename Function >
static bool _remove( const favorites::entry& e,Function function,bool newFormat )
{
	auto s = function( e,newFormat ) ;

	if( !s.isEmpty() && utility::pathExists( s ) ){

		return QFile::remove( s ) ;
	}else{
		return false ;
	}
}

void favorites::removeFavoriteEntry( const favorites::entry& e )
{
	if( _remove( e,_create_entry_path,false ) ){

		this->reload() ;

	}else if( _remove( e,_create_entry_path,true ) ){

		this->reload() ;
	}else{
		utility::debug() << "Failed To Remove Favorite Entry: " + e.volumePath ;
	}
}

favorites::entry::entry()
{
}

favorites::entry::entry( const QString& e,const QString& mountPath ) :
	volumePath( e ),mountPointPath( mountPath )
{
}

void favorites::temporaryFavoriteEntries::clear()
{
	m_tmpFavoriteList.clear() ;
	m_iter = m_tmpFavoriteList.before_begin() ;
}

const favorites::entry& favorites::temporaryFavoriteEntries::add( favorites::entry e )
{
	m_iter = m_tmpFavoriteList.emplace_after( m_iter,std::move( e ) ) ;
	return *m_iter ;
}
