#pragma once

#define BLACK_TEXT      "\033[1;30m"
#define BLACK_BACK      "\033[1;40m"

#define RED_TEXT        "\033[1;31m"
#define RED_BACK        "\033[1;41m"

#define GREEN_TEXT      "\033[1;32m"
#define GREEN_BACK      "\033[1;42m"

#define YELLOW_TEXT     "\033[1;33m"
#define YELLOW_BACK     "\033[1;43m"

#define BLUE_TEXT       "\033[1;34m"
#define BLUE_BACK       "\033[1;44m"

#define MAGENTA_TEXT    "\033[1;35m"
#define MAGENTA_BACK    "\033[1;45m"

#define CYAN_TEXT       "\033[1;36m"
#define CYAN_BACK       "\033[1;46m"

#define WHITE_TEXT      "\033[1;37m"
#define WHITE_BACK      "\033[1;47m"

#define NORMAL_TEXT     "\033[1;39m"
#define NORMAL_BACK     "\033[1;49m"

#define RESET           "\033[0m"
#define DELETE_LINE     "\033[F\033[K"

_Noreturn void fatal_error(char* error);
// EOF