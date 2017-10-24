
#undef PE_NULL
#undef PE2_NULL
#undef PE_EOF
#undef PE
#undef PE2
#undef PE_NOT
#undef PE_EOK
#undef PE_0

#define PE_NULL(function) function
#define PE2_NULL(function, info) function
#define PE_EOF(function) function
#define PE(function) function
#define PE2(function, info) function
#define PE_NOT(function, desiredRv) function

#define PE_0(function) function
#define PE_EOK(function) function
