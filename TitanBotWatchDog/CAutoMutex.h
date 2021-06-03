#pragma once
#include <Windows.h>
#include <cassert>

#define SCOPE_LOCK_MUTEX(hMutex) CMutexLock _tmp_mtx_capt(hMutex);

static bool mutex_is_free{ true };

// �����-��������, ��������� � ��������� ������ (Windows)
class CAutoMutex
{
	// ���������� ������������ �������
	HANDLE m_h_mutex;

	// ������ �����������
	CAutoMutex(const CAutoMutex&);
	CAutoMutex& operator=(const CAutoMutex&);

public:
	CAutoMutex()
	{
		m_h_mutex = CreateMutex(NULL, FALSE, NULL);
		assert(m_h_mutex);
	}

	~CAutoMutex() { CloseHandle(m_h_mutex); }

	HANDLE get() { return m_h_mutex; }
};

// �����-��������, ���������� � ������������� ������
class CMutexLock
{
	HANDLE m_mutex;

	// ��������� �����������
	CMutexLock(const CMutexLock&);
	CMutexLock& operator=(const CMutexLock&);
public:
	// �������� ������ ��� ��������������� �������
	CMutexLock(HANDLE mutex) : m_mutex(mutex)
	{
		const DWORD res = WaitForSingleObject(m_mutex, INFINITE);
		mutex_is_free = false;
		assert(res == WAIT_OBJECT_0);
	}
	// ����������� ������ ��� �������� �������
	~CMutexLock()
	{
		const BOOL res = ReleaseMutex(m_mutex);
		mutex_is_free = true;
		assert(res);
	}
};