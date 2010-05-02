// DllObjectFactory.cpp
//
// Copyright (c) 1995-1998 - Richard Langlois and Grokksoft Inc.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.
//

#include "stdafx.h"

#include <map>

#include "../ObjFac1/ObjFac1.h"
#include "../Parcel/ObjStream.h"

#include "DllObjectFactory.h"

#define new DEBUG_NEW

using HoverRace::ObjFac1::ObjFac1;
using HoverRace::Parcel::ObjStream;

// Base class for object factory "DLLs".
// (Originally, these were real DLLs which were loaded at runtime).
class MR_FactoryDll
{
	public:
		BOOL mDynamic;
		int mRefCount;

		virtual MR_ObjectFromFactory* GetObject(int classId) const = 0;

		// Initialisation
		MR_FactoryDll();
		virtual ~MR_FactoryDll();

};

// Using ObjFac1 (read from package file).
class PackageFactoryDll : public MR_FactoryDll
{
	typedef MR_FactoryDll SUPER;

	public:
		PackageFactoryDll();
		virtual ~PackageFactoryDll();

		virtual MR_ObjectFromFactory* GetObject(int classId) const;

	private:
		ObjFac1 *package;
};

// Using function pointer callback.
class LocalFactoryDll : public MR_FactoryDll
{
	typedef MR_FactoryDll SUPER;

	private:
		LocalFactoryDll() {}
	public:
		LocalFactoryDll(MR_DllObjectFactory::getObject_t getObject);
		virtual ~LocalFactoryDll();

		virtual MR_ObjectFromFactory* GetObject(int classId) const;

	private:
		MR_DllObjectFactory::getObject_t getObject;
};

// Local functions declarations
static MR_FactoryDll *GetDll(MR_UInt16 pDllId, BOOL pTrowOnError);

// Module variables
//TODO: Use a std::map instead.
typedef std::map<int,MR_FactoryDll*> gsDllList_t;
static gsDllList_t gsDllList;

// Module functions
void MR_DllObjectFactory::Init()
{
	// Nothing to do
}

void MR_DllObjectFactory::Clean(BOOL pOnlyDynamic)
{
	for (gsDllList_t::iterator iter = gsDllList.begin(); iter != gsDllList.end(); ) {
		int dllId = iter->first;
		MR_FactoryDll* dllPtr = iter->second;

		if ((dllPtr->mRefCount <= 0) && (dllPtr->mDynamic || !pOnlyDynamic)) {
			gsDllList.erase(iter++);
			delete dllPtr;
		}
		else {
			++iter;
		}
	}
}

/*
BOOL MR_DllObjectFactory::OpenDll(MR_UInt16 pDllId)
{
	return (GetDll(pDllId, FALSE) != NULL);
}
*/

void MR_DllObjectFactory::RegisterLocalDll(int pDllId, getObject_t pFunc)
{

	MR_FactoryDll *lDllPtr;

	ASSERT(pDllId != 0);						  // Number 0 is reserved for NULL entry
	ASSERT(pDllId != 1);						  // Number 1 is reserved for ObjFac1
	ASSERT(pFunc != NULL);

	lDllPtr = new LocalFactoryDll(pFunc);

	bool inserted = gsDllList.insert(gsDllList_t::value_type(pDllId, lDllPtr)).second;
	ASSERT(inserted);
}

void MR_DllObjectFactory::IncrementReferenceCount(int pDllId)
{
	gsDllList_t::iterator iter = gsDllList.find(pDllId);
	if (iter != gsDllList.end()) {
		iter->second->mRefCount++;
	}
	else {
		ASSERT(FALSE);							  // Dll not loaded
	}
}

void MR_DllObjectFactory::DecrementReferenceCount(int pDllId)
{
	gsDllList_t::iterator iter = gsDllList.find(pDllId);
	if (iter != gsDllList.end()) {
		iter->second->mRefCount--;
	}
	else {
		ASSERT(FALSE);							  // Dll discarded
	}
}

