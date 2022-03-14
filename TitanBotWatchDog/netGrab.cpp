#define DLL_EXPORT
#include "netGrab.h"

static const char *pCertFile = "titanbot.cer";

int NetGrabCounter::counter{ 0 };

static size_t curl_writer(char *ptr, size_t size, size_t nmemb, string* data)
{
	if (data)
	{
		data->append(ptr, size*nmemb);
		return size*nmemb;
	}
	else
		return 0;  // будет ошибка
}

__declspec(dllexport)
void netGrabSpace(string* resource, string* content) {
	static NetGrabCounter counter;
	CURL *curl_handle;
	curl_handle = curl_easy_init();

	if (curl_handle)
	{
		curl_easy_setopt(curl_handle, CURLOPT_URL, resource->c_str());
		if (resource->substr(0, 5) == "https") {
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl_handle, CURLOPT_CAINFO, pCertFile);
		}
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_writer);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, content);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 4);
		CURLcode res = curl_easy_perform(curl_handle);
		if (res)  std::cerr << curl_easy_strerror(res) << std::endl;
		curl_easy_cleanup(curl_handle);
		++counter;
	}
}

__declspec(dllexport)
void netGrabSpace(string* resource) {
	string content;
	netGrabSpace(resource, &content);
}

__declspec(dllexport)
void netGrabSpace(string* resource, string* content, HWND hwnd, UINT MessageNumber) {
	netGrabSpace(resource, content);
	if (hwnd != NULL) SendMessage(hwnd, MessageNumber, 0, 0);
}

__declspec(dllexport)
void netGrabAsync(string* resource, string* content, HWND hwnd, UINT msgNum) {
	void(*ngs)(string*, string*, HWND, UINT) = netGrabSpace;
	std::thread Thread{ ngs, resource, content, hwnd, msgNum };
	Thread.detach();
}

__declspec(dllexport)
void netGrabAsync(string* resource) {
	void(*ngs)(string*) = netGrabSpace;
	std::thread Thread{ ngs, resource};
	Thread.detach();
}

__declspec(dllexport)
void netGrabFile(string* resource, string* filename) {
	string content;
	netGrabSpace(resource, &content);

	ofstream testFile(*filename, ios::binary);
	if (!testFile) {
		cout << "Error. Cannot create test.html";
		return;
	}
	testFile << content;
	testFile.close();
}