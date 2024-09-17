// Copyright (c) 2020-2023, Manticore Software LTD (https://manticoresearch.com)
// All rights reserved
//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cassert>

#ifdef _MSC_VER
inline int PreadWrapper ( int iFD, void * pBuf, size_t tCount, int64_t iOff )
{
	if ( !tCount )
		return 0;

	HANDLE hFile;
	hFile = (HANDLE)_get_osfhandle(iFD);
	if ( hFile==INVALID_HANDLE_VALUE )
		return -1;

	OVERLAPPED tOverlapped;
	memset ( &tOverlapped, 0, sizeof(OVERLAPPED) );
	tOverlapped.Offset = (DWORD)( iOff & 0xFFFFFFFFULL );
	tOverlapped.OffsetHigh = (DWORD)( iOff>>32 );

	DWORD uNumBytesRead;
	if ( !ReadFile ( hFile, pBuf, (DWORD)tCount, &uNumBytesRead, &tOverlapped ) )
		return GetLastError()==ERROR_HANDLE_EOF ? 0 : -1;

	return uNumBytesRead;
}

#else
#if HAVE_PREAD
inline int PreadWrapper ( int iFD, void * pBuf, size_t tCount, off_t tOff )
{
	return ::pread ( iFD, pBuf, tCount, tOff );
}
#else
inline int PreadWrapper ( int iFD, void * pBuf, size_t tCount, off_t tOff )
{
	if ( lseek ( iFD, tOff, SEEK_SET )==(off_t)-1 )
		return -1;

	return read ( iFD, pBuf, tCount );
}
#endif
#endif	// _MSC_VER


class FileReader_c
{
public:
			FileReader_c() = default;
			~FileReader_c() { Close(); }

	bool Open ( const std::string & sName )
	{
	#ifdef _MSC_VER
		HANDLE tHandle = CreateFile ( sName.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		m_iFD = _open_osfhandle ( (intptr_t)tHandle, 0 );
	#else
		m_iFD = ::open ( sName.c_str(), O_RDONLY | O_BINARY, 0644 );
	#endif

		if ( m_iFD<0 )
			return false;

		m_sFile = sName;
		m_bOpened = true;
		return true;
	}

	void Close()
	{
		if ( !m_bOpened || m_iFD<0 )
			return;

		::close(m_iFD);
		m_iFD = -1;
	}

	int GetLine ( char * szBuffer, int iMaxLen )
	{
		int iOut = 0;
		iMaxLen--;

		while ( iOut<iMaxLen )
		{
			if ( m_tPtr>=m_tUsed )
			{
				ReadToBuffer();
				if ( m_tPtr>=m_tUsed )
				{
					if ( iOut==0 )
						return -1; // eof

					break;
				}
			}

			if ( m_pData[m_tPtr]=='\r' || m_pData[m_tPtr]=='\n' )
				break;

			szBuffer[iOut++] = m_pData[m_tPtr++];
		}

		while (true)
		{
			if ( m_tPtr>=m_tUsed )
				ReadToBuffer();

			if ( m_tPtr>=m_tUsed )
				break;

			if ( m_pData[m_tPtr++]=='\n' )
				break;
		}

		szBuffer[iOut] = '\0';
		return iOut;
	}

	bool IsError() const { return m_bError; }

private:
	static const size_t DEFAULT_SIZE = 262144;

	int         m_iFD = -1;
	bool        m_bOpened = false;
	std::string m_sFile;

	std::unique_ptr<uint8_t[]> m_pData;
	size_t      m_tSize = DEFAULT_SIZE;
	size_t      m_tUsed = 0;
	size_t      m_tPtr = 0;

	int64_t     m_iFilePos = 0;

	bool        m_bError = false;

	bool ReadToBuffer()
	{
		assert ( m_iFD>=0 );

		CreateBuffer();

		int64_t iNewFilePos = m_iFilePos + std::min ( m_tPtr, m_tUsed );
		int iRead = PreadWrapper ( m_iFD, m_pData.get(), m_tSize, iNewFilePos );
		if ( iRead<0 )
		{
			m_tPtr = m_tUsed = 0;
			m_bError = true;
			return false;
		}

		m_tUsed = iRead;
		m_tPtr = 0;
		m_iFilePos = iNewFilePos;

		return true;
	}

	void CreateBuffer()
	{
		if ( m_pData )
			return;

		m_pData = std::unique_ptr<uint8_t[]> ( new uint8_t[m_tSize] );
	}
};

bool SplitText ( char * szText, int iLen, char ** dTokens, int iRequiredTokens )
  {
    int iTokens = 0;
    const char * szEnd = szText + iLen;
    while ( szText < szEnd )
    {
        while ( *szText==' ' )
            szText++;

        if ( !*szText )
            break;

        char * szStart = szText;
        while ( *szText && *szText!=' ' )
            szText++;

        *szText = '\0';

        if ( iTokens<iRequiredTokens )
            dTokens[iTokens] = szStart;

        szText++;
        iTokens++;
    }

    return iTokens==iRequiredTokens;
  }
