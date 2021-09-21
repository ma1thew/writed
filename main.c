#define _XOPEN_SOURCE 600

#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmp.h>

#include <libnotify/notify.h>

char ut_line[UT_LINESIZE];

static void handle_exit(int signal) {
  notify_uninit();
  logout(ut_line);
}

static void clean_string(char *str) {
  char *src, *dest;
  bool stripped_start = false;
  bool should_increment = false;
  for (src = dest = str; *src != '\0'; src++) {
    *dest = *src;
    if (isprint(*dest) && *dest != ' ' && *dest != '\n' && *dest != '\r') {
      stripped_start = true;
      should_increment = true;
    }
    if (*dest == ' ' && stripped_start) {
      should_increment = true;
    }
    if (*dest == '\n' || *dest == '\r') {
      stripped_start = false;
    }
    if (should_increment) {
      should_increment = false;
      dest++;
    }
  }
  *dest = '\0';
}

int main(int argc, char **argv) {
  char *name_ptr;
  int master_fd, slave_fd;
  char buffer[1024];
  ssize_t bytes_read;
  int uid;
  struct utmp login_data;
  struct passwd *pw_entry;
  struct timeval current_time;
  NotifyNotification *notification;

  if ((master_fd = posix_openpt(O_RDWR)) < 0)
    return 1;
  if (grantpt(master_fd) < 0)
    return 1;
  if (unlockpt(master_fd) < 0)
    return 1;
  if ((name_ptr = ptsname(master_fd)) == NULL)
    return 1;

  if ((slave_fd = open(name_ptr, O_RDONLY)) < 0)
    return 1;

  memset(&login_data, 0, sizeof(login_data));

  uid = getuid();
  pw_entry = getpwuid(uid);
  if (!pw_entry)
    return 1;
  strncpy(login_data.ut_name, pw_entry->pw_name, sizeof(login_data.ut_name));

  if (!memcmp(name_ptr, "/dev/", 5))
    name_ptr += 5;
  strncpy(login_data.ut_line, name_ptr, sizeof(login_data.ut_line));
  strncpy(ut_line, name_ptr, sizeof(ut_line));

  notify_init("writed");

  signal(SIGTERM, handle_exit);
  signal(SIGINT, handle_exit);
  signal(SIGHUP, handle_exit);

  gettimeofday(&current_time, NULL);
  memcpy(&login_data.ut_tv, &current_time, sizeof(login_data.ut_tv));

  login_data.ut_type = USER_PROCESS;
  login_data.ut_pid = getpid();
  dup2(slave_fd, 0);
  login(&login_data);

  for (;;) {
    bytes_read = read(master_fd, buffer, sizeof(buffer));
    if (bytes_read < 0)
      break;
    else if (bytes_read > 0) {
      buffer[sizeof(buffer) - 1] = '\x0';
      clean_string(buffer);
      notification = notify_notification_new("System Broadcast", buffer, 0);
      notify_notification_show(notification, NULL);
      g_object_unref(G_OBJECT(notification));
    }
  }

  handle_exit(15);
  return 0;
}