MR_ObjectFromFactory *MR_DllObjectFactory::CreateObject(const MR_ObjectFromFactoryId &pId)
{
	MR_ObjectFromFactory *lReturnValue;

	MR_FactoryDll *lDllPtr = GetDll(pId.mDllId, TRUE);

	lReturnValue = lDllPtr->GetObject(pId.mClassId);

	return lReturnValue;
}

/**
 * Get a handle to the factory DLL.  The option of choosing which DLL has been deprecated.
 */
MR_FactoryDll *GetDll(MR_UInt16 pDllId, BOOL pThrowOnError)
{
	MR_FactoryDll *lDllPtr;

	ASSERT(pDllId != 0);						  // Number 0 is reserved for NULL entry

	// Create the factory DLL if it doesn't exist already.
	gsDllList_t::iterator iter = gsDllList.find(pDllId);
	if (iter == gsDllList.end()) {
		ASSERT(pDllId == 1);  // Number 1 is the package (ObjFac1) by convention from original code.
		lDllPtr = new PackageFactoryDll();
		gsDllList.insert(gsDllList_t::value_type(pDllId, lDllPtr));
	}
	else {
		return iter->second;
	}

	return lDllPtr;
}

// MR_ObjectFromFactory methods
MR_ObjectFromFactory::MR_ObjectFromFactory(const MR_ObjectFromFactoryId & pId)
{
	mId = pId;

	// Increment this object dll reference count
	// This will prevent the Dll from being discarted
	MR_DllObjectFactory::IncrementReferenceCount(mId.mDllId);

}

MR_ObjectFromFactory::~MR_ObjectFromFactory()
{

	// Decrement this object dll reference count
	// This will allow the dll to be freed if not needed anymore
	MR_DllObjectFactory::DecrementReferenceCount(mId.mDllId);

}

const MR_ObjectFromFactoryId &MR_ObjectFromFactory::GetTypeId() const
{
	return mId;
}

void MR_ObjectFromFactory::SerializePtr(ObjStream & pArchive, MR_ObjectFromFactory * &pPtr)
{
	MR_ObjectFromFactoryId lId = { 0, 0 };

	if(pArchive.IsWriting()) {
		if(pPtr != NULL) {
			lId = pPtr->mId;
		}

		lId.Serialize(pArchive);

		if(pPtr != NULL) {
			pPtr->Serialize(pArchive);
		}
	}
	else {
		lId.Serialize(pArchive);

		if(lId.mDllId == 0) {
			pPtr = NULL;
		}
		else {
			pPtr = MR_DllObjectFactory::CreateObject(lId);
			pPtr->Serialize(pArchive);
		}
	}
}

void MR_ObjectFromFactory::Serialize(ObjStream &pArchive)
{
	//CObject::Serialize(pArchive);

	// Notting to serialize at that point
	// Object type should be already initialize if Loading

}

// MR_ObjectFromFactoryId
void MR_ObjectFromFactoryId::Serialize(ObjStream &pArchive)
{
	if(pArchive.IsWriting()) {
		pArchive << mDllId << mClassId;

	}
	else {
		pArchive >> mDllId >> mClassId;
	}
}

int MR_ObjectFromFactoryId::operator ==(const MR_ObjectFromFactoryId & pId) const
{
	return ((mDllId == pId.mDllId) && (mClassId == pId.mClassId));
}


// class MR_FactoryDll methods 
MR_FactoryDll::MR_FactoryDll() :
	mDynamic(FALSE), mRefCount(0)
{
}

MR_FactoryDll::~MR_FactoryDll()
{
	ASSERT(mRefCount == 0);
}

PackageFactoryDll::PackageFactoryDll() :
	SUPER()
{
	mDynamic = TRUE;
	package = new ObjFac1();
}

PackageFactoryDll::~PackageFactoryDll()
{
	delete package;
}

MR_ObjectFromFactory* PackageFactoryDll::GetObject(int classId) const
{
	return package->GetObject(classId);
}

LocalFactoryDll::LocalFactoryDll(MR_DllObjectFactory::getObject_t getObject) :
	SUPER(), getObject(getObject)
{
}

LocalFactoryDll::~LocalFactoryDll()
{
}

MR_ObjectFromFactory* LocalFactoryDll::GetObject(int classId) const
{
	return getObject(classId);
}

