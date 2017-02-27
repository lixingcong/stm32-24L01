// 这是个牛逼的写法，但是程序清晰性不好。
#ifdef NRF_GLOBALS 
#define NRF_EXT
#else
#define NRF_EXT extern 
#endif

#define Led_ON()   GPIO_SetBits(GPIOC, GPIO_Pin_13);  	   //LED1
#define Led_OFF()  GPIO_ResetBits(GPIOC, GPIO_Pin_13); 	   //LED2

#define Select_NRF()     GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define NotSelect_NRF()    GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define TX_ADR_WIDTH    5   // 5 bytes TX(RX) address width
#define TX_PLOAD_WIDTH  32  // 1bytes TX payload

// TODO 2017年2月27日上午10:13:17 写成类似于msstatePAN的一个int32型的赋值形式
// 最好设定地址宽度为4字节 刚好能32位整数存下来
NRF_EXT unsigned char TX_ADDRESS_LOCAL[TX_ADR_WIDTH]; // Define a static TX address
NRF_EXT unsigned char TX_ADDRESS_DUMMY[TX_ADR_WIDTH]; // Define a static TX address

NRF_EXT unsigned char rx_buf[TX_PLOAD_WIDTH];
