## 日志输出模块
<br />

### 模块说明
本模块用于进行日志输出，并将输出的日志信息存储至../recored文件夹中 <br />
本模块中的文件操作均采用C方式进行编写，相比C++文件操作，C的速度会快上[四倍](https://blog.csdn.net/shudaxia123/article/details/50491451)左右<br />
<br />

### 参数说明
``` C++
/********文件参数相关********/
//用于存储日志缓存
char m_buf[] 
//用于存储文件
char m_dir_name[] 
//单个日志文件最大行数
//当文件行数大于此参数时，会创建新日志文件
char uint32_t m_max_line 
//当前文件行数
uint32_t m_cur_line
//当前m_fd对应的创建日
//日志记录以天为单位，如果到第二天将会创建新日志文件
int m_cur_day

/********文件读写相关********/
//基于C风格操作文件
//性能比C++快4倍
//参考链接：https://blog.csdn.net/shudaxia123/article/details/50491451
//日志文件描述符
FILE *m_fd;
//日志队列
blockQueue<string> *m_log_queue;
//是否是异步日志
bool m_is_aync;
//是否关闭日志
bool m_close_log;
//日志文件index
size_t m_log_index;

/********日志状态相关********/
//日志系统其否关闭
bool m_close_log;

/********多线程相关********/
//缓冲区锁
mutex m_mutex;
```

### 接口说明
#### 唯一实例   
static Log* singleton();

#### 初始化
void init(const char *dir_name, size_t max_line, size_t max_size, bool is_asyc, bool is_closed);

- dir_name: 文件名
- max_line: 单个文件最大行
- max_size: 任务队列最大容量
- is_asyc: 是否启动多线程
- is_closed: 日志系统是否关闭

#### 日志写入（同步）
void write_log(int level, const char *format, ...);

- level: 日志信息等级

    - 1: Debug
    - 2: Info
    - 3: Warning
    - 4: Error
- format: 格式化参数

#### 缓冲区刷新（线程安全）
void flush()

#### 日志写入（异步）
write_log_asyc()
