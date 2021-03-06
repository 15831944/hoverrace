
// Str.h
// Header for string support functions.
//
// Copyright (c) 2009 Michael Imamura.
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

#pragma once

#include "OS.h"

#ifdef _WIN32
#	ifdef MR_ENGINE
#		define MR_DllDeclare   __declspec( dllexport )
#	else
#		define MR_DllDeclare   __declspec( dllimport )
#	endif
#else
#	define MR_DllDeclare
#endif

namespace HoverRace {
namespace Util {

namespace Str {

	MR_DllDeclare wchar_t *Utf8ToWide(const char *s);
	MR_DllDeclare char *WideToUtf8(const wchar_t *ws);

	/** Utility class for easy conversion of UTF-8 to wide strings. */
	class MR_DllDeclare UW
	{
		wchar_t *cs;
		public:
			UW(const char *s=NULL) throw() : cs(Utf8ToWide(s)) { }
			explicit UW(const std::string &s) throw() : cs(Utf8ToWide(s.c_str())) { }
			~UW() throw() { OS::Free(cs); }
			operator const wchar_t*() const throw() { return cs; }
			operator const std::wstring() const { return cs; }
	};

	/** Utility class for easy conversion of wide strings to UTF-8. */
	class MR_DllDeclare WU
	{
		char *cs;
		public:
			WU(const wchar_t *ws=NULL) throw() : cs(WideToUtf8(ws)) { }
			explicit WU(const std::wstring &s) throw() : cs(WideToUtf8(s.c_str())) { }
			~WU() throw() { OS::Free(cs); }
			operator const char*() const throw() { return cs; }
			operator const std::string() const { return cs; }
	};

	/** Wrapper for a UTF-8 string, so non-wide UP/PU can be used like UW/WU. */
	class MR_DllDeclare UU
	{
		const char *cs;
		public:
			UU(const char *s=NULL) throw() : cs(s) { }
			explicit UU(const std::string &s) throw() : cs(s.c_str()) { }
			~UU() { }
			operator const char*() const throw() { return cs; }
			operator const std::string() const { return cs; }
	};

#	ifdef WITH_WIDE_PATHS
		typedef UW UP;
		typedef WU PU;
#	else
		typedef UU UP;
		typedef UU PU;
#	endif

}  // namespace Str

}  // namespace Util
}  // namespace HoverRace

// Convenience operators (to avoid explicit casts).

inline HoverRace::Util::OS::path_t &operator/=(HoverRace::Util::OS::path_t &path,
                                               const HoverRace::Util::Str::UP &s)
{
	return path /= static_cast<HoverRace::Util::OS::cpstr_t>(s);
}

inline HoverRace::Util::OS::path_t operator/(const HoverRace::Util::OS::path_t &path,
                                             const HoverRace::Util::Str::UP &s)
{
	return path / HoverRace::Util::OS::path_t(static_cast<HoverRace::Util::OS::cpstr_t>(s));
}

#undef MR_DllDeclare
