
/* StdAfx.h
	Precompiled header for client. */

#ifdef _WIN32

#	define VC_EXTRALEAN							  // Exclude rarely-used stuff from Windows headers

	// Minimum Windows version: XP
#	define WINVER 0x0501

#	define _CRT_SECURE_NO_DEPRECATE
#	define _SCL_SECURE_NO_DEPRECATE

#	include <windows.h>
#	include <typeinfo.h>
#	include <commctrl.h>

#	pragma warning(disable: 4251)
#	pragma warning(disable: 4275)

#	include <assert.h>
#	define ASSERT(e) assert(e)
#	define TRACE __noop

#	include "../../include/config-win32.h"

#else

#	include "../../include/compat/unix.h"
#	include "../../config.h"

#	include <string.h>
#	include <strings.h>

#endif

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#ifndef _WIN32
	// Xlib.h must be included *after* boost/foreach.hpp as a workaround for
	// https://svn.boost.org/trac/boost/ticket/3000
#	include <X11/Xlib.h>
#endif

#define PACKAGE_NAME_WIDEN(x) L ## x
#define PACKAGE_NAME_APPLY(x) PACKAGE_NAME_WIDEN(x)
#define PACKAGE_NAME_L PACKAGE_NAME_APPLY(PACKAGE_NAME)

#ifdef _
#	undef _
#endif
#ifdef ENABLE_NLS

#	include <libintl.h>
#	define _(x) gettext(x)

	// Our own little version of pgettext() so we don't need all of gettext.h.
#	define pgettext(p,x) pgettextImpl(p "\004" x, x)
	static inline const char *pgettextImpl(const char *full, const char *msg)
	{
		const char *retv = _(full);
		return (retv == full) ? msg : retv;
	}

#else

#	define _(x) (x)
#	define gettext(x) (x)
#	define dgettext(d,x) (x)
#	define dcgettext(d,x,c) (x)
#	define pgettext(p,x) (x)
#	define pgettextImpl(f,x) (x)

#endif  // ENABLE_NLS

#define HR_WEBSITE "http://www.hoverrace.com/"
