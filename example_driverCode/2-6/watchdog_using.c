//开启xxx的看门狗
void enable_watchdog()
{
	rWTCON = WTCON_DIV64 | WTCON_RSTEN;//64分频、开启复位信号
	rWTDAT = 0x8000;//计数目标
	rWTCON |= WTCON_ENABLE;//开启看门狗
}

//xxx的看门狗“喂狗”
void feed_dog()
{
	rWTCNT = 0x8000;
}

//看门狗的使用例程
void main()
{
	init_system();
	...
	enable_watchdog();//启动看门狗
	...
	while(1){
		...
		feed_dog();
	}
}