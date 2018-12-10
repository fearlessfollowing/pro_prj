#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sstream> 
#include <vector>
#include <sys/statfs.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <cmath>

#include <log/log.h>
#include <log/log_wrapper.h>

#define LOG_MAX_SIZE 30     /* 默认的日志文件最大为20MB */
#define LOG_BUF_SIZE 1024
#undef  TAG
#define TAG "LogWrapper"

using namespace std;

std::shared_ptr<LogWrapper> LogWrapper::mLogger = nullptr;

LogWrapper::LogWrapper(std::string path, std::string name, bool bSendToLogd)
{
    mLogFileMaxSize = LOG_MAX_SIZE * 1024 * 1024;
    mLogFileSavaPath = path;
    mLogFileName = name;
    mSendToLogd = bSendToLogd;

    if (access(mLogFileSavaPath.c_str(), 0)) {
		std::string cmd = "mkdir -p " + mLogFileSavaPath;
        system(cmd.c_str());
	}

	std::string file_name = mLogFileSavaPath +  "/" + mLogFileName;
	
	mLogFileHandle = fopen(file_name.c_str(), "a+");
	if (!mLogFileHandle) {
		fprintf(stderr, "open log file fail\n");
	}
}


LogWrapper::~LogWrapper()
{	
	if (mLogFileHandle) {
		fclose(mLogFileHandle);
		mLogFileHandle = nullptr;
	}
}



void LogWrapper::log(unsigned char level, const char* tag, const char* file, int line, const char* fmt, ...)
{	
	static char LogLevelString[][10] = {"VERBOSE", "DEBUG", "INFO ", "WARN", "ERROR", "FATAL"};

	if (level < mLogPrintlevel) {
		return;
	}

	va_list arg;
	std::lock_guard<std::mutex> lock(mLogMutex);
	
	/* to stdout */
	struct timeval tv;  
	gettimeofday(&tv, nullptr);
	struct tm *tmt = localtime(&tv.tv_sec);

	if (mSendToLogd) {
        va_list ap;
        char buf[LOG_BUF_SIZE] = {0};
        char file_line[128] = {0};

        snprintf(file_line, 128, "[%s:%d]", file, line);
        strcpy(buf, file_line);
        int iLen = strlen(buf);

        va_start(ap, fmt);
        vsnprintf(&buf[iLen], LOG_BUF_SIZE, fmt, ap);
        va_end(ap);

        __android_log_buf_write(LOG_ID_MAIN, level, tag, buf);
	}

	if (mLogFileHandle) {
		fprintf(mLogFileHandle, "%04d-%02d-%02d %02d:%02d:%02d:%06ld %s %s  ",
			tmt->tm_year + 1900,
			tmt->tm_mon + 1,
			tmt->tm_mday,
			tmt->tm_hour,
			tmt->tm_min,
			tmt->tm_sec,
			tv.tv_usec,
			LogLevelString[level],
            tag);

		va_start(arg, fmt);
		vfprintf(mLogFileHandle, fmt, arg);
		va_end(arg);

		fprintf(mLogFileHandle, " %s:%d\n", file, line);
		fflush(mLogFileHandle);
		checkIfChangeLogFile();
	}
}


void LogWrapper::checkIfChangeLogFile()
{
	std::string origfile = mLogFileSavaPath + "/" + mLogFileName;

	struct stat statbuff;  
	if (stat(origfile.c_str(), &statbuff) < 0) {  
		return;  
	}

	if (statbuff.st_size  < (int)mLogFileMaxSize) {
		return;
	}

    if (mLogFileHandle) {
        fclose(mLogFileHandle);
    }

	mLogFileHandle = nullptr;

	time_t timer = time(nullptr);
	struct tm *tmt = localtime(&timer);

	std::stringstream newfile;

    #if 0
	newfile << mLogFileSavaPath 
			<< "/"
			<< mLogFileName << "_"
			<< tmt->tm_year + 1900 << "_"
			<< tmt->tm_mon + 1 << "_"
			<< tmt->tm_mday << "_" 
			<< tmt->tm_hour << "_"
			<< tmt->tm_min << "_"
			<< tmt->tm_sec;
    #else 
	newfile << mLogFileSavaPath 
			<< "/"
			<< mLogFileName << ".back";
    #endif

    unlink(newfile.str().c_str());

	rename(origfile.c_str(), newfile.str().c_str());

	mLogFileHandle = fopen(origfile.c_str(), "a+");
	if (!mLogFileHandle) {
		printf("open log file fail\n");
	}

	// checkIfRemoveOldLogFile();
}



void LogWrapper::checkIfRemoveOldLogFile()
{
    struct statfs diskInfo;  
    if (statfs(mLogFileSavaPath.c_str(), &diskInfo)) {
        LOGERR(TAG, "statfs fail");
        return;
    }

    unsigned long long block_size = diskInfo.f_bsize; 
    unsigned long long available_size = diskInfo.f_bavail*block_size;  

    if ( (available_size>>20) > 100 ) return;

	DIR* dir = opendir(mLogFileSavaPath.c_str());
	if (dir == nullptr) {
		printf("open dir fail\n");
		return;
	}

	std::string prefix = mLogFileName + "_";
	std::vector<std::string> v_e;
	struct dirent *ptr = nullptr;
	while ((ptr = readdir(dir)) != nullptr) {
		std::string e = ptr->d_name;
		if (e.find(prefix, 0) == 0) {
			v_e.push_back(e);
		}
	}

	closedir(dir);

	if (v_e.empty()) 
        return;

	// sort(v_e.begin(), v_e.end());

	for (unsigned int i = 0; (i < v_e.size())&&(i < 2); i++) {
		std::string file_name = mLogFileSavaPath + "/" + v_e[i];
		remove(file_name.c_str());
		printf("remove log file:%s\n", file_name.c_str());
	}    
}

