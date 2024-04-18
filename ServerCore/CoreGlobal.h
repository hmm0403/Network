
#pragma once
// 싱글톤이나 전역변수 만들기 위한 용도
class ThreadManager;

extern std::unique_ptr<ThreadManager> GThreadManager;