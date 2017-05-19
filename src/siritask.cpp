﻿/*
 *
 *  Copyright (c) 2014-2015
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

#include "plugins.h"
#include "siritask.h"
#include "mountinfo.h"

#include <QDir>
#include <QString>
#include <QDebug>
#include <QFile>

using cs = siritask::status ;

static bool _create_folder( const QString& m )
{
	if( utility::pathExists( m ) ){

		return utility::reUseMountPoint() ;
	}else{
		return utility::createFolder( m ) ;
	}
}

static QString _makePath( const QString& e )
{
	return utility::Task::makePath( e ) ;
}

template< typename ... T >
static bool _deleteFolders( const T& ... m )
{
	bool s = false ;

	QDir e ;

	for( const auto& it : { m ... } ){

		s = e.rmdir( it ) ;
	}

	return s ;
}

static void _run_command_on_mount( const siritask::options& opt,const QString& app )
{
	auto exe = utility::runCommandOnMount() ;

	if( !exe.isEmpty() ){

		auto env = QProcessEnvironment::systemEnvironment() ;
		env.insert( "PATH",utility::executableSearchPaths( env.value( "PATH" ) ) ) ;

		auto a = _makePath( opt.cipherFolder ) ;
		auto b = _makePath( opt.plainFolder ) ;

		exe = QString( "%1 %2 %3 %4" ).arg( exe,a,b,app ) ;

		QProcess e ;

		e.setEnvironment( env.toStringList() ) ;

		e.startDetached( exe ) ;
	}
}

bool siritask::deleteMountFolder( const QString& m )
{
	if( utility::reUseMountPoint() ){

		return false ;
	}else{
		return _deleteFolders( m ) ;
	}
}

static QString _wrap_su( const QString& s )
{
	auto su = utility::executableFullPath( "su" ) ;

	if( su.isEmpty() ){

		return s ;
	}else{
		return QString( "%1 - -c \"%2\"" ).arg( su,QString( s ).replace( "\"","'" ) ) ;
	}
}

Task::future< bool >& siritask::encryptedFolderUnMount( const QString& cipherFolder,
							const QString& mountPoint,
							const QString& fileSystem )
{
	return Task::run< bool >( [ = ](){

		auto cmd = [ & ](){

			if( fileSystem == "ecryptfs" ){

				auto exe = utility::executableFullPath( "ecryptfs-simple" ) ;

				auto s = exe + " -k " + _makePath( cipherFolder ) ;

				if( utility::runningInMixedMode() ){

					return _wrap_su( s ) ;
				}else{
					return s ;
				}
			}else{
				if( utility::platformIsOSX() ){

					return "umount " + _makePath( mountPoint ) ;
				}else{
					return "fusermount -u " + _makePath( mountPoint ) ;
				}
			}
		}() ;

		if( utility::platformIsLinux() ){

			utility::Task::waitForOneSecond() ;
		}

		for( int i = 0 ; i < 5 ; i++ ){

			if( utility::Task::run( cmd,10000,fileSystem == "ecryptfs" ).get().success() ){

				return true ;
			}else{
				utility::Task::waitForOneSecond() ;
			}
		}

		return false ;
	} ) ;
}

static QString _args( const QString& exe,const siritask::options& opt,
		      const QString& configFilePath,
		      bool create )
{
	auto cipherFolder = _makePath( opt.cipherFolder ) ;

	auto mountPoint   = _makePath( opt.plainFolder ) ;

	const auto& type = opt.type ;

	auto mountOptions = [ & ](){

		if( !opt.idleTimeout.isEmpty() ){

			if( type == "cryfs" ){

				return QString( "--unmount-idle %1" ).arg( opt.idleTimeout ) ;

			}else if( type == "encfs" ){

				return QString( "--idle=%1" ).arg( opt.idleTimeout ) ;
			}
		}

		return QString() ;
	}() ;

	auto separator = [ & ](){

		if( type == "cryfs" ){

			return "--" ;

		}else if( type == "encfs" ){

			if( create ){

				return "-S --standard" ;
			}else{
				return "-S" ;
			}
		}else{
			return "" ;
		}
	}() ;

	auto configPath = [ & ](){

		if( type.isOneOf( "cryfs","gocryptfs","securefs","ecryptfs" ) ){

			if( !configFilePath.isEmpty() ){

				return "--config " + _makePath( configFilePath ) ;
			}
		}

		return QString() ;
	}() ;

	if( type.isOneOf( "gocryptfs","securefs" ) ){

		QString mode = [ & ](){

			if( opt.ro ){

				return "-o ro" ;
			}else{
				return "-o rw" ;
			}
		}() ;

		auto e = [ & ](){

			if( type == "gocryptfs" ){

				if( create ){

					auto e = QString( "%1 --init -nosyslog %2 %3" ) ;
					return e.arg( exe,configPath,cipherFolder ) ;
				}else{

					if( utility::platformIsOSX() ){

						mode += " -o fsname=gocryptfs@" + cipherFolder ;
					}

					auto e = QString( "%1 -nosyslog %2 %3 %4 %5" ) ;
					return e.arg( exe,mode,configPath,cipherFolder,mountPoint ) ;
				}
			}else{
				if( create ){

					auto e = QString( "%1 create %2 %3" ) ;
					return e.arg( exe,configPath,cipherFolder ) ;
				}else{
					auto e = QString( "%1 mount -b %2 %3 -o fsname=securefs@%4 -o subtype=securefs %5 %6" ) ;
					return e.arg( exe,configPath,mode,cipherFolder,cipherFolder,mountPoint ) ;
				}
			}
		}() ;

		if( opt.mountOptions.isEmpty() ){

			return e ;
		}else{
			return e + " -o " + opt.mountOptions ;
		}

	}else if( type.startsWith( "ecryptfs" ) ){

		auto _options = []( const std::initializer_list< const char * >& e ){

			QString q = "-o key=passphrase" ;

			for( const auto& it : e ){

				q += it ;
			}

			return q ;
		} ;

		auto mode = [ & ](){

			if( opt.ro ){

				return "--readonly" ;
			}else{
				return "" ;
			}
		}() ;

		auto e = QString( "%1 %2 %3 -a %4 %5 %6" ) ;

		auto s = [ & ]{

			if( create ){

				auto s = _options( { ",ecryptfs_passthrough=n",
						     ",ecryptfs_enable_filename_crypto=y",
						     ",ecryptfs_key_bytes=32",
						     ",ecryptfs_cipher=aes" } ) ;

				return e.arg( exe,s,mode,configPath,cipherFolder,mountPoint ) ;
			}else{
				return e.arg( exe,_options( {} ),mode,configPath,cipherFolder,mountPoint ) ;
			}
		}() ;

		if( opt.mountOptions.isEmpty() ){

			if( utility::runningInMixedMode() ){

				return _wrap_su( s ) ;
			}else{
				return s ;
			}
		}else{
			if( utility::runningInMixedMode() ){

				return _wrap_su( s + " -o " + opt.mountOptions ) ;
			}else{
				return s + " -o " + opt.mountOptions;
			}
		}

	}else{
		auto e = QString( "%1 %2 %3 %4 %5 %6 -o fsname=%7@%8 -o subtype=%9" ) ;

		auto opts = e.arg( exe,cipherFolder,mountPoint,mountOptions,configPath,
				   separator,type.name(),cipherFolder,type.name() ) ;

		if( opt.mountOptions.isEmpty() ){

			if( opt.ro ){

				return opts + " -o ro" ;
			}else{
				return opts + " -o rw" ;
			}
		}else{
			if( opt.ro ){

				return opts + " -o ro -o " + opt.mountOptions ;
			}else{
				return opts + " -o rw -o " + opt.mountOptions ;
			}
		}
	}
}

static siritask::status _status( const siritask::volumeType& app,bool s )
{
	if( s ){

		if( app == "cryfs" ){

			return cs::cryfsNotFound ;

		}else if( app == "encfs" ){

			return cs::encfsNotFound ;

		}else if( app == "securefs" ){

			return cs::securefsNotFound ;

		}else if( app.startsWith( "ecryptfs" ) ){

			return cs::ecryptfs_simpleNotFound ;
		}else{
			return cs::gocryptfsNotFound ;
		}
	}else{
		if( app == "cryfs" ){

			return cs::cryfs ;

		}else if( app == "encfs" ){

			return cs::encfs ;

		}else if( app == "securefs" ){

			return cs::securefs ;

		}else if( app.startsWith( "ecryptfs" ) ){

			return cs::ecryptfs ;
		}else{
			return cs::gocryptfs ;
		}
	}
}

static siritask::cmdStatus _status( const utility::Task& r,siritask::status s,bool stdOut )
{
	if( r.success() ){

		return siritask::status::success ;
	}

	siritask::cmdStatus e = { r.exitCode(),stdOut ? r.stdOut() : r.stdError() } ;

	auto msg = e.msg().toLower() ;

	/*
	 *
	 * When trying to figure out what error occured,check for status value
	 * if the backend supports them and fallback to parsing output strings
	 * if backend does not support error codes.
	 *
	 */

	if( s == siritask::status::ecryptfs ){

		if( msg.contains( "error: mount failed" ) ){

			e.setStatus( s ) ;
		}

	}else if( s == siritask::status::cryfs ){

		if( msg.contains( "password" ) ){

			e.setStatus( s ) ;
		}

	}else if( s == siritask::status::encfs ){

		if( msg.contains( "password" ) ){

			e.setStatus( s ) ;
		}

	}else if( s == siritask::status::gocryptfs ){

		/*
		 * This error code was added in gocryptfs 1.2.1
		 */
		if( e.exitCode() == 12 ){

			e.setStatus( s ) ;
		}else{
			if( msg.contains( "password" ) ){

				e.setStatus( s ) ;
			}
		}

	}else if( s == siritask::status::securefs ){

		if( msg.contains( "password" ) ){

			e.setStatus( s ) ;
		}
	}

	return e ;
}

