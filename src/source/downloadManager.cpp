// downloadManager.cpp implements a class to find and download software updates
// source file created by Allen Seitz 11/02/2015

#include "../headers/downloadManager.h"

DownloadManager::DownloadManager()
{
	serverUrl = "";
	char buffer[512] = "";
	FILE* fp = NULL;
	fopen_s(&fp, "serverurl.txt", "rt");
	if (fp != NULL)
	{
		fgets(buffer, 512, fp);
		fclose(fp);
		serverUrl = buffer;
	}
	resetState();
}

void DownloadManager::resetState()
{
	isDownloadInProgress = false;
	userCancelledDownload = false;
	currentUrl = "";
	currentFilename = "";
	currentProgressPercent = 0;
}

std::string DownloadManager::doWeNeedAnyUpdates()
{
	std::string retval = "";
	FILE* fp = NULL;
	if ( fopen_s(&fp, MANIFEST_FILENAME, "rt") != 0 )
	{
		globalError(UPDATE_MISSING_MANIFEST, "please restart");
		return "";
	}

	while (true)
	{
		char checkFile[256] = "", updateFile[256] = "", blankLine[256] = "";

		// read
		fgets(checkFile, 256, fp);
		fgets(updateFile, 256, fp);
		fgets(blankLine, 256, fp);
		if ( strlen(checkFile) > 0 )
		{
			checkFile[strlen(checkFile)-1] = 0; // chop off the trailing '\n'
			updateFile[strlen(updateFile)-1] = 0; // chop off the trailing '\n', it won't be null
		}

		if ( strlen(checkFile) == 0 || strcmp(checkFile, "NULL") == 0 )
		{
			break; // should reach this eventually
		}

		if ( !fileExists(checkFile) )
		{
			retval = updateFile; // found one!
			break;
		}
	}

	fclose(fp);
	return retval;
}

void DownloadManager::downloadFile(std::string url, std::string filename)
{
	if ( isDownloadInProgress )
	{
		return;
	}

	if (serverUrl.length() == 0)
	{
		globalError(UPDATE_MISSING_SERVERURL, "check serverurl.txt and restart");
		return;
	}

	// reset
	isDownloadInProgress = true;
	userCancelledDownload = false;
	currentUrl = serverUrl + url;
	currentFilename = filename;
	currentProgressPercent = 0;

	// thanks, no thanks, no cache please
	char arbitraryNumber[64] = "";
	_itoa_s(time(0), arbitraryNumber, 64, 10);
	std::string noCacheUrl = currentUrl + "?CacheBuster=" + arbitraryNumber;

	// download!
	HRESULT hr = URLDownloadToFile(0, noCacheUrl.c_str(), filename.c_str(), 0, static_cast<IBindStatusCallback*>(&progressDelegate));
}

std::string DownloadManager::getCurrentDownloadFilename()
{
	return currentFilename;
}

int DownloadManager::getCurrentDownloadProgress()
{
	if ( progressDelegate.downloadMaximum != 0 )
	{
		// prevent overflow
		ULONG numerator = progressDelegate.downloadProgress/100;
		ULONG denominator = progressDelegate.downloadMaximum/100 + 1;
		currentProgressPercent = numerator * 100 / denominator;
	}
	return currentProgressPercent;
}

void DownloadManager::cancelCurrentDownload()
{
	userCancelledDownload = true;
}

bool DownloadManager::isDownloadComplete()
{
	if ( isDownloadInProgress && progressDelegate.downloadProgress > 0 )
	{
		return progressDelegate.downloadProgress == progressDelegate.downloadMaximum;
	}

	return false;
}
