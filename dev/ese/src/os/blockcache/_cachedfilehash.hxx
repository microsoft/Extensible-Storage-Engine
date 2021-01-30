// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


class CCachedFileKey
{
    public:

        CCachedFileKey()
            :   m_volumeid( volumeidInvalid ),
                m_fileid( fileidInvalid ),
                m_fileserial( fileserialInvalid )
        {
        }

        CCachedFileKey( _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial )
            :   m_volumeid( volumeid ),
                m_fileid( fileid ),
                m_fileserial( fileserial )
        {
        }

        CCachedFileKey( _In_ CCachedFileTableEntryBase* const pcfte )
            :   CCachedFileKey( pcfte->Volumeid(), pcfte->Fileid(), pcfte->Fileserial() )
        {
        }

        CCachedFileKey( _In_ const CCachedFileKey& src )
        {
            *this = src;
        }

        const CCachedFileKey& operator=( _In_ const CCachedFileKey& src )
        {
            m_volumeid = src.m_volumeid;
            m_fileid = src.m_fileid;
            m_fileserial = src.m_fileserial;

            return *this;
        }

        VolumeId Volumeid() const { return m_volumeid; }
        FileId Fileid() const { return m_fileid; }
        FileSerial Fileserial() const { return m_fileserial; }
        UINT UiHash() const { return CCachedFileTableEntryBase::UiHash( m_volumeid, m_fileid, m_fileserial ); }

    private:

        VolumeId    m_volumeid;
        FileId      m_fileid;
        FileSerial  m_fileserial;
};


class CCachedFileEntry
{
    public:

        CCachedFileEntry()
            :   m_pcfte( NULL ),
                m_uiHash( 0 )
        {
        }

        CCachedFileEntry( _In_ CCachedFileTableEntryBase* const pcfte )
            :   m_pcfte( pcfte ),
                m_uiHash( pcfte->UiHash() )
        {
        }

        CCachedFileEntry( _In_ const CCachedFileEntry& src )
        {
            *this = src;
        }

        const CCachedFileEntry& operator=( _In_ const CCachedFileEntry& src )
        {
            m_pcfte = src.m_pcfte;
            m_uiHash = src.m_uiHash;

            return *this;
        }

        CCachedFileTableEntryBase* Pcfte() const { return m_pcfte; }
        UINT UiHash() const { return m_uiHash; }

    private:

        CCachedFileTableEntryBase*  m_pcfte;
        UINT                        m_uiHash;
};


typedef CDynamicHashTable<CCachedFileKey, CCachedFileEntry> CCachedFileHash;

INLINE typename CCachedFileHash::NativeCounter CCachedFileHash::CKeyEntry::Hash( _In_ const CCachedFileKey& key )
{
    return CCachedFileHash::NativeCounter( key.UiHash() );
}

INLINE typename CCachedFileHash::NativeCounter CCachedFileHash::CKeyEntry::Hash() const
{
    return CCachedFileHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CCachedFileHash::CKeyEntry::FEntryMatchesKey( _In_ const CCachedFileKey& key ) const
{
    if ( m_entry.UiHash() != key.UiHash() )
    {
        return fFalse;
    }

    if ( m_entry.Pcfte()->Volumeid() != key.Volumeid() )
    {
        return fFalse;
    }

    if ( m_entry.Pcfte()->Fileid() != key.Fileid() )
    {
        return fFalse;
    }

    if ( m_entry.Pcfte()->Fileserial() != key.Fileserial() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CCachedFileHash::CKeyEntry::SetEntry( _In_ const CCachedFileEntry& entry )
{
    m_entry = entry;
}

INLINE void CCachedFileHash::CKeyEntry::GetEntry( _In_ CCachedFileEntry* const pentry ) const
{
    *pentry = m_entry;
}