static siritask::cmdStatus _cmd( bool create,const siritask::options& opt,
		const QString& password,const QString& configFilePath )
{
	const auto& app = opt.type ;

	auto exe = app.executableFullPath() ;

	if( exe.isEmpty() ){

		return _status( app,true ) ;
	}else{
		auto e = utility::Task( _args( exe,opt,configFilePath,create ),20000,[](){

			auto env = QProcessEnvironment::systemEnvironment() ;

			env.insert( "CRYFS_NO_UPDATE_CHECK","TRUE" ) ;
			env.insert( "CRYFS_FRONTEND","noninteractive" ) ;

			env.insert( "LANG","C" ) ;

			env.insert( "PATH",utility::executableSearchPaths( env.value( "PATH" ) ) ) ;

			return env ;

		}(),password.toLatin1(),[](){},configFilePath.endsWith( "ecryptfs.config" ) ) ;

		auto s = _status( e,_status( app,false ),app == "encfs" ) ;

		if( s != siritask::status::success ){

			utility::debug() << s.report() ;
		}

		return s ;
	}
}

static QString _configFilePath( const siritask::options& opt )
{
	if( opt.configFilePath.startsWith( "/" ) || opt.configFilePath.isEmpty() ){

		return opt.configFilePath ;
	}else{
		return utility::homePath() + "/" + opt.configFilePath ;
	}
}

