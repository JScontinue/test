//S3C2410 UART UFCONn 寄存器的位掩码和默认设置
#define S3C2410_UFCON_FIFOMODE		(1<<0)
#define S3C2410_UFCON_TXTRIG0		(0<<6)
#define S3C2410_UFCON_RXTRIG8		(1<<4)
#define S3C2410_UFCON_RXTRIG12		(2<<4)

#define S3C2410_UFCON_RESETBOTS		(3<<1)
#define S3C2410_UFCON_RESETTX		(1<<2)
#define S3C2410_UFCON_RESETRX		(1<<1)

#define S3C2410_UFCON_DEFAULT		(S3C2410_UFCON_FIFOMODE | \
					S3C2410_UFCON_TXTRIG0 | \
					S3C2410_UFCON_RXTRIG8 )
					
//S3C2410 UART UFSTATn寄存器的位掩码
#define S3C2410_UFSTAT_TXFULL		(1<<9)
#define S3C2410_UFSTAT_RXRULL		(1<<8)
#define S3C2410_UFSTAT_TXMASK		(15<<4)
#define S3C2410_UFSTAT_TXSHIFT		(4)
#define S3C2410_UFSTAT_RXMASK		(15<<0)
#define S3C2410_UFSTAT_RXSHIFT		(0)

//S3C2410串口驱动的uart_driver结构体
#define S3C24XX_SERIAL_NAME		"ttySAC"
#define S3C24XX_SERIAL_DEVFS	"tts/"
#define S3C24XX_SERIAL_MAJOR	204
#define S3C24XX_SERIAL_MINOR	64

static struct uart_driver s3c24xx_uart_drv ={
	.owner	= "THIS_MODULE",
	.dev_name = "s3c2410_serial",
	.nr		= 3,
	.cons	= S3C24XX_SERIAL_CONSOLE,
	.driver_name = S3C24XX_SERIAL_NAME,
	.devfs_name = S3C24XX_SERIAL_DEVFS,
	.major	= S3C24XX_SERIAL_MAJOR,
	.minor	= S3C24XX_SERIAL_MINOR,
};

//S3C2410串口驱动的s3c24xx_uart_port结构体
struct s3c24xx_uart_port{
	unsigned char	rx_claimed;
	unsigned char	tx_claimed;
	struct s3c24xx_uart_info *info;
	struct s3c24xx_uart_clksrc *clksrc;
	struct clk	*clk;
	struct clk  *baudclk;
	struct uart_port	port;
};

static struct s3c24xx_uart_port s3c24xx_serial_ports[NR_PORTS] = {
	[0] = {
		.poet = {
			.lock	= SPIN_LOCK_UNLOCKED,
			.iotype = UPIO_MEM,
			.irq	= IRQ_S3CUART_RX0,
			.uartclk = 0,
			.fifosize = 16,
			.ops = &s3c24xx_serial_ops,
			.flags = UPF_BOOT_AUTOCONF,
			.line = 0,//端口索引：0
		}
	},
	[1] = {
		.poet = {
			.lock	= SPIN_LOCK_UNLOCKED,
			.iotype = UPIO_MEM,
			.irq	= IRQ_S3CUART_RX1,
			.uartclk = 0,
			.fifosize = 16,
			.ops = &s3c24xx_serial_ops,
			.flags = UPF_BOOT_AUTOCONF,
			.line = 1,//端口索引：1
		}
	},
#if NR_PORT > 2
	[2] = {
		.poet = {
			.lock	= SPIN_LOCK_UNLOCKED,
			.iotype = UPIO_MEM,
			.irq	= IRQ_S3CUART_RX2,
			.uartclk = 0,
			.fifosize = 16,
			.ops = &s3c24xx_serial_ops,
			.flags = UPF_BOOT_AUTOCONF,
			.line = 2,//端口索引：2
		}
	}
#endif

//S3C2410串口驱动的uart_ops结构体
static struct uart_ops s3c24xx_serial_ops = {
	.pm		= s3c24xx_serial_pm,
	.tx_empty	= s3c24xx_serial_tx_empty,//发送缓冲区空
	.get_mctrl	= s3c24xx_serial_get_mctr1,//得到modem控制设置
	.set_mctrl	= s3c24xx_serial_set_mctrl,//设置modem控制（MCR）
	.stop_tx	= s3c24xx_serial_stop_tx,//停止接收字符
	.start_tx	= s3c24xx_serial_start_tx,//开始传输字符
	.stop_rx	= s3c24xx_serial_stop_rx,//停止接收字符
	.enable_ms	= s3c24xx_serial_enable_ms,//modem状态中断使能
	.break_ctl	= s3c24xx_serial_break_ctl,//控制break信号的传输
	.startup	= s3c24xx_serial_start_up,//启动端口
	.shutdown	= s3c24xx_serial_shutdown,//禁用端口
	.set_termios= s3c24xx_serial_set_termios,//改变端口参数
	.type		= s3c24xx_serial_type,//返回描述特定端口的常量字符串指针
	.release_port=s3c24xx_serial_releade_port,//释放端口占用的内存和I/O资源
	.request_port=s3c24xx_serial_request_port,//申请端口所需的内存和I/O资源
	.config_port =s3c24xx_serial_config_port,//执行端口所需的自动配置步骤
	.verify_port =s3c24xx_serial_verify_port,//验证新的串行端口信息
};

//S3C2410串口驱动的s3c24xx_uart_info结构体
static struct s3c24xx_uart_info s3c2410_uart_inf = {
	.name		= "Samsung S3C2410 UART",
	.type		= PORT_S3C2410,
	.fifosize	= 16,
	.rx_fifomask = S3C2410_UFSTAT_RXMASK,
	.rx_fifoshift= S3C2410_UFSTAT_RXSHIFT,
	.rx_fifofull = S3C2410_UFSTAT_RXFULL,
	.tx_fifofull = S3C2410_UFSTAT_TXFULL,
	.tx_fifomask = S3C2410_UFSTAT_TXMASK,
	.tx_fifoshift= S3C2410_UFSTAT_TXSHIFT,
	.get_clksrc	 = s3c2410_serial_getsource,
	.set_clksrc  = s3c2410_serial_setsource,
	.reset_port  = s3c2410_serial_resetsource,
};

//S3C2410串口驱动s3c2410_uartcfg结构体
struct s3c2410_uartcfg{
	unsigned char hwport;//硬件端口号
	unsigned char unused;
	unsigned short flags;
	unsigned long uart_flags;//默认的UART标志
	
	unsigned long ucon;//端口的ucon值
	unsigned long ulcon;//端口的ulcon值
	unsigned long ufcon;//端口的ufcon值
	
	struct s3c24xx_uart_clksrc *clocks;
	unsigned int clocks_size;
};

