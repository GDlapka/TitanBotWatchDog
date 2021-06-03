#include "netGrabAsync.h"

using namespace std;

void runGrabAsync(string* address, int timeout) {
	{
		// занимаем мютекс
		SCOPE_LOCK_MUTEX(g_mutex.get());

		// изменяем общие данные
		g_address = *address;
		g_timeout = timeout;

		// здесь мютекс освобождается
	}
	std::thread t1(threadGrabAsync);

	t1.detach();
}



//проверка получения ответа
bool anwserReceived(std::string* _out_anwser) {
	if (mutex_is_free) {
		// занимаем мютекс
		SCOPE_LOCK_MUTEX(g_mutex.get());

		// изменяем общие данные
		*_out_anwser = g_anwser;

		// здесь мютекс освобождается
	}
	return mutex_is_free;
}

void threadGrabAsync(void) {
	// занимаем мютекс
	SCOPE_LOCK_MUTEX(g_mutex.get());

	// изменяем общие данные
	netGrabSpaceT(&g_address, &g_anwser, g_timeout);

	// здесь мютекс освобождается
}