Task::future< siritask::cmdStatus >& siritask::encryptedFolderMount( const options& opt,bool reUseMountPoint )
{
	return Task::run< siritask::cmdStatus >( [ opt,reUseMountPoint ]()->siritask::cmdStatus{

		auto _mount = [ reUseMountPoint ]( const QString& app,const options& copt,
				const QString& configFilePath )->siritask::cmdStatus{

			auto opt = copt ;

			opt.type = app ;

			if( _create_folder( opt.plainFolder ) || reUseMountPoint ){

				auto e = _cmd( false,opt,opt.key,configFilePath ) ;

				if( e == cs::success ){

					_run_command_on_mount( opt,app ) ;
				}else{
					siritask::deleteMountFolder( opt.plainFolder ) ;
				}

				return e ;
			}else{
				return cs::failedToCreateMountPoint ;
			}
		} ;

                if( opt.configFilePath.isEmpty() ){

                        if( utility::pathExists( opt.cipherFolder + "/cryfs.config" ) ){

				return _mount( "cryfs",opt,QString() ) ;

			}else if( utility::pathExists( opt.cipherFolder + "/gocryptfs.conf" ) ){

				return _mount( "gocryptfs",opt,QString() ) ;

			}else if( utility::pathExists( opt.cipherFolder + "/.securefs.json" ) ){

				return _mount( "securefs",opt,QString() ) ;

			}else if( utility::pathExists( opt.cipherFolder + "/.ecryptfs.config" ) ){

				return _mount( "ecryptfs",opt,opt.cipherFolder + "/.ecryptfs.config" ) ;
			}else{
                                auto encfs6 = opt.cipherFolder + "/.encfs6.xml" ;
                                auto encfs5 = opt.cipherFolder + "/.encfs5" ;
                                auto encfs4 = opt.cipherFolder + "/.encfs4" ;

                                if( utility::atLeastOnePathExists( encfs6,encfs5,encfs4 ) ){

					return _mount( "encfs",opt,QString() ) ;
                                }else{
					return cs::unknown ;
                                }
                        }
                }else{
			auto e = _configFilePath( opt ) ;

			if( utility::pathExists( e ) ){

				if( e.endsWith( "gocryptfs.conf" ) ){

					return _mount( "gocryptfs",opt,e ) ;

				}else if( e.endsWith( "securefs.json" ) ){

					return _mount( "securefs",opt,e ) ;

				}else if( e.endsWith( "ecryptfs.config" ) ){

					return _mount( "ecryptfs",opt,e ) ;

				}else if( e.endsWith( "cryfs.config" ) ){

					return _mount( "cryfs",opt,e ) ;
				}else{
					return cs::unknown ;
				}
			}else{
				return cs::unknown ;
			}
                }
	} ) ;
}

