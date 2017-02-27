// 这是个牛逼的写法，但是程序清晰性不好。
#ifdef NRF_GLOBALS 
#define NRF_EXT
#else
#define NRF_EXT extern 
#endif






