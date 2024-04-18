#include "pch.h"
#include "CoreTLS.h"

// 코드		X
// 스택		X
// 힙		O
// 데이터	O

// TLS		X -> 전역 공간에 나만 쓰고 싶은거

thread_local uint32 LThreadId = 0;