Task::future< siritask::cmdStatus >& siritask::encryptedFolderCreate( const options& opt )
{
	return Task::run< siritask::cmdStatus >( [ opt ]()->siritask::cmdStatus{

		if( _create_folder( opt.cipherFolder ) ){

			if( _create_folder( opt.plainFolder ) ){

				auto e = _cmd( true,opt,[ & ]()->QString{

					if( opt.type.isOneOf( "securefs","encfs" ) ){

						return opt.key + "\n" + opt.key ;
					}else{
						return opt.key ;
					}

				}(),[ & ](){

					auto e = _configFilePath( opt ) ;

					if( e.isEmpty() && opt.type == "ecryptfs" ){

						return opt.cipherFolder + "/.ecryptfs.config" ;
					}else{
						return e ;
					}
				}() ) ;

				if( e == cs::success ){

					if( opt.type.isOneOf( "gocryptfs","securefs" ) ){

						e = siritask::encryptedFolderMount( opt,true ).get() ;

						if( e != cs::success ){

							_deleteFolders( opt.cipherFolder,opt.plainFolder ) ;
						}
					}
				}else{
					_deleteFolders( opt.plainFolder,opt.cipherFolder ) ;
				}

				return e ;
			}else{
				_deleteFolders( opt.cipherFolder ) ;

				return cs::failedToCreateMountPoint ;
			}
		}else{
			return cs::failedToCreateMountPoint ;
		}
	} ) ;
}

Task::future< QVector< volumeInfo > >& siritask::updateVolumeList()
{
	return Task::run< QVector< volumeInfo > >( [](){

		auto _hash = []( const QString& e ){

			/*
			 * jenkins one at a time hash function.
			 *
			 * https://en.wikipedia.org/wiki/Jenkins_hash_function
			 */

			uint32_t hash = 0 ;

			auto p = e.toLatin1() ;

			auto key = p.constData() ;

			auto l = p.size() ;

			for( decltype( l ) i = 0 ; i < l ; i++ ){

				hash += *( key + i ) ;

				hash += ( hash << 10 ) ;

				hash ^= ( hash >> 6 ) ;
			}

			hash += ( hash << 3 ) ;

			hash ^= ( hash >> 11 ) ;

			hash += ( hash << 15 ) ;

			return QString::number( hash ) ;
		} ;

		auto _decode = []( QString path,bool set_offset ){

			path.replace( "\\012","\n" ) ;
			path.replace( "\\040"," " ) ;
			path.replace( "\\134","\\" ) ;
			path.replace( "\\011","\\t" ) ;

			if( set_offset ){

				return path.mid( path.indexOf( '@' ) + 1 ) ;
			}else{
				return path ;
			}
		} ;

		auto _fs = []( QString e ){

			e.replace( "fuse.","" ) ;

			return e ;
		} ;

		auto _ro = []( const QStringList& l ){

			return l.at( 5 ).mid( 0,2 ) ;
		} ;

		QVector< volumeInfo > e ;

		volumeInfo::mountinfo info ;

		for( const auto& it : mountinfo().mountedVolumes() ){

			if( volumeInfo::supported( it ) ){

				const auto& k = utility::split( it,' ' ) ;

				const auto s = k.size() ;

				if( s < 6 ){

					continue ;
				}

				const auto& cf = k.at( s - 2 ) ;

				const auto& m = k.at( 4 ) ;

				const auto& fs = k.at( s - 3 ) ;

				if( utility::startsWithAtLeastOne( cf,"encfs@",
								   "cryfs@",
								   "securefs@",
								   "gocryptfs@" ) ){

					info.volumePath = _decode( cf,true ) ;

				}else if( utility::equalsAtleastOne( fs,"fuse.gocryptfs",
								     "ecryptfs" ) ){

					info.volumePath = _decode( cf,false ) ;
				}else{
					info.volumePath = _hash( m ) ;
				}

				info.mountPoint   = _decode( m,false ) ;
				info.fileSystem   = _fs( fs ) ;
				info.mode         = _ro( k ) ;
				info.mountOptions = k.last() ;

				e.append( info ) ;
			}
		}

		return e ;
	} ) ;
}
