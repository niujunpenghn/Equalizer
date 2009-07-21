

/* Copyright (c) 2005-2009, Stefan Eilemann <eile@equalizergraphics.com> 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


using namespace eq::base;
#define EQ_PIPE_BUFFER_SIZE 515072
#define EQ_READ_BUFFER_SIZE 257536
#define EQ_WRITE_BUFFER_SIZE 128768

#ifdef WIN32
namespace eq
{ 
namespace net
{

NamedPipeConnection::NamedPipeConnection()
{
    memset( &_overlapped, 0, sizeof( _overlapped ));
    
    _description =  new ConnectionDescription;
    _description->type = CONNECTIONTYPE_NAMEDPIPE;
    _description->bandwidth = 768000;
}

NamedPipeConnection::~NamedPipeConnection()
{
    close();
}

//----------------------------------------------------------------------
// connect
//----------------------------------------------------------------------
bool NamedPipeConnection::connect()
{
    EQASSERT( _description->type == CONNECTIONTYPE_NAMEDPIPE );
     
    if( _state != STATE_CLOSED )
        return false;

    _state = STATE_CONNECTING;
    _fireStateChanged();

    if( !_createNamedPipe( ))
        return false;

    _initAIORead();
    _state = STATE_CONNECTED;
    _fireStateChanged();

    return true;
}

void NamedPipeConnection::close()
{
    if( !(_state == STATE_CONNECTED || _state == STATE_LISTENING ))
        return;

    if( isListening( ))
        _exitAIOAccept();
    else
        _exitAIORead();

    EQASSERT( _readFD > 0 ); 
    if( !DisconnectNamedPipe( _readFD ))
        EQERROR << "Could not close named pipe: " << EQ_PIPE_ERROR
                << std::endl;

    _readFD = INVALID_HANDLE_VALUE;
    _state = STATE_CLOSED;
    _fireStateChanged();
}

bool NamedPipeConnection::_createNamedPipe()
{
    if ( !WaitNamedPipe( _description->getFilename().c_str(), 20000 )) 
    { 
        EQERROR << "Can't create named pipe: " 
                << GetLastError() << std::endl; 
        return false; 

    }    
    _readFD = CreateFile( 
             _description->getFilename().c_str(),   // pipe name 
             GENERIC_READ |         // read and write access 
             GENERIC_WRITE, 
             0,                     // no sharing 
             0,                     // default security attributes
             OPEN_EXISTING,         // opens existing pipe 
             FILE_FLAG_OVERLAPPED,  // default attributes 
             0);                    // no template file 

    if( _readFD != INVALID_HANDLE_VALUE ) 
       return true;

    if( GetLastError() != ERROR_PIPE_BUSY ) 
    {
        EQERROR << "Can't create named pipe: " 
                << EQ_PIPE_ERROR << std::endl; 
        return false;
     
    }

    return _readFD != INVALID_HANDLE_VALUE;
}

//----------------------------------------------------------------------
// listen
//----------------------------------------------------------------------
bool NamedPipeConnection::listen()
{
    EQASSERT( _description->type == CONNECTIONTYPE_NAMEDPIPE );

    if( _state != STATE_CLOSED )
        return false;

    _state = STATE_CONNECTING;
    _fireStateChanged();

    _initAIOAccept();
    _state = STATE_LISTENING;
    _fireStateChanged();
    return true;
}


bool NamedPipeConnection::_connectToNewClient( HANDLE hPipe ) 
{ 
   // Start an overlapped connection for this pipe instance. 
   const bool fConnected = ConnectNamedPipe( hPipe, &_overlapped ); 

   EQASSERT( !fConnected );
 
   switch( GetLastError() ) 
   { 
      // The overlapped connection in progress. 
      case ERROR_IO_PENDING: 
         return true;
 
      // Client is already connected, so signal an event. 
      case ERROR_PIPE_CONNECTED: 
         if( SetEvent( _overlapped.hEvent ) ) 
            return true; 

         // fall through
      default: 
      {
         EQWARN << "ConnectNamedPipe failed : " << EQ_PIPE_ERROR << std::endl;
         return false;
      }
   } 
}

//----------------------------------------------------------------------
// Async IO handle
//----------------------------------------------------------------------
void NamedPipeConnection::_initAIORead()
{
    _overlapped.hEvent = CreateEvent( 0, FALSE, FALSE, 0 );
    EQASSERT( _overlapped.hEvent );

    if( !_overlapped.hEvent )
        EQERROR << "Can't create event for AIO notification: " 
                << EQ_PIPE_ERROR  << std::endl;
}

void NamedPipeConnection::_initAIOAccept()
{
    _initAIORead();
}

void NamedPipeConnection::_exitAIOAccept()
{
    _exitAIORead();
}
void NamedPipeConnection::_exitAIORead()
{
    if(  _overlapped.hEvent ) 
    {
        CloseHandle( _overlapped.hEvent );
        _overlapped.hEvent = 0;
    }
}

//----------------------------------------------------------------------
// accept
//----------------------------------------------------------------------
void NamedPipeConnection::acceptNB()
{
    EQASSERT( _state == STATE_LISTENING );
    ResetEvent( _overlapped.hEvent );


#if 0
    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor = 
        ( PSECURITY_DESCRIPTOR )malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );
    InitializeSecurityDescriptor( sa.lpSecurityDescriptor, 
                                  SECURITY_DESCRIPTOR_REVISION );
    // ACL is set as NULL in order to allow all access to the object.
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, true, 0, false);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
#endif

    // Start accept
    _readFD = CreateNamedPipe( 
             _description->getFilename().c_str(), // pipe name 
                     PIPE_ACCESS_DUPLEX |         // read/write access 
                     FILE_FLAG_OVERLAPPED,        // overlapped mode 
                     PIPE_TYPE_BYTE |             // message-type  
                     PIPE_READMODE_BYTE |         // message-read  
                     PIPE_WAIT,                   // blocking mode 
                     PIPE_UNLIMITED_INSTANCES,    // number of instances 
                     EQ_PIPE_BUFFER_SIZE,         // output buffer size 
                     EQ_PIPE_BUFFER_SIZE,         // input buffer size 
                     0,                           // default time-out (unused)
                     0 /*&sa*/);                  // default security attributes 

    if ( _readFD == INVALID_HANDLE_VALUE ) 
    {
        EQERROR << "Could not create named pipe: " 
                << EQ_PIPE_ERROR << " file : " << _description->getFilename()
                << std::endl;
        close();
        return;
    }

    _connectToNewClient( _readFD );
}

