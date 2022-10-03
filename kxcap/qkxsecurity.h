/*******************************************************************************************
*
* Copyright (C) 2022 Guangzhou AoYiDuo Network Technology Co.,Ltd. All Rights Reserved.
*
* Contact: http://www.aoyiduo.com
*
*   this file is used under the terms of the GPLv3[GNU GENERAL PUBLIC LICENSE v3]
* more information follow the website: https://www.gnu.org/licenses/gpl-3.0.en.html
*
*******************************************************************************************/

#ifndef QKXSECURITY_H__
#define QKXSECURITY_H__


#include <aclapi.h>

struct Trustee : public TRUSTEE {
  Trustee(const TCHAR* name,
          TRUSTEE_FORM form=TRUSTEE_IS_NAME,
          TRUSTEE_TYPE type=TRUSTEE_IS_UNKNOWN);
};

struct ExplicitAccess : public EXPLICIT_ACCESS {
  ExplicitAccess(const TCHAR* name,
                 TRUSTEE_FORM type,
                 DWORD perms,
                 ACCESS_MODE mode,
                 DWORD inherit=0);
};

// Helper class for building access control lists
struct AccessEntries {
  AccessEntries();
  ~AccessEntries();
  void allocMinEntries(int count);
  void addEntry(const TCHAR* trusteeName,
                DWORD permissions,
                ACCESS_MODE mode);
  void addEntry(const PSID sid,
                DWORD permissions,
                ACCESS_MODE mode);

  EXPLICIT_ACCESS* entries;
  int entry_count;
};

// Helper class for handling SIDs
struct Sid {
  Sid() {}
  operator PSID() const {return (PSID)buf;}
  PSID takePSID() {PSID r = (PSID)buf; buf = 0; return r;}

  static PSID copySID(const PSID sid);

  void setSID(const PSID sid);

  void getUserNameAndDomain(TCHAR** name, TCHAR** domain);

  struct Administrators;
  struct SYSTEM;
  struct FromToken;

private:
  Sid(const Sid&);
  Sid& operator=(const Sid&);
};

struct Sid::Administrators : public Sid {
  Administrators();
};
struct Sid::SYSTEM : public Sid {
  SYSTEM();
};
struct Sid::FromToken : public Sid {
  FromToken(HANDLE h);
};

// Helper class for handling & freeing ACLs
struct AccessControlList : public LocalMem {
  AccessControlList(int size) : LocalMem(size) {}
  AccessControlList(PACL acl_=0) : LocalMem(acl_) {}
  operator PACL() {return (PACL)ptr;}
};

// Create a new ACL based on supplied entries and, if supplied, existing ACL
PACL CreateACL(const AccessEntries& ae, PACL existing_acl=0);

// Helper class for memory-management of self-relative SecurityDescriptors
struct SecurityDescriptorPtr : LocalMem {
  SecurityDescriptorPtr(int size) : LocalMem(size) {}
  SecurityDescriptorPtr(PSECURITY_DESCRIPTOR sd_=0) : LocalMem(sd_) {}
  PSECURITY_DESCRIPTOR takeSD() {return (PSECURITY_DESCRIPTOR)takePtr();}
};

// Create a new self-relative Security Descriptor, owned by SYSTEM/Administrators,
//   with the supplied DACL and no SACL.  The returned value can be assigned
//   to a SecurityDescriptorPtr to be managed.
PSECURITY_DESCRIPTOR CreateSdWithDacl(const PACL dacl);

#endif
