
/* TrackDownloadDialog.cpp
	Implementation for the track downloader. */

#include "StdAfx.h"

#include "TrackDownloadDialog.h"

#include "resource.h"

#include <curl/curl.h>

#include <LiteUnzip.h>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <io.h>

#include "../Util/Config.h"

static const char *STATE_NAMES[] = {
	"Initializing",
	"Downloading",
	"Extracting",
	"Finished",
};

#define INIT_CAPACITY (5 * 1024 * 1024)
#define MAX_CAPACITY (20 * 1024 * 1024)

#define TRACK_HOST "http://www.hoverrace.com/"

/**
 * Constructor.
 * @param name The name of the track (no ".trk" extension, may not be blank).
 */
TrackDownloadDialog::TrackDownloadDialog(const std::string &name) :
	name(name), trackFilename(name), dlgHwnd(NULL), state(ST_INITIALIZING),
	bufSize(0), bufCapacity(INIT_CAPACITY)
{
	trackFilename += ".trk";
	trackDl = curl_easy_init();

	curlErrorBuf = new char[CURL_ERROR_SIZE];
	curlErrorBuf[0] = 0;

	dlBuf = (dlBuf_t *) malloc(INIT_CAPACITY * sizeof(dlBuf_t));
}

/// Destructor.
TrackDownloadDialog::~TrackDownloadDialog()
{
	free(dlBuf);
	delete[] curlErrorBuf;
	curl_easy_cleanup(trackDl);
}

/**
 * Display the modal dialog and start the download.
 *
 * This dialog is modal; it will block until the download is finished
 * or fails.
 *
 * @param hinst The app instance handle.
 * @param parent The parent window handle.
 * @return @c true If the download was successful (the track is now
 *         available) or @c false if the download failed.
 */
bool TrackDownloadDialog::ShowModal(HINSTANCE hinst, HWND parent)
{
	bufSize = 0;
	bufTotal = 0;
	cancel = false;

	// Ask to download the track.
	std::ostringstream oss;
	oss << "Would you like to download the track \"" << name << "\" now?";
	if (MessageBox(parent, oss.str().c_str(), "Track Download", MB_YESNO) == IDNO) {
		return false;
	}

	boost::thread thread(boost::bind(&TrackDownloadDialog::ThreadProc, this));

	DWORD dlgRetv = DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_TRACK_DOWNLOAD),
		parent, DlgFunc, reinterpret_cast<LPARAM>(this));
	if (dlgRetv == IDCANCEL) cancel = true;

	thread.join();

	return !cancel;
}

void TrackDownloadDialog::SetState(state_t st)
{
	state = st;

	OutputDebugString("State is now: ");
	OutputDebugString(STATE_NAMES[st]);
	OutputDebugString("\n");

	// Notify dialog of state change.
	if (dlgHwnd != NULL) {
		PostMessage(dlgHwnd, WM_APP, state, 0);
	}
}

/// Update the dialog with the current status.
void TrackDownloadDialog::UpdateDialogProgress(HWND hwnd)
{
	state_t curState = state;
	size_t curTotal = bufTotal;
	size_t curSize = bufSize;

	// Update progress bar.
	int pos;
	switch (curState) {
		case ST_INITIALIZING:
			pos = 0;
			break;

		case ST_DOWNLOADING:
			if (curTotal <= 0 || curSize <= 0) {
				pos = 0;
			} else if (curSize >= curTotal) {
				pos = 100;
			} else {
				pos = (curSize * 100) / (curTotal);
			}
			break;

		case ST_EXTRACTING:
		case ST_FINISHED:
			pos = 100;
			break;
	}
	SendDlgItemMessage(hwnd, IDC_DLPROGRESS, PBM_SETPOS, (WPARAM)pos, 0);

	// Update status text.
	if (curState < 0 || curState >= ST_LAST) {
		SetDlgItemText(hwnd, IDC_STATE, "Processing...");
	} else {
		std::ostringstream stateStr;
		if (curState == ST_DOWNLOADING) {
			stateStr << "Downloading";
			if (curTotal <= 0) {
				if (curSize > 0) {
					stateStr << " (" << (curSize / 1024) << " KB)";
				}
			} else if (curTotal > 0) {
				stateStr << " (" << pos << "% of " << (curTotal / 1024) << " KB)";
			}
		} else {
			stateStr << STATE_NAMES[curState];
		}
		stateStr << "...";
		SetDlgItemText(hwnd, IDC_STATE, stateStr.str().c_str());
	}
}

