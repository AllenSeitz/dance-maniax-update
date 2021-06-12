// downloadManager.h implements a class to find and download software updates
// source file created by Allen Seitz 11/02/2015

#ifndef _DOWNLOADMANAGER_H
#define _DOWNLOADMANAGER_H

#pragma comment(lib, "urlmon.lib")

#include "../headers/common.h"

#define MANIFEST_FILENAME "manifest.txt"
#define MANIFEST_BETA_FILENAME "manifest_beta.txt"

// used during async downloading
extern void alternateMainUpdateLoop();
extern RenderingManager rm;

// This class is used by the Windows API URLDownloadToFile() function.
// It implements the asynchronous callback for monitoring progress.
class DownloadProgress : public IBindStatusCallback {
public:
	DownloadProgress()
	{
		downloadProgress = 0;
		downloadMaximum = 0;
		userCancelledDownload = false;
	}

    HRESULT __stdcall QueryInterface(const IID &,void **) { 
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void) { 
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release(void) {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib) {
		downloadProgress = 0;
		downloadMaximum = 0;
		userCancelledDownload = false;

		return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) {
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed) {
        return E_NOTIMPL;
    }        
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk) {
        return E_NOTIMPL;
    }

    virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        //wcout << ulProgress << L" of " << ulProgressMax;
        //if (szStatusText) wcout << " " << szStatusText;
        //wcout << endl;
		al_trace("DOWNLOAD PROGRESS: %ul / %ul\n", downloadProgress, downloadMaximum);

		downloadProgress = ulProgress;
		downloadMaximum = ulProgressMax;

		// render the screen
		alternateMainUpdateLoop();
		rm.flip();

	    if (userCancelledDownload)
		{
			return E_ABORT;
		}

	    return S_OK;	
	}

	ULONG downloadProgress;
	ULONG downloadMaximum;
	bool userCancelledDownload;
};

class DownloadManager
{
public:

	DownloadManager();

	void resetState();
	// precondition: do not call this if a download is in progress! it can leak file handles
	// postcondition: ready to call downloadFile() again

	std::string doWeNeedAnyUpdates();
	// precondition: manifest.txt has been downloaded
	// postcondition: returns a file to download if manifest.txt lists a file which we don't have, or an empty string otherwise

	void downloadFile(std::string url, std::string filename);
	// precondition: works best if no other downloads are in progress

	std::string getCurrentDownloadFilename();
	// precondition: there is a download currently in progress because of downloadFile()
	// postcondition: returns the currentFilename, or an empty string

	int getCurrentDownloadProgress();
	// precondition: there is a download currently in progress because of downloadFile()
	// postcondition: returns [0-100], but note that "100" may not mean the file is complete

	void cancelCurrentDownload();
	// precondition: there is a download currently in progress because of downloadFile()
	// postcondition: cancels the file and may leave a fragment in the user's temp folder

	bool isDownloadComplete();
	// returns true if there is no download in progress (including if no download was started)

protected:
    DownloadProgress progressDelegate;
	std::string serverUrl;

	bool isDownloadInProgress;
	bool userCancelledDownload;
	std::string currentUrl;
	std::string currentFilename;
	ULONG currentProgressPercent;
};

#endif