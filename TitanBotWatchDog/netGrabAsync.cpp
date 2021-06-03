#include "netGrabAsync.h"

using namespace std;

void runGrabAsync(string* address, int timeout) {
	{
		// �������� ������
		SCOPE_LOCK_MUTEX(g_mutex.get());

		// �������� ����� ������
		g_address = *address;
		g_timeout = timeout;

		// ����� ������ �������������
	}
	std::thread t1(threadGrabAsync);

	t1.detach();
}



//�������� ��������� ������
bool anwserReceived(std::string* _out_anwser) {
	if (mutex_is_free) {
		// �������� ������
		SCOPE_LOCK_MUTEX(g_mutex.get());

		// �������� ����� ������
		*_out_anwser = g_anwser;

		// ����� ������ �������������
	}
	return mutex_is_free;
}

void threadGrabAsync(void) {
	// �������� ������
	SCOPE_LOCK_MUTEX(g_mutex.get());

	// �������� ����� ������
	netGrabSpaceT(&g_address, &g_anwser, g_timeout);

	// ����� ������ �������������
}