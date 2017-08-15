/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#ifndef C4RELOC_H
#define C4RELOC_H

#include <stack>
#include <limits>

class C4Reloc
{
public:
	enum PathType
	{
		PATH_Regular,
		PATH_PreferredInstallationLocation,
		PATH_IncludingSubdirectories,
	};
	struct PathInfo
	{
		StdCopyStrBuf strBuf;
		PathType pathType;
		PathInfo(const StdCopyStrBuf buf, PathType pathType): strBuf(buf), pathType(pathType) {}
		bool operator==(const PathInfo&other) {return pathType==other.pathType && strBuf==other.strBuf;}
		operator const char*() {return strBuf.getData();}
	};
	typedef std::vector<PathInfo> PathList;

	// Provide an own iterator that is able to resolve subdirectories on-the-fly.
	class const_iterator
	{
	public:
		const_iterator() = default;
		const_iterator(const const_iterator&) = default;
		const_iterator(const const_iterator&& other) :
			temporaryPathInfo(std::move(other.temporaryPathInfo)),
			pathListIter(std::move(other.pathListIter)),
			subdirIters(std::move(other.subdirIters))
		{}
		~const_iterator() = default;
		const_iterator& operator++(); //prefix increment
		const PathInfo & operator*() const;

		const_iterator &operator=(const const_iterator &other)
		{
			this->pathListIter = other.pathListIter;
			this->subdirIters = other.subdirIters;
			return *this;
		}
		friend bool operator==(const const_iterator&, const const_iterator&);
		friend bool operator!=(const const_iterator&, const const_iterator&);

	protected:
		mutable std::unique_ptr<PathInfo> temporaryPathInfo;
		PathList::const_iterator pathListIter;
		std::stack<DirectoryIterator> subdirIters;

		friend class C4Reloc;
	};
	typedef const_iterator iterator;

	// Can also be used for re-init, drops custom paths added with AddPath.
	// Make sure to call after Config.Load.
	void Init();

	bool AddPath(const char* path, PathType pathType = PATH_Regular);

	iterator begin() const;
	iterator end() const;

	bool Open(C4Group& group, const char* filename) const;
	bool LocateItem(const char* filename, StdStrBuf& str) const;
private:
	PathList Paths;
};

extern C4Reloc Reloc;

#endif // C4RELOC_H