// Thread function.
void TrackDownloadDialog::ThreadProc()
{
	MR_Config *cfg = MR_Config::GetInstance();

	SetState(ST_INITIALIZING);

	// Error buffer.
	curl_easy_setopt(trackDl, CURLOPT_ERRORBUFFER, curlErrorBuf);

	// Callback functions.
	curl_easy_setopt(trackDl, CURLOPT_WRITEFUNCTION, WriteFunc);
	curl_easy_setopt(trackDl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(trackDl, CURLOPT_PROGRESSFUNCTION, ProgressFunc);
	curl_easy_setopt(trackDl, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(trackDl, CURLOPT_NOPROGRESS, 0);

	// Misc HTTP options.
	curl_easy_setopt(trackDl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(trackDl, CURLOPT_FAILONERROR, 1);
	/* Enabling content encoding makes the Content-Length not work currently.
	curl_easy_setopt(trackDl, CURLOPT_ENCODING, "");  // Enable all encodings.
	curl_easy_setopt(trackDl, CURLOPT_HTTP_CONTENT_DECODING, 1);
	*/

	// TODO: Add platform.
	std::ostringstream oss;
	oss << "HoverRace/" << cfg->GetVersion() << " (Win32)";
	curl_easy_setopt(trackDl, CURLOPT_USERAGENT, oss.str().c_str());

	std::string url(TRACK_HOST "tracks/download.php?name=");
	char *param = curl_easy_escape(trackDl, name.c_str(), name.length());
	url += param;
	curl_free(param);
	curl_easy_setopt(trackDl, CURLOPT_URL, url.c_str());

	if (!cancel) {
		SetState(ST_DOWNLOADING);
		int curlRetv = curl_easy_perform(trackDl);
		if (curlRetv != 0 && curlRetv != CURLE_ABORTED_BY_CALLBACK) {
			MessageBox(dlgHwnd, curlErrorBuf, "HoverRace", MB_ICONWARNING | MB_OK);
			cancel = true;
		}
	}

	if (!cancel) {
		SetState(ST_EXTRACTING);

		if (bufSize <= 128) {
			std::ostringstream oss;
			oss << "Sorry, track \"" << name << "\" is not available from " TRACK_HOST;
			MessageBox(dlgHwnd, oss.str().c_str(), "HoverRace", MB_ICONINFORMATION | MB_OK);
			cancel = true;
		} else {
			if (!ExtractTrackFile()) {
				std::ostringstream oss;
				oss << "Track download failed: The file \"" << trackFilename <<
					"\" was not found in the archive downloaded from " TRACK_HOST;
				MessageBox(dlgHwnd, oss.str().c_str(), "HoverRace", MB_ICONWARNING | MB_OK);
			}
		}
	}

	SetState(ST_FINISHED);

	if (dlgHwnd != NULL) {
		EndDialog(dlgHwnd, IDOK);
	}
}

// Dialog callback.
BOOL TrackDownloadDialog::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	BOOL retv = FALSE;

	switch (message) {

		case WM_INITDIALOG:
			dlgHwnd = hwnd;
			SetDlgItemText(hwnd, IDC_TRACK_NAME, name.c_str());
			UpdateDialogProgress(hwnd);
			retv = TRUE;
			break;

		case WM_DESTROY:
			dlgHwnd = NULL;
			break;

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDCANCEL:
					cancel = true;
					EndDialog(hwnd, IDCANCEL);
					retv = TRUE;
					break;
			}
			break;

		case WM_APP: // State changed.
			UpdateDialogProgress(hwnd);
			break;
	}

	return retv;
}

/// Global dialog callback dispatcher.
BOOL CALLBACK TrackDownloadDialog::DlgFunc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// Determine which instance to route the message to.
	TrackDownloadDialog *dlg;
	if (message == WM_INITDIALOG) {
		dlg = reinterpret_cast<TrackDownloadDialog*>(lparam);
		SetWindowLong(hwnd, GWL_USERDATA, lparam);
	} else {
		dlg = reinterpret_cast<TrackDownloadDialog*>(GetWindowLong(hwnd, GWL_USERDATA));
		if (message == WM_DESTROY) {
			SetWindowLong(hwnd, GWL_USERDATA, 0);
		}
	}

	return (dlg == NULL) ? FALSE : dlg->DlgProc(hwnd, message, wparam, lparam);
}