ConnectionPtr NamedPipeConnection::acceptSync()
{
    CHECK_THREAD( _recvThread );
    if( _state != STATE_LISTENING )
        return 0;

    // complete accept
    DWORD got   = 0;
    DWORD flags = 0;
    if( !GetOverlappedResult( _readFD, &_overlapped, &got, TRUE ))
    {
        if (GetLastError() == ERROR_PIPE_CONNECTED) 
        {        
            return 0; 
        }
        EQWARN << "Accept completion failed: " << EQ_PIPE_ERROR
               << ", closing named pipe" << std::endl;
         
        close();
        return 0;
    }


    NamedPipeConnection* newConnection = new NamedPipeConnection;
    ConnectionPtr conn( newConnection );

    newConnection->setDescription( _description );
    newConnection->_readFD  = _readFD;
    newConnection->_state = STATE_CONNECTED;
    newConnection->_initAIORead();

    newConnection->_state  = STATE_CONNECTED;
    _readFD = INVALID_HANDLE_VALUE;

    EQINFO << "accepted connection" << std::endl;
    return conn;
}

//----------------------------------------------------------------------
// read
//----------------------------------------------------------------------
void NamedPipeConnection::readNB( void* buffer, const uint64_t bytes )
{
    if( _state == STATE_CLOSED )
        return;

    ResetEvent( _overlapped.hEvent );
    DWORD use = EQ_MIN( bytes, EQ_READ_BUFFER_SIZE );

    if( !ReadFile( _readFD,         // pipe handle 
                   buffer,          // buffer to receive reply 
                   use,             // size of buffer 
                   0,               // number of bytes read 
                   &_overlapped )    // not overlapped 
                   &&   (  GetLastError() != ERROR_IO_PENDING ) )
    {
        EQWARN << "Could not start overlapped receive: " << EQ_PIPE_ERROR
               << ", closing connection" << std::endl;
        close();
    }
}

int64_t NamedPipeConnection::readSync( void* buffer, const uint64_t bytes )
{
    CHECK_THREAD( _recvThread );

    if( _readFD == INVALID_HANDLE_VALUE )
    {
        EQERROR << "Invalid read handle" << std::endl;
        return -1;
    }

    DWORD got   = 0;
    if( !GetOverlappedResult( _readFD, &_overlapped, &got, true ))
    {
        if( GetLastError() == ERROR_PIPE_CONNECTED ) 
        {        
            return 0; 
        } 

        EQWARN << "Read complete failed: " << EQ_PIPE_ERROR 
               << ", closing connection" << std::endl;
        close();
        return 0;

    }

    return got;
}

int64_t NamedPipeConnection::write( const void* buffer, const uint64_t bytes )
{
    if( _readFD == INVALID_HANDLE_VALUE )
        return -1;

    DWORD wrote;
    DWORD use = EQ_MIN( bytes, EQ_WRITE_BUFFER_SIZE );

    if( WriteFile( _readFD,      // pipe handle 
                   buffer ,      // message 
                   use,          // message length 
                   &wrote,       // bytes written 
                   0 ))
    {
        return wrote;
    }

    EQWARN << "Write error: " << EQ_PIPE_ERROR << std::endl;
 
    return -1;  
}
}
}
#else
#  error "File is only for WIN32 builds"
#endif
