#ifndef SHANDALAR_DUEL_RUNTIME_H
#define SHANDALAR_DUEL_RUNTIME_H

#include <windows.h>

DWORD WINAPI start_duel_thread(LPVOID unused);
#if defined(_MSC_VER)
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
void abort_duel_run(DWORD result);

#endif