/// Write callback for libcurl.
size_t TrackDownloadDialog::WriteProc(void *ptr, size_t size, size_t nmemb)
{
	// Add to buffer.

	size_t bytesToAdd = size * nmemb;  // We trust that this won't overflow.
	if (bufSize + bytesToAdd > bufCapacity) {
		while (bufSize + bytesToAdd > bufCapacity) bufCapacity *= 2;

		// If we've downloaded this much but haven't gotten a Content-Length,
		// then chances are we're just endlessly streaming junk and wasting
		// precious bandwidth.
		if (bufTotal <= 0 && bufCapacity > MAX_CAPACITY) {
			MessageBox(dlgHwnd,
				"There was a problem downloading the track from " TRACK_HOST "\n"
				"Please visit " TRACK_HOST " and download the track manually.",
				"HoverRace", MB_OK);
			return 0;
		}

		dlBuf = (dlBuf_t*)realloc(dlBuf, bufCapacity * sizeof(dlBuf_t));
	}

	memcpy(dlBuf + bufSize, ptr, bytesToAdd);
	bufSize += bytesToAdd;

	return bytesToAdd;
}

/// Global write callback dispatcher.
size_t TrackDownloadDialog::WriteFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return reinterpret_cast<TrackDownloadDialog*>(stream)->WriteProc(ptr, size, nmemb);
}

/// Progress callback for libcurl.
size_t TrackDownloadDialog::ProgressProc(double dlTotal, double dlNow)
{
	bufTotal = static_cast<size_t>(dlTotal);

	// Notify dialog of progress change.
	if (dlgHwnd != NULL) {
		PostMessage(dlgHwnd, WM_APP, state, 0);
	}

	return cancel ? 1 : 0;
}

/// Global progress callback dispatcher.
size_t TrackDownloadDialog::ProgressFunc(void *clientp, double dlTotal, double dlNow, double, double)
{
	return reinterpret_cast<TrackDownloadDialog*>(clientp)->ProgressProc(dlTotal, dlNow);
}

/**
 * Extracts the track file from the current buffer.
 * @return @c true if the track extracted successfully, @c false otherwise.
 */
bool TrackDownloadDialog::ExtractTrackFile()
{
	std::string destFilename = MR_Config::GetInstance()->GetTrackPath(name);
	bool retv = false;

	if (dlBuf[0] == 0x50 && dlBuf[1] == 0x4b) {
		// Extract from ZIP archive.
		HUNZIP huz;
		ZIPENTRY zent;

		UnzipOpenBuffer(&huz, dlBuf, bufSize, 0);

		memset(&zent, 0, sizeof(zent));
		lstrcpy(zent.Name, trackFilename.c_str());
		DWORD unzRetv = UnzipFindItem(huz, &zent, 1);
		retv = (unzRetv == ZR_OK);

		if (retv) {
			unzRetv = UnzipItemToFile(huz, destFilename.c_str(), &zent);
			retv = (unzRetv == ZR_OK);
		}

		UnzipClose(huz);
	}
	else {
		// Raw file.
		FILE *outFile = fopen(destFilename.c_str(), "wb");
		if (outFile != NULL) {
			fwrite(dlBuf, bufSize, 1, outFile);
			fclose(outFile);
		}
	}

#ifdef _WIN32
	/* fix modification time and creation time */

	/* what the fuck? "CreateFile()" opens existing files?  Misnomer much? */
	HANDLE file = CreateFile(destFilename.c_str(),
								FILE_WRITE_ATTRIBUTES,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
	/* that was WAY too hard */
	FILETIME ft;
	SYSTEMTIME st;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	if(SetFileTime(file, &ft, &ft, &ft) == 0) {
		/* error */
		ASSERT(FALSE);
	}

	CloseHandle(file);
	/* I swear, that was ten times harder than it should have been */
#endif


	return retv;
}
