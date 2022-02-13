
#include "util.h"
#include "log.h"
#include "fiber.h"

#include <execinfo.h>
// timer
#include <sys/time.h>
// opendir readdir
#include <dirent.h>
// access
#include <unistd.h>
// opendir
// #include <sys/types.h>
// strcmp
#include <string.h>

// lstat
#include <sys/types.h>
#include <sys/stat.h>

// kill
#include <signal.h>



namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

/**
 * @brief 获取当前线程 id
 *
 * @return pid_t
 */
pid_t GetThreadId() { return syscall(SYS_gettid); }

/**
 * @brief 获取协程 id
 *
 * @return uint32_t
 */
uint32_t GetFiberId() {
    // return 0;
    return ljrserver::Fiber::GetFiberId();
}

/**
 * @brief 函数调用栈
 *
 * @param bt
 * @param size
 * @param skip
 */
void Backtrace(std::vector<std::string> &bt, int size, int skip) {
    void **array = (void **)malloc((sizeof(void *) * size));
    size_t s = ::backtrace(array, size);

    char **strings = backtrace_symbols(array, s);
    if (strings == NULL) {
        LJRSERVER_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for (size_t i = skip; i < s; i++) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

/**
 * @brief 函数调用栈 string
 *
 * @param size
 * @param skip
 * @param prefix
 * @return std::string
 */
std::string BacktraceToString(int size, int skip, const std::string &prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);

    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); i++) {
        ss << prefix << bt[i] << std::endl;
    }

    return ss.str();
}

/**
 * @brief 获取时间 ms
 *
 * @return uint64_t
 */
uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

/**
 * @brief 获取时间 us
 *
 * @return uint64_t
 */
uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

/**
 * @brief 时间字符串
 *
 * @param ts
 * @return std::string
 */
std::string Time2Str(time_t ts, const std::string &format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    // 格式化时间
    strftime(buf, sizeof(buf), format.c_str(), &tm);

    return buf;
}

/**
 * @brief 列出文件夹下所有文件
 *
 * @param files
 * @param path
 * @param subfix
 */
void FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string &path, const std::string &subfix) {
    if (access(path.c_str(), 0)) {
        // 文件路径不存在
        return;
    }

    // 打开文件夹
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        // 打开文件夹失败
        return;
    }

    struct dirent *dp = nullptr;
    // 循环读取文件夹
    while ((dp = readdir(dir)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            // 文件夹 递归读取
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if (dp->d_type == DT_REG) {
            // 正常文件
            std::string filename(dp->d_name);
            if (subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if (filename.size() < subfix.size()) {
                    continue;
                }
                // 判断后缀
                if (filename.substr(filename.length() - subfix.size()) ==
                    subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    // 关闭文件夹
    closedir(dir);
}

static int __lstat(const char *file, struct stat *st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if (st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char *dirname) {
    if (access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/**
 * @brief mkdir
 *
 * @param dirname
 * @return true
 * @return false
 */
bool FSUtil::Mkdir(const std::string &dirname) {
    if (__lstat(dirname.c_str()) == 0) {
        return true;
    }
    char *path = strdup(dirname.c_str());
    char *ptr = strchr(path + 1, '/');
    do {
        for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if (__mkdir(path) != 0) {
                break;
            }
        }
        if (ptr != nullptr) {
            break;
        } else if (__mkdir(path) != 0) {
            break;
        }
        free(path);
        return true;
    } while (0);
    free(path);
    return false;
}

/**
 * @brief 通过 pid file 来判断程序是否已经启动
 *
 * @param pidfile
 * @return true
 * @return false
 */
bool FSUtil::IsRunningPidfile(const std::string &pidfile) {
    if (__lstat(pidfile.c_str()) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if (!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if (line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if (pid <= 1) {
        return false;
    }
    if (kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

}  // namespace ljrserver