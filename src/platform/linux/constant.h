#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#ifdef _LINUX_

#define ENV_STRING              "LINUX"
#define CHAR_SLASH              '/'
#define ERR_EXIT                -1
#define PROGRESS_BUS_STRING     "\033[?25l%s[%-s] %d%%\033[?25h\033[0m\r"

#endif // _LINUX_

#endif // __CONSTANT_H__

